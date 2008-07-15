//# MosaicFT.cc: Implementation of MosaicFT class
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
//# $Id: MosaicFT.cc,v 1.36 2006/06/26 21:57:14 kgolap Exp $

#include <msvis/MSVis/VisibilityIterator.h>
#include <casa/Quanta/UnitMap.h>
#include <casa/Quanta/MVTime.h>
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
#include <synthesis/MeasurementComponents/MosaicFT.h>
#include <scimath/Mathematics/RigidVector.h>
#include <msvis/MSVis/StokesVector.h>
#include <synthesis/MeasurementEquations/StokesImageUtil.h>
#include <msvis/MSVis/VisBuffer.h>
#include <msvis/MSVis/VisSet.h>
#include <images/Images/ImageInterface.h>
#include <images/Images/PagedImage.h>
#include <images/Images/WCBox.h>
#include <images/Images/SubImage.h>
#include <images/Images/ImageRegion.h>
#include <casa/Containers/Block.h>
#include <casa/Containers/Record.h>
#include <casa/Arrays/ArrayLogical.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/MaskedArray.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Slice.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Cube.h>
#include <casa/Arrays/MatrixIter.h>
#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>
#include <casa/Exceptions/Error.h>
#include <lattices/Lattices/ArrayLattice.h>
#include <lattices/Lattices/SubLattice.h>
#include <lattices/Lattices/LCBox.h>
#include <lattices/Lattices/LatticeExpr.h>
#include <lattices/Lattices/LatticeCache.h>
#include <lattices/Lattices/LatticeFFT.h>
#include <lattices/Lattices/LatticeIterator.h>
#include <lattices/Lattices/LatticeStepper.h>
#include <casa/Utilities/CompositeNumber.h>
#include <casa/OS/Timer.h>
#include <casa/OS/HostInfo.h>
#include <casa/sstream.h>

