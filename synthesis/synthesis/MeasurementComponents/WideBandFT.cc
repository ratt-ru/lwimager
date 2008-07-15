//# WideBandFT.cc: Implementation of WideBandFT class
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
//# $Id: WideBandFT.cc,v 1.2 2006/10/16 01:30:37 mmarquar Exp $

#include <msvis/MSVis/VisibilityIterator.h>
#include <casa/Quanta/UnitMap.h>
#include <casa/Quanta/UnitVal.h>
#include <measures/Measures/Stokes.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/SpectralCoordinate.h>
#include <coordinates/Coordinates/StokesCoordinate.h>
#include <coordinates/Coordinates/Projection.h>
#include <ms/MeasurementSets/MSColumns.h>
#include <casa/BasicSL/Constants.h>
#include <scimath/Mathematics/FFTServer.h>
#include <synthesis/MeasurementComponents/WideBandFT.h>
#include <scimath/Mathematics/RigidVector.h>
#include <msvis/MSVis/StokesVector.h>
#include <synthesis/MeasurementEquations/StokesImageUtil.h>
#include <msvis/MSVis/VisBuffer.h>
#include <msvis/MSVis/VisSet.h>
#include <images/Images/ImageInterface.h>
#include <images/Images/PagedImage.h>
#include <casa/Containers/Block.h>
#include <casa/Containers/Record.h>
#include <casa/Arrays/ArrayLogical.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/MaskedArray.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Cube.h>
#include <casa/Arrays/MatrixIter.h>
#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>
#include <casa/Exceptions/Error.h>
#include <lattices/Lattices/ArrayLattice.h>
#include <measures/Measures/UVWMachine.h>
#include <lattices/Lattices/SubLattice.h>
#include <lattices/Lattices/LCBox.h>
#include <lattices/Lattices/LatticeCache.h>
#include <lattices/Lattices/LatticeFFT.h>
#include <lattices/Lattices/LatticeIterator.h>
#include <lattices/Lattices/LatticeStepper.h>
#include <scimath/Mathematics/ConvolveGridder.h>
#include <casa/Utilities/CompositeNumber.h>
#include <casa/OS/Timer.h>
#include <casa/sstream.h>

namespace casa { //# NAMESPACE CASA - BEGIN

Int adbg=0;
	
WideBandFT::WideBandFT(Long icachesize, Int itilesize, String iconvType, Float padding,
	       Bool usezero)
: FTMachine(), padding_p(padding), imageCache(0), cachesize(icachesize), tilesize(itilesize),
  gridder(0), isTiled(False), arrayLattice(0), lattice(0), convType(iconvType),
  maxAbsData(0.0), centerLoc(IPosition(4,0)), offsetLoc(IPosition(4,0)),
  usezero_p(usezero), noPadding_p(False), usePut2_p(False), 
  machineName_p("WideBandFT"),ntaylor_p(0)
{
	if(adbg)cout << "WideBandFT constructor 1" << endl;
}

WideBandFT::WideBandFT(Long icachesize, Int itilesize, String iconvType,
	       MPosition mLocation, Float padding, Bool usezero)
: FTMachine(), padding_p(padding), imageCache(0), cachesize(icachesize),
  tilesize(itilesize), gridder(0), isTiled(False), arrayLattice(0),
  lattice(0), convType(iconvType), maxAbsData(0.0), centerLoc(IPosition(4,0)),
  offsetLoc(IPosition(4,0)), usezero_p(usezero), noPadding_p(False), 
  usePut2_p(False), machineName_p("WideBandFT"),ntaylor_p(0)
{
  mLocation_p=mLocation;
  tangentSpecified_p=False;
	if(adbg)cout << "WideBandFT constructor 2" << endl;
}

WideBandFT::WideBandFT(Long icachesize, Int itilesize, String iconvType,
	       MDirection mTangent, Float padding, Bool usezero)
: FTMachine(), padding_p(padding), imageCache(0), cachesize(icachesize),
  tilesize(itilesize), gridder(0), isTiled(False), arrayLattice(0),
  lattice(0), convType(iconvType), maxAbsData(0.0), centerLoc(IPosition(4,0)),
  offsetLoc(IPosition(4,0)), usezero_p(usezero), noPadding_p(False), 
  usePut2_p(False), machineName_p("WideBandFT"),ntaylor_p(0)
{
  mTangent_p=mTangent;
  tangentSpecified_p=True;
	if(adbg)cout << "WideBandFT constructor 3" << endl;
}

WideBandFT::WideBandFT(Long icachesize, Int itilesize, String iconvType,
	       MPosition mLocation, MDirection mTangent, Float padding,
	       Bool usezero)
: FTMachine(), padding_p(padding), imageCache(0), cachesize(icachesize),
  tilesize(itilesize), gridder(0), isTiled(False), arrayLattice(0),
  lattice(0), convType(iconvType), maxAbsData(0.0), centerLoc(IPosition(4,0)),
  offsetLoc(IPosition(4,0)), usezero_p(usezero), noPadding_p(False), 
  usePut2_p(False),machineName_p("WideBandFT")
{
  mLocation_p=mLocation;
  mTangent_p=mTangent;
  tangentSpecified_p=True;
	if(adbg)cout << "WideBandFT constructor 4" << endl;
}

WideBandFT::WideBandFT(const RecordInterface& stateRec)
: FTMachine()
{
	if(adbg)cout << "WideBandFT constructor 5" << endl;
  // Construct from the input state record
  String error;
  if (!fromRecord(error, stateRec)) {
    throw (AipsError("Failed to create gridder: " + error));
  };
}

//---------------------------------------------------------------------- 
WideBandFT& WideBandFT::operator=(const WideBandFT& other)
{
  if(this!=&other) {
    imageCache=other.imageCache;
    cachesize=other.cachesize;
    tilesize=other.tilesize;
    gridder=other.gridder;
    isTiled=other.isTiled;
    lattice=other.lattice;
    //lattice1=other.lattice1;
    //arrayLattice1=other.arrayLattice1;
    latticespec=other.latticespec;
    arrayLatticespec=other.arrayLatticespec;
    convType=other.convType;
    maxAbsData=other.maxAbsData;
    centerLoc=other.centerLoc;
    offsetLoc=other.offsetLoc;
    padding_p=other.padding_p;
    usezero_p=other.usezero_p;
  };
  return *this;
};

//----------------------------------------------------------------------
WideBandFT::WideBandFT(const WideBandFT& other)
{
  operator=(other);
}

//----------------------------------------------------------------------
void WideBandFT::init() {

  logIO() << LogOrigin("WideBandFT", "init")  << LogIO::NORMAL;

  ok();

  // Padding is possible only for non-tiled processing
  if((padding_p*padding_p*image->shape().product())>cachesize) {
    isTiled=True;
    nx    = image->shape()(0);
    ny    = image->shape()(1);
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }
  else {
    // We are padding.
    isTiled=False;
    if(!noPadding_p){
      CompositeNumber cn(uInt(image->shape()(0)*2));    
      nx    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(0))-0.5));
      ny    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(1))-0.5));
    }
    else{
      nx    = image->shape()(0);
      ny    = image->shape()(1);
    }
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }

  sumWeight.resize(npol, nchan);

  uvScale.resize(2);
  uvScale(0)=(Float(nx)*image->coordinates().increment()(0)); 
  uvScale(1)=(Float(ny)*image->coordinates().increment()(1)); 
  uvOffset.resize(2);
  uvOffset(0)=nx/2;
  uvOffset(1)=ny/2;

  // Now set up the gridder. The possibilities are BOX and SF
  if(gridder) delete gridder; gridder=0;
  gridder = new ConvolveGridder<Double, Complex>(IPosition(2, nx, ny),
						 uvScale, uvOffset,
						 convType);

  // Set up image cache needed for gridding. For BOX-car convolution
  // we can use non-overlapped tiles. Otherwise we need to use
  // overlapped tiles and additive gridding so that only increments
  // to a tile are written.
  if(imageCache) delete imageCache; imageCache=0;

  if(isTiled) {
    Float tileOverlap=0.5;
    if(convType=="box") {
      tileOverlap=0.0;
    }
    else {
      tileOverlap=0.5;
      tilesize=max(12,tilesize);
    }
    IPosition tileShape=IPosition(4,tilesize,tilesize,npol,nchan);
    Vector<Float> tileOverlapVec(4);
    tileOverlapVec=0.0;
    tileOverlapVec(0)=tileOverlap;
    tileOverlapVec(1)=tileOverlap;
    Int tmpCacheVal=static_cast<Int>(cachesize);
    imageCache=new LatticeCache <Complex> (*image, tmpCacheVal, tileShape, 
					   tileOverlapVec,
					   (tileOverlap>0.0));

  }
}

