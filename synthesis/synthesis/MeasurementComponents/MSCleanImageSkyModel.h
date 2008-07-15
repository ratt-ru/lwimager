//# MSCleanImageSkyModel.h: Definition for MSCleanImageSkyModel
//# Copyright (C) 1996,1997,1998,1999,2000
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
//# Correspondence concerning AIPS++ should be adressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//#
//# $Id: MSCleanImageSkyModel.h,v 19.4 2004/11/30 17:50:48 ddebonis Exp $

#ifndef SYNTHESIS_MSCLEANIMAGESKYMODEL_H
#define SYNTHESIS_MSCLEANIMAGESKYMODEL_H

#include <synthesis/MeasurementComponents/CleanImageSkyModel.h>
#include <lattices/Lattices/LatticeCleanProgress.h>

namespace casa { //# NAMESPACE CASA - BEGIN

//forward
class SkyEquation;

// <summary> 
// MS Clean Image Sky Model: Image Sky Model implementing the MS Clean algorithm
// </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
//   <li> <linkto class=LatticeCleaner>LatticeCleaner</linkto> module
//   <li> <linkto class=ImageSkyModel>ImageSkyModel</linkto> module
//   <li> <linkto class=LinearModel>LinearModel</linkto> module
// </prerequisite>
//
// <etymology>
// MSCleanImageSkyModel implements the MS Clean algorithm.
// It is derived from <linkto class=SkyModel>SkyModel</linkto>.
// </etymology>
//
// <synopsis> 
// The MS Clean is the Multi-Scale clean, collecting flux
// in several different scale size Gaussian components.
// It is highly effective in imaging extended structure
// and results in residuals with excellent statistics.
//
// The MS Clean is implemented using the 
// <linkto class=LatticeCleaner>LatticeCleaner</linkto>
// class.
//
// Masking is optionally performed using a mask image: only points
// where the mask is non-zero are searched for Gaussian components. 
// This can cause some difficulty, as the different Gaussian scale
// sizes will extend beyond the mask by different amounts.
// If no mask is specified
// all points in the inner quarter of the image are cleaned.
// </synopsis> 
//
// <example>
// See the example for <linkto class=SkyModel>SkyModel</linkto>.
// </example>
//
// <motivation>
// </motivation>
//
// <todo asof="99/04/07">
// <ul> Improve the masking.
// </todo>

class MSCleanImageSkyModel : public CleanImageSkyModel {
public:

  // Create a MSCleanImageSkyModel with nScales, we figure out what they are
  MSCleanImageSkyModel(const Int nscales);

  // Create a MSCleanImageSkyModel, you provide the scale sizes, IN PIXELS
  // for example:  Vector<Float> scales(4); scales(0) = 0.0;  scales(1) = 3.0;  
  // scales(2) = 10.0;  scales(3) = 30.0; 
  MSCleanImageSkyModel(const Vector<Float>& useScaleSize);

  // destructor
  ~MSCleanImageSkyModel();

  // Solve for this SkyModel
  virtual Bool solve (SkyEquation& me);

private:
  enum Scale_Method{NSCALES, USERVECTOR};
  Scale_Method method_p;

  Int nscales_p;
  Vector<Float> userScaleSizes_p;

  LatticeCleanProgress  *progress_p;

};


} //# NAMESPACE CASA - END

#endif