namespace casa { //# NAMESPACE CASA - BEGIN

MosaicFT::MosaicFT(MeasurementSet& ms, SkyJones& sj,
		   Long icachesize, Int itilesize, 
		   Bool usezero)
  : FTMachine(), ms_p(&ms), sj_p(&sj),
    imageCache(0),  cachesize(icachesize), tilesize(itilesize),
    isTiled(False), arrayLattice(0), lattice(0), weightLattice(0),
    maxAbsData(0.0), centerLoc(IPosition(4,0)), offsetLoc(IPosition(4,0)),
    mspc(0), msac(0), pointingToImage(0), usezero_p(usezero),gridder(0), 
    skyCoverage_p(0), convFunctionMap_p(-1), machineName_p("MosaicFT") 
{
  convSize=0;
  tangentSpecified_p=False;
  lastIndex_p=0;
  doneWeightImage_p=False;
  convWeightImage_p=0;
    
  // We should get rid of the ms dependence in the constructor
  // not used
}

MosaicFT::MosaicFT(const RecordInterface& stateRec)
  : FTMachine(), convFunctionMap_p(-1)
{
  // Construct from the input state record
  String error;
  if (!fromRecord(error, stateRec)) {
    throw (AipsError("Failed to create MosaicFT: " + error));
  };
}

//---------------------------------------------------------------------- 
MosaicFT& MosaicFT::operator=(const MosaicFT& other)
{
  if(this!=&other) {
    ms_p=other.ms_p;
    sj_p=other.sj_p;
    imageCache=other.imageCache;
    cachesize=other.cachesize;
    tilesize=other.tilesize;
    isTiled=other.isTiled;
    lattice=other.lattice;
    arrayLattice=other.arrayLattice;
    maxAbsData=other.maxAbsData;
    centerLoc=other.centerLoc;
    offsetLoc=other.offsetLoc;
    pointingToImage=other.pointingToImage;
    usezero_p=other.usezero_p;
    skyCoverage_p=(TempImage<Float> *)other.skyCoverage_p->cloneII();
    convWeightImage_p=(TempImage<Complex> *)other.convWeightImage_p->cloneII();
    convFunctionMap_p.clear();
  };
  return *this;
};

//----------------------------------------------------------------------
MosaicFT::MosaicFT(const MosaicFT& other): convFunctionMap_p(-1)
{
  operator=(other);
}

//----------------------------------------------------------------------
void MosaicFT::init() {
  
  if((image->shape().product())>cachesize) {
    isTiled=True;
  }
  else {
    isTiled=False;
  }
  //For now only isTiled False works.
  isTiled=False;
  nx    = image->shape()(0);
  ny    = image->shape()(1);
  npol  = image->shape()(2);
  nchan = image->shape()(3);


  //  if(skyCoverage_p==0){
  //    Double memoryMB=HostInfo::memoryTotal()/1024.0/(20.0);
  //    skyCoverage_p=new TempImage<Float> (IPosition(4,nx,ny,1,1),
  //					image->coordinates(), memoryMB);
    //Setting it to zero
//   (*skyCoverage_p)-=(*skyCoverage_p);
//  }
  sumWeight.resize(npol, nchan);
  
  convSupport=0;

  uvScale.resize(2);
  uvScale=0.0;
  uvScale(0)=Float(nx)*image->coordinates().increment()(0); 
  uvScale(1)=Float(ny)*image->coordinates().increment()(1); 
    
  uvOffset.resize(2);
  uvOffset(0)=nx/2;
  uvOffset(1)=ny/2;
  
  if(gridder) delete gridder; gridder=0;
  gridder = new ConvolveGridder<Double, Complex>(IPosition(2, nx, ny),
						 uvScale, uvOffset,
						 "SF");

  // Set up image cache needed for gridding. 
  if(imageCache) delete imageCache; imageCache=0;
  
  if(isTiled) {
    Float tileOverlap=0.5;
    tilesize=min(256,tilesize);
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
MosaicFT::~MosaicFT() {
  if(imageCache) delete imageCache; imageCache=0;
  if(arrayLattice) delete arrayLattice; arrayLattice=0;
}

void MosaicFT::findConvFunction(const ImageInterface<Complex>& iimage,
				const VisBuffer& vb) {
  
  //  if(convSize>0) return;
  if(checkPBOfField(vb)) return;

  logIO() << LogOrigin("MosaicFT", "findConvFunction")  << LogIO::NORMAL;
  
  ok();
  
  // Get the coordinate system
  CoordinateSystem coords(iimage.coordinates());
  
  // Set up the convolution function. 
  convSampling=1;
  convSize=nx;
  
  // Make a two dimensional image to calculate the
  // primary beam. We want this on a fine grid in the
  // UV plane 
  Int directionIndex=coords.findCoordinate(Coordinate::DIRECTION);
  AlwaysAssert(directionIndex>=0, AipsError);
  DirectionCoordinate dc=coords.directionCoordinate(directionIndex);
  directionCoord=coords.directionCoordinate(directionIndex);
  Vector<Double> sampling;
  sampling = dc.increment();
  sampling*=Double(convSampling);
  sampling*=Double(nx)/Double(convSize);
  dc.setIncrement(sampling);
  
  Vector<Double> unitVec(2);
  unitVec=convSize/2;
  dc.setReferencePixel(unitVec);
  
  // Set the reference value to that of the pointing position of the
  // current buffer since that's what will be applied
  //####will need to use this when using a much smaller convsize than image.
  //  getXYPos(vb, 0);
  // dc.setReferenceValue(worldPosMeas.getAngle().getValue());
  
  coords.replaceCoordinate(dc, directionIndex);
  //  coords.list(logIO(), MDoppler::RADIO, IPosition(), IPosition());
  
  IPosition pbShape(4, convSize, convSize, 1, 1);
  TempImage<Complex> twoDPB(pbShape, coords);



  convFunc.resize(convSize, convSize);
  convFunc=0.0;

  IPosition start(4, 0, 0, 0, 0);
  IPosition pbSlice(4, convSize, convSize, 1, 1);
  
  // Accumulate terms 
  Matrix<Complex> screen(convSize, convSize);
  screen=1.0;
  // Either the SkyJones
  twoDPB.putSlice(screen, start);
  sj_p->apply(twoDPB, twoDPB, vb, 0); 
 
  //*****Test
  TempImage<Complex> twoDPB2(pbShape, coords);
  {
    TempImage<Float> screen2(pbShape, coords);
    Matrix<Float> screenoo(convSize, convSize);
    screenoo.set(1.0);
    screen2.putSlice(screenoo,start);
    sj_p->applySquare(screen2, screen2, vb, 0);
    LatticeExpr<Complex> le(screen2);
    twoDPB2.copyData(le);
  }
  //****************
  
  //addBeamCoverage(twoDPB);


  Bool writeImages=True;
 
  // Now FFT and get the result back
  LatticeFFT::cfft2d(twoDPB);
  LatticeFFT::cfft2d(twoDPB2);

    // Write out FT of screen as an image
  if(0) {
    CoordinateSystem ftCoords(coords);
    directionIndex=ftCoords.findCoordinate(Coordinate::DIRECTION);
    AlwaysAssert(directionIndex>=0, AipsError);
    dc=coords.directionCoordinate(directionIndex);
    Vector<Bool> axes(2); axes(0)=True;axes(1)=True;
    Vector<Int> shape(2); shape(0)=convSize;shape(1)=convSize;
    Coordinate* ftdc=dc.makeFourierCoordinate(axes,shape);
    ftCoords.replaceCoordinate(*ftdc, directionIndex);
    delete ftdc; ftdc=0;
    ostringstream os1;
    os1 << "FTScreen_" << vb.fieldId() ;
    PagedImage<Float> thisScreen(pbShape, ftCoords, String(os1));
    LatticeExpr<Float> le(abs(twoDPB));
    thisScreen.copyData(le);
  }

  convFunc=twoDPB.get(True);
  //convFunc/=max(abs(convFunc));
  Float maxAbsConvFunc=max(amplitude(convFunc));
  
  Float minAbsConvFunc=min(amplitude(convFunc));
  convSupport=-1;
  Bool found=False;
  Int trial=0;
  for (trial=convSize/2-2;trial>0;trial--) {
    if(abs(convFunc(convSize/2,convSize/2-trial)) >  (1.0e-2*maxAbsConvFunc)) {
      found=True;
      break;
    }
  }
  if(!found){
    if((maxAbsConvFunc-minAbsConvFunc) > (1.0e-2*maxAbsConvFunc)) 
      found=True;
    // if it drops by more than 2 magnitudes per pixel
    trial=3;
  }

  if(found) {
    convSupport=Int(0.5+Float(trial)/Float(convSampling))+1;
  }
  else {
    logIO() << "Convolution function is misbehaved - support seems to be zero"
	    << LogIO::EXCEPTION;
  }
  
  // Normalize such that plane 0 sums to 1 (when jumping in
  // steps of convSampling)
  
  Double pbSum=0.0;
  for (Int iy=-convSupport;iy<=convSupport;iy++) {
    for (Int ix=-convSupport;ix<=convSupport;ix++) {
      Complex val=convFunc(ix*convSampling+convSize/2,
			   iy*convSampling+convSize/2);
      
      pbSum+=sqrt(real(val)*real(val)+ imag(val)*imag(val));
    }
  }

  if(pbSum>0.0) {
    convFunc*=Complex(1.0/pbSum,0.0);
  }
  else {
    logIO() << "Convolution function integral is not positive"
	    << LogIO::EXCEPTION;
  }
  
  //##########################################
  logIO() << "Convolution support = " << convSupport
	  << " pixels in Fourier plane"
	  << LogIO::POST;

  convSupportBlock_p.resize(actualConvIndex_p+1);
  convSizes_p.resize(actualConvIndex_p+1);
  //Only one beam for now...but later this should be able to
  // take all the beams for the different antennas.
  convSupportBlock_p[actualConvIndex_p]= new Vector<Int>(1);
  convSizes_p[actualConvIndex_p]= new Vector<Int> (1);
  (*convSupportBlock_p[actualConvIndex_p])[0]=convSupport;
  convFunctions_p.resize(actualConvIndex_p+1);
  convWeights_p.resize(actualConvIndex_p+1);
  convFunctions_p[actualConvIndex_p]= new Cube<Complex>();
  convWeights_p[actualConvIndex_p]= new Cube<Complex>();
  Int newConvSize=2*(convSupport+2)*convSampling;
  //NEED to chop this right ...and in the centre
  if(newConvSize < convSize){
    IPosition blc(2, (convSize/2)-(newConvSize/2),
		  (convSize/2)-(newConvSize/2));
    IPosition trc(2, (convSize/2)+(newConvSize/2-1),
		  (convSize/2)+(newConvSize/2-1));
    convFunctions_p[actualConvIndex_p]->resize(newConvSize, newConvSize, 1);
    convFunctions_p[actualConvIndex_p]->xyPlane(0)=convFunc(blc,trc);
    convSize=newConvSize;
    convWeights_p[actualConvIndex_p]->resize(newConvSize, newConvSize, 1);
    convWeights_p[actualConvIndex_p]->xyPlane(0)=twoDPB2.get(True)(blc,trc)*Complex(1.0/pbSum,0.0);

    convFunc.resize();//break any reference
    weightConvFunc_p.resize();
    convFunc.reference(convFunctions_p[actualConvIndex_p]->xyPlane(0));
    weightConvFunc_p.reference(convWeights_p[actualConvIndex_p]->xyPlane(0));
    (*convSizes_p[actualConvIndex_p])[0]=convSize;
  }
  else{
    convFunctions_p[actualConvIndex_p]->resize(convSize, convSize,1);
    convFunctions_p[actualConvIndex_p]->xyPlane(0)=convFunc;
  }


  if(0) {
    PagedImage<Float> thisScreen(skyCoverage_p->shape(), 
				 skyCoverage_p->coordinates(), "Screen");
    thisScreen.copyData(*skyCoverage_p);
  }

}

void MosaicFT::initializeToVis(ImageInterface<Complex>& iimage,
			       const VisBuffer& vb)
{
  image=&iimage;
  
  ok();
  
  //  if(convSize==0) {
    init();
    findConvFunction(*image, vb);
    //  }
  
  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);
  
  if((image->shape().product())>cachesize) {
    isTiled=True;
  }
  else {
    isTiled=False;
  }
  //For now isTiled=False
  isTiled=False;
  nx    = image->shape()(0);
  ny    = image->shape()(1);
  npol  = image->shape()(2);
  nchan = image->shape()(3);

  // If we are memory-based then read the image in and create an
  // ArrayLattice otherwise just use the PagedImage
  if(isTiled) {
    lattice=image;
  }
  else {
    IPosition gridShape(4, nx, ny, npol, nchan);
    griddedData.resize(gridShape);
    griddedData=Complex(0.0);
    
    IPosition stride(4, 1);
    IPosition blc(4, (nx-image->shape()(0))/2,
		  (ny-image->shape()(1))/2, 0, 0);
    IPosition trc(blc+image->shape()-stride);
    
    IPosition start(4, 0);
    griddedData(blc, trc) = image->getSlice(start, image->shape());
    
    if(arrayLattice) delete arrayLattice; arrayLattice=0;
    arrayLattice = new ArrayLattice<Complex>(griddedData);
    lattice=arrayLattice;
  }
  
  AlwaysAssert(lattice, AipsError);
  
  logIO() << LogIO::DEBUGGING << "Starting FFT of image" << LogIO::POST;
  
  if(!sj_p) {

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
  
  logIO() << LogIO::DEBUGGING << "Finished FFT" << LogIO::POST;
  
}


void MosaicFT::initializeToVis(ImageInterface<Complex>& iimage,
			       const VisBuffer& vb,
			       Array<Complex>& griddedVis,
			       Vector<Double>& uvscale){
  
  initializeToVis(iimage, vb);
  griddedVis.assign(griddedData); //using the copy for storage
  uvscale.assign(uvScale);
  
}

void MosaicFT::finalizeToVis()
{
  if(isTiled) {
    
    logIO() << LogOrigin("MosaicFT", "finalizeToVis")  << LogIO::NORMAL;
    
    AlwaysAssert(imageCache, AipsError);
    AlwaysAssert(image, AipsError);
    ostringstream o;
    imageCache->flush();
    imageCache->showCacheStatistics(o);
    logIO() << o.str() << LogIO::POST;
  }
  if(pointingToImage) delete pointingToImage; pointingToImage=0;
}


// Initialize the FFT to the Sky. Here we have to setup and initialize the
// grid. 
void MosaicFT::initializeToSky(ImageInterface<Complex>& iimage,
			       Matrix<Float>& weight,
			       const VisBuffer& vb)
{
  // image always points to the image
  image=&iimage;
  
  //  if(convSize==0) {
    init();
    findConvFunction(*image, vb);
    //  }
  
  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);
  
  if((image->shape().product())>cachesize) {
    isTiled=True;
  }
  else {
    isTiled=False;
  }
  //For now isTiled has to be false
  isTiled=False;
  nx    = image->shape()(0);
  ny    = image->shape()(1);
  npol  = image->shape()(2);
  nchan = image->shape()(3);

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
    if( !doneWeightImage_p && (convWeightImage_p==0)){
      
      convWeightImage_p=new  TempImage<Complex> (iimage.shape(), 
						 iimage.coordinates());




      convWeightImage_p->set(Complex(0.0));
      weightLattice=convWeightImage_p;

    }
  }
  else {
    IPosition gridShape(4, nx, ny, npol, nchan);
    griddedData.resize(gridShape);
    griddedData=Complex(0.0);
    if(arrayLattice) delete arrayLattice; arrayLattice=0;
    arrayLattice = new ArrayLattice<Complex>(griddedData);
    lattice=arrayLattice;
      
    if( !doneWeightImage_p && (convWeightImage_p==0)){
     
      
 
      convWeightImage_p=new  TempImage<Complex> (iimage.shape(), 
						 iimage.coordinates());
      griddedWeight.resize(gridShape);
      IPosition stride(4, 1);
      IPosition blc(4, (nx-image->shape()(0))/2,
		    (ny-image->shape()(1))/2, 0, 0);
      IPosition trc(blc+image->shape()-stride);
      
      griddedWeight(blc, trc).set(Complex(0.0));

      if(weightLattice) delete weightLattice; weightLattice=0;
      weightLattice = new ArrayLattice<Complex>(griddedWeight);

    }
    //Get the Stokes coordinates right  his should go in StokeImageUtil
   if(!doneWeightImage_p) {
      Vector<Int> whichStokes(npol);
      CoordinateSystem coords=convWeightImage_p->coordinates();
      Int stokesIndex=coords.findCoordinate(Coordinate::STOKES);
      switch(npol) {
      case 1:
	whichStokes(0)=Stokes::I;
	break;
      case 2:
	whichStokes(0)=Stokes::I;
	whichStokes(1)=Stokes::V;
      break;
      default:
	whichStokes(0)=Stokes::I;
	whichStokes(1)=Stokes::Q;
	whichStokes(2)=Stokes::U;
	whichStokes(3)=Stokes::V;
      }
      StokesCoordinate newStokesCoord(whichStokes);
      coords.replaceCoordinate(newStokesCoord, stokesIndex);
      convWeightImage_p->setCoordinateInfo(coords);

   }

  }
  AlwaysAssert(lattice, AipsError);
  
}

void MosaicFT::finalizeToSky()
{
  
  // Now we flush the cache and report statistics
  // For memory based, we don't write anything out yet.
  if(isTiled) {
    logIO() << LogOrigin("MosaicFT", "finalizeToSky")  << LogIO::NORMAL;
    
    AlwaysAssert(image, AipsError);
    AlwaysAssert(imageCache, AipsError);
    imageCache->flush();
    ostringstream o;
    imageCache->showCacheStatistics(o);
    logIO() << o.str() << LogIO::POST;
  }
  if(!doneWeightImage_p){
    
    LatticeFFT::cfft2d(*weightLattice, False);
    skyCoverage_p=new TempImage<Float> (convWeightImage_p->shape(), convWeightImage_p->coordinates());
    IPosition blc(4, (nx-image->shape()(0))/2,
		    (ny-image->shape()(1))/2, 0, 0);
    IPosition stride(4, 1);
    IPosition trc(blc+image->shape()-stride);
    
    // Do the copy
    IPosition start(4, 0);
    convWeightImage_p->put(griddedWeight(blc, trc));


    StokesImageUtil::To(*skyCoverage_p, *convWeightImage_p);
    delete convWeightImage_p;
    convWeightImage_p=0;
    doneWeightImage_p=True;

    if(0){
      PagedImage<Float> thisScreen(skyCoverage_p->shape(), 
				   skyCoverage_p->coordinates(), "Screen");
      thisScreen.copyData(*skyCoverage_p);
    }
    
  }

  if(pointingToImage) delete pointingToImage; pointingToImage=0;
}

Array<Complex>* MosaicFT::getDataPointer(const IPosition& centerLoc2D,
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
#define gmosft gmosft_
#define dmosft dmosft_
#endif

extern "C" { 
  void gmosft(Double*,
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
	      Int*,
	      Complex*,
	      Int*,
	      Int*,
	      Double*,
	      Complex*,
	      Complex*,
	      Int*);
  void dmosft(Double*,
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
	      Int*,
	      Complex*,
	      Int*,
	      Int*);
}
void MosaicFT::put(const VisBuffer& vb, Int row, Bool dopsf,
		   FTMachine::Type type)
{


  findConvFunction(*image, vb);
  const Cube<Complex> *data;
  if(type==FTMachine::MODEL){
    data=&(vb.modelVisCube());
  }
  else if(type==FTMachine::CORRECTED){
    data=&(vb.correctedVisCube());
  }
  else{
    data=&(vb.visCube());
  }


  Bool isCopy;
  const Complex *datStorage=data->getStorage(isCopy);
 
  
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
  
  doUVWRotation_p=True;
  rotateUVW(uvw, dphase, vb);
  refocus(uvw, vb.antenna1(), vb.antenna2(), dphase, vb);

  // Get the pointing positions. This can easily consume a lot 
  // of time thus we are for now assuming a field per 
  // vb chunk...need to change that accordingly if we start using
  // multiple pointings per vb.
  //Warning 

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
  
  
  //Here we redo the match or use previous match
  
  //Channel matching for the actual spectral window of buffer
  if(doConversion_p[vb.spectralWindow()]){
    matchChannel(vb.spectralWindow(), vb);
  }
  else{
    chanMap.resize();
    chanMap=multiChanMap_p[vb.spectralWindow()];
  }


  //Tell the gridder to grid the weights too ...need to do that once only
  Int doWeightGridding=1;
  if(doneWeightImage_p)
    doWeightGridding=-1;
   
  if(isTiled) {
    Double invLambdaC=vb.frequency()(0)/C::c;
    Vector<Double> uvLambda(2);
    Vector<Int> centerLoc2D(2);
    centerLoc2D=0;
    
    // Loop over all rows
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      
      // Calculate uvw for this row at the center frequency
      uvLambda(0)=uvw(0,rownr)*invLambdaC;
      uvLambda(1)=uvw(1,rownr)*invLambdaC;
      centerLoc2D=gridder->location(centerLoc2D, uvLambda);
      
      // Is this point on the grid?
      if(gridder->onGrid(centerLoc2D)) {
      
	// Get the tile
	Array<Complex>* dataPtr=getDataPointer(centerLoc2D, False);
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
	IPosition s(flags.shape());
	// Now pass all the information down to a 
	// FORTRAN routine to do the work
	/*	gmosft(uvw.getStorage(del),
	       dphase.getStorage(del),
	       datStorage,
	       &s(0),
	       &s(1),
	       &idopsf,
	       flags.getStorage(del),
	       rowFlags.getStorage(del),
	       vb.imagingWeight().getStorage(del),
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
	       &convSupport,
	       &convSize,
	       &convSampling,
	       convFunc.getStorage(del),
	       chanMap.getStorage(del),
	       polMap.getStorage(del),
	       sumWeight.getStorage(del));
	*/
	//NEED to make a getdataposition for weights too
      }
    }
  }
  else {
    Bool del;
    IPosition s(flags.shape());
    gmosft(uvw.getStorage(del),
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
	   &convSupport,
	   &convSize,
	   &convSampling,
	   convFunc.getStorage(del),
	   chanMap.getStorage(del),
	   polMap.getStorage(del),
	   sumWeight.getStorage(del),
	   griddedWeight.getStorage(del),
	   weightConvFunc_p.getStorage(del),
	   &doWeightGridding
	   );
  }

  data->freeStorage(datStorage, isCopy);

}

void MosaicFT::get(VisBuffer& vb, Int row)
{
  

  findConvFunction(*image, vb);
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
  
  doUVWRotation_p=True;
  rotateUVW(uvw, dphase, vb);
  refocus(uvw, vb.antenna1(), vb.antenna2(), dphase, vb);
  
  Cube<Int> flags(vb.flagCube().shape());
  flags=0;
  flags(vb.flagCube())=True;
  
  
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
    Double invLambdaC=vb.frequency()(0)/C::c;
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
      dmosft(uvw.getStorage(del),
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
	     &convSupport,
	     &convSize,
	     &convSampling,
	     convFunc.getStorage(del),
	     chanMap.getStorage(del),
	     polMap.getStorage(del));
      }
    }
  }
  else {
    Bool del;
    IPosition s(vb.modelVisCube().shape());
    dmosft(uvw.getStorage(del),
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
	   &convSupport,
	   &convSize,
	   &convSampling,
	   convFunc.getStorage(del),
	   chanMap.getStorage(del),
	   polMap.getStorage(del));
  }
}