// This is nasty, we should use CountedPointers here.
WideBandFT::~WideBandFT() {
  if(imageCache) delete imageCache; imageCache=0;
  if(arrayLattice) delete arrayLattice; arrayLattice=0;
  //if(arrayLattice1) delete arrayLattice1; arrayLattice1=0;
  if(arrayLatticespec.nelements())
  {
	  for(Int i=0;i<(Int)arrayLatticespec.nelements();i++)
		  if(arrayLatticespec[i]) delete arrayLatticespec[i];
	  arrayLatticespec.resize(0);
  }
  if(gridder) delete gridder; gridder=0;
}

// Initialize for a transform from the Sky domain. This means that
// we grid-correct, and FFT the image

void WideBandFT::initializeToVis(ImageInterface<Complex>& iimage,
			     const VisBuffer& vb)
{
  image=&iimage;

  ok();

  init();

  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);

  // Need to reset nx, ny for padding
  // Padding is possible only for non-tiled processing
  if((padding_p*padding_p*image->shape().product())>cachesize) {
    isTiled=True;
    nx    = image->shape()(0);
    ny    = image->shape()(1);
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }
  else {
    // We are padding.
    isTiled=False;
    CompositeNumber cn(uInt(image->shape()(0)*2));
    if(!noPadding_p){
      nx    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(0))-0.5));
      ny    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(1))-0.5));
    }
    else{
      nx    = image->shape()(0);
      ny    = image->shape()(1);
    }
      
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }

  // If we are memory-based then read the image in and create an
  // ArrayLattice otherwise just use the PagedImage
  if(isTiled) {
    lattice=image;
  }
  else {
     IPosition gridShape(4, nx, ny, npol, nchan);
     griddedData.resize(gridShape);
     //griddedData can be a reference of image data...if not using model col
     //hence using an undocumented feature of resize that if 
     //the size is the same as old data it is not changed.
     if(!usePut2_p) griddedData.set(0);
     

     IPosition stride(4, 1);
     IPosition blc(4, (nx-image->shape()(0))/2, (ny-image->shape()(1))/2, 0, 0);
     IPosition trc(blc+image->shape()-stride);

     IPosition start(4, 0);
     griddedData(blc, trc) = image->getSlice(start, image->shape());

     if(arrayLattice) delete arrayLattice; arrayLattice=0;
     arrayLattice = new ArrayLattice<Complex>(griddedData);
     lattice=arrayLattice;
  }

  AlwaysAssert(lattice, AipsError);

  logIO() << LogIO::DEBUGGING
	  << "Starting grid correction and FFT of image" << LogIO::POST;

  // Do the Grid-correction. 
    {
      Vector<Complex> correction(nx);
      correction=Complex(1.0, 0.0);
      // Do the Grid-correction
      IPosition cursorShape(4, nx, 1, 1, 1);
      IPosition axisPath(4, 0, 1, 2, 3);
      LatticeStepper lsx(lattice->shape(), cursorShape, axisPath);
      LatticeIterator<Complex> lix(*lattice, lsx);
      for(lix.reset();!lix.atEnd();lix++) {
        gridder->correctX1D(correction, lix.position()(1));
        lix.rwVectorCursor()/=correction;
      }
    }
  
    // Now do the FFT2D in place
    LatticeFFT::cfft2d(*lattice);
    
    logIO() << LogIO::DEBUGGING
	    << "Finished grid correction and FFT of image" << LogIO::POST;
    
}

void WideBandFT::wbinitializeToVis(ImageInterface<Complex>& iimage,
		PtrBlock<ImageInterface<Complex>* > & iimagespec,
			     const VisBuffer& vb, Int model, Int ntaylor)
{
  image=&iimage;
//  image1=&iimage1;

  ntaylor_p = ntaylor;

  Int index =  model * (ntaylor);
  imagespec.resize(ntaylor);
  for(Int taylor=0;taylor<ntaylor;taylor++)
	  imagespec[taylor] = iimagespec[index+taylor];
  
  ok();

  init();

  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);

  // Need to reset nx, ny for padding
  // Padding is possible only for non-tiled processing
  if((padding_p*padding_p*image->shape().product())>cachesize) {
    isTiled=True;
    nx    = image->shape()(0);
    ny    = image->shape()(1);
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }
  else {
    // We are padding.
    isTiled=False;
    CompositeNumber cn(uInt(image->shape()(0)*2));
    if(!noPadding_p){
      nx    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(0))-0.5));
      ny    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(1))-0.5));
    }
    else{
      nx    = image->shape()(0);
      ny    = image->shape()(1);
    }
      
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }

  // If we are memory-based then read the image in and create an
  // ArrayLattice otherwise just use the PagedImage
  if(isTiled) {
    lattice=image;
    //lattice1=image1;
  }
  else {
     IPosition gridShape(4, nx, ny, npol, nchan);
     griddedData.resize(gridShape);
     //griddedData1.resize(gridShape);
    
     griddedDataspec.resize(ntaylor);
    for(Int taylor=0;taylor<ntaylor;taylor++)
    {
	    griddedDataspec[taylor]->resize(gridShape);
//	    (*griddedDataspec[taylor]) = Complex(0.0);
    }
  
     //griddedData can be a reference of image data...if not using model col
     //hence using an undocumented feature of resize that if 
     //the size is the same as old data it is not changed.
     if(!usePut2_p) 
     {
	     griddedData.set(0);
	     for(Int taylor=0;taylor<ntaylor;taylor++)
		     griddedDataspec[taylor]->set(0);
     }
     

     IPosition stride(4, 1);
     IPosition blc(4, (nx-image->shape()(0))/2, (ny-image->shape()(1))/2, 0, 0);
     IPosition trc(blc+image->shape()-stride);

     IPosition start(4, 0);
     griddedData(blc, trc) = image->getSlice(start, image->shape());
     //griddedData1(blc, trc) = image1->getSlice(start, image1->shape());
     for(Int taylor=0;taylor<ntaylor;taylor++)
	     (*griddedDataspec[taylor])(blc, trc) = imagespec[taylor]->getSlice(start, imagespec[taylor]->shape());
     
     if(arrayLattice) delete arrayLattice; arrayLattice=0;
     arrayLattice = new ArrayLattice<Complex>(griddedData);
     lattice=arrayLattice;
     
   //  if(arrayLattice1) delete arrayLattice1; arrayLattice1=0;
   //  arrayLattice1 = new ArrayLattice<Complex>(griddedData1);
   //  lattice1=arrayLattice1;
    
     if(arrayLatticespec.nelements())
    {
	    for(Int i=0;i<(Int)arrayLatticespec.nelements();i++)
		    if(arrayLatticespec[i]) delete arrayLatticespec[i];
	    arrayLatticespec.resize(0);
    }

    latticespec.resize(ntaylor);
    arrayLatticespec.resize(ntaylor);
    for(Int taylor=0;taylor<ntaylor;taylor++)
    {
	    arrayLatticespec[taylor] = new ArrayLattice<Complex>(*griddedDataspec[taylor]);
	    latticespec[taylor] = arrayLatticespec[taylor];
	    AlwaysAssert(latticespec[taylor], AipsError);
    }
  }

  AlwaysAssert(lattice, AipsError);
 // AlwaysAssert(lattice1, AipsError);

  logIO() << LogIO::DEBUGGING
	  << "Starting grid correction and FFT of image" << LogIO::POST;

  // Do the Grid-correction. 
    {
      PtrBlock<LatticeIterator<Complex>* > lixspec;
      lixspec.resize(ntaylor);
      
      Vector<Complex> correction(nx);
      correction=Complex(1.0, 0.0);
      // Do the Grid-correction
      IPosition cursorShape(4, nx, 1, 1, 1);
      IPosition axisPath(4, 0, 1, 2, 3);
      LatticeStepper lsx(lattice->shape(), cursorShape, axisPath);
      LatticeIterator<Complex> lix(*lattice, lsx);
      //LatticeIterator<Complex> lix1(*lattice1, lsx);
      
      for(Int i=0;i<ntaylor;i++) 
	      lixspec[i] = new LatticeIterator<Complex>(*latticespec[i],lsx);
      
      for(lix.reset();!lix.atEnd();lix++) 
      {
	      gridder->correctX1D(correction, lix.position()(1));
	      lix.rwVectorCursor()/=correction;
	      
	      //lix1.rwVectorCursor()/=correction;
	      for(Int i=0;i<ntaylor;i++) 
	      {
		      lixspec[i]->rwVectorCursor() /= correction;
		      (*lixspec[i])++;
	      }
      }

      for(Int i=0;i<ntaylor;i++)   delete lixspec[i];
      
    }
  
    // Now do the FFT2D in place
    LatticeFFT::cfft2d(*lattice);
    //LatticeFFT::cfft2d(*lattice1);
    for(Int i=0;i<ntaylor;i++)  LatticeFFT::cfft2d(*latticespec[i]);
    
    logIO() << LogIO::DEBUGGING
	    << "Finished grid correction and FFT of image" << LogIO::POST;
    
}



