//# Exp.cc: Implementation of Exp (tabulated exponential) class
//# Copyright (C) 1997,1998,1999,2000,2001,2002,2003
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
//# $Id: Exp.h,v 1.2 2006/01/11 14:22:43 gvandiep Exp $

#if !defined(EXP_H)
#define EXP_H

#include <stdlib.h>
#include <math.h>

namespace casa
{
template<class T> class Exp
{
public:
  Exp<T>() {EStep=0; ETable=NULL;Size=0;};
  Exp<T>(int n, T Step) {EStep=Size=0;ETable=NULL;Build(n,Step);};
  ~Exp<T>() {if (ETable) free(ETable);};

  inline void build(int n, T Step)
  {
    if (ETable) free(ETable);

    ETable=(T *)malloc(sizeof(T)*n);
    Size = n;
    EStep = Step;

    for (int i=0;i<n;i++) ETable[i]=exp(-i*Step);
  }
  inline T operator()(T arg) 
  {
    int N=(int)(-arg/EStep); 

    //    return (fabs(N)>=Size)?0:((ETable[N]-ETable[N+1])*arg + ETable[N]);

    return (abs(N)>=Size)?0:ETable[N];
  }
private:
  T EStep;
  T *ETable;
  int Size;
};
}; 
#endif
