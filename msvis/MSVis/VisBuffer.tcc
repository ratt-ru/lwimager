//# VisBuffer.tcc 
//# Copyright (C) 2011
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//#
//# This library is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//# 
//# You should have received a copy of the GNU General Public License
//# along with this library; if not, write to the Free Software
//# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
#include <msvis/MSVis/VisBuffer.h>
#include <casa/Arrays/Cube.h>
#include <cmath>

namespace casa { //# NAMESPACE CASA - BEGIN
template<class T>
void VisBuffer::chanAveVisCube(Cube<T>& data, Int nChanOut)
{
  IPosition csh(data.shape());
  Int nChan0 = csh(1);

  if(nChan0 < nChanOut)
    // It's possible that data has already been averaged.  I could try
    // refilling data if I knew which column to use, but I don't.
    // Chuck it to the caller.
    throw(AipsError("Can't average " + String(nChan0) + " channels to " +
                    String(nChanOut) + " channels!"));
  if(nChan0 == nChanOut)
    return;                     // No-op.

  csh(1) = nChanOut;

  Vector<Int>& chans(channel());
  Cube<T> newCube(csh);
  newCube = T(0.0);
  Int nCor = nCorr();
  Int ichan(0);

  const Bool doSpWt(visIter_p->existsWeightSpectrum());

  // Make sure weightSpectrum() is unaveraged.
  if(doSpWt && weightSpectrum().shape()(1) < nChan0)
    fillWeightSpectrum();

  Vector<Double> totwt(nCor);
  for(Int row = 0; row < nRow(); ++row){
    if(!flagRow()(row)){
      ichan = 0;
      for(Int ochan = 0; ochan < nChanOut; ++ochan){
	totwt = 0;
	while(chans[ichan] >= chanAveBounds_p(ochan, 0) &&
              chans[ichan] <= chanAveBounds_p(ochan, 1) &&
              ichan < nChan0){
	  for(Int icor = 0; icor < nCor; ++icor){
	    if(!flagCube()(icor, ichan, row)){
              Double wt = 1.0;
              
              if(doSpWt){ 
                wt = weightSpectrum()(icor, ichan, row);
                newCube(icor, ochan, row) += wt * data(icor, ichan, row);
              }
              else      // Mathematically equiv. but cheaper.
                newCube(icor, ochan, row) += data(icor, ichan, row);

	      totwt[icor] += wt;
	    }
          }
	  ++ichan;
	}
	for(Int icor = 0; icor < nCor; ++icor){
	  if(totwt[icor] > 0.0)
	    //newCube(icor, ochan, row) *= T(1.0 / totwt[icor]);
	    newCube(icor, ochan, row) *= 1.0 / totwt[icor];
	}
      }
    }
  }
  data.reference(newCube);        // Install averaged info
}

template<class T>
void VisBuffer::chanAccCube(Cube<T>& cube, Int nChanOut)
{
  IPosition csh(cube.shape());
  Int nChan0 = csh(1);
  csh(1) = nChanOut;

  if(nChan0 < nChanOut)
    // It's possible that cube has already been squeezed.  I could try
    // refilling data if I knew which column to use, but I don't.
    // Chuck it to the caller.
    throw(AipsError("Can't accumulate " + String(nChan0) + " channels to " +
                    String(nChanOut) + " channels!"));
  if(nChan0 == nChanOut)
    return;                     // No-op.

  Vector<Int>& chans(channel());
  Cube<T> newCube(csh);
  newCube = T(0.0);
  Int nCor = nCorr();
  Int ichan(0);

  for(Int row = 0; row < nRow(); ++row){
    if(!flagRow()(row)){
      ichan = 0;
      for(Int ochan = 0; ochan < nChanOut; ++ochan){
	while(chans[ichan] >= chanAveBounds_p(ochan, 0) &&
              chans[ichan] <= chanAveBounds_p(ochan, 1) &&
              ichan < nChan0){
	  for(Int icor = 0; icor < nCor; ++icor)
	    if(!flagCube()(icor, ichan, row))
              newCube(icor, ochan, row) += cube(icor, ichan, row);
	  ++ichan;
	}
      }
    }
  }
  cube.reference(newCube);        // Install accumulated cube
}

} //# NAMESPACE CASA - END