void WideBandFT::initializeToVis(ImageInterface<Complex>& iimage,
			     const VisBuffer& vb, Array<Complex>& griddedVis,
			     Vector<Double>& uvscale, 
			     UVWMachine* &uvwmachine){



  noPadding_p=True;
  initializeToVis(iimage, vb);
  griddedVis.resize(griddedData.shape());  
  griddedVis=griddedData; //using the copy for storage
  uvscale.resize(2);
  uvscale=uvScale;
  uvwmachine = new UVWMachine(*uvwMachine_p);

}



void WideBandFT::finalizeToVis()
{
  if(isTiled) {

    logIO() << LogOrigin("WideBandFT", "finalizeToVis")  << LogIO::NORMAL;

    AlwaysAssert(imageCache, AipsError);
    AlwaysAssert(image, AipsError);
    ostringstream o;
    imageCache->flush();
    imageCache->showCacheStatistics(o);
    logIO() << o.str() << LogIO::POST;
  }
}

void WideBandFT::initializeToSky(ImageInterface<Complex>& iimage,
			     Matrix<Float>& weight, const VisBuffer& vb)
{
  // image always points to the image
  image=&iimage;

  init();

  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);


  // Need to reset nx, ny for padding
  // Padding is possible only for non-tiled processing
  if((padding_p*padding_p*image->shape().product())>cachesize) {
    isTiled=True;
    nx    = image->shape()(0);
    ny    = image->shape()(1);
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }
  else {
    // We are padding.
    isTiled=False;
    CompositeNumber cn(uInt(image->shape()(0)*2));    
    if (!noPadding_p){
      nx    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(0))-0.5));
      ny    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(1))-0.5));
    }
    else{

      nx    = image->shape()(0);
      ny    = image->shape()(1);
    }
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }

  sumWeight=0.0;
  weight.resize(sumWeight.shape());
  weight=0.0;

  // Initialize for in memory or to disk gridding. lattice will
  // point to the appropriate Lattice, either the ArrayLattice for
  // in memory gridding or to the image for to disk gridding.
  if(isTiled) {
    imageCache->flush();
    image->set(Complex(0.0));
    lattice=image;
  }
  else {
    IPosition gridShape(4, nx, ny, npol, nchan);
    griddedData.resize(gridShape);
    griddedData=Complex(0.0);
    //iimage.get(griddedData, False);
    if(arrayLattice) delete arrayLattice; arrayLattice=0;
    arrayLattice = new ArrayLattice<Complex>(griddedData);
    lattice=arrayLattice;
  }
  AlwaysAssert(lattice, AipsError);

}



// Initialize the FFT to the Sky. Here we have to setup and initialize the
// grid. 
//void WideBandFT::wbinitializeToSky(ImageInterface<Complex>& iimage,
//		 		ImageInterface<Complex>& iimage1,
//			     Matrix<Float>& weight, const VisBuffer& vb)
void WideBandFT::wbinitializeToSky(ImageInterface<Complex>& iimage,
		 		PtrBlock<ImageInterface<Complex>*> & iimagespec,
			     Matrix<Float>& weight, const VisBuffer& vb,
			     Int model, Int ntaylor)
{
  // image always points to the image
  image=&iimage;
  
  //image1=&iimage1;

  ntaylor_p = ntaylor;

  Int index =  model * (ntaylor);
  /*
  if(imagespec.nelements()!=ntaylor)
  {
	  // clean up too...
	    imagespec.resize(ntaylor);
	    for(Int taylor=0;taylor<ntaylor;taylor++)
		    imagespec[taylor] = new ImageInterface<Complex>();
  }
  */
	  
  imagespec.resize(ntaylor);
  for(Int taylor=0;taylor<ntaylor;taylor++)
	  imagespec[taylor] = iimagespec[index+taylor];

  init();

  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);


  // Need to reset nx, ny for padding
  // Padding is possible only for non-tiled processing
  if((padding_p*padding_p*image->shape().product())>cachesize) {
    isTiled=True;
    nx    = image->shape()(0);
    ny    = image->shape()(1);
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }
  else {
    // We are padding.
    isTiled=False;
    CompositeNumber cn(uInt(image->shape()(0)*2));    
    if (!noPadding_p){
      nx    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(0))-0.5));
      ny    = cn.nextLargerEven(Int(padding_p*Float(image->shape()(1))-0.5));
    }
    else{

      nx    = image->shape()(0);
      ny    = image->shape()(1);
    }
    npol  = image->shape()(2);
    nchan = image->shape()(3);
  }

  sumWeight=0.0;
  weight.resize(sumWeight.shape());
  weight=0.0;

  // Initialize for in memory or to disk gridding. lattice will
  // point to the appropriate Lattice, either the ArrayLattice for
  // in memory gridding or to the image for to disk gridding.
  if(isTiled) {
    imageCache->flush();
    image->set(Complex(0.0));
    lattice=image;
  }
  else {
    IPosition gridShape(4, nx, ny, npol, nchan);
    
    if(adbg)cout << "About to setup griddedData stuff" << endl;
    griddedData.resize(gridShape);
    griddedData=Complex(0.0);
    
    //griddedData1.resize(gridShape);
    //griddedData1=Complex(0.0);
    
    griddedDataspec.resize(ntaylor);
    for(Int taylor=0;taylor<ntaylor;taylor++)
    {
	    griddedDataspec[taylor] = new Array<Complex>();
	    griddedDataspec[taylor]->resize(gridShape);
	    (*griddedDataspec[taylor]) = Complex(0.0);
    }
	    
   
    if(adbg)cout << "Done with griddedData setup, now doing arrayLattice" << endl;
    
    //iimage.get(griddedData, False);
    if(arrayLattice) delete arrayLattice; arrayLattice=0;
    arrayLattice = new ArrayLattice<Complex>(griddedData);
    lattice=arrayLattice;
    
    //if(arrayLattice1) delete arrayLattice1; arrayLattice1=0;
    //arrayLattice1 = new ArrayLattice<Complex>(griddedData1);
    //lattice1=arrayLattice1;
    
    if(arrayLatticespec.nelements())
    {
	    for(Int i=0;i<(Int)arrayLatticespec.nelements();i++)
		    if(arrayLatticespec[i]) delete arrayLatticespec[i];
	    arrayLatticespec.resize(0);
    }

    latticespec.resize(ntaylor);
    arrayLatticespec.resize(ntaylor);
    for(Int taylor=0;taylor<ntaylor;taylor++)
    {
	    arrayLatticespec[taylor] = new ArrayLattice<Complex>(*griddedDataspec[taylor]);
	    latticespec[taylor] = arrayLatticespec[taylor];
	    AlwaysAssert(latticespec[taylor], AipsError);
    }
    
    if(adbg)cout << "Done with arraylattice too " << endl;
  }
  AlwaysAssert(lattice, AipsError);
