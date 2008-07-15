//# SDGrid.cc: Implementation of SDGrid class
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
//# $Id: SDGrid.cc,v 19.17 2005/12/29 01:39:16 kgolap Exp $

#include <msvis/MSVis/VisibilityIterator.h>
#include <casa/Quanta/UnitMap.h>
#include <casa/Quanta/UnitVal.h>
#include <casa/Quanta/MVTime.h>
#include <measures/Measures/Stokes.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/SpectralCoordinate.h>
#include <coordinates/Coordinates/StokesCoordinate.h>
#include <coordinates/Coordinates/Projection.h>
#include <ms/MeasurementSets/MSColumns.h>
#include <casa/BasicSL/Constants.h>
#include <synthesis/MeasurementComponents/SDGrid.h>
#include <synthesis/MeasurementComponents/SkyJones.h>
#include <synthesis/MeasurementComponents/SimpCompGridMachine.h>
#include <scimath/Mathematics/RigidVector.h>
#include <msvis/MSVis/StokesVector.h>
#include <synthesis/MeasurementEquations/StokesImageUtil.h>
#include <msvis/MSVis/VisBuffer.h>

#include <components/ComponentModels/Flux.h>
#include <components/ComponentModels/PointShape.h>
#include <components/ComponentModels/ConstantSpectrum.h>

#include <images/Images/ImageInterface.h>
#include <images/Images/TempImage.h>
#include <images/Images/PagedImage.h>
#include <casa/Containers/Block.h>
#include <casa/Arrays/MaskedArray.h>
#include <casa/Arrays/ArrayLogical.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Slice.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Cube.h>
#include <casa/Arrays/MatrixIter.h>
#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>
#include <casa/Exceptions/Error.h>
#include <lattices/Lattices/ArrayLattice.h>
#include <lattices/Lattices/LatticeCache.h>
#include <lattices/Lattices/LatticeExpr.h>
#include <lattices/Lattices/LCBox.h>
#include <lattices/Lattices/SubLattice.h>
#include <lattices/Lattices/LatticeIterator.h>
#include <lattices/Lattices/LatticeStepper.h>
#include <casa/OS/Timer.h>
#include <casa/sstream.h>

