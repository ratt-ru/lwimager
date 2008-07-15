//# SDDataSampling.h: Definition for SDDataSampling
//# Copyright (C) 1996,1997,1998,1999,2000,2001
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
//# $Id: SDDataSampling.h,v 19.4 2004/11/30 17:50:47 ddebonis Exp $

#ifndef SYNTHESIS_SDDATASAMPLING_H
#define SYNTHESIS_SDDATASAMPLING_H

#include <synthesis/DataSampling/DataSampling.h>
#include <synthesis/MeasurementComponents/SkyJones.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>

namespace casa { //# NAMESPACE CASA - BEGIN

class ROMSPointingColumns;

// <summary> Provides sampling of single dish data for esimation algorithms
// </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
// </prerequisite>
//
// <etymology>
// Samples single dish Data as needed for various estimation algorithms
// </etymology>
//
// <synopsis> 
// Esimation algorithms such as the Pixon method need sampled
// and unitless versions of the data. This class is derived from
// DataSampling. It adds methods specific to single dish data.
// </synopsis> 
//
// <example>
// </example>
//
// <motivation>
// Define an interface to allow efficient processing of chunks of 
// data
// </motivation>
//
// <todo asof="01/03/03">
// <ul> Derive more examples
// </todo>

class SDDataSampling : public DataSampling {
public:

  SDDataSampling(MeasurementSet& ms, SkyJones& sj,
		 const CoordinateSystem& coords,
		 const IPosition& shape,
		 const Quantity& sigma);

  // Copy constructor
  SDDataSampling(const SDDataSampling &other);

  // Assignment operator
  SDDataSampling &operator=(const SDDataSampling &other);

  ~SDDataSampling();

private:

  Int lastIndex_p;

  Int nRows_p;

  Int getIndex(const ROMSPointingColumns& mspc, const Double& time);

  void ok();

};

} //# NAMESPACE CASA - END

#endif