//  AlwaysAssert(lattice1, AipsError);

}


void WideBandFT::initializeToSky(ImageInterface<Complex>& iimage,
			     Matrix<Float>& weight, const VisBuffer& vb,
			     Vector<Double>& uvscale,
			     UVWMachine* & uvwmachine)
{
  // image always points to the image
  image=&iimage;

  init();

  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);

  isTiled=False;
  noPadding_p=True;
  nx    = image->shape()(0);
  ny    = image->shape()(1);
  npol  = image->shape()(2);
  nchan = image->shape()(3);
 

  sumWeight=0.0;
  weight.resize(sumWeight.shape());
  weight=0.0;

  iimage.get(griddedData, False);

  if(uvwmachine == NULL){
    uvwmachine = new UVWMachine(*uvwMachine_p);
    uvscale.resize(2);
    uvscale=uvScale;
  }
  /* if(arrayLattice) delete arrayLattice; arrayLattice=0;
  arrayLattice = new ArrayLattice<Complex>(griddedData);
  lattice=arrayLattice;


  AlwaysAssert(lattice, AipsError);
  */

}

void WideBandFT::finalizeToSky()
{  
  AlwaysAssert(lattice, AipsError);
  // Now we flush the cache and report statistics
  // For memory based, we don't write anything out yet.
  if(isTiled) {
    logIO() << LogOrigin("WideBandFT", "finalizeToSky")  << LogIO::NORMAL;

    AlwaysAssert(image, AipsError);
    AlwaysAssert(imageCache, AipsError);
    imageCache->flush();
    ostringstream o;
    imageCache->showCacheStatistics(o);
    logIO() << o.str() << LogIO::POST;
  }
}


void WideBandFT::finalizeToSky(ImageInterface<Complex>& iimage)
{

  if(isTiled){
    iimage.get(griddedData, False);
  }
  else{
     IPosition stride(4, 1);
     IPosition blc(4, (nx-iimage.shape()(0))/2, (ny-iimage.shape()(1))/2, 0, 0);
     IPosition trc(blc+iimage.shape()-stride);

     IPosition start(4, 0);
     
     griddedData(blc, trc) = iimage.getSlice(start, iimage.shape());
     

  }

  if(arrayLattice) delete arrayLattice; arrayLattice=0;
  arrayLattice = new ArrayLattice<Complex>(griddedData);
  lattice=arrayLattice;
  
  AlwaysAssert(lattice, AipsError);
  // Now we flush the cache and report statistics
  // For memory based, we don't write anything out yet.
  if(isTiled) {
    logIO() << LogOrigin("WideBandFT", "finalizeToSky")  << LogIO::NORMAL;

    AlwaysAssert(image, AipsError);
    AlwaysAssert(imageCache, AipsError);
    imageCache->flush();
    ostringstream o;
    imageCache->showCacheStatistics(o);
    logIO() << o.str() << LogIO::POST;
  }
}

Array<Complex>* WideBandFT::getDataPointer(const IPosition& centerLoc2D,
				       Bool readonly) {
  Array<Complex>* result;
  // Is tiled: get tiles and set up offsets
  centerLoc(0)=centerLoc2D(0);
  centerLoc(1)=centerLoc2D(1);
  result=&imageCache->tile(offsetLoc,centerLoc, readonly);
  gridder->setOffset(IPosition(2, offsetLoc(0), offsetLoc(1)));
  return result;
}

#define NEED_UNDERSCORES
#if defined(NEED_UNDERSCORES)
#define ggridft ggridft_
#define dgridft dgridft_
#endif

extern "C" { 
   void ggridft(Double*,
                Double*,
		const Complex*,
                Int*,
                Int*,
                Int*,
		const Int*,
		const Int*,
		const Float*,
		Int*,
		Int*,
		Double*,
		Double*,
		Complex*,
                Int*,
		Int*,
		Int *,
		Int *,
		const Double*,
		const Double*,
                Int*,
		Int*,
		Double*,
		Int*,
		Int*,
		Double*);
   void dgridft(Double*,
                Double*,
		Complex*,
                Int*,
                Int*,
		const Int*,
		const Int*,
		Int*,
		Int*,
		Double*,
		Double*,
		const Complex*,
                Int*,
		Int*,
		Int *,
		Int *,
		const Double*,
		const Double*,
                Int*,
		Int*,
		Double*,
		Int*,
		Int*);
}
void WideBandFT::put(const VisBuffer& vb, Int row, Bool dopsf, 
		 FTMachine::Type type)
{


  gridOk(gridder->cSupport()(0));

  const Cube<Complex> *data;
  data=0;
  if(!dopsf){
    if(type==FTMachine::MODEL){
      data=&(vb.modelVisCube());
    }
    else if(type==FTMachine::CORRECTED){
      data=&(vb.correctedVisCube());
    }
    else{
      data=&(vb.visCube());
    }
  }
  Bool isCopy;
  const Complex *datStorage;

  if(!dopsf)
    datStorage=data->getStorage(isCopy);
  else
    datStorage=0;
  // If row is -1 then we pass through all rows
  Int startRow, endRow, nRow;
  if (row==-1) {
    nRow=vb.nRow();
    startRow=0;
    endRow=nRow-1;
  } else {
    nRow=1;
    startRow=row;
    endRow=row;
  }

  // Get the uvws in a form that Fortran can use and do that
  // necessary phase rotation. On a Pentium Pro 200 MHz
  // when null, this step takes about 50us per uvw point. This
  // is just barely noticeable for Stokes I continuum and
  // irrelevant for other cases.
  Matrix<Double> uvw(3, vb.uvw().nelements());
  uvw=0.0;
  Vector<Double> dphase(vb.uvw().nelements());
  dphase=0.0;
  //NEGATING to correct for an image inversion problem
  for (Int i=startRow;i<=endRow;i++) {
    for (Int idim=0;idim<2;idim++) uvw(idim,i)=-vb.uvw()(i)(idim);
    uvw(2,i)=vb.uvw()(i)(2);
  }
   
  rotateUVW(uvw, dphase, vb);
  refocus(uvw, vb.antenna1(), vb.antenna2(), dphase, vb);

  // Take care of translation of Bools to Integer
  Int idopsf=0;
  if(dopsf) idopsf=1;

  Cube<Int> flags(vb.flagCube().shape());
  flags=0;
  flags(vb.flagCube())=True;

  Vector<Int> rowFlags(vb.nRow());
  rowFlags=0;
  rowFlags(vb.flagRow())=True;
  if(!usezero_p) {
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      if(vb.antenna1()(rownr)==vb.antenna2()(rownr)) rowFlags(rownr)=1;
    }
  }
  

  //Check if ms has changed then cache new spw and chan selection
  if(vb.newMS())
    matchAllSpwChans(vb);
  
  //Here we redo the match or use previous match
  
  //Channel matching for the actual spectral window of buffer
  if(doConversion_p[vb.spectralWindow()]){
    matchChannel(vb.spectralWindow(), vb);
  }
  else{
    chanMap.resize();
    chanMap=multiChanMap_p[vb.spectralWindow()];
  }

