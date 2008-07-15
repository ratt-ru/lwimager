//# BeamSkyJones.h: Definitions of interface for BeamSkyJones 
//# Copyright (C) 1996,1997,1998,1999,2000,2002,2003
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
//# $Id: BeamSkyJones.h,v 19.11 2006/04/07 21:17:25 wyoung Exp $

#ifndef SYNTHESIS_BEAMSKYJONES_H
#define SYNTHESIS_BEAMSKYJONES_H

#include <casa/aips.h>
#include <casa/Containers/Block.h>
#include <casa/Exceptions/Error.h>
#include <measures/Measures/MDirection.h>
#include <measures/Measures.h>
#include <measures/Measures/Stokes.h>
#include <synthesis/MeasurementComponents/SkyJones.h>
#include <synthesis/MeasurementComponents/PBMath.h>


namespace casa { //# NAMESPACE CASA - BEGIN

//#forward
class SkyModel;
class MeasurementSet;
      
//# Need forward declaration for Solve in the Jones Matrices
class SkyEquation;
class ImageRegion;

// <summary> beam-like sky-plane effects for the SkyEquation </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
// <li> <linkto class="SkyEquation">SkyEquation</linkto> class
// <li> <linkto class="SkyJones">SkyJones</linkto> class
// <li> <linkto class="PBMathInterface">PBMathInterface</linkto> class
// </prerequisite>
//
// <etymology>
// BeamSkyJones, derived from SkyJones, describes an interface to
// beam-based SkyJones objects.  Like SkyJones, it too is an Abstract
// Base Class, but implements the beam-related methods.
// </etymology>
//
// <synopsis> 
//
// <motivation>
// The properties of sky-plane based calibration effects must be described
// for the <linkto class="SkyEquation">SkyEquation</linkto>; this class
// encapsulates the antenna beam-based aspects which are present in at least
// a few other specific SkyJones (VPSkyJones and DBeamSkyJones).
// </motivation>
//
// <todo asof="98/11/01">
// <li> Solvable part needs implementation: we need to derive an
// image of gradients of the elements of the Jones matrix. See VisJones
// for how to do this.
// <li> The MS, version II, will have a beam subtable, which will have PBMaths
// for each antenna.  Until this becomes available, we need to do some
// fudging; for example, we 
// </todo>

class BeamSkyJones : virtual public SkyJones {

public:

  // Eventually, the MS will have all the beam information in its Beam Subtable.
  // Till then, we either guess the PB to use or explicitly define it upon construction.
  // Construct from a Measurement Set, figure out the most appropriate PBMath object
  // from MS information
  BeamSkyJones(MeasurementSet &ms, 
	       const Quantity &parallacticAngleIncrement = Quantity(720.0, "deg"), // def= 1 PA interval
	       BeamSquint::SquintType doSquint = BeamSquint::NONE,  // def= no beam squint offsets
	       const Quantity &skyPositionThreshold = Quantity(180,"deg"));  // def= assume there is no change of
	                                                     // this operator due to position offset


  // Virtual destructor (this is a virtual base class)
  virtual ~BeamSkyJones() = 0;

  // Print out information concerning the state of this object
  virtual void showState(LogIO& os);

  // Apply Jones matrix to an image (and adjoint)
  // No "applyInverse" is available from the SkyJones classes,
  // you can get them directly from PBMath or you can get
  // the equivalent effect by dividing by grad grad Chi^2
  // in ImageSkyModel.
  // <group>
  ImageInterface<Complex>& apply(const ImageInterface<Complex>& in,
				 ImageInterface<Complex>& out,
				 const VisBuffer& vb, Int row,
				 Bool forward=True);
  ImageInterface<Float>& applySquare(const ImageInterface<Float>& in,
				     ImageInterface<Float>& out,
				     const VisBuffer& vb, Int row);
  // </group>

  // Apply Jones matrix to a sky component (and adjoint)  
  // <group>
  SkyComponent& apply(SkyComponent& in,
		      SkyComponent& out,
		      const VisBuffer& vb, Int row,
		      Bool forward = True);
  SkyComponent& applySquare(SkyComponent& in,
			    SkyComponent& out,
			    const VisBuffer& vb, Int row);
  // </group>

