//# VisibilityIterator.cc: Step through MeasurementEquation by visibility
//# Copyright (C) 1996,1997,1998,1999,2000,2001,2002,2003
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

#include <msvis/MSVis/VisIterator.h>
#include <msvis/MSVis/VisBuffer.h>
#include <scimath/Mathematics/InterpolateArray1D.h>
#include <casa/Arrays/ArrayLogical.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/MaskedArray.h>
#include <casa/Exceptions/Error.h>
#include <casa/Utilities/Assert.h>
#include <casa/Utilities/Sort.h>
#include <ms/MeasurementSets/MSColumns.h>
#include <casa/Quanta/MVTime.h>
#include <tables/Tables/TableDesc.h>
#include <tables/Tables/ColDescSet.h>
#include <tables/Tables/TableRecord.h>
#include <tables/Tables/TiledColumnStMan.h>
#include <tables/Tables/TiledStManAccessor.h>

namespace casa { //# NAMESPACE CASA - BEGIN

ROVisIterator::ROVisIterator() {}

// const of MS is cast away, but we don't actually change it.
// (just to share code between RO version and RW version of iterator)
ROVisIterator::ROVisIterator(const MeasurementSet &ms,
			     const Block<Int>& sortColumns,
			     Double timeInterval)
  : ROVisibilityIterator(ms,sortColumns,timeInterval),
    useNewSlicer_p(False)
{

  // Initialize multi-slicers with empty slices
  chanSlices_p.resize(numberSpw());
  chanSlices_p.set(Vector<Slice>());
  corrSlices_p.resize(numberPol());
  corrSlices_p.set(Vector<Slice>());

}

ROVisIterator::ROVisIterator(const ROVisIterator& other)
{
    operator=(other);
}

ROVisIterator::~ROVisIterator() {}

ROVisIterator& 
ROVisIterator::operator=(const ROVisIterator& other) 
{
  if (this!=&other) {
    ROVisibilityIterator::operator=(other);

    // TBD: new slicer stuff

  }
  return *this;
}

ROVisIterator & ROVisIterator::operator++(int)
{
  if (!more_p) return *this;
  advance();
  return *this;
}

ROVisIterator & ROVisIterator::operator++()
{
  if (!more_p) return *this;
  advance();
  return *this;
}



void ROVisIterator::updateSlicer()
{
  useNewSlicer_p=True;

  //    cout << "Using new slicer!..." << flush;
  
  newSlicer_p.resize(2);
  newWtSlicer_p.resize(1);
  
  //    cout << "newSlicer_p.shape() = " << newSlicer_p.shape() << endl;
  
  useSlicer_p=False;
  
  // Refer to correct slices for the current spwid/polid
  newSlicer_p(0).reference(corrSlices_p(this->polarizationId()));
  newSlicer_p(1).reference(chanSlices_p(this->spectralWindow()));
  
  newWtSlicer_p(0).reference(corrSlices_p(this->polarizationId()));

  setTileCache();
}

// (Alternative syntax for ROVisIter::chanIds)
Vector<Int>& ROVisIterator::channel(Vector<Int>& chan) const
{ return this->chanIds(chan); }


Vector<Int>& ROVisIterator::chanIds(Vector<Int>& chanids) const
{
  return chanIds(chanids,msIter_p.spectralWindowId());
}

Vector<Int>& ROVisIterator::chanIds(Vector<Int>& chanids, Int spw) const
{

  chanids.resize(this->numberChan(spw));

  // if non-trivial slices available
  if (chanSlices_p(spw).nelements() > 0 ) {
    
    Vector<Slice> slices(chanSlices_p(spw));
    Int nslices=slices.nelements();
    
    Int ich0=0;
    for (Int i=0;i<nslices;++i) {
      Int nchan=slices(i).length();
      Int start=slices(i).start();
      for (Int ich=0;ich<nchan;++ich,++ich0) 
	chanids(ich0)=start+ich;
    }
    
  }
  else {
    // all channels selected...
    indgen(chanids);
  }
  return chanids;
}

void ROVisIterator::setSelTable()
{
    ROVisibilityIterator::setSelTable();

    // The following code (which uses Table::operator() to create
    // a new RefTable) is computationally expensive. This could
    // be optimized by using the same method as in the
    // VisibilityIterator base class (which is to not create
    // RefTables but instead access the table column directly in
    // msIter_p.table() using VisibilityIterator::selRows_p).

    // Doing so would mean replacing calls like
    //     colSigma.getColumn(newWtSlicer_p,sigmat,True);
    // with
    //     colSigma.getColumnCells(selRows_p,newWtSlicer_p,sigmat,True);
    //
    // However the ArrayColumn class does allow passing both a 
    // Vector<Vector<Slice> > and a RefRows parameter to get-/putColumn.
    // Such put/get functions must be first implemented.

    Vector<uInt> rows(curNumRow_p);
    indgen(rows,uInt(curStartRow_p));
    selTable_p=msIter_p.table()(rows);
    attachColumns(attachTable());
}

void ROVisIterator::slicesToMatrices(Vector<Matrix<Int> >& matv,
                                     const Vector<Vector<Slice> >& slicesv,
                                     const Vector<Int>& widthsv) const
{
  Int nspw = slicesv.nelements();

  matv.resize(nspw);
  uInt selspw = 0;
  for(uInt spw = 0; spw < nspw; ++spw){
    uInt nSlices = slicesv[spw].nelements();

    // Figure out how big to make matv[spw].
    uInt totOutChan = 0;
    
    Int width = (nSlices > 0) ? widthsv[selspw] : 1;
    if(width < 1)
      throw(AipsError("Cannot channel average with width < 1"));

    for(uInt slicenum = 0; slicenum < nSlices; ++slicenum){
      const Slice& sl = slicesv[spw][slicenum];
      Int firstchan = sl.start();
      Int lastchan = sl.all() ? firstchan + chanWidth_p[spw] - 1 : sl.end();
      Int inc = sl.all() ? 1 : sl.inc();

      // Even if negative increments are desirable, the for loop below has a <.
      if(inc < 1)
        throw(AipsError("The channel increment must be >= 1"));

      // This formula is very dependent on integer division.  Don't rearrange it.
      totOutChan += 1 + ((lastchan - firstchan) / inc) / (1 + (width - 1) / inc);
    }
    matv[spw].resize(totOutChan, 4);

    // Index of input channel in SELECTED list, i.e.
    // mschan = vi.chanIds(chanids, spw)[selchanind].
    uInt selchanind = 0;

    // Fill matv with channel boundaries.
    uInt outChan = 0;
    for(uInt slicenum = 0; slicenum < nSlices; ++slicenum){
      const Slice& sl = slicesv[spw][slicenum];
      Int firstchan = sl.start();
      Int lastchan = sl.all() ? firstchan + chanWidth_p[spw] - 1 : sl.end();
      Int inc = sl.all() ? 1 : sl.inc(); // Default to no skipping

      // Again, these depend on integer division.  Don't rearrange them.
      Int selspan = 1 + (width - 1) / inc;
      Int span = inc * selspan;

      for(Int mschan = firstchan; mschan <= lastchan; mschan += span){
        // The start and end in MS channel #s.
        matv[spw](outChan, 0) = mschan;
        matv[spw](outChan, 1) = mschan + width - 1;

        // The start and end in selected reckoning.
        matv[spw](outChan, 2) = selchanind;
        selchanind += selspan;
        matv[spw](outChan, 3) = selchanind - 1;
        ++outChan;
      }
    }
    if(nSlices > 0)     // spw was selected
      ++selspw;
  }
}

// Return native correlation _indices_
Vector<Int>& ROVisIterator::corrIds(Vector<Int>& corrids) const
{
  Int pol = msIter_p.polarizationId();

  corrids.resize(this->numberCorr(pol));

  
  Vector<Slice> slices(corrSlices_p(pol));
  Int nslices=slices.nelements();
  
  // if non-trivial slices available
  if (nslices > 0 ) {
    
    Int icor0=0;
    for (Int i=0;i<nslices;++i) {
      Int ncorr=slices(i).length();
      Int start=slices(i).start();
      for (Int icor=0;icor<ncorr;++icor,++icor0) 
	corrids(icor0)=start+icor;
    }
    
  }
  else {
    // all corrs selected...
    indgen(corrids);
  }

  return corrids;
}


Vector<Int>& ROVisIterator::corrType(Vector<Int>& corrTypes) const
{

  // TBD:  Use corrIds instead of mask

  // Get the nominal corrType list
  Int polId = msIter_p.polarizationId();
  Vector<Int> nomCorrTypes;
  msIter_p.msColumns().polarization().corrType().get(polId,nomCorrTypes,True);

  if (corrSlices_p(polId).nelements() > 0) {
    Vector<Bool> corrmask(nomCorrTypes.nelements(),False);
    Vector<Slice> corrsel(corrSlices_p(polId));
    for (uInt i=0;i<corrsel.nelements();++i)
      corrmask(corrsel(i).start())=True;
    
    // Reference the selected subset
    corrTypes.reference(nomCorrTypes(corrmask).getCompressedArray());

  }
  else
    corrTypes.reference(nomCorrTypes);

  //    cout << "corrTypes = " << corrTypes << endl;
	
  return corrTypes;

}

Cube<Bool>& ROVisIterator::flag(Cube<Bool>& flags) const
{
  if (useNewSlicer_p) colFlag.getColumn(newSlicer_p,flags,True);
  else colFlag.getColumn(flags,True);
  return flags;
}

Vector<Double>& ROVisIterator::frequency(Vector<Double>& freq) const
{
  
  // We need to change some internals transparently
  ROVisIterator* self = const_cast<ROVisIterator*>(this);

  if (!freqCacheOK_p) {
    self->freqCacheOK_p=True;
    const Vector<Double>& chanFreq=msIter_p.frequency();
    Vector<Int> chan;
    self->channel(chan);
    Int nch=chan.nelements();
    self->frequency_p.resize(nch);
    for (Int i=0;i<nch;++i)
      self->frequency_p(i)=chanFreq(chan(i));
  }
  // Always copy to output
  freq.resize(frequency_p.nelements());
  freq=frequency_p;

  return freq;
}

Cube<Complex>& 
ROVisIterator::visibility(Cube<Complex>& vis, DataColumn whichOne) const
{
  if (useNewSlicer_p) getDataColumn(whichOne,newSlicer_p,vis);
  else getDataColumn(whichOne,vis);
  return vis;
}

void ROVisIterator::getDataColumn(DataColumn whichOne, 
				  const Vector<Vector<Slice> >& slices,
				  Cube<Complex>& data) const
{ 
  // Return the visibility (observed, model or corrected);
  // deal with DATA and FLOAT_DATA seamlessly for observed data.
  switch (whichOne) {
  case Observed:
    if (floatDataFound_p) {
      Cube<Float> dataFloat;
      colFloatVis.getColumn(slices,dataFloat,True);
      data.resize(dataFloat.shape());
      convertArray(data,dataFloat);
    } else {
      colVis.getColumn(slices,data,True);
    };
    break;
  case Corrected:
    colCorrVis.getColumn(slices,data,True);
    break;
  case Model:
    colModelVis.getColumn(slices,data,True);
    break;
  };
 
};

Matrix<Float>& ROVisIterator::sigmaMat(Matrix<Float>& sigmat) const
{
  if (useNewSlicer_p) colSigma.getColumn(newWtSlicer_p,sigmat,True);
  else {
    sigmat.resize(nPol_p,curNumRow_p);
    colSigma.getColumn(sigmat);
  }
  return sigmat;
}

Matrix<Float>& ROVisIterator::weightMat(Matrix<Float>& wtmat) const
{
  if (useNewSlicer_p) colWeight.getColumn(newWtSlicer_p,wtmat,True);
  else {
    wtmat.resize(nPol_p,curNumRow_p);
    colWeight.getColumn(wtmat);
  }
  return wtmat;
}

Cube<Float>& ROVisIterator::weightSpectrum(Cube<Float>& wtsp) const
{
  if (!colWeightSpectrum.isNull()) {
    if (useNewSlicer_p) colWeightSpectrum.getColumn(newSlicer_p,wtsp,True);
    else {
      wtsp.resize(nPol_p,nChan_p,curNumRow_p);
      colWeightSpectrum.getColumn(wtsp);
    }
  } else {
    wtsp.resize(0,0,0);
  }
  return wtsp;
}

void ROVisIterator::selectChannel(const Vector<Vector<Slice> >& chansel) {
  //  cout << "selectChannel!...vss..." << flush;

  if (chansel.nelements() != uInt(numberSpw()))
    throw(AipsError("Specified channel slices has incorrect number of spws."));
  
  chanSlices_p.resize(numberSpw(),False);
  chanSlices_p=chansel;

  // Enforce use of the new slicer downstream
  useNewSlicer_p=True;
    
  //  cout << "done." << endl;

}

void ROVisIterator::selectCorrelation(const Vector<Vector<Slice> >& corrsel) {

  //  cout << "selectCorrelation!...vvs..." << flush;

  if (corrsel.nelements() != uInt(numberPol()))
    throw(AipsError("Specified correlation slices has incorrect number of polIds."));
  
  corrSlices_p.resize(numberPol(),False);
  corrSlices_p=corrsel;

  // Enforce use of the new slicer downstream
  useNewSlicer_p=True;
    
  //  cout << "done." << endl;

}

Vector<Matrix<Int> >& ROVisIterator::setChanAveBounds(Float factor,
						      Vector<Matrix<Int> >& bounds)
{
  if(!useNewSlicer_p)
    throw(AipsError("Help!"));

  // For every spw...
  bounds.resize(numberSpw());

  // If factor greater than zero, fill the bounds non-trivially
  if (factor > 0) {
 
    // Decipher averaging factor
    Int width = 1;
    if(factor > 1.0)
      width = Int(factor); // factor supplied in channel units
 
    // Calculate bounds for each spw
    for(Int ispw = 0; ispw < numberSpw(); ++ispw){
   
      // Number of SELECTED channels PRIOR to averaging
      Int nChan0 = numberChan(ispw);
   
      // factor might have been supplied as a fraction;
      //  width is then potentially spw-dependent
      if(factor <= 1.0)
	width = Int(factor * Float(nChan0));
   
      // Get the selected channel list
      Vector<Int> chans;
      chanIds(chans, ispw);

      // The nominal number of output channels depends on the full
      // range of channels selected (not the number of them)
      //  (will be revised later, if nec.)
      Int nChanOut0 = 1 + (chans[nChan0 - 1] - chans[0]) / width;
   
      // Initialize the bounds container for this spw
      Matrix<Int>& currBounds(bounds[ispw]);
      currBounds.resize(nChanOut0, 4);
      //currBounds.set(0);
   
      Int outChan = 0;
      Int firstchan = chans[0];
      Int lastchan = chans[nChan0 - 1];

      // Index of input channel in SELECTED list, i.e.
      // ich = vi.chanIds(chanids, spw)[selchanind].
      uInt selchanind = 0;

      for(Int ich = firstchan; ich <= lastchan; ich += width){
        Int w = width;

        // Use a clamped width in case (lastchan - firstchan + 1) % width != 0.
        if(ich + w - 1 > lastchan)
          w = lastchan + 1 - ich;

        // The start and end in MS channel #s.
        currBounds(outChan, 0) = ich;
        currBounds(outChan, 1) = ich + w - 1;

        // The start and end in selected reckoning.
        currBounds(outChan, 2) = selchanind;
        selchanind += w;
        currBounds(outChan, 3) = selchanind - 1;

        // for(uInt ii = 0; ii < 4; ++ii)
        //   cerr << "currBounds(" << outChan << ", " << ii << ") = "
        //        << currBounds(outChan, ii) << endl;
        ++outChan;
      }
    } // ispw

  } // factor > 0

  // Internal reference  (needed?)
  chanAveBounds_p.reference(bounds);

  // Return the bounds Vector reference
  return bounds;
}

// Vector<Matrix<Int> >& ROVisIterator::setChanAveBounds(Float factor,
// 						      Vector<Matrix<Int> >& bounds) {

//   if (!useNewSlicer_p) throw(AipsError("Help!"));

//   // For every spw...
//   bounds.resize(numberSpw());

//   // If factor greater than zero, fill the bounds non-trivially
//   if (factor>0) {
    
//     // Decipher averaging factor
//     Int width(1);
//     if (factor>1.0) width=Int(factor); // factor supplied in channel units
    
//     // Calculate bounds for each spw
//     for (Int ispw=0;ispw<numberSpw();++ispw) {
      
//       // Number of SELECTED channels PRIOR to averaging
//       Int nChan0=numberChan(ispw);
      
//       // factor might have been supplied in factional units;
//       //  width is then potentially spw-dependent
//       if (factor<=1.0)
// 	width=Int(factor*Float(nChan0));
      
//       // Get the selected channel list
//       Vector<Int> chans;
//       chanIds(chans,ispw);

//       // The nominal number of output channels depends on the full
//       // range of channels selected (not the number of them)
//       //  (will be revised later, if nec.)
//       Int nChanOut0((chans(nChan0-1)-chans(0)+1+width)/width);
      
//       // Initialize the bounds container for this spw
//       Matrix<Int>& currBounds(bounds(ispw));
//       currBounds.resize(nChanOut0,4);
//       currBounds.set(0);
      
//       // Actual output channel count; initialization
//       Int nChanOut=1;
//       Int lo(chans(0));
//       currBounds(0,0)=lo;
//       currBounds(0,2)=0;
//       for (Int ich=0;ich<nChan0;++ich) 
// 	if ( (chans(ich)-lo+1)>width ) {
// 	  currBounds(nChanOut-1,1)=chans(ich-1);   // end of previous
// 	  currBounds(nChanOut-1,3)=ich-1;
// 	  lo=currBounds(nChanOut,0)=chans(ich);    // start of next
// 	  currBounds(nChanOut,2)=ich;    // start of next
// 	  ++nChanOut;
// 	}
//       currBounds(nChanOut-1,1)=chans(nChan0-1);    // end of final set
//       currBounds(nChanOut-1,3)=(nChan0-1);    // end of final set
      
// //       for(uInt ii = 0; ii < 4; ++ii)
// //         cerr << "currBounds(" << nChanOut - 1 << ", " << ii << ") = "
// //              << currBounds(nChanOut - 1, ii) << endl;

//       // contract bounds container, if necessary
//       if (nChanOut<nChanOut0)
// 	currBounds.resize(nChanOut,4,True);
      
//     } // ispw


//   } // factor > 0

//   // Internal reference  (needed?)
//   chanAveBounds_p.reference(bounds);

//   // Return the bounds Vector reference
//   return bounds;

// }

Int ROVisIterator::numberChan(Int spw) const {

  // Nominally all channels this spw
  Int nchan=msColumns().spectralWindow().numChan()(spw);

  if (useNewSlicer_p) {
    Int nslices=chanSlices_p(spw).nelements();
    if (nslices > 0 ) {
      nchan=0;
      for (Int isl=0;isl<nslices;++isl) 
	nchan+=chanSlices_p(spw)(isl).length();
    }
  }

  return nchan;

}


Int ROVisIterator::numberCorr(Int pol) const {

  // Nominally all correlations this pol
  Int ncorr=msColumns().polarization().numCorr()(pol);

  if (useNewSlicer_p) {
    Int nslices=corrSlices_p(pol).nelements();
    if (nslices > 0 ) {
      // Accumulate from slice lengths
      ncorr=0;
      for (Int isl=0;isl<nslices;++isl) 
	ncorr+=corrSlices_p(pol)(isl).length();
    }
  }

  return ncorr;

}

void ROVisIterator::getCol(const ROScalarColumn<Bool> &column, Vector<Bool> &array, Bool resize) const
{
    column.getColumn(array, resize);
}

void ROVisIterator::getCol(const ROScalarColumn<Int> &column, Vector<Int> &array, Bool resize) const
{
    column.getColumn(array, resize);
}

void ROVisIterator::getCol(const ROScalarColumn<Double> &column, Vector<Double> &array, Bool resize) const
{
    column.getColumn(array, resize);
}

void ROVisIterator::getCol(const ROArrayColumn<Bool> &column, Array<Bool> &array, Bool resize) const
{
    column.getColumn(array, resize);
}

void ROVisIterator::getCol(const ROArrayColumn<Float> &column, Array<Float> &array, Bool resize) const
{
    column.getColumn(array, resize);
}

void ROVisIterator::getCol(const ROArrayColumn<Double> &column, Array<Double> &array, Bool resize) const
{
    column.getColumn(array, resize);
}

void ROVisIterator::getCol(const ROArrayColumn<Complex> &column, Array<Complex> &array, Bool resize) const
{
    column.getColumn(array, resize);
}

void ROVisIterator::getCol(const ROArrayColumn<Bool> &column, const Slicer &slicer, Array<Bool> &array, Bool resize) const
{
    column.getColumn(slicer, array, resize);
}

void ROVisIterator::getCol(const ROArrayColumn<Float> &column, const Slicer &slicer, Array<Float> &array, Bool resize) const
{
    column.getColumn(slicer, array, resize);
}

void ROVisIterator::getCol(const ROArrayColumn<Complex> &column, const Slicer &slicer, Array<Complex> &array, Bool resize) const
{
    column.getColumn(slicer, array, resize);
}


Vector<RigidVector<Double,3> >& 
ROVisIterator::uvw(Vector<RigidVector<Double,3> >& uvwvec) const
{
    uvwvec.resize(curNumRow_p);
    getCol(colUVW, uvwMat_p,True);
    // get a pointer to the raw storage for quick access
    Bool deleteIt;
    Double* pmat = uvwMat_p.getStorage(deleteIt);
    for (uInt row=0; row<curNumRow_p; row++, pmat+=3) uvwvec(row)=pmat;
    return uvwvec;
}


const Table
ROVisIterator::attachTable() const
{
    return selTable_p;
}

VisIterator::VisIterator() {}

VisIterator::VisIterator(MeasurementSet &MS, 
			 const Block<Int>& sortColumns, 
			 Double timeInterval)
  : ROVisIterator(MS, sortColumns, timeInterval)
{}

VisIterator::VisIterator(const VisIterator & other)
{
    operator=(other);
}

VisIterator::~VisIterator() {}

VisIterator& 
VisIterator::operator=(const VisIterator& other)
{
    if (this!=&other) {
	ROVisIterator::operator=(other);
        selTable_p=other.selTable_p;
	RWcolFlag.reference(other.RWcolFlag);
        RWcolFlagRow.reference(other.RWcolFlagRow);
	RWcolVis.reference(other.RWcolVis);
	RWcolFloatVis.reference(other.RWcolFloatVis);
	RWcolModelVis.reference(other.RWcolModelVis);
	RWcolCorrVis.reference(other.RWcolCorrVis);
	RWcolWeight.reference(other.RWcolWeight);
        RWcolWeightSpectrum.reference(other.RWcolWeightSpectrum);
	RWcolSigma.reference(other.RWcolSigma);
    }
    return *this;
}

VisIterator & VisIterator::operator++(int)
{
  if (!more_p) return *this;
  advance();
  return *this;
}

VisIterator & VisIterator::operator++()
{
  if (!more_p) return *this;
  advance();
  return *this;
}

void VisIterator::attachColumns(const Table &t)
{
  ROVisibilityIterator::attachColumns(t);
  //todo: should cache this (update once per ms)
  const ColumnDescSet& cds=t.tableDesc().columnDescSet();
  if (cds.isDefined(MS::columnName(MS::DATA))) {
      RWcolVis.attach(t,MS::columnName(MS::DATA));
  };
  if (cds.isDefined(MS::columnName(MS::FLOAT_DATA))) {
    floatDataFound_p=True;
    RWcolFloatVis.attach(t,MS::columnName(MS::FLOAT_DATA));
  } else {
    floatDataFound_p=False;
  };
  if (cds.isDefined("MODEL_DATA")) 
    RWcolModelVis.attach(t,"MODEL_DATA");
  if (cds.isDefined("CORRECTED_DATA")) 
    RWcolCorrVis.attach(t,"CORRECTED_DATA");
  RWcolWeight.attach(t,MS::columnName(MS::WEIGHT));
  if (cds.isDefined("WEIGHT_SPECTRUM"))
    RWcolWeightSpectrum.attach(t,"WEIGHT_SPECTRUM");
  RWcolSigma.attach(t,MS::columnName(MS::SIGMA));
  RWcolFlag.attach(t,MS::columnName(MS::FLAG));
  RWcolFlagRow.attach(t,MS::columnName(MS::FLAG_ROW));
}

void VisIterator::setFlagRow(const Vector<Bool>& rowflags)
{
  RWcolFlagRow.putColumn(rowflags);
};

void VisIterator::setVis(const Cube<Complex>& vis, DataColumn whichOne)
{
  
  if (useNewSlicer_p) putDataColumn(whichOne,newSlicer_p,vis);
  else putDataColumn(whichOne,vis);

}

void VisIterator::setFlag(const Cube<Bool>& flags)
{
  if (useNewSlicer_p) RWcolFlag.putColumn(newSlicer_p,flags);
  else RWcolFlag.putColumn(flags);
}

void VisIterator::setVisAndFlag(const Cube<Complex>& vis,
				       const Cube<Bool>& flag,
				       DataColumn whichOne)
{
  this->setFlag(flag);
  this->setVis(vis,whichOne);
}

void VisIterator::setWeightMat(const Matrix<Float>& weightMat)
{
  if (useNewSlicer_p) RWcolWeight.putColumn(newWtSlicer_p,weightMat);
  else RWcolWeight.putColumn(weightMat);
}

void VisIterator::setWeightSpectrum(const Cube<Float>& weightSpectrum)
{
  if (!colWeightSpectrum.isNull()) {
    if (useNewSlicer_p) RWcolWeightSpectrum.putColumn(newSlicer_p,weightSpectrum);
    else RWcolWeightSpectrum.putColumn(weightSpectrum);
  }
  else 
    throw(AipsError("Can't set WEIGHT_SPECTRUM -- it doesn't exist!"));
}



void VisIterator::putDataColumn(DataColumn whichOne,
				const Vector<Vector<Slice> >& slices,
				const Cube<Complex>& data)
{
  // Set the visibility (observed, model or corrected);
  // deal with DATA and FLOAT_DATA seamlessly for observed data.
  switch (whichOne) {
  case Observed:
    if (floatDataFound_p) {
      Cube<Float> dataFloat=real(data);
      RWcolFloatVis.putColumn(slices,dataFloat);
    } else {
      RWcolVis.putColumn(slices,data);
    };
    break;
  case Corrected:
    RWcolCorrVis.putColumn(slices,data);
    break;
  case Model:
    RWcolModelVis.putColumn(slices,data);
    break;
  };
};  

void VisIterator::putDataColumn(DataColumn whichOne,
				const Cube<Complex>& data)
{
  // Set the visibility (observed, model or corrected);
  // deal with DATA and FLOAT_DATA seamlessly for observed data.
  switch (whichOne) {
  case Observed:
    if (floatDataFound_p) {
      Cube<Float> dataFloat=real(data);
      RWcolFloatVis.putColumn(dataFloat);
    } else {
      RWcolVis.putColumn(data);
    };
    break;
  case Corrected:
    RWcolCorrVis.putColumn(data);
    break;
  case Model:
    RWcolModelVis.putColumn(data);
    break;
  };
};  

Vector<uInt>& ROVisIterator::rowIds(Vector<uInt>& rowids) const
{
  rowids.resize(curNumRow_p);
  rowids=selTable_p.rowNumbers();
  return rowids;
}

void ROVisIterator::setTileCache()
{
  // Set the cache size when the DDID changes (as opposed to MS) to avoid 
  // overreading in a case like:
  // hcubes: [2, 256], [4, 64]
  // tileshape: [4, 64]
  // spws (ddids): [4,64], [2, 256], [2, 256], [4,64], [2, 256], [4,64]
  // and spw = '0:1~7,1:1~7,2:100~200,3:20~40,4:200~230,5:40~50'
  //
  // For hypercube 0, spw 2 needs 3 tiles, but spw 4 only needs 1, AND the last
  // tile at that.  So if hypercube 0 used a cache of 3 tiles throughout, every
  // read of 4:200~230 would likely also read the unwanted channels 0~127 of
  // the next row.
  //
  if(!curStartRow_p == 0 && !msIter_p.newDataDescriptionId())
    return;
  const MeasurementSet& thems = msIter_p.ms();
  if(thems.tableType() == Table::Memory)
    return;
  const ColumnDescSet& cds=thems.tableDesc().columnDescSet();

  // Get the first row number for this DDID.
  Vector<uInt> rownums;
  rowIds(rownums);
  uInt startrow = rownums[0];
  
  Vector<String> columns(8);
  // complex
  columns(0)=MS::columnName(MS::DATA);
  columns(1)=MS::columnName(MS::CORRECTED_DATA);
  columns(2)=MS::columnName(MS::MODEL_DATA);
  // boolean
  columns(3)=MS::columnName(MS::FLAG);
  // float
  columns(4)=MS::columnName(MS::WEIGHT_SPECTRUM);
  columns(5)=MS::columnName(MS::WEIGHT);
  columns(6)=MS::columnName(MS::SIGMA);
  // double
  columns(7)=MS::columnName(MS::UVW);
  //
  for(uInt k = 0; k < columns.nelements(); ++k){
    if(cds.isDefined(columns(k))){
      const ColumnDesc& cdesc = cds[columns(k)];
      String dataManType="";
      
      dataManType = cdesc.dataManagerType();
      // We have to check WEIGHT_SPECTRUM as it tends to exist but not have
      // valid data.
      if(columns[k] == MS::columnName(MS::WEIGHT_SPECTRUM) &&
         !existsWeightSpectrum())
        dataManType="";

      // Sometimes columns may not contain anything yet
      if((columns[k]==MS::columnName(MS::DATA) && (colVis.isNull() ||
                                                   !colVis.isDefined(0))) || 
         (columns[k]==MS::columnName(MS::MODEL_DATA) && (colModelVis.isNull() ||
                                                         !colModelVis.isDefined(0))) ||
         (columns[k]==MS::columnName(MS::CORRECTED_DATA) && (colCorrVis.isNull() ||
                                                             !colCorrVis.isDefined(0))) ||
         (columns[k]==MS::columnName(MS::FLAG) && (colFlag.isNull() ||
                                                   !colFlag.isDefined(0))) ||
         (columns[k]==MS::columnName(MS::WEIGHT) && (colWeight.isNull() ||
                                                     !colWeight.isDefined(0))) ||
         (columns[k]==MS::columnName(MS::SIGMA) && (colSigma.isNull() ||
                                                    !colSigma.isDefined(0))) ||
         (columns[k]==MS::columnName(MS::UVW) && (colUVW.isNull() ||
                                                  !colUVW.isDefined(0))) ){
        dataManType="";
      }
          
      if(dataManType.contains("Tiled") &&
         !String(cdesc.dataManagerGroup()).empty()){
        try {      
          ROTiledStManAccessor tacc=ROTiledStManAccessor(thems, 
                                                         cdesc.dataManagerGroup());

          // This is for the data columns, WEIGHT_SPECTRUM and FLAG only.
          if((columns[k] != MS::columnName(MS::WEIGHT)) && 
             (columns[k] != MS::columnName(MS::UVW))){
            // Figure out how many tiles are needed to span the selected channels.
            const IPosition tileShape(tacc.tileShape(startrow));
            Vector<Int> ids;
            chanIds(ids);
            uInt startTile = ids[0] / tileShape[1];
            uInt endTile = ids[ids.nelements() - 1] / tileShape[1];
            uInt cachesize = endTile - startTile + 1;

            // and the selected correlations.
            corrIds(ids);
            startTile = ids[0] / tileShape[0];
            endTile = ids[ids.nelements() - 1] / tileShape[0];
            cachesize *= endTile - startTile + 1;

            // Safer until I know which of correlations and channels varies faster on
            // disk.
            const IPosition hShape(tacc.hypercubeShape(startrow));
            cachesize = hShape[0] * hShape[1] / (tileShape[0] * tileShape[1]);

            tacc.setCacheSize(startrow, cachesize);
          }
          else
            tacc.setCacheSize(startrow, 1);
        }
        catch (AipsError x) {
          //It failed so leave the caching as is.
          continue;
        }
      }
    }
  }
}

void VisIterator::putCol(ScalarColumn<Bool> &column, const Vector<Bool> &array)
{
    column.putColumn(array);
}

void VisIterator::putCol(ArrayColumn<Bool> &column, const Array<Bool> &array)
{
    column.putColumn(array);
}

void VisIterator::putCol(ArrayColumn<Float> &column, const Array<Float> &array)
{
    column.putColumn(array);
}

void VisIterator::putCol(ArrayColumn<Complex> &column, const Array<Complex> &array)
{
    column.putColumn(array);
}

void VisIterator::putCol(ArrayColumn<Bool> &column, const Slicer &slicer, const Array<Bool> &array)
{
    column.putColumn(slicer, array);
}

void VisIterator::putCol(ArrayColumn<Float> &column, const Slicer &slicer, const Array<Float> &array)
{
    column.putColumn(slicer, array);
}

void VisIterator::putCol(ArrayColumn<Complex> &column, const Slicer &slicer, const Array<Complex> &array)
{
    column.putColumn(slicer, array);
}


} //# NAMESPACE CASA - END