// DO THE TRANSFORM - CALL FORTRAN

    Bool del;
    IPosition s(flags.shape());
    ggridft(uvw.getStorage(del),
	    dphase.getStorage(del),
	    datStorage,
	    &s(0),
	    &s(1),
	    &idopsf,
	    flags.getStorage(del),
	    rowFlags.getStorage(del),
	    vb.imagingWeight().getStorage(del),
	    &s(2),
	    &row,
	    uvScale.getStorage(del),
	    uvOffset.getStorage(del),
	    griddedData.getStorage(del),
	    &nx,
	    &ny,
	    &npol,
	    &nchan,
	    vb.frequency().getStorage(del),
	    &C::c,
	    &(gridder->cSupport()(0)),
	    &(gridder->cSampling()),
	    gridder->cFunction().getStorage(del),
	    chanMap.getStorage(del),
	    polMap.getStorage(del),
	    sumWeight.getStorage(del));

  
  if(dopsf) // Call the gridder again only if the PSF is being created.
  {
    Matrix<Float> specWeights;
  
    for(Int taylor=0;taylor<ntaylor_p;taylor++)
    {
  // modify weights vb.imagingWeight()
    specWeights = vb.imagingWeight();
    modifyWeights(vb,specWeights, taylor+1.0);
  // Call the fortran gridder again.
    ggridft(uvw.getStorage(del),
	    dphase.getStorage(del),
	    datStorage,
	    &s(0),
	    &s(1),
	    &idopsf,
	    flags.getStorage(del),
	    rowFlags.getStorage(del),
	    specWeights.getStorage(del),
	    &s(2),
	    &row,
	    uvScale.getStorage(del),
	    uvOffset.getStorage(del),
	    (*griddedDataspec[taylor]).getStorage(del),
	    &nx,
	    &ny,
	    &npol,
	    &nchan,
	    vb.frequency().getStorage(del),
	    &C::c,
	    &(gridder->cSupport()(0)),
	    &(gridder->cSampling()),
	    gridder->cFunction().getStorage(del),
	    chanMap.getStorage(del),
	    polMap.getStorage(del),
	    sumWeight.getStorage(del));
    }
  }
  if(!dopsf)
    data->freeStorage(datStorage, isCopy);

}

void WideBandFT::put(const VisBuffer& vb, TempImage<Complex>& iimage, Vector<Double>& scale, Int row, UVWMachine * uvwMachine, 
		 Bool dopsf){

  usePut2_p=True;

  UVWMachine *storedMachine;
  storedMachine=uvwMachine_p;
  if(uvwMachine){
    uvwMachine_p=uvwMachine;
  }


  Bool refOfImage=iimage.canReferenceArray();
  griddedData.resize();
  iimage.get(griddedData);
  Int nX=griddedData.shape()(0);
  Int nY=griddedData.shape()(1);

  gridOk(gridder->cSupport()(0));


  // If row is -1 then we pass through all rows
  Int startRow, endRow, nRow;
  if (row==-1) {
    nRow=vb.nRow();
    startRow=0;
    endRow=nRow-1;
  } else {
    nRow=1;
    startRow=row;
    endRow=row;
  }

  // Get the uvws in a form that Fortran can use and do that
  // necessary phase rotation. On a Pentium Pro 200 MHz
  // when null, this step takes about 50us per uvw point. This
  // is just barely noticeable for Stokes I continuum and
  // irrelevant for other cases.
  Matrix<Double> uvw(3, vb.uvw().nelements());
  uvw=0.0;
  Vector<Double> dphase(vb.uvw().nelements());
  dphase=0.0;
  //NEGATING to correct for an image inversion problem
  for (Int i=startRow;i<=endRow;i++) {
    for (Int idim=0;idim<2;idim++) uvw(idim,i)=-vb.uvw()(i)(idim);
    uvw(2,i)=vb.uvw()(i)(2);
  }
   
  rotateUVW(uvw, dphase, vb);
  refocus(uvw, vb.antenna1(), vb.antenna2(), dphase, vb);

  // Take care of translation of Bools to Integer
  Int idopsf=0;
  if(dopsf) idopsf=1;

  Cube<Int> flags(vb.flagCube().shape());
  flags=0;
  flags(vb.flagCube())=True;

  Vector<Int> rowFlags(vb.nRow());
  rowFlags=0;
  rowFlags(vb.flagRow())=True;
  if(!usezero_p) {
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      if(vb.antenna1()(rownr)==vb.antenna2()(rownr)) rowFlags(rownr)=1;
    }
  }

  //Check if ms has changed then cache new spw and chan selection
  if(vb.newMS())
    matchAllSpwChans(vb);
  
  
  //Here we redo the match or use previous match
  
  //Channel matching for the actual spectral window of buffer
  if(doConversion_p[vb.spectralWindow()]){
    matchChannel(vb.spectralWindow(), vb);
  }
  else{
    chanMap.resize();
    chanMap=multiChanMap_p[vb.spectralWindow()];
  }



  Bool del;
  IPosition s(vb.visCube().shape());
  ggridft(uvw.getStorage(del),
	  dphase.getStorage(del),
	  vb.visCube().getStorage(del),
	  &s(0),
	  &s(1),
	  &idopsf,
	  flags.getStorage(del),
	  rowFlags.getStorage(del),
	  vb.imagingWeight().getStorage(del),
	  &s(2),
	  &row,
	  scale.getStorage(del),
	  uvOffset.getStorage(del),
	  griddedData.getStorage(del),
	  &nX,
	  &nY,
	  &npol,
	  &nchan,
	  vb.frequency().getStorage(del),
	  &C::c,
	  &(gridder->cSupport()(0)),
	  &(gridder->cSampling()),
	  gridder->cFunction().getStorage(del),
	  chanMap.getStorage(del),
	  polMap.getStorage(del),
	  sumWeight.getStorage(del));
  
  

  if(!refOfImage){ iimage.put(griddedData);};

  //restoring to its previous state
  uvwMachine_p=storedMachine;
  
}