  // Understand if things have changed since last PB application
  // <group>

  // Reset to pristine state
  virtual void reset();

  // Has this operator changed since the last Application?
  // (or more properly, since the last update() ) 
  virtual Bool changed(const VisBuffer& vb, Int row);

  // Does the operator change in this visbuffer or since the last call?
  // May not be useful -- check it out:  m.a.h. Dec 30 1999
  virtual Bool change(const VisBuffer& vb);

  // Does this operator changed in this VisBuffer,
  // starting with row1?
  // If yes, we return in row2, the last row that has the
  // same SkyJones as row1.
  // NOTE: need to first call changed(const VisBuffer& vb, Int row) and
  // shield the user from the case where the fieldID has changed
  // (which only changes in blocks)
  virtual Bool changedBuffer(const VisBuffer& vb, Int row1, Int& row2);

  // Update the FieldID, Telescope, pointingDirection, Parallactic angle info
  void update(const VisBuffer& vb, Int row);

  // if (changed) {update()}
  virtual void assure (const VisBuffer& vb, Int row);
  // </group>

  // Return the type of this Jones matrix (actual type of derived class).
  virtual Type type() {return SkyJones::E;};

  // Apply gradient
  virtual ImageInterface<Complex>& 
  applyGradient(ImageInterface<Complex>& result, const VisBuffer& vb,
		Int row);

  virtual SkyComponent&
  applyGradient(SkyComponent& result, const VisBuffer& vb,
		Int row);

  // Is this solveable?
  virtual Bool isSolveable() {return False;};

  // Initialize for gradient search
  virtual void initializeGradients();

  // Finalize for gradient search
  virtual void finalizeGradients();
 
  // Add to Gradient Chisq
  virtual void addGradients(const VisBuffer& vb, Int row, const Float sumwt, 
			    const Float chisq, const Matrix<Complex>& c, 
			    const Matrix<Float>& f);
 
  // Solve
  virtual Bool solve (SkyEquation& se);
  
  // Manage the PBMath objects
  // <group>
  // set the PB based on telescope name, antennaID and feedID
  // If antennaID or feedID is -1, the PBMath object is set for
  // all antennae or feeds, respectively. These are the default
  // values to retain the previous interface.
  //
  // Note. It would be nice to change the interface and make antennaID
  // and feedID the second and the third parameter, respectively.
  void setPBMath(const String &telescope, PBMath &myPBmath,
                 const Int &antennaID = -1, const Int &feedID = -1);
  
  // get the PBMath object; returns False if that one doesn't exist,
  // True if it does exist and is OK
  // whichAnt is an index into an array of PBMaths, which is different
  // for all telescope/antenna/feed combinations
  // Not sure why we need such a low-level method declared as public,
  // retained to preserve old interface
  Bool getPBMath(uInt whichAnt, PBMath &myPBMath) const;
  
  // get the PBMath object; returns False if that one doesn't exist,
  // True if it does exist and is OK
  // antennaID and feedID default to -1 to preserve the old interface
  // TODO: change the interface and make antennaID and feedID the
  // second and third parameter respectively to have a better looking code
  Bool getPBMath(const String &telescope, PBMath &myPBMath,
                 const Int &antennaID = -1, const Int &feedID = -1) const;

  Quantity getPAIncrement() {return Quantity(parallacticAngleIncrement_p,"rad");}

  Quantity getSkyPositionThreshold() {return Quantity(skyPositionThreshold_p,"rad");}
  
  // Return true if all antennas share a common VP
  Bool isHomogeneous() const;
  //</group>
  