void MosaicFT::get(VisBuffer& vb, Cube<Complex>& modelVis, 
		   Array<Complex>& griddedVis, Vector<Double>& scale,
		   Int row)
{
  
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
  
  
  //Channel matching for the actual spectral window of buffer
  if(doConversion_p[vb.spectralWindow()]){
    matchChannel(vb.spectralWindow(), vb);
  }
  else{
    chanMap.resize();
    chanMap=multiChanMap_p[vb.spectralWindow()];
  }
  
  Vector<Int> rowFlags(vb.nRow());
  rowFlags(vb.flagRow())=True;
  rowFlags=0;
  if(!usezero_p) {
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      if(vb.antenna1()(rownr)==vb.antenna2()(rownr)) rowFlags(rownr)=1;
    }
  }
  
  Bool del;
  IPosition s(modelVis.shape());
  dmosft(uvw.getStorage(del),
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
	 &convSupport,
	 &convSize,
	 &convSampling,
	 convFunc.getStorage(del),
	 chanMap.getStorage(del),
	 polMap.getStorage(del));
  
}


// Finalize the FFT to the Sky. Here we actually do the FFT and
// return the resulting image
ImageInterface<Complex>& MosaicFT::getImage(Matrix<Float>& weights,
					    Bool normalize) 
{
  AlwaysAssert(lattice, AipsError);
  AlwaysAssert(image, AipsError);
  
  logIO() << LogOrigin("MosaicFT", "getImage") << LogIO::NORMAL;
  
  weights.resize(sumWeight.shape());
  
  convertArray(weights, sumWeight);
  
  // If the weights are all zero then we cannot normalize
  // otherwise we don't care.
  if(max(weights)==0.0) {
    if(normalize) {
      logIO() << LogIO::SEVERE << "No useful data in MosaicFT: weights all zero"
	      << LogIO::POST;
    }
    else {
      logIO() << LogIO::WARN << "No useful data in MosaicFT: weights all zero"
	      << LogIO::POST;
    }
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

      IPosition cursorShape(4, inx, 1, 1, 1);
      IPosition axisPath(4, 0, 1, 2, 3);
      LatticeStepper lsx(lattice->shape(), cursorShape, axisPath);
      LatticeIterator<Complex> lix(*lattice, lsx);
      for(lix.reset();!lix.atEnd();lix++) {
	Int pol=lix.position()(2);
	Int chan=lix.position()(3);
	if(weights(pol, chan)>0.0) {
	  if(!sj_p) {
	    gridder->correctX1D(correction, lix.position()(1));
	    lix.rwVectorCursor()/=correction;
	  }
	  if(normalize) {
	    Complex rnorm(Float(inx)*Float(iny)/weights(pol,chan));
	    lix.rwCursor()*=rnorm;
	  }
	  //	  else {
	  //	    Complex rnorm(Float(inx)*Float(iny));
	  //	    lix.rwCursor()*=rnorm;
	  //	  }
	}
	else {
	  lix.woCursor()=0.0;
	}
      }
    }

    if(!isTiled) {
      // Check the section from the image BEFORE converting to a lattice 
      IPosition blc(4, (nx-image->shape()(0))/2,
		    (ny-image->shape()(1))/2, 0, 0);
      IPosition stride(4, 1);
      IPosition trc(blc+image->shape()-stride);
      
      // Do the copy
      IPosition start(4, 0);
      image->put(griddedData(blc, trc));
    }
  }
  
  return *image;
}