void WideBandFT::get(VisBuffer& vb, Int row)
{

  Cube<Complex> specModelVis;

  gridOk(gridder->cSupport()(0));
  // If row is -1 then we pass through all rows
  Int startRow, endRow, nRow;
  if (row==-1) {
    nRow=vb.nRow();
    startRow=0;
    endRow=nRow-1;
    vb.modelVisCube()=Complex(0.0,0.0);
  } else {
    nRow=1;
    startRow=row;
    endRow=row;
    vb.modelVisCube().xyPlane(row)=Complex(0.0,0.0);
  }

  // Get the uvws in a form that Fortran can use
  Matrix<Double> uvw(3, vb.uvw().nelements());
  uvw=0.0;
  Vector<Double> dphase(vb.uvw().nelements());
  dphase=0.0;
  //NEGATING to correct for an image inversion problem
  for (Int i=startRow;i<=endRow;i++) {
    for (Int idim=0;idim<2;idim++) uvw(idim,i)=-vb.uvw()(i)(idim);
    uvw(2,i)=vb.uvw()(i)(2);
  }

  rotateUVW(uvw, dphase, vb);
  refocus(uvw, vb.antenna1(), vb.antenna2(), dphase, vb);

  Cube<Int> flags(vb.flagCube().shape());
  flags=0;
  flags(vb.flagCube())=True;

  //Check if ms has changed then cache new spw and chan selection
  if(vb.newMS())
    matchAllSpwChans(vb);


  //Here we redo the match or use previous match
  
  //Channel matching for the actual spectral window of buffer
  if(doConversion_p[vb.spectralWindow()]){
    matchChannel(vb.spectralWindow(), vb);
  }
  else{
    chanMap.resize();
    chanMap=multiChanMap_p[vb.spectralWindow()];
  }


  

  Vector<Int> rowFlags(vb.nRow());
  rowFlags=0;
  rowFlags(vb.flagRow())=True;
  if(!usezero_p) {
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      if(vb.antenna1()(rownr)==vb.antenna2()(rownr)) rowFlags(rownr)=1;
    }
  }
  
  if(isTiled) {
    Double invLambdaC=vb.frequency()(nvischan/2)/C::c;
    Vector<Double> uvLambda(2);
    Vector<Int> centerLoc2D(2);
    centerLoc2D=0;
    
    // Loop over all rows
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      
      // Calculate uvw for this row at the center frequency
      uvLambda(0)=uvw(0, rownr)*invLambdaC;
      uvLambda(1)=uvw(1, rownr)*invLambdaC;
      centerLoc2D=gridder->location(centerLoc2D, uvLambda);
      
      // Is this point on the grid?
      if(gridder->onGrid(centerLoc2D)) {
	
        // Get the tile
	Array<Complex>* dataPtr=getDataPointer(centerLoc2D, True);
	Int aNx=dataPtr->shape()(0);
	Int aNy=dataPtr->shape()(1);
	
	// Now use FORTRAN to do the gridding. Remember to 
	// ensure that the shape and offsets of the tile are 
	// accounted for.
	Bool del;
        Vector<Double> actualOffset(2);
        for (Int i=0;i<2;i++) {
          actualOffset(i)=uvOffset(i)-Double(offsetLoc(i));
	}
	IPosition s(vb.modelVisCube().shape());
	dgridft(uvw.getStorage(del),
		dphase.getStorage(del),
		vb.modelVisCube().getStorage(del),
                &s(0),
		&s(1),
		flags.getStorage(del),
		rowFlags.getStorage(del),
		&s(2),
		&rownr,
		uvScale.getStorage(del),
		actualOffset.getStorage(del),
		dataPtr->getStorage(del),
		&aNx,
		&aNy,
		&npol,
		&nchan,
		vb.frequency().getStorage(del),
		&C::c,
		&(gridder->cSupport()(0)),
		&(gridder->cSampling()),
		gridder->cFunction().getStorage(del),
		chanMap.getStorage(del),
		polMap.getStorage(del));
      }
    }
  }
  else {

    // create empty one - to put in the 2nd term
    //specModelVis = vb.modelVisCube();

    Bool del;
    IPosition s(vb.modelVisCube().shape());
    dgridft(uvw.getStorage(del),
	    dphase.getStorage(del),
	    vb.modelVisCube().getStorage(del),
	    &s(0),
	    &s(1),
	    flags.getStorage(del),
	    rowFlags.getStorage(del),
	    &s(2),
	    &row,
	    uvScale.getStorage(del),
	    uvOffset.getStorage(del),
	    griddedData.getStorage(del),
	    &nx,
	    &ny,
	    &npol,
	    &nchan,
	    vb.frequency().getStorage(del),
	    &C::c,
	    &(gridder->cSupport()(0)),
	    &(gridder->cSampling()),
	    gridder->cFunction().getStorage(del),
	    chanMap.getStorage(del),
	    polMap.getStorage(del));

#if 1
  specModelVis = vb.modelVisCube();
  for(Int taylor=0;taylor<ntaylor_p;taylor++)
  {
    dgridft(uvw.getStorage(del),
	    dphase.getStorage(del),
	    specModelVis.getStorage(del),
	    &s(0),
	    &s(1),
	    flags.getStorage(del),
	    rowFlags.getStorage(del),
	    &s(2),
	    &row,
	    uvScale.getStorage(del),
	    uvOffset.getStorage(del),
	    (*griddedDataspec[taylor]).getStorage(del),
	    &nx,
	    &ny,
	    &npol,
	    &nchan,
	    vb.frequency().getStorage(del),
	    &C::c,
	    &(gridder->cSupport()(0)),
	    &(gridder->cSampling()),
	    gridder->cFunction().getStorage(del),
	    chanMap.getStorage(del),
	    polMap.getStorage(del));

    //cout << "before taylor : " << taylor+1.0 << "   val : " << specModelVis(0,0,3) << endl;
    modifySpecModelVis(vb,specModelVis, taylor+1.0);
    vb.modelVisCube() += specModelVis;

    //cout << "taylor : " << taylor+1.0 << "   val : " << specModelVis(0,0,3) << endl;
  }
#endif
  }
}

void WideBandFT::get(VisBuffer& vb, Cube<Complex>& modelVis, 
		 Array<Complex>& griddedVis, Vector<Double>& scale,
		 UVWMachine *uvwMachine,
		 Int row)
{

  gridOk(gridder->cSupport()(0));
  Int nX=griddedVis.shape()(0);
  Int nY=griddedVis.shape()(1);
  Vector<Double> offset(2);
  offset(0)=Double(nX)/2.0;
  offset(1)=Double(nY)/2.0;
  // If row is -1 then we pass through all rows
  Int startRow, endRow, nRow;
  if (row==-1) {
    nRow=vb.nRow();
    startRow=0;
    endRow=nRow-1;
    modelVis.set(Complex(0.0,0.0));
  } else {
    nRow=1;
    startRow=row;
    endRow=row;
    modelVis.xyPlane(row)=Complex(0.0,0.0);
  }


  UVWMachine * storedMachine;
  storedMachine=uvwMachine_p;
  if(uvwMachine){
    uvwMachine_p=uvwMachine;

  }

  // Get the uvws in a form that Fortran can use
  Matrix<Double> uvw(3, vb.uvw().nelements());
  uvw=0.0;
  Vector<Double> dphase(vb.uvw().nelements());
  dphase=0.0;
  //NEGATING to correct for an image inversion problem
  for (Int i=startRow;i<=endRow;i++) {
    for (Int idim=0;idim<2;idim++) uvw(idim,i)=-vb.uvw()(i)(idim);
    uvw(2,i)=vb.uvw()(i)(2);
  }

  rotateUVW(uvw, dphase, vb);
  refocus(uvw, vb.antenna1(), vb.antenna2(), dphase, vb);

  Cube<Int> flags(vb.flagCube().shape());
  flags=0;
  flags(vb.flagCube())=True;


  //Check if ms has changed then cache new spw and chan selection
  if(vb.newMS())
    matchAllSpwChans(vb);

  //Here we redo the match or use previous match
  
  //Channel matching for the actual spectral window of buffer
  if(doConversion_p[vb.spectralWindow()]){
    matchChannel(vb.spectralWindow(), vb);
  }
  else{
    chanMap.resize();
    chanMap=multiChanMap_p[vb.spectralWindow()];
  }


  

  Vector<Int> rowFlags(vb.nRow());
  rowFlags=0;
  rowFlags(vb.flagRow())=True;
  if(!usezero_p) {
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      if(vb.antenna1()(rownr)==vb.antenna2()(rownr)) rowFlags(rownr)=1;
    }
  }
  
    Bool del;
    IPosition s(modelVis.shape());
    dgridft(uvw.getStorage(del),
	    dphase.getStorage(del),
	    modelVis.getStorage(del),
	    &s(0),
	    &s(1),
	    flags.getStorage(del),
	    rowFlags.getStorage(del),
	    &s(2),
	    &row,
	    scale.getStorage(del),
	    offset.getStorage(del),
	    griddedVis.getStorage(del),
	    &nX,
	    &nY,
	    &npol,
	    &nchan,
	    vb.frequency().getStorage(del),
	    &C::c,
	    &(gridder->cSupport()(0)),
	    &(gridder->cSampling()),
	    gridder->cFunction().getStorage(del),
	    chanMap.getStorage(del),
	    polMap.getStorage(del));
  

    uvwMachine_p=storedMachine;


}

// Finalize the FFT to the Sky. Here we actually do the FFT and
// return the resulting image
ImageInterface<Complex>& WideBandFT::getImage(Matrix<Float>& weights, Bool normalize) 
{
  AlwaysAssert(lattice, AipsError);
  AlwaysAssert(gridder, AipsError);
  AlwaysAssert(image, AipsError);
  logIO() << LogOrigin("GridFT", "getImage") << LogIO::NORMAL;

  weights.resize(sumWeight.shape());

  convertArray(weights, sumWeight);
  // If the weights are all zero then we cannot normalize
  // otherwise we don't care.
  if(normalize&&max(weights)==0.0) {
    logIO() << LogIO::SEVERE << "No useful data in GridFT: weights all zero"
	    << LogIO::POST;
  }
  else {

    const IPosition latticeShape = lattice->shape();
    
    logIO() << LogIO::DEBUGGING
	    << "Starting FFT and scaling of image" << LogIO::POST;
    
    // x and y transforms
    LatticeFFT::cfft2d(*lattice,False);
    

    {
      Int inx = lattice->shape()(0);
      Int iny = lattice->shape()(1);
      Vector<Complex> correction(inx);
      correction=Complex(1.0, 0.0);
      // Do the Grid-correction
      IPosition cursorShape(4, inx, 1, 1, 1);
      IPosition axisPath(4, 0, 1, 2, 3);
      LatticeStepper lsx(lattice->shape(), cursorShape, axisPath);
      LatticeIterator<Complex> lix(*lattice, lsx);
      for(lix.reset();!lix.atEnd();lix++) {
	Int pol=lix.position()(2);
	Int chan=lix.position()(3);
	if(weights(pol, chan)>0.0) {
	  gridder->correctX1D(correction, lix.position()(1));
	  lix.rwVectorCursor()/=correction;
	  if(normalize) {
	    Complex rnorm(Float(inx)*Float(iny)/weights(pol,chan));
	    lix.rwCursor()*=rnorm;
	  }
	  else {
	    Complex rnorm(Float(inx)*Float(iny));
	    lix.rwCursor()*=rnorm;
	  }
	}
	else {
	  lix.woCursor()=0.0;
	}
      }
    }

    if(!isTiled) {
      // Check the section from the image BEFORE converting to a lattice 
      IPosition blc(4, (nx-image->shape()(0))/2, (ny-image->shape()(1))/2, 0, 0);
      IPosition stride(4, 1);
      IPosition trc(blc+image->shape()-stride);
      // Do the copy
      IPosition start(4, 0);
      image->put(griddedData(blc, trc));
    }
  }
    
  return *image;
}