namespace casa {

SDGrid::SDGrid(MeasurementSet& ms, SkyJones& sj, Int icachesize, Int itilesize,
	       String iconvType, Int userSupport)
  : FTMachine(), ms_p(&ms), sj_p(&sj), imageCache(0), wImageCache(0),
  cachesize(icachesize), tilesize(itilesize),
  isTiled(False), wImage(0), arrayLattice(0),  wArrayLattice(0), lattice(0), wLattice(0), convType(iconvType),
    pointingToImage(0), userSetSupport_p(userSupport)
{
  //  mspc=new MSPointingColumns(ms_p->pointing());
  lastIndex_p=0;
}

SDGrid::SDGrid(MeasurementSet& ms, MPosition& mLocation, SkyJones& sj, Int icachesize, Int itilesize,
	       String iconvType, Int userSupport)
  : FTMachine(), ms_p(&ms), sj_p(&sj), imageCache(0), wImageCache(0),
  cachesize(icachesize), tilesize(itilesize),
  isTiled(False), wImage(0), arrayLattice(0),  wArrayLattice(0), lattice(0), wLattice(0), convType(iconvType),
    pointingToImage(0), userSetSupport_p(userSupport)
{
  mLocation_p=mLocation;
  //  mspc=new MSPointingColumns(ms_p->pointing());
  lastIndex_p=0;
}

SDGrid::SDGrid(MeasurementSet& ms, Int icachesize, Int itilesize,
	       String iconvType, Int userSupport)
  : FTMachine(), ms_p(&ms), sj_p(0), imageCache(0), wImageCache(0),
  cachesize(icachesize), tilesize(itilesize),
  isTiled(False), wImage(0), arrayLattice(0),  wArrayLattice(0), lattice(0), wLattice(0), convType(iconvType),
    pointingToImage(0), userSetSupport_p(userSupport)
{
  //  mspc=new MSPointingColumns(ms_p->pointing());
  lastIndex_p=0;
}

SDGrid::SDGrid(MeasurementSet& ms, MPosition &mLocation, Int icachesize, Int itilesize,
	       String iconvType, Int userSupport)
  : FTMachine(), ms_p(&ms), sj_p(0), imageCache(0), wImageCache(0),
  cachesize(icachesize), tilesize(itilesize),
  isTiled(False), wImage(0), arrayLattice(0),  wArrayLattice(0), lattice(0), wLattice(0), convType(iconvType),
    pointingToImage(0), userSetSupport_p(userSupport)
{
  mLocation_p=mLocation;
  //  mspc=new MSPointingColumns(ms_p->pointing());
  lastIndex_p=0;
}

//---------------------------------------------------------------------- 
SDGrid& SDGrid::operator=(const SDGrid& other)
{
  if(this!=&other) {
    ms_p=other.ms_p;
    sj_p=other.sj_p;
    imageCache=other.imageCache;
    wImage=other.wImage;
    wImageCache=other.wImageCache;
    cachesize=other.cachesize;
    tilesize=other.tilesize;
    isTiled=other.isTiled;
    lattice=other.lattice;
    arrayLattice=other.arrayLattice;
    wLattice=other.wLattice;
    wArrayLattice=other.wArrayLattice;
    convType=other.convType;
    mspc=other.mspc;
    pointingToImage=other.pointingToImage;
    userSetSupport_p=other.userSetSupport_p;
  };
  return *this;
};

//----------------------------------------------------------------------
// Odds are that it changed.....
Bool SDGrid::changed(const VisBuffer& vb) {
  return False;
}

//----------------------------------------------------------------------
SDGrid::SDGrid(const SDGrid& other)
{
  operator=(other);
}

#define NEED_UNDERSCORES
#if defined(NEED_UNDERSCORES)
#define grdsf grdsf_
#endif

extern "C" { 
   void grdsf(Double*, Double*);
}

//----------------------------------------------------------------------
void SDGrid::init() {

  logIO() << LogOrigin("SDGrid", "init")  << LogIO::NORMAL;

  ok();

  if((image->shape().product())>cachesize) {
    isTiled=True;
  }
  else {
    isTiled=False;
  }
  nx    = image->shape()(0);
  ny    = image->shape()(1);
  npol  = image->shape()(2);
  nchan = image->shape()(3);

  sumWeight.resize(npol, nchan);

  // Set up image cache needed for gridding. For BOX-car convolution
  // we can use non-overlapped tiles. Otherwise we need to use
  // overlapped tiles and additive gridding so that only increments
  // to a tile are written.
  if(imageCache) delete imageCache; imageCache=0;

  convType=downcase(convType);
  logIO() << "Convolution function : " << convType << LogIO::POST;
  if(convType=="pb") {
  }
  else if(convType=="box") {
    convSupport=(userSetSupport_p >= 0) ? userSetSupport_p : 0;
    logIO() << "Support : " << convSupport << " pixels" << LogIO::POST;
    convSampling=100;
    convSize=convSampling*(2*convSupport+2);
    convFunc.resize(convSize);
    convFunc=0.0;
    for (Int i=0;i<convSize/2;i++) {
      convFunc(i)=1.0;
    }
  }
  else if(convType=="sf") {
    // SF
    convSupport=(userSetSupport_p >= 0) ? userSetSupport_p : 3;
    logIO() << "Support : " << convSupport << " pixels" << LogIO::POST;
    convSampling=100;
    convSize=convSampling*(2*convSupport+2);
    convFunc.resize(convSize);
    convFunc=0.0;
    for (Int i=0;i<convSampling*convSupport;i++) {
      Double nu=Double(i)/Double(convSupport*convSampling);
      Double val;
      grdsf(&nu, &val);
      convFunc(i)=(1.0-nu*nu)*val;
    }
  }
  else {
    logIO_p << "Unknown convolution function " << convType << LogIO::EXCEPTION;
  }

  if(wImage) delete wImage; wImage=0;
  wImage = new TempImage<Float>(image->shape(), image->coordinates());

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
    imageCache=new LatticeCache <Complex> (*image, cachesize, tileShape, 
					   tileOverlapVec,
					   (tileOverlap>0.0));

    wImageCache=new LatticeCache <Float> (*wImage, cachesize, tileShape, 
					   tileOverlapVec,
					   (tileOverlap>0.0));

  }
}

