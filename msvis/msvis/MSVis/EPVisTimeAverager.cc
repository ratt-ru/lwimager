//# EPVisTimeAverager.cc: Implementation of EPVisTimeAverager.h
//# Copyright (C) 1996,1997,1998,1999,2000,2002,2003
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$
//----------------------------------------------------------------------------

#include <msvis/MSVis/EPVisTimeAverager.h>
#include <casa/Exceptions/Error.h>
#include <casa/Arrays/Cube.h>

#define SETROW(NDX,R) {NDX(2)=R;}
#define SETCHAN(NDX,C) {NDX(1)=C;}
#define SETPOLN(NDX,P) {NDX(0)=P;}

namespace casa {
//
//----------------------------------------------------------------------------
//
EPVisTimeAverager::EPVisTimeAverager(const Int& nAnt, const Double& interval,
				 const Bool& prenorm)
  : nAnt_p(nAnt), interval_p(interval), prenorm_p(prenorm), handleUVW(False),
    whichVBDataStructure(VISIBILITY)
{
// Construct from the number of antennas and the averaging interval
// Input:
//    nAnt                 const Int&       No. of antennas
//    interval             const Double&    Time interval (in seconds).
//    prenorm              const Bool&      Pre-normalization flag
// Output to private data:
//    nAnt_p               Int              No. of antennas
//    interval_p           Double           Time interval (in seconds).
//    prenorm_p            Bool             Pre-normalization flag
//
  // Reset the averager
  handleUVW=False;
  whichVBDataStructure = VISIBILITY;
  reset();
};
//
//----------------------------------------------------------------------------
//
EPVisTimeAverager::EPVisTimeAverager(const Int& nAnt, const Double& interval,
				 const Bool& prenorm,Bool doUVW, 
				 VBDataStructure whichDtStr) 
  : nAnt_p(nAnt), interval_p(interval), prenorm_p(prenorm), handleUVW(doUVW),
    whichVBDataStructure(whichDtStr)
{
// Construct from the number of antennas and the averaging interval
// Input:
//    nAnt                 const Int&       No. of antennas
//    interval             const Double&    Time interval (in seconds).
//    prenorm              const Bool&      Pre-normalization flag
// Output to private data:
//    nAnt_p               Int              No. of antennas
//    interval_p           Double           Time interval (in seconds).
//    prenorm_p            Bool             Pre-normalization flag
//
  // Reset the averager
  reset();
};
//
//----------------------------------------------------------------------------
//
EPVisTimeAverager::~EPVisTimeAverager()
{
// Null default destructor
//
};
//
//----------------------------------------------------------------------------
//
void EPVisTimeAverager::reset()
{
// Reset the averager
// Output to private data:
//    tStart_p         Double        Start time of current accumulation
//    firstInterval_p  Bool          Is this the first interval ?
//    nChan_p          Int           No. of channels in the averaging buffer
//    avrow_p          Int           Starting row of current accumulation
//    avBuf_p          VisBuffer     Averaging buffer
//
  tStart_p = 0.0;
  firstInterval_p = True;
  nChan_p = 0;
  avrow_p = 0;
  getInVisCube(whichVBDataStructure);
};
//
//----------------------------------------------------------------------------
//Accumulate in
//VisBuffer.{visibility,modelVisibility,correctedVisibility} buffers.
//
void EPVisTimeAverager::accumulateVisibility (const VisBuffer& vb)
{
// Accumulate a VisBuffer
// Input:
//    vb               const VisBuffer&     VisBuffer to accumulate
// Output to private data:
//    tStart_p         Double               Start time of current accumulation
//    nChan_p          Int                  No. of channels in the avg. buffer
//    avrow_p          Int                  Start row of current accumulation
//    avBuf_p          VisBuffer            Averaging buffer
//
  // Check if avBuf_p initialization required; if so,
  // assign to vb to establish a two-way connection to
  // the underlying VisIter. 
  if (firstInterval_p) {
    // Initialize the averaging buffer
    avBuf_p = vb;
    avBuf_p.nRow();
    avBuf_p.nRow() = 0;
  };

  // Iterate through the current VisBuffer
  Int row = 0;
  while (row < vb.nRow()) {
    // Find the next unflagged time
    while (row < vb.nRow() && vb.flagRow()(row)) {
      row++;
    };
    if (row < vb.nRow()) {
      Double thisTime = vb.time()(row);
      // Round down to the start of an accumulation interval
      //      if (interval_p > 0) {
      //	thisTime = Int (thisTime / interval_p) * interval_p;
      //      };

      // Check for the first accumulation interval
      if (firstInterval_p) {
	tStart_p = thisTime;
	firstInterval_p = False;
	nChan_p = vb.nChannel();
	// Initialize the first accumulation interval
	initialize();
      };

      // Check for end of the current accumulation interval
      if ((vb.time()(row) - tStart_p) > interval_p) {
	// Normalize
	normalize();
	// Advance indices to the next accumulation interval
	tStart_p = thisTime;
	avrow_p += hashFunction(nAnt_p-1, nAnt_p-1) + 1;
	// Initialize the next accumulation interval
	initialize();
      };

      // Add the VisBuffer row to the current accumulation
      // 
      // Only accumulate VisBuffers with the same number of channels
      if (vb.nChannel() != nChan_p) {
	throw(AipsError("EPVisTimeAverager: frequency channels do not conform"));
      };

      Int ant1 = vb.antenna1()(row);
      Int ant2 = vb.antenna2()(row);
      // Calculate row from antenna numbers with the hash function.
      Int outrow = avrow_p + hashFunction (ant1, ant2);
      Float wt = vb.weight()(row);
      avBuf_p.flagRow()(outrow) = False;
      // Subtract offset to avoid roundoff problems with float weights
      avBuf_p.time()(outrow) += (vb.time()(row)-tStart_p) * wt;
      avBuf_p.weight()(outrow) += wt;

      // Compute the pre-normalization (if requested)
      CStokesVector preNormFact = Complex(0,0);
      Int nPreNorm = 0;
      if (prenorm_p) {
	for (Int chn=0; chn<vb.nChannel(); chn++) {
	  if (!vb.flag()(chn,row)) {
	    preNormFact += outVisibility(chn,row);
	    nPreNorm++;
	  };
	};
	for (uInt k=0; k < 4; k++) {
	  preNormFact(k) = static_cast<Float>(nPreNorm) / preNormFact(k);
	};
      };

      CStokesVector tmp;
      for (Int chn=0; chn<vb.nChannel(); chn++) {
	if (!vb.flag()(chn,row)) {
	  avBuf_p.flag()(chn,outrow) = False;
	  tmp = outVisibility(chn,row);
	  if (prenorm_p) tmp *= preNormFact;
	  tmp *= wt;
	  inVisibility(chn,outrow) += tmp;
	}
      };

      if (handleUVW) avBuf_p.uvw()(outrow) += vb.uvw()(row);

      // Increment the row number
      row++;
    }; // if (row < vb.nRow())
  }; // while (row < vb.nRow())
};
//
//----------------------------------------------------------------------------
//Accumulate in
//VisBuffer.{visCube,modelVisCube,correctedVisCube} buffers.
//
void EPVisTimeAverager::accumulateVisCubes(const VisBuffer& vb)
{
// Accumulate a VisBuffer
// Input:
//    vb               const VisBuffer&     VisBuffer to accumulate
// Output to private data:
//    tStart_p         Double               Start time of current accumulation
//    nChan_p          Int                  No. of channels in the avg. buffer
//    avrow_p          Int                  Start row of current accumulation
//    avBuf_p          VisBuffer            Averaging buffer
//
  // Check if avBuf_p initialization required; if so,
  // assign to vb to establish a two-way connection to
  // the underlying VisIter. 
  if (firstInterval_p) {
    // Initialize the averaging buffer
    avBuf_p = vb;
    avBuf_p.nRow();
    avBuf_p.nRow() = 0;
  };

  // Iterate through the current VisBuffer
  Int row = 0;
  IPosition ndx(3,0);
  while (row < vb.nRow()) {
    // Find the next unflagged time
    while (row < vb.nRow() && vb.flagRow()(row)) {
      row++;
    };
    if (row < vb.nRow()) {
      Double thisTime = vb.time()(row);
      // Round down to the start of an accumulation interval
      //      if (interval_p > 0) {
      //	thisTime = Int (thisTime / interval_p) * interval_p;
      //      };

      // Check for the first accumulation interval
      if (firstInterval_p) {
	tStart_p = thisTime;
	firstInterval_p = False;
	nChan_p = vb.nChannel();
	// Initialize the first accumulation interval
	initialize();
      };

      // Check for end of the current accumulation interval
      if ((vb.time()(row) - tStart_p) > interval_p) {
	// Normalize
	normalize();
	// Advance indices to the next accumulation interval
	tStart_p = thisTime;
	avrow_p += hashFunction(nAnt_p-1, nAnt_p-1) + 1;
	// Initialize the next accumulation interval
	initialize();
      };

      // Add the VisBuffer row to the current accumulation
      // 
      // Only accumulate VisBuffers with the same number of channels
      if (vb.nChannel() != nChan_p) {
	throw(AipsError("EPVisTimeAverager: frequency channels do not conform"));
      };

      Int ant1 = vb.antenna1()(row);
      Int ant2 = vb.antenna2()(row);
      // Calculate row from antenna numbers with the hash function.
      Int outrow = avrow_p + hashFunction (ant1, ant2);
      Float wt = vb.weight()(row);
      avBuf_p.flagRow()(outrow) = False;
      // Subtract offset to avoid roundoff problems with float weights
      avBuf_p.time()(outrow) += (vb.time()(row)-tStart_p) * wt;
      avBuf_p.weight()(outrow) += wt;

      // Compute the pre-normalization (if requested)
      Complex preNormFact=Complex(0,0);

      Int nPreNorm = 0;
      SETROW(ndx,row);
      if (prenorm_p) 
	{
	  for (Int chn=0; chn<vb.nChannel(); chn++) 
	    if (!vb.flagCube()(0,chn,row)) 
	      {
		SETCHAN(ndx,chn);
		preNormFact += outVisCube(ndx);
		nPreNorm++;
	      };
	  
	  preNormFact = static_cast<Float>(nPreNorm) / preNormFact;
	};

      Complex tmp;
      for (Int chn=0; chn<vb.nChannel(); chn++) 
	if (!vb.flagCube()(0,chn,row)) 
	  {
	    avBuf_p.flagCube()(0,chn,outrow) = False;
	    SETCHAN(ndx,chn);
	    SETROW(ndx,row);
	    tmp = outVisCube(ndx);
	    if (prenorm_p) tmp *= preNormFact;
	    tmp *= wt;
	    SETROW(ndx,outrow);
	    inVisCube(ndx) += tmp;
	  }

      if (handleUVW) avBuf_p.uvw()(outrow) += vb.uvw()(row);

      // Increment the row number
      row++;
    }; // if (row < vb.nRow())
  }; // while (row < vb.nRow())
};
//
//----------------------------------------------------------------------------
//
void EPVisTimeAverager::finalizeAverage (VisBuffer& avBuf)
{
// Finalize the average, and return the result
// Output:
//    avBuf         VisBuffer&       Averaged buffer
//
  // Normalize the current accumulation interval
  normalize();

  // Return the averaged buffer
  avBuf = avBuf_p;
  //
  // The above does not copy cubes!
  // 
  if ((whichVBDataStructure==VISCUBE)      |
      (whichVBDataStructure==MODELVISCUBE) |
      (whichVBDataStructure==CORRECTEDVISCUBE))
    {
      cout << "Copying data!!!!" << endl;
      //      copyData(avBuf);
    }
};
//
//----------------------------------------------------------------------------
// Initialize the VisBuffer.{visibility,model,corrected} buffers
//
void EPVisTimeAverager::initializeVisibility()
{
// Initialize the next accumulation interval
// Output to private data:
//    avBuf_p          VisBuffer       Averaging buffer
//    
  Int nRowAdd = hashFunction (nAnt_p-1, nAnt_p-1) + 1;
  avBuf_p.nRow() += nRowAdd;
  avBuf_p.nChannel() = nChan_p;

  // Resize and initialize the VisBuffer columns used here
  Int nRow = avBuf_p.nRow();
  avBuf_p.antenna1().resize(nRow, True);
  avBuf_p.antenna2().resize(nRow, True);
  // Fill in the antenna numbers for all rows
  Int row = avrow_p;
  for (Int ant1=0; ant1 < nAnt_p; ant1++) {
    for (Int ant2 = ant1; ant2 < nAnt_p; ant2++) {
      avBuf_p.antenna1()(row) = ant1;
      avBuf_p.antenna2()(row) = ant2;
      row++;
    }
  }

  avBuf_p.time().resize(nRow, True); 
  Matrix<CStokesVector> tmpVis = inVisibility;
  inVisibility.resize(nChan_p, nRow);
  Int chn;
  for (row = 0; row < nRow-nRowAdd; row++) {
    for (chn = 0; chn < nChan_p; chn++) {
      inVisibility(chn,row) = tmpVis(chn,row);
    };
  };
  avBuf_p.weight().resize(nRow, True); 
  avBuf_p.flagRow().resize(nRow, True); 
  Matrix<Bool> tmpFlag = avBuf_p.flag();
  avBuf_p.flag().resize(nChan_p, nRow);
  for (row = 0; row < nRow-nRowAdd; row++) {
    for (chn = 0; chn < nChan_p; chn++) {
      avBuf_p.flag()(chn,row) = tmpFlag(chn,row);
    };
  };

  for (row = avrow_p; row < nRow; row++) {
    avBuf_p.time()(row) = 0.0;
    for (Int chn = 0; chn < nChan_p; chn++) {
      inVisibility(chn,row) = CStokesVector();
      avBuf_p.flag()(chn,row) = True;
    };
    avBuf_p.weight()(row) = 0.0f;
    avBuf_p.flagRow()(row) = True;
  };

  if (handleUVW) avBuf_p.uvw().resize(nRow,0);

};
//
//----------------------------------------------------------------------------
// Initialize the VisBuffer.{visCube,modelVisCube,correctedVisCube} buffers.
//
void EPVisTimeAverager::initializeVisCubes()
{
// Initialize the next accumulation interval
// Output to private data:
//    avBuf_p          VisBuffer       Averaging buffer
//    
  Int nRowAdd = hashFunction (nAnt_p-1, nAnt_p-1) + 1;
  avBuf_p.nRow() += nRowAdd;
  avBuf_p.nChannel() = nChan_p;

  // Resize and initialize the VisBuffer columns used here
  Int nRow = avBuf_p.nRow();
  avBuf_p.antenna1().resize(nRow, True);
  avBuf_p.antenna2().resize(nRow, True);
  // Fill in the antenna numbers for all rows
  Int row = avrow_p;
  for (Int ant1=0; ant1 < nAnt_p; ant1++) {
    for (Int ant2 = ant1; ant2 < nAnt_p; ant2++) {
      avBuf_p.antenna1()(row) = ant1;
      avBuf_p.antenna2()(row) = ant2;
      row++;
    }
  }

  avBuf_p.time().resize(nRow, True); 

  IPosition ndx(3,0);
  SETROW(ndx,nRow); SETCHAN(ndx,nChan_p);
  SETPOLN(ndx,1);
  Cube<Complex> tmpVis = inVisCube;
  inVisCube.resize(ndx);
  SETPOLN(ndx,0);

  Int chn;
  for (row = 0; row < nRow-nRowAdd; row++) 
    {
      SETROW(ndx,row);

      for (chn = 0; chn < nChan_p; chn++) 
	{
	  SETCHAN(ndx,chn);
	  inVisCube(ndx) = tmpVis(ndx);
	};
    };

  avBuf_p.weight().resize(nRow, True); 
  avBuf_p.flagRow().resize(nRow, True); 

  //  Matrix<Bool> tmpFlag = avBuf_p.flag();
  Cube<Bool> tmpFlag = avBuf_p.flagCube();
  avBuf_p.flagCube().resize(1,nChan_p, nRow);

  for (row = 0; row < nRow-nRowAdd; row++) {
    for (chn = 0; chn < nChan_p; chn++) {
      avBuf_p.flagCube()(0,chn,row) = tmpFlag(0,chn,row);
    };
  };

  for (row = avrow_p; row < nRow; row++) 
    {
      SETROW(ndx,row);
      avBuf_p.time()(row) = 0.0;
      for (Int chn = 0; chn < nChan_p; chn++) 
	{
	  SETCHAN(ndx,chn);
	  inVisCube(ndx) = 0.0;
	  avBuf_p.flagCube()(0,chn,row) = True;
	};
      avBuf_p.weight()(row) = 0.0f;
      avBuf_p.flagRow()(row) = True;
    };

  if (handleUVW) avBuf_p.uvw().resize(nRow,0);

};
//
//----------------------------------------------------------------------------
// Normalize the VisBuffer.{visibility,model,corrected} buffers
//
void EPVisTimeAverager::normalizeVisibility()
{
// Normalize the current accumulation interval
// Output to private data:
//    avBuf_p         VisBuffer&       Averaged buffer
//  
  for (Int row=avrow_p; row<avBuf_p.nRow(); row++) {
    Float wt=avBuf_p.weight()(row);
    if (wt==0.0f) avBuf_p.flagRow()(row)=True;
    if (!avBuf_p.flagRow()(row)) {
      avBuf_p.time()(row)/=wt;
      avBuf_p.time()(row)+=tStart_p;
      for (Int chn=0; chn<avBuf_p.nChannel(); chn++) {
	if (!avBuf_p.flag()(chn,row)) {
	  inVisibility(chn,row)*=1.0f/wt;
	}
      }
    }
  }
};
//
//----------------------------------------------------------------------------
// Normalize the VisBuffer.{visCube,modelVisCube,corrrectedVisCube} buffers
//
void EPVisTimeAverager::normalizeVisCubes()
{
// Normalize the current accumulation interval
// Output to private data:
//    avBuf_p         VisBuffer&       Averaged buffer
//  
  IPosition ndx(3,0);
  for (Int row=avrow_p; row<avBuf_p.nRow(); row++) 
    {
      SETROW(ndx,row);
      Float wt=avBuf_p.weight()(row);
      if (wt==0.0f) avBuf_p.flagRow()(row)=True;
      if (!avBuf_p.flagRow()(row)) 
	{
	  avBuf_p.time()(row)/=wt;
	  avBuf_p.time()(row)+=tStart_p;
	  for (Int chn=0; chn<avBuf_p.nChannel(); chn++) 
	    {
	      SETCHAN(ndx,chn);
	      if (!avBuf_p.flagCube()(0,chn,row)) inVisCube(ndx)*=1.0f/wt;
	    }
	}
    }
};
//
//----------------------------------------------------------------------------
//
Int EPVisTimeAverager::hashFunction (const Int& ant1, const Int& ant2)
{
// Compute row index in an accumulation interval for an
// interferometer index (ant1, ant2).
// Input:
//    ant1            const Int&      Antenna 1
//    ant2            const Int&      Antenna 2
// Output:
//    hashFunction    Int             Row offset in current accumulation
//
  Int index;
  index = nAnt_p * ant1 - (ant1 * (ant1 - 1)) / 2 + ant2 - ant1;
  return index;
};
//
//----------------------------------------------------------------------------
// Extract the appropriate data structure from the internal VisBuffer into
// generic internal variables (by referencing to the VisBuffer buffers).
// Also set the function pointers for initializer, accumulater and normalizer
// appropriately.
//
void EPVisTimeAverager::getInVisCube(VBDataStructure which)
{
  Bool Cubes=False;
  switch (which)
    {
    case MODELVISCUBE: 
      {
	inVisCube.reference(avBuf_p.modelVisCube());
	Cubes=True;
	break;
      }
    case VISCUBE: 
      {
	inVisCube.reference(avBuf_p.visCube());
	Cubes=True;
	break;
      }
    case CORRECTEDVISCUBE: 
      {
	inVisCube.reference(avBuf_p.correctedVisCube());
	Cubes=True;
	break;
      }
      
    case VISIBILITY: 
      {
	inVisibility.reference(avBuf_p.visibility());
	break;
      }
    case MODELVISIBILITY: 
      {
	inVisibility.reference(avBuf_p.modelVisibility());
	break;
      }
    case CORRECTEDVISIBILITY: 
      {
	inVisibility.reference(avBuf_p.correctedVisibility());
	break;
      }

    default: throw (AipsError("Unknown type in EPVisTimeAverager::getInVisCube(): "));
    };

  if (Cubes)
    {
      Initializer = &EPVisTimeAverager::initializeVisCubes;
      Normalizer =  &EPVisTimeAverager::normalizeVisCubes;
      Accumulater = &EPVisTimeAverager::accumulateVisCubes;
    }
  else
    {
      Initializer = &EPVisTimeAverager::initializeVisibility;
      Normalizer  = &EPVisTimeAverager::normalizeVisibility;
      Accumulater = &EPVisTimeAverager::accumulateVisibility;
    }
    
}
//----------------------------------------------------------------------------
void EPVisTimeAverager::getOutVisCube(const VisBuffer& vb, VBDataStructure which)
{
  Bool Cubes=False;
  switch (which)
    {
    case MODELVISCUBE: 
      {
	outVisCube.reference((Cube<Complex>&)vb.modelVisCube());
	Cubes=True;
	break;
      }
    case VISCUBE: 
      {
	outVisCube.reference((Cube<Complex>&)vb.visCube());
	Cubes=True;
	break;
      }
    case CORRECTEDVISCUBE: 
      {
	outVisCube.reference((Cube<Complex>&)vb.correctedVisCube());
	Cubes=True;
	break;
      }
      
    case VISIBILITY: 
      {
	outVisibility.reference((Matrix<CStokesVector>&)vb.visibility());
	break;
      }
    case MODELVISIBILITY: 
      {
	outVisibility.reference((Matrix<CStokesVector>&)vb.modelVisibility());
	break;
      }
    case CORRECTEDVISIBILITY: 
      {
	outVisibility.reference((Matrix<CStokesVector>&)vb.correctedVisibility());
	break;
      }

    default: throw (AipsError("Unknown type in EPVisTimeAverager::getOutVisCube(): "));
    };

  if (Cubes)
    {
      Initializer = &EPVisTimeAverager::initializeVisCubes;
      Normalizer =  &EPVisTimeAverager::normalizeVisCubes;
      Accumulater = &EPVisTimeAverager::accumulateVisCubes;
    }
  else
    {
      Initializer = &EPVisTimeAverager::initializeVisibility;
      Normalizer  = &EPVisTimeAverager::normalizeVisibility;
      Accumulater = &EPVisTimeAverager::accumulateVisibility;
    }
    
}
//----------------------------------------------------------------------------
// Copy the internal Cube to the VisBuffer Cubes.
//
void EPVisTimeAverager::copyData(VisBuffer& vb)
{
  switch (whichVBDataStructure)
    {
    case MODELVISCUBE: 
      {
	vb.setModelVisCube(inVisCube);
	break;
      }
    case VISCUBE: 
      {
	vb.setVisCube(inVisCube);
	break;
      }
    case CORRECTEDVISCUBE: 
      {
	vb.setCorrectedVisCube(inVisCube);
	break;
      }
    default: throw (AipsError("Unknown type in EPVisTimeAverager::copyData(VisBuffer&): "));
    };
}
}