//// WIDEBAND
// Finalize the FFT to the Sky. Here we actually do the FFT and
// return the resulting image
ImageInterface<Complex>& WideBandFT::wbgetImage(PtrBlock<ImageInterface<Complex>* > & inpimagespec, Matrix<Float>& weights, const Bool normalize, Int model, Int ntaylor) 
{
  AlwaysAssert(lattice, AipsError);
  //AlwaysAssert(lattice1, AipsError);
  AlwaysAssert(gridder, AipsError);
  AlwaysAssert(image, AipsError);
  //AlwaysAssert(image1, AipsError);
  logIO() << LogOrigin("WideBandFT", "getImage") << LogIO::NORMAL;

  //Int ntaylor = inpimagespec.nelements();
  ntaylor_p = ntaylor;
  
  weights.resize(sumWeight.shape());

  convertArray(weights, sumWeight);
  // If the weights are all zero then we cannot normalize
  // otherwise we don't care.
  if(normalize&&max(weights)==0.0) {
    logIO() << LogIO::SEVERE << "No useful data in WideBandFT: weights all zero"
	    << LogIO::POST;
  }
  else {

    const IPosition latticeShape = lattice->shape();
    
    logIO() << LogIO::DEBUGGING
	    << "Starting FFT and scaling of image" << LogIO::POST;
    
    // x and y transforms
    LatticeFFT::cfft2d(*lattice,False);
    for(Int taylor=0;taylor<ntaylor;taylor++)
	    LatticeFFT::cfft2d(*latticespec[taylor],False);
    
    {
      Int inx = lattice->shape()(0);
      Int iny = lattice->shape()(1);
      Vector<Complex> correction(inx);
      correction=Complex(1.0, 0.0);
      // Do the Grid-correction
      IPosition cursorShape(4, inx, 1, 1, 1);
      IPosition axisPath(4, 0, 1, 2, 3);
      LatticeStepper lsx(lattice->shape(), cursorShape, axisPath);
      
      PtrBlock<LatticeIterator<Complex>* > lixspec;
      lixspec.resize(ntaylor);
      
      LatticeIterator<Complex> lix(*lattice, lsx);
      for(Int i=0;i<ntaylor;i++) 
      {
	      lixspec[i] = new LatticeIterator<Complex>(*latticespec[i],lsx);
	      lixspec[i]->reset();
      }
      //LatticeIterator<Complex> lix1(*lattice1, lsx);
      
      for(lix.reset();!lix.atEnd();lix++) 
      {
	      Int pol=lix.position()(2);
	      Int chan=lix.position()(3);
	      /* URV */ //if(weights(pol, chan)>0.0) 
	      if(weights(pol, chan)!=0.0) 
	      {
		      gridder->correctX1D(correction, lix.position()(1));
		      lix.rwVectorCursor()/=correction;
		      for(Int i=0;i<ntaylor;i++) lixspec[i]->rwVectorCursor() /= correction;
		      if(normalize) 
		      {
			      Complex rnorm(Float(inx)*Float(iny)/weights(pol,chan));
			      lix.rwCursor()*=rnorm;
			      for(Int i=0;i<ntaylor;i++) lixspec[i]->rwCursor()*=rnorm;
		      }
		      else 
		      {
			      Complex rnorm(Float(inx)*Float(iny));
			      lix.rwCursor()*=rnorm;
			      for(Int i=0;i<ntaylor;i++) lixspec[i]->rwCursor()*=rnorm;
		      }
	      }
	      else 
	      {
		      lix.woCursor()=0.0;
		      for(Int i=0;i<ntaylor;i++) lixspec[i]->woCursor()=0.0;
	      }
	      for(Int i=0;i<ntaylor;i++) (*lixspec[i])++;
	      
      }
      for(Int i=0;i<ntaylor;i++) delete lixspec[i];
    }

    if(!isTiled) {
      // Check the section from the image BEFORE converting to a lattice 
      IPosition blc(4, (nx-image->shape()(0))/2, (ny-image->shape()(1))/2, 0, 0);
      IPosition stride(4, 1);
      IPosition trc(blc+image->shape()-stride);
      // Do the copy
      IPosition start(4, 0);
      image->put(griddedData(blc, trc));
      for(Int taylor=0;taylor<ntaylor;taylor++)
	      imagespec[taylor]->put((*griddedDataspec[taylor])(blc, trc));
    }
  }
    
  Int index =  model * (ntaylor);
    for(Int taylor=0;taylor<ntaylor;taylor++)
	    inpimagespec[index+taylor]->copyData(*imagespec[taylor]);
  return *image;
}

// Get weight image
void WideBandFT::getWeightImage(ImageInterface<Float>& weightImage, Matrix<Float>& weights) 
{

  logIO() << LogOrigin("WideBandFT", "getWeightImage") << LogIO::NORMAL;

  weights.resize(sumWeight.shape());
  convertArray(weights,sumWeight);

  const IPosition latticeShape = weightImage.shape();
    
  Int nx=latticeShape(0);
  Int ny=latticeShape(1);

  IPosition loc(2, 0);
  IPosition cursorShape(4, nx, ny, 1, 1);
  IPosition axisPath(4, 0, 1, 2, 3);
  LatticeStepper lsx(latticeShape, cursorShape, axisPath);
  LatticeIterator<Float> lix(weightImage, lsx);
  for(lix.reset();!lix.atEnd();lix++) {
    Int pol=lix.position()(2);
    Int chan=lix.position()(3);
    lix.rwCursor()=weights(pol,chan);
  }
}

Bool WideBandFT::toRecord(String& error, RecordInterface& outRec, 
			Bool withImage) {

  // Save the current WideBandFT object to an output state record
  Bool retval = True;

  Double cacheVal=(Double)cachesize;
  outRec.define("cache", cacheVal);
  outRec.define("tile", tilesize);
  outRec.define("gridfunction", convType);

  Vector<Double> phaseValue(2);
  String phaseUnit;
  phaseValue=mTangent_p.getAngle().getValue();
  phaseUnit= mTangent_p.getAngle().getUnit();
  outRec.define("phasevalue", phaseValue);
  outRec.define("phaseunit", phaseUnit);

  Vector<Double> dirValue(3);
  String dirUnit;
  dirValue=mLocation_p.get("m").getValue();
  dirUnit=mLocation_p.get("m").getUnit();
  outRec.define("dirvalue", dirValue);
  outRec.define("dirunit", dirUnit);

  outRec.define("padding", padding_p);
  outRec.define("maxdataval", maxAbsData);

  Vector<Int> center_loc(4), offset_loc(4);
  for (Int k=0; k<4 ; k++){
    center_loc(k)=centerLoc(k);
    offset_loc(k)=offsetLoc(k);
  }
  outRec.define("centerloc", center_loc);
  outRec.define("offsetloc", offset_loc);
  outRec.define("sumofweights", sumWeight);
  if(withImage && image){ 
    ImageInterface<Complex>& tempimage(*image);
    Record imageContainer;
    String error;
    retval = (retval || tempimage.toRecord(error, imageContainer));
    outRec.defineRecord("image", imageContainer);
  }
return retval;
}

