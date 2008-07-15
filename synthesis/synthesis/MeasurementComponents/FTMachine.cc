//# FTMachine.cc: Implementation of FTMachine class
//# Copyright (C) 1997,1998,1999,2001,2002,2003
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
//# $Id: FTMachine.cc,v 19.21 2006/05/05 09:41:20 tcornwel Exp $

#include <msvis/MSVis/VisibilityIterator.h>
#include <casa/Quanta/Quantum.h>
#include <casa/Quanta/UnitMap.h>
#include <casa/Quanta/UnitVal.h>
#include <measures/Measures/Stokes.h>
#include <casa/Quanta/Euler.h>
#include <casa/Quanta/RotMatrix.h>
#include <measures/Measures/MDirection.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/SpectralCoordinate.h>
#include <coordinates/Coordinates/StokesCoordinate.h>
#include <coordinates/Coordinates/Projection.h>
#include <ms/MeasurementSets/MSColumns.h>
#include <casa/BasicSL/Constants.h>
#include <synthesis/MeasurementComponents/FTMachine.h>
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
#include <casa/Arrays/MatrixMath.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/MatrixIter.h>
#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>
#include <casa/Exceptions/Error.h>
#include <scimath/Mathematics/NNGridder.h>
#include <scimath/Mathematics/ConvolveGridder.h>
#include <measures/Measures/UVWMachine.h>

#include <casa/System/ProgressMeter.h>

#include <casa/OS/Timer.h>
#include <casa/sstream.h>
#include <casa/iostream.h>