// This is nasty, we should use CountedPointers here.
SDGrid::~SDGrid() {
  if(imageCache) delete imageCache; imageCache=0;
  if(arrayLattice) delete arrayLattice; arrayLattice=0;
  if(wImage) delete wImage; wImage=0;
  if(wImageCache) delete wImageCache; wImageCache=0;
  if(wArrayLattice) delete wArrayLattice; wArrayLattice=0;
}

void SDGrid::findPBAsConvFunction(const ImageInterface<Complex>& image,
				  const VisBuffer& vb) {

  // Get the coordinate system and increase the sampling by 
  // a factor of ~ 100.
  CoordinateSystem coords(image.coordinates());

  // Set up the convolution function: make the buffer plenty
  // big so that we can trim it back
  convSupport=128;
  convSampling=100;
  convSize=convSampling*convSupport;
  
  // Make a one dimensional image to calculate the
  // primary beam. We oversample this by a factor of 
  // convSampling.
  Int directionIndex=coords.findCoordinate(Coordinate::DIRECTION);
  AlwaysAssert(directionIndex>=0, AipsError);
  DirectionCoordinate dc=coords.directionCoordinate(directionIndex);
  Vector<Double> sampling;
  sampling = dc.increment();
  sampling*=1.0/Double(convSampling);
  dc.setIncrement(sampling);

  // Set the reference value to the first pointing in the coordinate
  // system used in the POINTING table.
  {
    uInt row=0;
    const ROMSPointingColumns& act_mspc = vb.msColumns().pointing();
    // uInt pointIndex=getIndex(*mspc, vb.time()(row), vb.timeInterval()(row));
    uInt pointIndex=getIndex(act_mspc, vb.time()(row), 
			     vb.timeInterval()(row));
    if((pointIndex<0)||(pointIndex>=act_mspc.time().nrow())) {
      ostringstream o;
      o << "Failed to find pointing information for time " <<
	MVTime(vb.time()(row)/86400.0);
      logIO_p << String(o) << LogIO::EXCEPTION;
    }
    worldPosMeas=act_mspc.directionMeas(pointIndex);
  }
  directionCoord=coords.directionCoordinate(directionIndex);
  dc.setReferenceValue(worldPosMeas.getAngle().getValue());
  Vector<Double> unitVec(2);
  unitVec=0.0;
  dc.setReferencePixel(unitVec);

  coords.replaceCoordinate(dc, directionIndex);

  IPosition pbShape(4, convSize, 2, 1, 1);
  IPosition start(4, 0, 0, 0, 0);

  TempImage<Complex> onedPB(pbShape, coords);

  onedPB.set(Complex(1.0, 0.0));

  AlwaysAssert(sj_p, AipsError);
  sj_p->apply(onedPB, onedPB, vb, 0);
  IPosition pbSlice(4, convSize, 1, 1, 1);
  Vector<Float> tempConvFunc=real(onedPB.getSlice(start, pbSlice, True));

  // Find number of significant points
  uInt cfLen=0;
  for(uInt i=0;i<tempConvFunc.nelements();i++) {
    if(tempConvFunc(i)<1e-4) break;
    cfLen++;
  }
  if(cfLen<1) {
    logIO() << LogIO::DEBUGGING
	    << "Possible problem in primary beam calculation: no points in gridding function"
	    << " - no points to be gridded on this image?" << LogIO::POST;
    cfLen=1;
  }
  Vector<Float> trimConvFunc=tempConvFunc(Slice(0,cfLen-1,1));

  // Now fill in the convolution function vector
  convSupport=cfLen/convSampling;
  convSampling=100;
  convSize=convSampling*(2*convSupport+2);
  convFunc.resize(convSize);
  convFunc=0.0;
  convFunc(Slice(0,cfLen-1,1))=trimConvFunc(Slice(0,cfLen-1,1));

}

