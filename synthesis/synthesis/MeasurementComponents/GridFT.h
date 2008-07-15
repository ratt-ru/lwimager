//# GridFT.h: Definition for GridFT
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
//# $Id: GridFT.h,v 19.12 2005/09/06 20:12:09 kgolap Exp $

#ifndef SYNTHESIS_GRIDFT_H
#define SYNTHESIS_GRIDFT_H

#include <synthesis/MeasurementComponents/FTMachine.h>
#include <casa/Arrays/Matrix.h>
#include <scimath/Mathematics/FFTServer.h>
#include <msvis/MSVis/VisBuffer.h>
#include <images/Images/ImageInterface.h>
#include <images/Images/ImageInterface.h>
#include <casa/Containers/Block.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <scimath/Mathematics/ConvolveGridder.h>
#include <lattices/Lattices/LatticeCache.h>
#include <lattices/Lattices/ArrayLattice.h>


namespace casa { //# NAMESPACE CASA - BEGIN

class UVWMachine;
// <summary>  An FTMachine for Gridded Fourier transforms </summary>

// <use visibility=export>

// <reviewed reviewer="" date="" tests="" demos="">

// <prerequisite>
//   <li> <linkto class=FTMachine>FTMachine</linkto> module
//   <li> <linkto class=SkyEquation>SkyEquation</linkto> module
//   <li> <linkto class=VisBuffer>VisBuffer</linkto> module
// </prerequisite>
//
// <etymology>
// FTMachine is a Machine for Fourier Transforms. GridFT does
// Grid-based Fourier transforms.
// </etymology>
//
// <synopsis> 
// The <linkto class=SkyEquation>SkyEquation</linkto> needs to be able
// to perform Fourier transforms on visibility data. GridFT
// allows efficient Fourier Transform processing using a 
// <linkto class=VisBuffer>VisBuffer</linkto> which encapsulates
// a chunk of visibility (typically all baselines for one time)
// together with all the information needed for processing
// (e.g. UVW coordinates).
//
// Gridding and degridding in GridFT are performed using a
// novel sort-less algorithm. In this approach, the gridded plane is
// divided into small patches, a cache of which is maintained in memory
// using a general-purpose <linkto class=LatticeCache>LatticeCache</linkto> class. As the (time-sorted)
// visibility data move around slowly in the Fourier plane, patches are
// swapped in and out as necessary. Thus, optimally, one would keep at
// least one patch per baseline.  
//
// A grid cache is defined on construction. If the gridded uv plane is smaller
// than this, it is kept entirely in memory and all gridding and
// degridding is done entirely in memory. Otherwise a cache of tiles is
// kept an paged in and out as necessary. Optimally the cache should be
// big enough to hold all polarizations and frequencies for all
// baselines. The paging rate will then be small. As the cache size is
// reduced below this critical value, paging increases. The algorithm will
// work for only one patch but it will be very slow!
//
// This scheme works well for arrays having a moderate number of
// antennas since the saving in space goes as the ratio of
// baselines to image size. For the ATCA, VLBA and WSRT, this ratio is
// quite favorable. For the VLA, one requires images of greater than
// about 200 pixels on a side to make it worthwhile.
//
// The FFT step is done plane by plane for images having less than
// 1024 * 1024 pixels on each plane, and line by line otherwise.
//
// The gridding and degridding steps are implemented in Fortran
// for speed. In gridding, the visibilities are added onto the
// grid points in the neighborhood using a weighting function.
// In degridding, the value is derived by a weight summ of the
// same points, using the same weighting function.
// </synopsis> 
//
// <example>
// See the example for <linkto class=SkyModel>SkyModel</linkto>.
// </example>
//
// <motivation>
// Define an interface to allow efficient processing of chunks of 
// visibility data
// </motivation>
//
// <todo asof="97/10/01">
// <ul> Deal with large VLA spectral line case 
// </todo>

class GridFT : public FTMachine {
public:

  // Constructor: cachesize is the size of the cache in words
  // (e.g. a few million is a good number), tilesize is the
  // size of the tile used in gridding (cannot be less than
  // 12, 16 works in most cases), and convType is the type of
  // gridding used (SF is prolate spheriodal wavefunction,
  // and BOX is plain box-car summation). mLocation is
  // the position to be used in some phase rotations. If
  // mTangent is specified then the uvw rotation is done for
  // that location iso the image center.
  // <group>
  GridFT(Long cachesize, Int tilesize, String convType="SF",
	 Float padding=1.0, Bool usezero=True);
  GridFT(Long cachesize, Int tilesize, String convType,
	 MPosition mLocation, Float padding=1.0, Bool usezero=True);
  GridFT(Long cachesize, Int tilesize, String convType,
	 MDirection mTangent, Float padding=1.0, Bool usezero=True);
  GridFT(Long cachesize, Int tilesize, String convType,
	 MPosition mLocation, MDirection mTangent, Float passing=1.0,
	 Bool usezero=True);
  // </group>

