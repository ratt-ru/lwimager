//# SimACoh.h: Definition for Simulated additive errors
//# Copyright (C) 1996,1997,1999
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
//# $Id: SimACoh.h,v 19.5 2004/11/30 17:50:56 ddebonis Exp $

#ifndef SYNTHESIS_SIMACOH_H
#define SYNTHESIS_SIMACOH_H

#include <casa/aips.h>
#include <casa/BasicMath/Random.h>
#include <synthesis/MeasurementComponents/ACoh.h>

namespace casa { //# NAMESPACE CASA - BEGIN

// <summary> 
// SimACoh: Model additive noise errors for the <linkto class="VisEquation">VisEquation</linkto>
// </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
//   <li> <linkto module="MeasurementComponents">MeasurementComponents</linkto> module
//   <li> <linkto class="VisSet">VisSet</linkto> class
//   <li> <linkto class="VisEquation">VisEquation</linkto> class
// </prerequisite>
//
// <etymology>
// SimACoh describes random additive errors to be used in
// the <linkto class="VisEquation">VisEquation</linkto>. 
// </etymology>
//
// <synopsis> 
//
// See <linkto class="VisEquation">VisEquation</linkto> for definition of the
// Visibility Measurement Equation. See <linkto class="ACoh">ACoh</linkto>
// for how SimACoh is to be used.
// </synopsis> 
//
// <motivation>
// The properties of an additive component must be described
// for the <linkto class="VisEquation">VisEquation</linkto>.
// </motivation>
//
// <todo asof="97/10/01">
// </todo>

class SimACoh  : public ACoh
{
public:
    // Construct from VisSet, seed and rms of additive noise (normal dist.)
    SimACoh(Int seed=1, Double rms=0.1);
 
    // Apply additive error (Inverse is minus)
    // <group>
    virtual VisBuffer& apply(VisBuffer& vb);
    virtual VisBuffer& applyInv(VisBuffer& vb);
    // </group>
private:
    Bool solve(VisEquation& ve) {return True;}
    
    SimACoh();
    MLCG rndGen_p;
    Normal noiseDist_p;
};


} //# NAMESPACE CASA - END

#endif