namespace casa { //# NAMESPACE CASA - BEGIN

FTMachine::FTMachine() : image(0), uvwMachine_p(0), tangentSpecified_p(False),
			 distance_p(0.0), lastFieldId_p(-1),lastMSId_p(-1), 
			 freqFrameValid_p(False)
{
}

LogIO& FTMachine::logIO() {return logIO_p;};

//---------------------------------------------------------------------- 
FTMachine& FTMachine::operator=(const FTMachine& other)
{
  if(this!=&other) {
    image=other.image;
    uvwMachine_p=other.uvwMachine_p;
  };
  return *this;
};

//----------------------------------------------------------------------
Bool FTMachine::changed(const VisBuffer& vb) {
  return False;
}

//----------------------------------------------------------------------
FTMachine::FTMachine(const FTMachine& other)
{
  operator=(other);
}

//----------------------------------------------------------------------
void FTMachine::initMaps(const VisBuffer& vb) {

  logIO() << LogOrigin("FTMachine", "initMaps") << LogIO::NORMAL;

  AlwaysAssert(image, AipsError);


  // Set the frame for the UVWMachine
  mFrame_p=MeasFrame(MEpoch(Quantity(vb.time()(0), "s")), mLocation_p);

  // First get the CoordinateSystem for the image and then find
  // the DirectionCoordinate
  CoordinateSystem coords=image->coordinates();
  Int directionIndex=coords.findCoordinate(Coordinate::DIRECTION);
  AlwaysAssert(directionIndex>=0, AipsError);
  DirectionCoordinate
    directionCoord=coords.directionCoordinate(directionIndex);

  // Now we need MDirection of the image phase center. This is
  // what we define it to be. So we define it to be the
  // center pixel. So we have to do the conversion here.
  // This is independent of padding since we just want to know 
  // what the world coordinates are for the phase center
  // pixel
  {
    Vector<Double> pixelPhaseCenter(2);
    pixelPhaseCenter(0)=Double(image->shape()(0))/2.0;
    pixelPhaseCenter(1)=Double(image->shape()(1))/2.0;
    directionCoord.toWorld(mImage_p, pixelPhaseCenter);
  }

  // Decide if uvwrotation is not necessary, if phasecenter and
  // image center are with in one pixel distance; Save some 
  //  computation time especially for spectral cubes.
  {
    Vector<Double> equal= (mImage_p.getAngle()-
			   vb.phaseCenter().getAngle()).getValue();
    if((abs(equal(0)) < abs(directionCoord.increment()(0))) 
       && (abs(equal(1)) < abs(directionCoord.increment()(1)))){
      doUVWRotation_p=False;
    }
    else{
      doUVWRotation_p=True;
    }
  }
  // Get the object distance in meters
  Record info(image->miscInfo());
  if(info.isDefined("distance")) {
    info.get("distance", distance_p);
    if(abs(distance_p)>0.0) {
      logIO() << "Distance to object is set to " << distance_p/1000.0
	      << "km: applying focus correction" << LogIO::POST;
    }
  }

  // Set up the UVWMachine. 
  if(uvwMachine_p) delete uvwMachine_p; uvwMachine_p=0;
  String observatory=vb.msColumns().observation().telescopeName()(0);
  if(observatory.contains("ATCA") || observatory.contains("WSRT")){
    uvwMachine_p=new UVWMachine(mImage_p, vb.phaseCenter(), mFrame_p, 
				True, False);
  }
  else{
    uvwMachine_p=new UVWMachine(mImage_p, vb.phaseCenter(), mFrame_p, 
			      False, True);
  }
  AlwaysAssert(uvwMachine_p, AipsError);

  lastFieldId_p=vb.fieldId();
  lastMSId_p=vb.msId();

  // Set up maps
  Int spectralIndex=coords.findCoordinate(Coordinate::SPECTRAL);
  AlwaysAssert(spectralIndex>-1, AipsError);
  spectralCoord_p=coords.spectralCoordinate(spectralIndex);

  //Destroy any conversion layer Freq coord if freqframe is not valid
  if(!freqFrameValid_p){
    MFrequency::Types imageFreqType=spectralCoord_p.frequencySystem();
    spectralCoord_p.setFrequencySystem(imageFreqType);   
    spectralCoord_p.setReferenceConversion(imageFreqType, 
					   MEpoch(Quantity(vb.time()(0), "s")),
					   mLocation_p,
					   mImage_p);
  }

  // Channel map: do this properly by looking up the frequencies
  // If a visibility channel does not map onto an image
  // pixel then we set the corresponding chanMap to -1.
  // This means that put and get must always check for this
  // value (see e.g. GridFT)


  nvischan  = vb.frequency().nelements();
  if(selectedSpw_p.nelements() < 1){
    Vector<Int> myspw(1);
    myspw[0]=vb.spectralWindow();
    setSpw(myspw, freqFrameValid_p);
  }

  matchAllSpwChans(vb);

  chanMap.resize();
  chanMap=multiChanMap_p[vb.spectralWindow()];


  {
    logIO() << LogIO::DEBUGGING << "Channel Map: " << chanMap << LogIO::POST;
  }
  // Should never get here
  if(max(chanMap)>=nchan||min(chanMap)<-1) {
    logIO() << "Illegal Channel Map: " << chanMap << LogIO::EXCEPTION;
  }

  // Polarization map
  Int stokesIndex=coords.findCoordinate(Coordinate::STOKES);
  AlwaysAssert(stokesIndex>-1, AipsError);
  StokesCoordinate stokesCoord=coords.stokesCoordinate(stokesIndex);

  Vector<Int> visPolMap(vb.corrType());
  nvispol=visPolMap.nelements();
  AlwaysAssert(nvispol>0, AipsError);
  polMap.resize(nvispol);
  polMap=-1;
  isIOnly=False;
  Int pol=0;
  Bool found=False;
  // First we try matching Stokes in the visibilities to 
  // Stokes in the image that we are gridding into.
  for (pol=0;pol<nvispol;pol++) {
    Int p=0;
    if(stokesCoord.toPixel(p, Stokes::type(visPolMap(pol)))) {
      AlwaysAssert(p<npol, AipsError);
      polMap(pol)=p;
      found=True;
    }
  }
  // If this fails then perhaps we were looking to grid I
  // directly. If so then we need to check that the parallel
  // hands are present in the visibilities.
  if(!found) {
    Int p=0;
    if(stokesCoord.toPixel(p, Stokes::I)) {
      polMap=-1;
      if(vb.polFrame()==VisibilityIterator::Linear) {
        p=0;
        for (pol=0;pol<nvispol;pol++) {
          if(Stokes::type(visPolMap(pol))==Stokes::XX)
	    {polMap(pol)=0;p++;found=True;};
          if(Stokes::type(visPolMap(pol))==Stokes::YY)
	    {polMap(pol)=0;p++;found=True;};
	}
      }
      else {
        p=0;
        for (pol=0;pol<nvispol;pol++) {
          if(Stokes::type(visPolMap(pol))==Stokes::LL)
	    {polMap(pol)=0;p++;found=True;};
          if(Stokes::type(visPolMap(pol))==Stokes::RR)
	    {polMap(pol)=0;p++;found=True;};
	}
      }
      if(!found) {
	logIO() <<  "Cannot find polarization map: visibility polarizations = "
	  << visPolMap << LogIO::EXCEPTION;
      }
      else {
	isIOnly=True;
	logIO() << LogIO::DEBUGGING << "Transforming I only" << LogIO::POST;
      }
    }; 
  }
  logIO() << LogIO::DEBUGGING << "Polarization map = "<< polMap
  	  << LogIO::POST;
}

FTMachine::~FTMachine() 
{
 if(uvwMachine_p) delete uvwMachine_p; uvwMachine_p=0;
}

void FTMachine::rotateUVW(Matrix<Double>& uvw, Vector<Double>& dphase,
			  const VisBuffer& vb)
{



 //the uvw rotation is done for common tangent reprojection or if the 
 //image center is different from the phasecenter

  if(doUVWRotation_p || tangentSpecified_p){
    ok();
    
    mFrame_p.resetEpoch(MEpoch(Quantity(vb.time()(0), "s")));
    
    // Set up the UVWMachine only if the field id has changed. If
    // the tangent plane is specified then we need a UVWMachine that
    // will reproject to that plane iso the image plane
    if((vb.fieldId()!=lastFieldId_p) || (vb.msId()!=lastMSId_p)) {
      
      String observatory=vb.msColumns().observation().telescopeName()(0);
      if(uvwMachine_p) delete uvwMachine_p; uvwMachine_p=0;
      if(tangentSpecified_p) {
	if(observatory.contains("ATCA") || observatory.contains("WSRT")){
	  uvwMachine_p=new UVWMachine(mTangent_p, vb.phaseCenter(), mFrame_p,
				      True, False);
	}
	else{
	  uvwMachine_p=new UVWMachine(mTangent_p, vb.phaseCenter(), mFrame_p,
				      False, True);
	}
      }
      else {
	if(observatory.contains("ATCA") || observatory.contains("WSRT")){
	  uvwMachine_p=new UVWMachine(mImage_p, vb.phaseCenter(), mFrame_p,
				      True, False);
	}
	else{
	  uvwMachine_p=new UVWMachine(mImage_p, vb.phaseCenter(), mFrame_p);
	}
      }
      lastFieldId_p=vb.fieldId();
      lastMSId_p=vb.msId();
    }
    
    AlwaysAssert(uvwMachine_p, AipsError);
   
    // Always force a recalculation 
    uvwMachine_p->reCalculate();
    
    // Now do the conversions
    uInt nrows=dphase.nelements();
    Vector<Double> thisRow(3);
    thisRow=0.0;
    uInt i;
    for (uInt row=0;row<nrows;row++) {
      for (i=0;i<3;i++) thisRow(i)=uvw(i,row);
      uvwMachine_p->convertUVW(dphase(row), thisRow);
      for (i=0;i<3;i++) uvw(i,row)=thisRow(i);
    }
  }
  
}

//
// Refocus the array on a point at finite distance
//
void FTMachine::refocus(Matrix<Double>& uvw, const Vector<Int>& ant1,
			const Vector<Int>& ant2,
			Vector<Double>& dphase, const VisBuffer& vb)
{

  ok();

  if(abs(distance_p)>0.0) {

    nAntenna_p=max(vb.antenna2())+1;

    // Positions of antennas
    Matrix<Double> antPos(3,nAntenna_p);
    antPos=0.0;
    Vector<Int> nAntPos(nAntenna_p);
    nAntPos=0;
    
    uInt aref = min(ant1);
    
    // Now find the antenna locations: for this we just reference to a common
    // point. We ignore the time variation within this buffer.
    uInt nrows=dphase.nelements();
    for (uInt row=0;row<nrows;row++) {
      uInt a1=ant1(row);
      uInt a2=ant2(row);
      for(uInt dim=0;dim<3;dim++) {
	antPos(dim, a1)+=uvw(dim, row);
	antPos(dim, a2)-=uvw(dim, row);
      }
      nAntPos(a1)+=1;
      nAntPos(a2)+=1;
    }
    
    // Now remove the reference location
    Vector<Double> center(3);
    for(uInt dim=0;dim<3;dim++) {
      center(dim) = antPos(dim,aref)/nAntPos(aref);
    }
    
    // Now normalize
    for (uInt ant=0; ant<nAntenna_p; ant++) {
      if(nAntPos(ant)>0) {
	for(uInt dim=0;dim<3;dim++) {
	  antPos(dim,ant)/=nAntPos(ant);
	  antPos(dim,ant)-=center(dim);
	}
      }
    }
    
    // Now calculate the offset needed to focus the array,
    // including the w term. This must have the correct asymptotic
    // form so that at infinity no net change occurs
    for (uInt row=0;row<nrows;row++) {
      uInt a1=ant1(row);
      uInt a2=ant2(row);

      Double d1=distance_p*distance_p-2*distance_p*antPos(2,a1);
      Double d2=distance_p*distance_p-2*distance_p*antPos(2,a2);
      for(uInt dim=0;dim<3;dim++) {
	d1+=antPos(dim,a1)*antPos(dim,a1);
	d2+=antPos(dim,a2)*antPos(dim,a2);
      }
      d1=sqrt(d1);
      d2=sqrt(d2);
      for(uInt dim=0;dim<2;dim++) {
	dphase(row)-=(antPos(dim,a1)*antPos(dim,a1)-antPos(dim,a2)*antPos(dim,a2))/(2*distance_p);
      }
      uvw(0,row)=distance_p*(antPos(0,a1)/d1-antPos(0,a2)/d2);
      uvw(1,row)=distance_p*(antPos(1,a1)/d1-antPos(1,a2)/d2);
      uvw(2,row)=distance_p*(antPos(2,a1)/d1-antPos(2,a2)/d2)+dphase(row);
    }
  }
}

void FTMachine::ok() {
  AlwaysAssert(image, AipsError);
  AlwaysAssert(uvwMachine_p, AipsError);
}

Bool FTMachine::toRecord(String& error, RecordInterface& outRecord, 
			 Bool withImage) {
  // Save the FTMachine to a Record; currently undefined for the base
  // FTMachine class.
  //
  error="Not defined";
  outRecord = Record();
  return False;
};
  
Bool FTMachine::fromRecord(String& error, const RecordInterface& inRecord) {
  // Restore an FTMachine from a Record; currently undefined for the 
  // base FTMachine class
  //
  error="Not defined";
  return False;
};

// Make a plain straightforward honest-to-God image. This returns
// a complex image, without conversion to Stokes. The representation
// is that required for the visibilities. This version always works
// but has unnecessary operations for synthesis.
//----------------------------------------------------------------------
void FTMachine::makeImage(FTMachine::Type type, 
		       VisSet& vs,
		       ImageInterface<Complex>& theImage,
		       Matrix<Float>& weight) {

  logIO() << LogOrigin("FTMachine", "makeImage") << LogIO::NORMAL;

  // If we want to calculate the PSF, we'll have to fill in the MODEL_DATA
  // column
  if(type==FTMachine::PSF) {

    VisIter& vi(vs.iter());
    
    // Loop over all visibilities and pixels
    VisBuffer vb(vi);
    
    // Initialize put (i.e. transform to Sky) for this model
    vi.origin();

    logIO() << "Calculating MODEL_DATA column from point source model" << LogIO::POST;
    TempImage<Float> pointImage(theImage.shape(), theImage.coordinates());
    IPosition start(4, theImage.shape()(0)/2, theImage.shape()(1)/2, 0, 0);
    IPosition shape(4, 1, 1, 1, theImage.shape()(3));
    Array<Float> line(shape);
    pointImage.set(0.0);
    line=1.0;
    pointImage.putSlice(line, start);
    TempImage<Complex> cPointImage(theImage.shape(), theImage.coordinates());
    StokesImageUtil::From(cPointImage, pointImage);
    if(vb.polFrame()==MSIter::Linear) {
      StokesImageUtil::changeCStokesRep(cPointImage, SkyModel::LINEAR);
    }
    else {
      StokesImageUtil::changeCStokesRep(cPointImage, SkyModel::CIRCULAR);
    }
    initializeToVis(cPointImage, vb);
    // Loop over all visibilities
    for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
      for (vi.origin(); vi.more(); vi++) {
	get(vb, -1);
	vi.setVis(vb.modelVisCube(),VisibilityIterator::Model);
      }
    }
    finalizeToVis();
  }

