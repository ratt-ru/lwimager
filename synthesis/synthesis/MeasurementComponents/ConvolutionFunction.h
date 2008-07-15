//# ConvolutionFunction.h: Definition for ConvolutionFunction
//# Copyright (C) 1996,1997,1998,1999,2000,2002
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
//# $Id: ConvolutionFunction.h,v 1.1 2005/05/29 04:54:06 sbhatnag Exp $

#ifndef SYNTHESIS_CONVOLUTIONFUNCTION_H
#define SYNTHESIS_CONVOLUTIONFUNCTION_H

#include <casa/Arrays/Vector.h>
#define CF_TYPE Double

namespace casa{

  class ConvolutionFunction
  {
  public:
    ConvolutionFunction() {};
    ConvolutionFunction(Int dim) {nDim=dim;};
    virtual ~ConvolutionFunction() {};
    
    virtual void setDimension(Int n){nDim = n;};
    virtual CF_TYPE getValue(Vector<CF_TYPE>& coord, Vector<CF_TYPE>& offset) {return 0;};
  private:
    Int nDim;
  };

};

#endif