// Get weight image
void MosaicFT::getWeightImage(ImageInterface<Float>& weightImage,
			      Matrix<Float>& weights) 
{
  
  logIO() << LogOrigin("MosaicFT", "getWeightImage") << LogIO::NORMAL;
  
  weights.resize(sumWeight.shape());
  convertArray(weights,sumWeight);
  
  weightImage.copyData(*skyCoverage_p);

 

}

Bool MosaicFT::toRecord(String& error, RecordInterface& outRec, 
			Bool withImage) {
  
  // Save the current MosaicFT object to an output state record
  Bool retval = True;
  Double cacheVal=(Double) cachesize;
  outRec.define("cache", cacheVal);
  outRec.define("tile", tilesize);
  
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

Bool MosaicFT::fromRecord(String& error, const RecordInterface& inRec)
{
  Bool retval = True;
  imageCache=0; lattice=0; arrayLattice=0;
  Double cacheVal;
  inRec.get("cache", cacheVal);
  cachesize=(Long)cacheVal;
  inRec.get("tile", tilesize);
  
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
      IPosition blc(4, (nx-image->shape()(0))/2,
		    (ny-image->shape()(1))/2, 0, 0);
      IPosition start(4, 0);
      IPosition stride(4, 1);
      IPosition trc(blc+image->shape()-stride);
      griddedData(blc, trc) = image->getSlice(start, image->shape());
      
      if(arrayLattice) delete arrayLattice; arrayLattice=0;
      arrayLattice = new ArrayLattice<Complex>(griddedData);
      lattice=arrayLattice;
    }
    
    AlwaysAssert(lattice, AipsError);
    AlwaysAssert(image, AipsError);
  };
  return retval;
}