Bool WideBandFT::fromRecord(String& error, const RecordInterface& inRec)
{
  Bool retval = True;
  gridder=0; imageCache=0; lattice=0; arrayLattice=0; arrayLattice1=0;
  Double cacheVal;
  inRec.get("cache", cacheVal);
  cachesize=(Long)cacheVal;
  inRec.get("tile", tilesize);
  inRec.get("gridfunction", convType);

  Vector<Double> phaseValue(2);
  inRec.get("phasevalue",phaseValue);
  String phaseUnit;
  inRec.get("phaseunit",phaseUnit);
  Quantity val1(phaseValue(0), phaseUnit);
  Quantity val2(phaseValue(1), phaseUnit); 
  MDirection phasecenter(val1, val2);

  mTangent_p=phasecenter;
  // This should be passed down too but the tangent plane is 
  // expected to be specified in all meaningful cases.
  tangentSpecified_p=True;  
  Vector<Double> dirValue(3);
  String dirUnit;
  inRec.get("dirvalue", dirValue);
  inRec.get("dirunit", dirUnit);
  MVPosition dummyMVPos(dirValue(0), dirValue(1), dirValue(2));
  MPosition mLocation(dummyMVPos, MPosition::ITRF);
  mLocation_p=mLocation;

  inRec.get("padding", padding_p);
  inRec.get("maxdataval", maxAbsData);

  Vector<Int> center_loc(4), offset_loc(4);
  inRec.get("centerloc", center_loc);
  inRec.get("offsetloc", offset_loc);
  uInt ndim4 = 4;
  centerLoc=IPosition(ndim4, center_loc(0), center_loc(1), center_loc(2), 
		      center_loc(3));
  offsetLoc=IPosition(ndim4, offset_loc(0), offset_loc(1), offset_loc(2), 
		      offset_loc(3));
  inRec.get("sumofweights", sumWeight);
  if(inRec.nfields() > 12 ){
    Record imageAsRec=inRec.asRecord("image");
    if(!image) { 
      image= new TempImage<Complex>(); 
    };
    String error;
    retval = (retval || image->fromRecord(error, imageAsRec));    
 
    // Might be changing the shape of sumWeight
    init(); 

    if(isTiled) {
      lattice=image;
    }
    else {
      // Make the grid the correct shape and turn it into an array lattice
      // Check the section from the image BEFORE converting to a lattice 
      IPosition gridShape(4, nx, ny, npol, nchan);
      griddedData.resize(gridShape);
      griddedData=Complex(0.0);
      IPosition blc(4, (nx-image->shape()(0))/2, (ny-image->shape()(1))/2, 0, 0);
      IPosition start(4, 0);
      IPosition stride(4, 1);
      IPosition trc(blc+image->shape()-stride);
      griddedData(blc, trc) = image->getSlice(start, image->shape());
      
      if(arrayLattice) delete arrayLattice; arrayLattice=0;
      arrayLattice = new ArrayLattice<Complex>(griddedData);
      lattice=arrayLattice;
    }

    AlwaysAssert(lattice, AipsError);
    AlwaysAssert(gridder, AipsError);
    AlwaysAssert(image, AipsError);
  };
  return retval;
}

void WideBandFT::ok() {
  AlwaysAssert(image, AipsError);
}

// Make a plain straightforward honest-to-God image. This returns
// a complex image, without conversion to Stokes. The representation
// is that required for the visibilities.
//----------------------------------------------------------------------
void WideBandFT::makeImage(FTMachine::Type type, 
		       VisSet& vs,
		       ImageInterface<Complex>& theImage,
		       Matrix<Float>& weight) {


  logIO() << LogOrigin("WideBandFT", "makeImage") << LogIO::NORMAL;

  if(type==FTMachine::COVERAGE) {
    logIO() << "Type COVERAGE not defined for Fourier transforms" << LogIO::EXCEPTION;
  }


  // Initialize the gradients
  ROVisIter& vi(vs.iter());

  // Loop over all visibilities and pixels
  VisBuffer vb(vi);
  
  // Initialize put (i.e. transform to Sky) for this model
  vi.origin();

  if(vb.polFrame()==MSIter::Linear) {
    StokesImageUtil::changeCStokesRep(theImage, SkyModel::LINEAR);
  }
  else {
    StokesImageUtil::changeCStokesRep(theImage, SkyModel::CIRCULAR);
  }
  
  initializeToSky(theImage,weight,vb);

  // Loop over the visibilities, putting VisBuffers
  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    for (vi.origin(); vi.more(); vi++) {
      
      switch(type) {
      case FTMachine::RESIDUAL:
	vb.visCube()=vb.correctedVisCube();
	vb.visCube()-=vb.modelVisCube();
        put(vb, -1, False);
        break;
      case FTMachine::MODEL:
	vb.visCube()=vb.modelVisCube();
        put(vb, -1, False);
        break;
      case FTMachine::CORRECTED:
	vb.visCube()=vb.correctedVisCube();
        put(vb, -1, False);
        break;
      case FTMachine::PSF:
	vb.visCube()=Complex(1.0,0.0);
        put(vb, -1, True);
        break;
#if 0
      case FTMachine::SPECTRALPSF:
	vb.visCube()=Complex(1.0,0.0);
	/* Make the B0 psf */
        put(vb, -1, True);
	/* Save it */

	/* Make the B1 psf */
	modifyWeights(vb, power, True);
        put(vb, -1, True);
	/* Save it */

	/* Change back the weights */
	modifyWeights(vb, power, False);
	
        break;
#endif
      case FTMachine::OBSERVED:
      default:
        put(vb, -1, False);
        break;
      }
    }
  }
  finalizeToSky();
  // Normalize by dividing out weights, etc.
  getImage(weight, True);
}

// Modify Weights for Spectral psfs.
Bool WideBandFT::modifyWeights(const VisBuffer& vb, Matrix<Float> &specwts, Float power)
{
	Int nRow=vb.nRow();
	Int nChan=vb.nChannel();
	Float freq=0.0,factor=1.0,mulfactor=1.0;
	Vector<Double> selfreqlist(vb.frequency());
	Float refFreq = (selfreqlist(selfreqlist.endPosition()) + selfreqlist(IPosition(selfreqlist.ndim(),0)))/2.0;
	
	for (Int row=0; row<nRow; row++) 
	for (Int chn=0; chn<nChan; chn++) 
	{
		freq = selfreqlist(IPosition(1,chn));
		factor = refFreq/(selfreqlist(selfreqlist.endPosition())-refFreq);
		mulfactor = ((freq-refFreq)/refFreq)*factor;
		
		specwts(chn,row) *= pow(mulfactor,power);
	}
	return True;
}// end of modifyWeights

// Modify Visibilities while degridding the higher order terms.
Bool WideBandFT::modifySpecModelVis(const VisBuffer& vb, Cube<Complex> &specvis, Float power)
{
	Int nRow=vb.nRow();
	Int nChan=vb.nChannel();
	Float freq=0.0,factor=1.0,mulfactor=1.0;
	Vector<Double> selfreqlist(vb.frequency());
	Float refFreq = (selfreqlist(selfreqlist.endPosition()) + selfreqlist(IPosition(selfreqlist.ndim(),0)))/2.0;
//	cout << "Ref Freq : " << refFreq << endl;

	  for (Int row=0; row<nRow; row++) 
	  for (Int chn=0; chn<nChan; chn++) 
	  {
		  freq = selfreqlist(IPosition(1,chn));
		  factor = refFreq/(selfreqlist(selfreqlist.endPosition())-refFreq);
		  mulfactor = ((freq-refFreq)/refFreq)*factor;
		  
		  specvis(0,chn,row) *= pow(mulfactor,power);
		  specvis(1,chn,row) *= pow(mulfactor,power);
	  }

	return True;
}// end of modifySpecModelVis



String WideBandFT::name(){

  return machineName_p;


}

} //# NAMESPACE CASA - END

