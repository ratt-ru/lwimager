//# EPointDesc.h: Define the format of EPointJones cal tables
//# Copyright (C) 1996,1997,1998,2001,2003
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
//# $Id: EPointDesc.h,v 1.1 2005/05/29 04:20:42 sbhatnag Exp $

#ifndef CALIBRATION_EPOINTDESC_H
#define CALIBRATION_EPOINTDESC_H

#include <calibration/CalTables/TimeVarVJDesc.h>
#include <calibration/CalTables/SolvableVJDesc.h>
namespace casa {
// <summary> 
// EPointDesc: Define the format of GJones cal tables
// </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
//<li><linkto class="CalTableDesc">CalTableDesc</linkto> module
//<li><linkto class="VisJonesDesc">VisJonesDesc</linkto> module
//<li><linkto class="SolvableVisJonesDesc">SolvableVisJonesDesc</linkto> module
// </prerequisite>
//
// <etymology>
// From "GJones" and "descriptor".
// </etymology>
//
// <synopsis>
// GJonesDesc defines the format of GJones calibration tables in terms 
// of table descriptors <linkto class="TableDesc">TableDesc</linkto>, 
// as used in the Table system. GJones matrices define the electronic
// gain corrections in the Measurement Equation formalism. At present, 
// this class returns only the default table formats.
// </etymology>
//
// <example>
// <srcblock>
// </srcblock>
// </example>
//
// <motivation>
// See CalTableDesc.
// </motivation>
//
// <todo asof="98/01/01">
// Generate calibration table descriptors from a VisSet or VisBuffer object
// </todo>

class EPointDesc : public SolvableVisJonesDesc
{
public:
  // Default null constructor, and destructor
  EPointDesc();
  // Construct from the Jones table type
  EPointDesc (const String& type);

  virtual ~EPointDesc() {};

  // Return the table descriptors for the main calibration table:
  virtual TableDesc calMainDesc();
  
  TableDesc addCols(TableDesc& td);

protected:
  TableDesc itsTableDesc;
};
}
#endif
