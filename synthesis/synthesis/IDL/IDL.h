//# Pixon.h: Definition for Pixon
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
//# $Id: IDL.h,v 19.5 2004/11/30 17:50:47 ddebonis Exp $

#ifndef SYNTHESIS_IDL_H
#define SYNTHESIS_IDL_H

#include <casa/Arrays/Vector.h>
#include <casa/BasicSL/String.h>

#ifdef HAVE_IDL_LIB
#include <casa/stdio.h>
#include "export.h"
#endif

namespace casa { //# NAMESPACE CASA - BEGIN

// <summary> A class to start IDL and execute commands, etc.
// </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
//   <li> IDL libraries
// </prerequisite>
//
// <etymology>
// IDL is the class for running IDL from inside AIPS++
// </etymology>
//
// <synopsis> 
// We can start IDL, execute commands, send and retreive named
// arrays, etc using this class. The initial use of this class
// is to allow Pixon processing to be performed using the IDL
// code.
// </synopsis> 
//
// <example>
// </example>
//
// <motivation>
// Get access to IDL functionality
// </motivation>
//
// <todo asof="01/03/03">
// <ul> Add more interface functions
// <ul> Turn into DO
// </todo>

class IDL  {
public:

  // The constructor will start IDL.
  IDL(Int options=0);

  // Copy constructor
  IDL(const IDL &other);

  // Assignment operator
  IDL &operator=(const IDL &other);

  // The destructor will stop IDL.
  ~IDL();

  // Run a sequence of IDL commands
  Bool runCommands(const Vector<String>& commands, Bool log=True);

  // Run a single IDL command
  Bool runCommand(const String& command, Bool log=True);

  // Send an array to IDL and name it
  Bool sendArray(const Array<Float>& a, String aname);

  // Get an array from IDL
  Array<Float> getArray(String aname);

  // Get an scalar from IDL
  Float getFloat(String aname);

  // Get an scalar from IDL
  Int getInt(String aname);

  // Set the path inside IDL
  Bool setPath(const Vector<String>&);

protected:

  char* getIDLName(const String);

#ifdef HAVE_IDL_LIB
  IDL_VPTR getPointer(const String);
#endif

  void ok();

};

} //# NAMESPACE CASA - END

#endif