// Initialize for a transform from the Sky domain. This means that
// we grid-correct, and FFT the image
void SDGrid::initializeToVis(ImageInterface<Complex>& iimage,
			     const VisBuffer& vb)
{
  image=&iimage;

  ok();

  init();

  if(convType=="pb") {
    findPBAsConvFunction(*image, vb);
  }

  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);

  // First get the CoordinateSystem for the image and then find
  // the DirectionCoordinate
  CoordinateSystem coords=image->coordinates();
  Int directionIndex=coords.findCoordinate(Coordinate::DIRECTION);
  AlwaysAssert(directionIndex>=0, AipsError);
  directionCoord=coords.directionCoordinate(directionIndex);

  if((image->shape().product())>cachesize) {
    isTiled=True;
  }
  else {
    isTiled=False;
  }
  nx    = image->shape()(0);
  ny    = image->shape()(1);
  npol  = image->shape()(2);
  nchan = image->shape()(3);

  // If we are memory-based then read the image in and create an
  // ArrayLattice otherwise just use the PagedImage
  if(isTiled) {
    lattice=image;
    wLattice=wImage;
  }
  else {
    // Make the grid the correct shape and turn it into an array lattice
    IPosition gridShape(4, nx, ny, npol, nchan);
    griddedData.resize(gridShape);
    griddedData = Complex(0.0);

    wGriddedData.resize(gridShape);
    wGriddedData = 0.0;

    if(arrayLattice) delete arrayLattice; arrayLattice=0;
    arrayLattice = new ArrayLattice<Complex>(griddedData);

    if(wArrayLattice) delete wArrayLattice; wArrayLattice=0;
    wArrayLattice = new ArrayLattice<Float>(wGriddedData);
    wArrayLattice->set(0.0);
    wLattice=wArrayLattice;

    // Now find the SubLattice corresponding to the image
    IPosition blc(4, (nx-image->shape()(0))/2, (ny-image->shape()(1))/2, 0, 0);
    IPosition stride(4, 1);
    IPosition trc(blc+image->shape()-stride);
    LCBox gridBox(blc, trc, gridShape);
    SubLattice<Complex> gridSub(*arrayLattice, gridBox, True); 

    // Do the copy
    gridSub.copyData(*image);

    lattice=arrayLattice;
  }

  AlwaysAssert(lattice, AipsError);
  AlwaysAssert(wLattice, AipsError);

}

