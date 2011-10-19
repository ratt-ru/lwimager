//# VisBuffAccumulator.h: class to average VisBuffers in time
//# Copyright (C) 2000,2002
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

#ifndef MSVIS_VISBUFFACCUMULATOR_H
#define MSVIS_VISBUFFACCUMULATOR_H

#include <casa/aips.h>
#include <msvis/MSVis/VisBuffer.h>
#include <msvis/MSVis/CalVisBuffer.h>

namespace casa { //# NAMESPACE CASA - BEGIN

// <summary>
// A class to average VisBuffers in time
// </summary>
//
// <use visibility=export>
//
// <reviewed reviewer="" date="yyyy/mm/dd" tests="" demos="">
// </reviewed>

// <prerequisite>
//   <li> VisBuffer
// </prerequisite>
//
// <etymology>
// From "visibility", "time" and "averaging".
// </etymology>
//
// <synopsis>
// This class averages VisBuffers in time.
// </synopsis>
//
// <example>
// </example>
//
// <motivation>
// Collect all time averaging capabilities for VisBuffer averaging.
// </motivation>
//
// <thrown>
//    <li>
//    <li>
// </thrown>
//
// <todo asof="2000/09/01">
//   <li> averaging over other indices.
// </todo>

class VisBuffAccumulator
{
public:
  // Construct from the number of antennas, the averaging interval and
  // the pre-normalization flag
  VisBuffAccumulator (const Int& nAnt, const Double& interval, 
		      const Bool& prenorm);

  // Null destructor
  ~VisBuffAccumulator();

  // Reset the averager
  void reset();

  // Accumulate a VisBuffer
  void accumulate (const VisBuffer& vb);

  // Finalize averaging, and return the result
  void finalizeAverage();

  // Return a reference to the result
  // TBD: is it ok to return a CVB as a VB reference?  (do I need an
  //      explicit cast here?
  VisBuffer& aveVisBuff() { return avBuf_p; }
  CalVisBuffer& aveCalVisBuff() { return avBuf_p; }

  // Global timestamp info
  Double& timeStamp() { return globalTime_p; };
  Double& timeStampWt() { return globalTimeWt_p; };

  // The number of VisBuffers that have been accumulated.
  uInt nBuf() {return nBuf_p;}

  // Return a map from row numbers in the VisBuffer returned by aveVisBuff() or
  // aveCalVisBuff() to row numbers in the input VisBuffer.  Only useful if
  // nBuf_p == 1 or you are sure that the last input VisBuffer will meet your
  // needs (i.e. all the input VisBuffers had same set of antennas and the
  // metadata you want also matches).  hurl controls whether an exception will
  // be thrown if nBuf() != 1.  Unfilled rows point to -1.
  const Vector<Int>& outToInRow(const Bool hurl=true){
    if(hurl && nBuf_p != 1)
      throw_err("outToInRow", "The output to input row map is unreliable");
    return outToInRow_p;
  }

protected:
  // Averaging buffer
  CalVisBuffer avBuf_p;

  // Number of correlations and channels
  Int nCorr_p, nChan_p;

private:
  // Prohibit null constructor, copy constructor and assignment for now
  VisBuffAccumulator();
  VisBuffAccumulator& operator= (const VisBuffAccumulator&);
  VisBuffAccumulator (const VisBuffAccumulator&);

  // Diagnostic printing level
  Int& prtlev() { return prtlev_; };

  // Initialize the next accumulation interval
  void initialize(const Bool& copydata);

  // Normalize the current accumulation
  void normalize();

  // Hash function to return the row offset for an interferometer (ant1, ant2)
  Int hashFunction (const Int& ant1, const Int& ant2);

  // Shuffle error handling elsewhere in an attempt to let the calling function
  // be efficient and inlinable.
  void throw_err(const String& origin, const String &msg);

  // Number of antennas
  Int nAnt_p;

  // Averaging interval
  Double interval_p;

  // Per-interval timestamp averaging
  Double aveTime_p;
  Double aveTimeWt_p;

  // Global timestamp average
  Double globalTime_p;
  Double globalTimeWt_p;

  // Pre-normalization flag
  Bool prenorm_p;

  // Start time and row of current accumulation
  Double tStart_p;
  Int avrow_p;

  // Flag to mark the first accumulation interval
  Bool firstInterval_p;
  
  // Diagnostic print level
  Int prtlev_;

  // How many VisBuffers have been accumulated.
  uInt nBuf_p;

  // A map from avBuf_p's row numbers to row numbers in the VisBuffer used to
  // fill avBuf_p.  Only useful if nBuf_p == 1.  Unfilled rows point to -1.
  Vector<Int> outToInRow_p;
};


} //# NAMESPACE CASA - END

#endif


