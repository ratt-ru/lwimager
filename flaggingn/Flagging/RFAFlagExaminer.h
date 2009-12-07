
//# RFAFlagExaminer.h: this defines RFAFlagExaminer
//# Copyright (C) 2000,2001
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
#ifndef FLAGGING_RFAFLAGEXAMINER_H
#define FLAGGING_RFAFLAGEXAMINER_H

#include <flagging/Flagging/RFAFlagCubeBase.h> 
#include <flagging/Flagging/RFASelector.h> 
#include <flagging/Flagging/RFDataMapper.h>
#include <casa/Arrays/LogiVector.h>
    
namespace casa { //# NAMESPACE CASA - BEGIN

// <summary>
// RFAFlagExaminer: Examine the flags and get statistics. perform extensions too.
// </summary>

// <use visibility=local>

// <reviewed reviewer="" date="" tests="" demos="">
// </reviewed>

// <prerequisite>
//   <li> RFASelector
// </prerequisite>
//
// <etymology>
// RedFlaggerAgent Selector
// </etymology>
//
// <synopsis>
// RFAFlagExaminer accepts a whole bunch of options to select a subset of the
// MS (by time, antenna, baseline, channel/frequency, etc.), and to flag/unflag 
// the whole selection, or specific parts of it (autocorrelations, specific 
// time slots, VLA quacks, etc.)
// </synopsis>
//
// <todo asof="2001/04/16">
//   <li> add this feature
//   <li> fix this bug
//   <li> start discussion of this possible extension
// </todo>

class RFAFlagExaminer : public RFASelector
{
public:
// constructor. 
  RFAFlagExaminer ( RFChunkStats &ch,const RecordInterface &parm ); 
  virtual ~RFAFlagExaminer ();
  
  /*
  virtual uInt estimateMemoryUse () { return RFAFlagCubeBase::estimateMemoryUse()+2; }
  virtual Bool newChunk ( Int &maxmem );
//  virtual IterMode iterTime ( uInt it );
//  virtual IterMode iterRow  ( uInt ir );
*/
  virtual void iterFlag ( uInt it );

  virtual void startData(){RFAFlagCubeBase::startData();return;};
  virtual void startFlag();
    //virtual void iterFlag( uInt it , Bool resetFlags=True);
  virtual void endFlag();
  virtual void finalize();
  virtual void initialize();
  virtual void initializeIter(uInt it);
  virtual void finalizeIter(uInt it);
  virtual String getID() {return String("FlagExaminer");};

  virtual Record getResult();

//  virtual String getDesc ();
//  static const RecordInterface & getDefaults ();

protected:
    void processRow  ( uInt ifr,uInt it ) ;
    Int totalflags,totalcount;
    Int totalrowflags,totalrowcount;

    // accumulated over all chunks
    Int 
      accumTotalFlags, accumTotalCount, accumRowFlags, 
      accumTotalRowCount, accumTotalRowFlags;

    Int inTotalFlags, inTotalCount, inTotalRowFlags, inTotalRowCount;
    Int outTotalFlags, outTotalCount, outTotalRowFlags, outTotalRowCount;
};

    
    

} //# NAMESPACE CASA - END

#endif
