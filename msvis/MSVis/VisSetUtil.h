//# VisSetUtil.h: Definitions for Stokes Image utilities
//# Copyright (C) 1996,1997,1998
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
//# $Id$

#ifndef MSVIS_VISSETUTIL_H
#define MSVIS_VISSETUTIL_H

#include <casa/aips.h>
#include <casa/BasicSL/Complex.h>
#include <casa/Quanta/Quantum.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <msvis/MSVis/VisSet.h>
#include <msvis/MSVis/VisibilityIterator.h>

namespace casa { //# NAMESPACE CASA - BEGIN

// <summary> 
// Utilities for operating on VisSets.
// </summary>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
// </prerequisite>
//
// <etymology>
// </etymology>
//
// <synopsis> 
// </synopsis> 
//
// <example>
// <srcblock>
// </srcblock>
// </example>
//
// <motivation>
// </motivation>
//
// <todo asof="">
// </todo>


class VisSetUtil {
  
public:
  // Calculate sensitivity
  static void Sensitivity(VisSet &vs, Quantity& pointsourcesens, Double& relativesens,
			  Double& sumwt);
  static void Sensitivity(ROVisibilityIterator &vi, Quantity& pointsourcesens, 
			  Double& relativesens,
			  Double& sumwt);
  // Hanning smoothing of spectral channels
  static void HanningSmooth(VisSet &vs, const String& dataCol="corrected", 
			    const Bool& doFlagAndWeight=True);
  static void HanningSmooth(VisibilityIterator &vi, const String& dataCol="corrected",
			    const Bool& doFlagAndWeight=True);
  // Subtract/add model from/to corrected visibility data
  static void UVSub(VisSet &vs, Bool reverse=False);
  static void UVSub(VisibilityIterator &vs, Bool reverse=False);
};

} //# NAMESPACE CASA - END

#endif