  // Construct from a Record containing the GridFT state
  GridFT(const RecordInterface& stateRec);

  // Copy constructor
  GridFT(const GridFT &other);

  // Assignment operator
  GridFT &operator=(const GridFT &other);

  ~GridFT();

  // Initialize transform to Visibility plane using the image
  // as a template. The image is loaded and Fourier transformed.
  void initializeToVis(ImageInterface<Complex>& image,
		       const VisBuffer& vb);
  // This version returns the gridded vis...should be used in conjunction 
  // with the version of 'get' that needs the gridded visdata 
  void initializeToVis(ImageInterface<Complex>& image,
		       const VisBuffer& vb, Array<Complex>& griddedVis,
		       Vector<Double>& uvscale, UVWMachine* &uvwMachine);

  // Finalize transform to Visibility plane: flushes the image
  // cache and shows statistics if it is being used.
  void finalizeToVis();

  // Initialize transform to Sky plane: initializes the image
  void initializeToSky(ImageInterface<Complex>& image,  Matrix<Float>& weight,
		       const VisBuffer& vb);

  void initializeToSky(ImageInterface<Complex>& image,  Matrix<Float>& weight,
		       const VisBuffer& vb, Vector<Double>& uvscale,
		       UVWMachine* & uvwmachine);

  // Finalize transform to Sky plane: flushes the image
  // cache and shows statistics if it is being used. DOES NOT
  // DO THE FINAL TRANSFORM!
  void finalizeToSky();

  void finalizeToSky(ImageInterface<Complex>& iimage);

  // Get actual coherence from grid by degridding
  void get(VisBuffer& vb, Int row=-1);

  // Get the coherence from grid return it in the degrid 
  // is used especially when scratch columns are not 
  // present in ms.
  virtual void get(VisBuffer& vb, Cube<Complex>& degrid, 
		   Array<Complex>& griddedVis, Vector<Double>& scale, 
		   UVWMachine *uvwMachine,
		   Int row=-1);


  // Put coherence to grid by gridding.
  void put(const VisBuffer& vb, Int row=-1, Bool dopsf=False,
	   FTMachine::Type type=FTMachine::OBSERVED);

  void put(const VisBuffer& vb, TempImage<Complex>& iimage, 
	   Vector<Double>& scale, Int row=-1, UVWMachine *uvwMachine=0, 
	   Bool dopsf=False);

  // Make the entire image
  void makeImage(FTMachine::Type type,
		 VisSet& vs,
		 ImageInterface<Complex>& image,
		 Matrix<Float>& weight);
  
  // Get the final image: do the Fourier transform and
  // grid-correct, then optionally normalize by the summed weights
  ImageInterface<Complex>& getImage(Matrix<Float>&, Bool normalize=True);
 
  // Get the final weights image
  void getWeightImage(ImageInterface<Float>&, Matrix<Float>&);

  // Save and restore the GridFT to and from a record
  virtual Bool toRecord(String& error, RecordInterface& outRec, 
			Bool withImage=False);
  virtual Bool fromRecord(String& error, const RecordInterface& inRec);

  // Can this FTMachine be represented by Fourier convolutions?
  virtual Bool isFourier() {return True;}

  virtual void setNoPadding(Bool nopad){noPadding_p=nopad;};

  virtual String name();
protected:


  // Padding in FFT
  Float padding_p;

  // Get the appropriate data pointer
  Array<Complex>* getDataPointer(const IPosition&, Bool);

  void ok();

  void init();

  // Is this record on Grid? check both ends. This assumes that the
  // ends bracket the middle
  Bool recordOnGrid(const VisBuffer& vb, Int rownr) const;

  // Image cache
  LatticeCache<Complex> * imageCache;

  // Sizes
  Long cachesize;
  Int  tilesize;

  // Gridder
  ConvolveGridder<Double, Complex>* gridder;

  // Is this tiled?
  Bool isTiled;

  // Array lattice
  Lattice<Complex> * arrayLattice;

  // Lattice. For non-tiled gridding, this will point to arrayLattice,
  //  whereas for tiled gridding, this points to the image
  Lattice<Complex>* lattice;

  String convType;

  Float maxAbsData;

  // Useful IPositions
  IPosition centerLoc, offsetLoc;

  // Image Scaling and offset
  Vector<Double> uvScale, uvOffset;

  // Array for non-tiled gridding
  Array<Complex> griddedData;

  Int priorCacheSize;

  // Grid/degrid zero spacing points?

  Bool usezero_p;

  //force no padding
  Bool noPadding_p;

  //Check if using put that avoids non-necessary reads
  Bool usePut2_p;

  //machine name
  String machineName_p;
};

} //# NAMESPACE CASA - END

#endif