  // Get the ImageRegion of the primary beam on an Image for a given pointing
  // Note: ImageRegion is not necesarily constrained to lie within the
  // image region (for example, if the pointing center is near the edge of the
  // image).  fPad: extra padding over the primary beam supporrt, 
  // fractional (ie, 1.2 for 20% padding), in all directions.
  // (note: we do not properly treat squint yet, this will cover it for now)
  // iChan: frequency channel to take: lowest frequency channel is safe for all
  //
  // Potential problem: this ImageRegion includes all Stokes and Frequency Channels
  // present in the input image.
  ImageRegion*  extent (const ImageInterface<Complex>& im, 
			const VisBuffer& vb,  
			const Int irow=-1,			
			const Float fPad=1.2,  
			const Int iChan=0, 
			const SkyJones::SizeType sizeType=SkyJones::COMPOSITE);

  ImageRegion*  extent (const ImageInterface<Float>& im, 
			const VisBuffer& vb,  const Int irow=-1,
			const Float fPad=1.2,  const Int iChan=0, 
			const SkyJones::SizeType sizeType=SkyJones::COMPOSITE);

  // summarize the PBMaths contained here.
  // n = -1 => terse table
  // n =  0 => table plus constructor values
  // n =  m => plot m samples of the PB profile
  virtual void summary(Int n=0);

private:  

  MeasurementSet* ms_p;

  String telescope_p;

  Int lastFieldId_p;

  Int lastArrayId_p;

  Int lastMSId_p;

  BeamSquint::SquintType doSquint_p;

  Double  parallacticAngleIncrement_p; // a parallactic angle threshold
                        // beyond which the operator is considered to be
			// changed (in radians)
  Double  skyPositionThreshold_p;     // a sky position threshold beyond
                        // which the operator is considered to be changed
			// (in radians)
  Block<Double> lastParallacticAngles_p; // a cache of parallactic angles
                        // used when the operator was applied last time.
			// One value in radians for each beam model in PBMaths.
			// A zero-length block means that the operator
			// has never been applied
  Block<MDirection> lastDirections_p; // a chache of directions
                        // used when the operator was applied last time.
			// One element for each beam model in PBMaths.
			// A zero-length block means that the operator
			// has never been applied

  // One or more PBMaths (a common one for the
  // entire array, or one for each antenna)
  // This requires some sorting out for heterogeneous arrays!
  Block<PBMath> myPBMaths_p;  
  // Names of telescopes (parralel with PBMaths
  Block<String> myTelescopes_p;

  // Antenna IDs (parallel with PBMaths)
  Block<Int> myAntennaIDs_p;
  // Feed IDs (parallel with PBMaths)
  Block<Int> myFeedIDs_p;

  // cache of the indices to the PBMaths container for antenna/feed 1 and 2  
  mutable const VisBuffer *lastUpdateVisBuffer_p; // to ensure that the cache
                                          // is filled for the correct
					  // VisBuffer. The value is used
					  // for comparison only
  mutable Int lastUpdateRow_p;  // to ensure that the cache is filled for
                                // correct row in the VisBuffer
  mutable Int lastUpdateIndex1_p; // index of the first antenna/feed
  mutable Int lastUpdateIndex2_p; // index of the second antenna/feed
  //

  mutable Bool hasBeenApplied;  // True if the operator has been applied at least once

  // update the indices cache. This method could be made protected in the
  // future (need access functions for lastUpdateIndex?_p to benefit from it)
  // Cache will be valid for a given VisBuffer and row
  void updatePBMathIndices(const VisBuffer &vb, Int row) const;

protected:
  // return True if two directions are close enough to consider the
  // operator unchanged, False otherwise
  Bool directionsCloseEnough(const MDirection &dir1,
                             const MDirection &dir2) const throw(AipsError);
  			     
  // return index of compareTelescope, compareAntenna and compareFeed in
  // myTelescopes_p, myAntennaIDs and myFeedIDs; -1 if not found
  // if compareAntenna or compareTelescope is -1, this means that the
  // PBMath class is the same for all antennae/feeds. An exception will
  // be raised, if separate PBMath objects have been assigned by setPBMath
  // for different feeds/antennae but -1 is used for query.
  //
  // It would be good to rename this function to indexBeams as this name
  // is more appropriate. 
  //
  Int indexTelescope(const String & compareTelescope,
                     const Int &compareAntenna=-1,
		     const Int &compareFeed=-1) const;

};
 

} //# NAMESPACE CASA - END

#endif