void SDGrid::finalizeToVis()
{
  if(isTiled) {

    logIO() << LogOrigin("SDGrid", "finalizeToVis")  << LogIO::NORMAL;

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
void SDGrid::initializeToSky(ImageInterface<Complex>& iimage,
			     Matrix<Float>& weight, const VisBuffer& vb)
{
  // image always points to the image
  image=&iimage;

  ok();

  init();

  if(convType=="pb") {
    findPBAsConvFunction(*image, vb);
  }

  // Initialize the maps for polarization and channel. These maps
  // translate visibility indices into image indices
  initMaps(vb);

  if((image->shape().product())>cachesize) {
    isTiled=True;
  }
  else {
    isTiled=False;
  }
  nx    = image->shape()(0);
  ny    = image->shape()(1);
  npol  = image->shape()(2);
  nchan = image->shape()(3);

  sumWeight=0.0;
  weight.resize(sumWeight.shape());
  weight=0.0;

  // First get the CoordinateSystem for the image and then find
  // the DirectionCoordinate
  CoordinateSystem coords=image->coordinates();
  Int directionIndex=coords.findCoordinate(Coordinate::DIRECTION);
  AlwaysAssert(directionIndex>=0, AipsError);
  directionCoord=coords.directionCoordinate(directionIndex);

  // Initialize for in memory or to disk gridding. lattice will
  // point to the appropriate Lattice, either the ArrayLattice for
  // in memory gridding or to the image for to disk gridding.
  if(isTiled) {
    imageCache->flush();
    image->set(Complex(0.0));
    lattice=image;
    wLattice=wImage;
  }
  else {
    IPosition gridShape(4, nx, ny, npol, nchan);
    griddedData.resize(gridShape);
    griddedData=Complex(0.0);
    if(arrayLattice) delete arrayLattice; arrayLattice=0;
    arrayLattice = new ArrayLattice<Complex>(griddedData);
    lattice=arrayLattice;
    wGriddedData.resize(gridShape);
    wGriddedData=0.0;
    if(wArrayLattice) delete wArrayLattice; wArrayLattice=0;
    wArrayLattice = new ArrayLattice<Float>(wGriddedData);
    wLattice=wArrayLattice;
  }
  AlwaysAssert(lattice, AipsError);
  AlwaysAssert(wLattice, AipsError);
}

void SDGrid::finalizeToSky()
{

  // Now we flush the cache and report statistics
  // For memory based, we don't write anything out yet.
  if(isTiled) {
    logIO() << LogOrigin("SDGrid", "finalizeToSky")  << LogIO::NORMAL;

    AlwaysAssert(image, AipsError);
    AlwaysAssert(imageCache, AipsError);
    imageCache->flush();
    ostringstream o;
    imageCache->showCacheStatistics(o);
    logIO() << o.str() << LogIO::POST;
  }

  if(pointingToImage) delete pointingToImage; pointingToImage=0;
}

Array<Complex>* SDGrid::getDataPointer(const IPosition& centerLoc2D,
				       Bool readonly) {
  Array<Complex>* result;
  // Is tiled: get tiles and set up offsets
  centerLoc(0)=centerLoc2D(0);
  centerLoc(1)=centerLoc2D(1);
  result=&imageCache->tile(offsetLoc, centerLoc, readonly);
  return result;
}
Array<Float>* SDGrid::getWDataPointer(const IPosition& centerLoc2D,
				      Bool readonly) {
  Array<Float>* result;
  // Is tiled: get tiles and set up offsets
  centerLoc(0)=centerLoc2D(0);
  centerLoc(1)=centerLoc2D(1);
  result=&wImageCache->tile(offsetLoc, centerLoc, readonly);
  return result;
}

#define NEED_UNDERSCORES
#if defined(NEED_UNDERSCORES)
#define ggridsd ggridsd_
#define dgridsd dgridsd_
#endif

extern "C" { 
   void ggridsd(Double*,
		const Complex*,
                Int*,
                Int*,
                Int*,
		const Int*,
		const Int*,
		const Float*,
		Int*,
		Int*,
		Complex*,
		Float*,
                Int*,
		Int*,
		Int *,
		Int *,
                Int*,
		Int*,
		Float*,
		Int*,
		Int*,
		Double*);
   void dgridsd(Double*,
		Complex*,
                Int*,
                Int*,
		const Int*,
		const Int*,
		Int*,
		Int*,
		const Complex*,
                Int*,
		Int*,
		Int *,
		Int *,
                Int*,
		Int*,
		Float*,
		Int*,
		Int*);
}

void SDGrid::put(const VisBuffer& vb, Int row, Bool dopsf, 
		 FTMachine::Type type)
{
  LogIO os(LogOrigin("SDGrid", "put"));
  
  gridOk(convSupport);
 
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


  //Check if ms has changed then cache new spw and chan selection
  if(vb.newMS()){
    matchAllSpwChans(vb);
    lastIndex_p=0;
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
  flags.resize(vb.flagCube().shape());
  flags=0;
  flags(vb.flagCube())=True;
  Vector<Int> rowFlags(vb.flagRow().nelements());
  rowFlags=0;
  for (Int rownr=startRow; rownr<=endRow; rownr++) {
    if(vb.flagRow()(rownr)) rowFlags(rownr)=1;
  }
  
  // Take care of translation of Bools to Integer
  Int idopsf=0;
  if(dopsf) idopsf=1;

  if(isTiled) {
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      
      if(getXYPos(vb, rownr)) {
	
	IPosition centerLoc2D(2, Int(xyPos(0)), Int(xyPos(1)));
	Array<Complex>* dataPtr=getDataPointer(centerLoc2D, False);
	Array<Float>*  wDataPtr=getWDataPointer(centerLoc2D, False);
	Int aNx=dataPtr->shape()(0);
	Int aNy=dataPtr->shape()(1);
	Vector<Double> actualPos(2);
	for (Int i=0;i<2;i++) {
	  actualPos(i)=xyPos(i)-Double(offsetLoc(i));
	}
	// Now use FORTRAN to do the gridding. Remember to 
	// ensure that the shape and offsets of the tile are 
	// accounted for.
	{
	  Bool del;
	  IPosition s(data->shape());
	  ggridsd(actualPos.getStorage(del),
		  datStorage,
		  &s(0),
		  &s(1),
		  &idopsf,
		  flags.getStorage(del),
		  rowFlags.getStorage(del),
		  vb.imagingWeight().getStorage(del),
		  &s(2),
		  &rownr,
		  dataPtr->getStorage(del),
		  wDataPtr->getStorage(del),
		  &aNx,
		  &aNy,
		  &npol,
		  &nchan,
		  &convSupport,
		  &convSampling,
		  convFunc.getStorage(del),
		  chanMap.getStorage(del),
		  polMap.getStorage(del),
		  sumWeight.getStorage(del));
	}
      }
    }
  }
  else {
    Matrix<Double> xyPositions(2, endRow-startRow+1);
    xyPositions=0.0;
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      if(getXYPos(vb, rownr)) {
	xyPositions(0, rownr)=xyPos(0);
	xyPositions(1, rownr)=xyPos(1);
      }
    }
    {
      Bool del;
      IPosition s(data->shape());
      ggridsd(xyPositions.getStorage(del),
	      datStorage,
	      &s(0),
	      &s(1),
	      &idopsf,
	      flags.getStorage(del),
	      rowFlags.getStorage(del),
	      vb.imagingWeight().getStorage(del),
	      &s(2),
	      &row,
	      griddedData.getStorage(del),
	      wGriddedData.getStorage(del),
	      &nx,
	      &ny,
	      &npol,
	      &nchan,
	      &convSupport,
	      &convSampling,
	      convFunc.getStorage(del),
	      chanMap.getStorage(del),
	      polMap.getStorage(del),
	      sumWeight.getStorage(del));
    }
  }
  data->freeStorage(datStorage, isCopy);

}

void SDGrid::get(VisBuffer& vb, Int row)
{
  LogIO os(LogOrigin("SDGrid", "get"));

  gridOk(convSupport);
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


  //Check if ms has changed then cache new spw and chan selection
  if(vb.newMS()){
    matchAllSpwChans(vb);
    lastIndex_p=0;
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


  // NOTE: with MS V2.0 the pointing could change per antenna and timeslot
  //
  flags.resize(vb.flagCube().shape());
  flags=0;
  flags(vb.flagCube())=True;
  Vector<Int> rowFlags(vb.flagRow().nelements());
  rowFlags=0;
  for (Int rownr=startRow; rownr<=endRow; rownr++) {
    if(vb.flagRow()(rownr)) rowFlags(rownr)=1;
  }

  if(isTiled) {
    
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      
      if(getXYPos(vb, rownr)) {
	  
	  // Get the tile
	IPosition centerLoc2D(2, Int(xyPos(0)), Int(xyPos(1)));
	Array<Complex>* dataPtr=getDataPointer(centerLoc2D, True);
	Int aNx=dataPtr->shape()(0);
	Int aNy=dataPtr->shape()(1);
	
	// Now use FORTRAN to do the gridding. Remember to 
	// ensure that the shape and offsets of the tile are 
	// accounted for.
	Bool del;
	Vector<Double> actualPos(2);
	for (Int i=0;i<2;i++) {
	  actualPos(i)=xyPos(i)-Double(offsetLoc(i));
	}
	IPosition s(vb.modelVisCube().shape());
	dgridsd(actualPos.getStorage(del),
		vb.modelVisCube().getStorage(del),
		&s(0),
		&s(1),
		flags.getStorage(del),
		rowFlags.getStorage(del),
		&s(2),
		&rownr,
		dataPtr->getStorage(del),
		&aNx,
		&aNy,
		&npol,
		&nchan,
		&convSupport,
		&convSampling,
		convFunc.getStorage(del),
		chanMap.getStorage(del),
		polMap.getStorage(del));
      }
    }
  }
  else {
    Matrix<Double> xyPositions(2, endRow-startRow+1);
    xyPositions=0.0;
    for (Int rownr=startRow; rownr<=endRow; rownr++) {
      if(getXYPos(vb, rownr)) {
	xyPositions(0, rownr)=xyPos(0);
	xyPositions(1, rownr)=xyPos(1);
      }
    }

    Bool del;
    IPosition s(vb.modelVisCube().shape());
    dgridsd(xyPositions.getStorage(del),
	    vb.modelVisCube().getStorage(del),
	    &s(0),
	    &s(1),
	    flags.getStorage(del),
	    rowFlags.getStorage(del),
	    &s(2),
	    &row,
	    griddedData.getStorage(del),
	    &nx,
	    &ny,
	    &npol,
	    &nchan,
	    &convSupport,
	    &convSampling,
	    convFunc.getStorage(del),
	    chanMap.getStorage(del),
	    polMap.getStorage(del));
  }
}

// Finalize : optionally normalize by weight image
ImageInterface<Complex>& SDGrid::getImage(Matrix<Float>& weights,
					  Bool normalize) 
{
  AlwaysAssert(lattice, AipsError);
  AlwaysAssert(image, AipsError);

  logIO() << LogOrigin("SDGrid", "getImage") << LogIO::NORMAL;

  weights.resize(sumWeight.shape());

  convertArray(weights,sumWeight);

  // If the weights are all zero then we cannot normalize
  // otherwise we don't care.
  if(normalize) {
    if(max(weights)==0.0) {
      logIO() << LogIO::SEVERE << "No useful data in SDGrid: weights all zero"
	      << LogIO::POST;
    }
    lattice->copyData((LatticeExpr<Complex>)(iif((*wLattice<=0.0), 0.0,
						 (*lattice)/(*wLattice))));
  }
  
  if(!isTiled) {
    // Now find the SubLattice corresponding to the image
    IPosition gridShape(4, nx, ny, npol, nchan);
    IPosition blc(4, (nx-image->shape()(0))/2,
		  (ny-image->shape()(1))/2, 0, 0);
    IPosition stride(4, 1);
    IPosition trc(blc+image->shape()-stride);
    LCBox gridBox(blc, trc, gridShape);
    SubLattice<Complex> gridSub(*arrayLattice, gridBox); 
    
    // Do the copy
    image->copyData(gridSub);
  }
  return *image;
}

// Return weights image
void SDGrid::getWeightImage(ImageInterface<Float>& weightImage, Matrix<Float>& weights)
{
  AlwaysAssert(lattice, AipsError);
  AlwaysAssert(image, AipsError);

  logIO() << LogOrigin("SDGrid", "getWeightImage") << LogIO::NORMAL;

  weights.resize(sumWeight.shape());
  convertArray(weights,sumWeight);

  weightImage.copyData(*wArrayLattice);
}

void SDGrid::ok() {
  AlwaysAssert(image, AipsError);
}

// Get the index into the pointing table for this time. Note that the 
// in the pointing table, TIME specifies the beginning of the spanned
// time range, whereas for the main table, TIME is the centroid.
// Note that the behavior for multiple matches is not specified! i.e.
// if there are multiple matches, the index returned depends on the
// history of previous matches. It is deterministic but not obvious.
// One could cure this by searching but it would be considerably
// costlier.
Int SDGrid::getIndex(const ROMSPointingColumns& mspc, const Double& time,
		     const Double& interval) {
  Int start=lastIndex_p;
  // Search forwards
  Int nrows=mspc.time().nrow();
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

Bool SDGrid::getXYPos(const VisBuffer& vb, Int row) {


  const ROMSPointingColumns& act_mspc=vb.msColumns().pointing();
  uInt pointIndex=getIndex(act_mspc, vb.time()(row), vb.timeInterval()(row));
  if((pointIndex<0)||(pointIndex>=act_mspc.time().nrow())) {
    ostringstream o;
    o << "Failed to find pointing information for time " <<
      MVTime(vb.time()(row)/86400.0) << ": Omitting this point";
    logIO_p << LogIO::DEBUGGING << String(o) << LogIO::POST;
    //    logIO_p << String(o) << LogIO::POST;
    return False;
  }

  MEpoch epoch(Quantity(vb.time()(row), "s"));
  if(!pointingToImage) {
    // Set the frame 
    MPosition nullPos;
    mFrame_p=MeasFrame(epoch, FTMachine::mLocation_p);

    worldPosMeas=act_mspc.directionMeas(pointIndex);
    // Make a machine to convert from the worldPosMeas to the output
    // Direction Measure type for the relevant frame
    MDirection::Ref outRef(directionCoord.directionType(), mFrame_p);
    pointingToImage = new MDirection::Convert(worldPosMeas, outRef);
					      
    if(!pointingToImage) {
      logIO_p << "Cannot make direction conversion machine" << LogIO::EXCEPTION;
    }
  }
  else {
    mFrame_p.resetEpoch(epoch);
    mFrame_p.resetPosition(FTMachine::mLocation_p);
  }
  worldPosMeas=(*pointingToImage)(act_mspc.directionMeas(pointIndex));
  Bool result=directionCoord.toPixel(xyPos, worldPosMeas);
  if(!result) {
    logIO_p << "Failed to find pixel location for " 
	    << worldPosMeas.getAngle().getValue() << LogIO::EXCEPTION;
    return False;
  }
  return result;

  // Convert to pixel coordinates
}

} //#End casa namespace