void MosaicFT::ok() {
  AlwaysAssert(image, AipsError);
}

// Make a plain straightforward honest-to-God image. This returns
// a complex image, without conversion to Stokes. The representation
// is that required for the visibilities.
//----------------------------------------------------------------------
void MosaicFT::makeImage(FTMachine::Type type, 
			 VisSet& vs,
			 ImageInterface<Complex>& theImage,
			 Matrix<Float>& weight) {
  
  
  logIO() << LogOrigin("MosaicFT", "makeImage") << LogIO::NORMAL;
  
  if(type==FTMachine::COVERAGE) {
    logIO() << "Type COVERAGE not defined for Fourier transforms"
	    << LogIO::EXCEPTION;
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

Bool MosaicFT::getXYPos(const VisBuffer& vb, Int row) {
  
  
  const ROMSPointingColumns& act_mspc=vb.msColumns().pointing();
  uInt pointIndex=getIndex(act_mspc, vb.time()(row), vb.timeInterval()(row));
  if((pointIndex<0)||(pointIndex>=act_mspc.time().nrow())) {
    //    ostringstream o;
    //    o << "Failed to find pointing information for time " <<
    //      MVTime(vb.time()(row)/86400.0);
    //    logIO_p << LogIO::DEBUGGING << String(o) << LogIO::POST;
    //    logIO_p << String(o) << LogIO::POST;
    
    worldPosMeas = vb.msColumns().field().phaseDirMeas(vb.fieldId());
  }
  else {
   
      worldPosMeas=act_mspc.directionMeas(pointIndex);
      // Make a machine to convert from the worldPosMeas to the output
      // Direction Measure type for the relevant frame
 

 
  }

  if(!pointingToImage) {
    // Set the frame - choose the first antenna. For e.g. VLBI, we
    // will need to reset the frame per antenna
    FTMachine::mLocation_p=vb.msColumns().antenna().positionMeas()(0);
    mFrame_p=MeasFrame(MEpoch(Quantity(vb.time()(row), "s")),
		       FTMachine::mLocation_p);
    MDirection::Ref outRef(directionCoord.directionType(), mFrame_p);
    pointingToImage = new MDirection::Convert(worldPosMeas, outRef);
  
    if(!pointingToImage) {
      logIO_p << "Cannot make direction conversion machine" << LogIO::EXCEPTION;
    }
  }
  else {
    mFrame_p.resetEpoch(MEpoch(Quantity(vb.time()(row), "s")));
  }
  
  worldPosMeas=(*pointingToImage)(worldPosMeas);
 
  Bool result=directionCoord.toPixel(xyPos, worldPosMeas);
  if(!result) {
    logIO_p << "Failed to find pixel location for " 
	    << worldPosMeas.getAngle().getValue() << LogIO::EXCEPTION;
    return False;
  }
  return result;
  
}
// Get the index into the pointing table for this time. Note that the 
// in the pointing table, TIME specifies the beginning of the spanned
// time range, whereas for the main table, TIME is the centroid.
// Note that the behavior for multiple matches is not specified! i.e.
// if there are multiple matches, the index returned depends on the
// history of previous matches. It is deterministic but not obvious.
// One could cure this by searching but it would be considerably
// costlier.
Int MosaicFT::getIndex(const ROMSPointingColumns& mspc, const Double& time,
		       const Double& interval) {
  Int start=lastIndex_p;
  // Search forwards
  Int nrows=mspc.time().nrow();
  if(nrows<1) {
    //    logIO_p << "No rows in POINTING table - cannot proceed" << LogIO::EXCEPTION;
    return -1;
  }
  for (Int i=start;i<nrows;i++) {
    Double midpoint = mspc.time()(i); // time in POINTING table is midpoint
    // If the interval in the pointing table is negative, use the last
    // entry. Note that this may be invalid (-1) but in that case 
    // the calling routine will generate an error
    if(mspc.interval()(i)<0.0) {
      return lastIndex_p;
    }
    // Pointing table interval is specified so we have to do a match
    else {
      // Is the midpoint of this pointing table entry within the specified
      // tolerance of the main table entry?
      if(abs(midpoint-time) < (mspc.interval()(i)/2.0)) {
	lastIndex_p=i;
	return i;
      }
    }
  }
  // Search backwards
  for (Int i=start;i>=0;i--) {
    Double midpoint = mspc.time()(i); // time in POINTING table is midpoint
    if(mspc.interval()(i)<0.0) {
      return lastIndex_p;
    }
    // Pointing table interval is specified so we have to do a match
    else {
      // Is the midpoint of this pointing table entry within the specified
      // tolerance of the main table entry?
      if(abs(midpoint-time) < (mspc.interval()(i)/2.0)) {
	lastIndex_p=i;
	return i;
      }
    }
  }
  // No match!
  return -1;
}


Bool MosaicFT::checkPBOfField(const VisBuffer& vb){


  Int fieldid=vb.fieldId();
  Int msid=vb.msId();
  String mapid=String::toString(msid)+String("_")+String::toString(fieldid);
  if(convFunctionMap_p.ndefined() == 0){
    convFunctionMap_p.define(mapid, 0);    
    actualConvIndex_p=0;
    return False;
  }
   
 if(!convFunctionMap_p.isDefined(mapid)){
    actualConvIndex_p=convFunctionMap_p.ndefined();
    convFunctionMap_p.define(mapid, actualConvIndex_p);
    return False;
  }
  else{
    actualConvIndex_p=convFunctionMap_p(mapid);
    convFunc.resize(); // break any reference
    weightConvFunc_p.resize(); 
    //Here we will need to use the right xyPlane for different PA range.
    convFunc.reference(convFunctions_p[actualConvIndex_p]->xyPlane(0));
    weightConvFunc_p.reference(convWeights_p[actualConvIndex_p]->xyPlane(0));
    //Again this for one time of antenna only later should be fixed for all 
    // antennas independently
    convSupport=(*convSupportBlock_p[actualConvIndex_p])[0];
    convSize=(*convSizes_p[actualConvIndex_p])[0];

  }
 
 return True;

}


void MosaicFT::addBeamCoverage(ImageInterface<Complex>& pbImage){

  CoordinateSystem cs(pbImage.coordinates());
  //  IPosition blc(4,0,0,0,0);
  //  IPosition trc(pbImage.shape());
  //  trc(0)=trc(0)-1;
  //  trc(1)=trc(1)-1;
  // trc(2)=0;
  //  trc(3)=0;
  WCBox *wbox= new WCBox(LCBox(pbImage.shape()), cs);
  SubImage<Float> toAddTo(*skyCoverage_p, ImageRegion(wbox), True);
  TempImage<Float> beamStokes(pbImage.shape(), cs);
  StokesImageUtil::To(beamStokes, pbImage);
  //  toAddTo.copyData((LatticeExpr<Float>)(toAddTo + beamStokes ));
  skyCoverage_p->copyData((LatticeExpr<Float>)(*skyCoverage_p + beamStokes ));


}

String MosaicFT::name(){
  return machineName_p;
}

} //# NAMESPACE CASA - END