  ROVisIter& vi(vs.iter());

  // Loop over all visibilities and pixels
  VisBuffer vb(vi);
  
  // Initialize put (i.e. transform to Sky) for this model
  vi.origin();

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
      case FTMachine::PSF:
      case FTMachine::MODEL:
	//vb.visCube()=vb.modelVisCube();
        put(vb, -1, False, FTMachine::MODEL);
        break;
      case FTMachine::CORRECTED:
	//vb.visCube()=vb.correctedVisCube();
        put(vb, -1, False, FTMachine::CORRECTED);
        break;
      case FTMachine::COVERAGE:
	vb.visCube()=Complex(1.0);
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


Bool FTMachine::setSpw(Vector<Int>& spws, Bool validFrame){

  freqFrameValid_p=validFrame;
  if(spws.nelements() >= 1){
    selectedSpw_p.resize();
    selectedSpw_p=spws;
    multiChanMap_p.resize(max(spws)+1);
    return True;
  }

  return False;
}

Bool FTMachine::matchAllSpwChans(const VisBuffer& vb){

  vb.allSelectedSpectralWindows(selectedSpw_p, nVisChan_p);
  doConversion_p.resize(max(selectedSpw_p)+1);
  doConversion_p.set(False);
  
  multiChanMap_p.resize(max(selectedSpw_p)+1, True);
  
  Bool anymatchChan=False;
  for (uInt k=0; k < selectedSpw_p.nelements(); ++k){ 
    Bool matchthis=matchChannel(selectedSpw_p[k], vb);
    anymatchChan= (anymatchChan || matchthis);
    
  }
  if (!anymatchChan){
    logIO() << "No overlap in frequency between image channels and selected data found "
            << " Check your data selection and image parameters" 
	    << LogIO::EXCEPTION;
    return False;
    
  }

  return True;

}

Bool FTMachine::matchChannel(const Int& spw, 
			     const VisBuffer& vb){


  if(nVisChan_p[spw] < 0)
    logIO() << " Spectral window " << spw+1 
	    << " does not seem to have been selected" << LogIO::EXCEPTION;
  nvischan  = nVisChan_p[spw];
  chanMap.resize(nvischan);
  chanMap.set(-1);
  Vector<Double> lsrFreq(0);
  Bool convert=False;
 
  if(freqFrameValid_p){
    vb.lsrFrequency(spw, lsrFreq, convert);
    doConversion_p[spw]=convert;
  }
  else{
    lsrFreq=vb.lsrFrequency();
  }
  

  Vector<Double> c(1);
  c=0.0;
  Vector<Double> f(1);
  Int nFound=0;

  for (Int chan=0;chan<nvischan;chan++) {
    f(0)=lsrFreq[chan];
    if(spectralCoord_p.toPixel(c, f)) {
      Int pixel=Int(floor(c(0)+0.5));
      if(pixel>-1&&pixel<nchan) {
	chanMap(chan)=pixel;
        nFound++;
 	if(nvischan>1&&(chan==0||chan==nvischan-1)) {
	  logIO() << LogIO::DEBUGGING
		  << "Selected visibility channel : " << chan+1
		  << " has frequency "
 		  <<  MFrequency(Quantity(f(0), "Hz")).get("GHz").getValue()
 		  << " GHz and maps to image pixel " << pixel+1 << LogIO::POST;
 	}
      }
    }
  }

  multiChanMap_p[spw].resize();
  multiChanMap_p[spw]=chanMap;

  if(nFound==0) {
    /*
    logIO()  << "Visibility channels in spw " << spw+1 
	     <<      " of ms " << vb.msId() << " is not being used " 
	     << LogIO::WARN << LogIO::POST;
    */
     return False;
  }


  return True;

}

void FTMachine::gridOk(Int convSupport){

  if (nx <= 2*convSupport) {
    logIO_p 
      << "number of pixels on x axis is smaller that the gridding support "
      << 2*convSupport   << " Please use a larger value " 
      << LogIO::EXCEPTION;
  }
  
  if (ny <= 2*convSupport) {
    logIO_p 
      << "number of pixels on y axis is smaller that the gridding support "
      << 2*convSupport   << " Please use a larger value " 
      << LogIO::EXCEPTION;
  }

}














} //# NAMESPACE CASA - END

