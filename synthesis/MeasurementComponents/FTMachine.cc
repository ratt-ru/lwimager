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
//# $Id$

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
#include <casa/Arrays/ArrayIter.h>
#include <casa/Arrays/ArrayLogical.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/MatrixMath.h>
#include <casa/Arrays/MaskedArray.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/MatrixIter.h>
#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>
#include <casa/Utilities/BinarySearch.h>
#include <casa/Exceptions/Error.h>
#include <scimath/Mathematics/NNGridder.h>
#include <scimath/Mathematics/ConvolveGridder.h>
#include <measures/Measures/UVWMachine.h>

#include <casa/System/ProgressMeter.h>

#include <casa/OS/Timer.h>
#include <casa/sstream.h>
#include <casa/iostream.h>

namespace casa { //# NAMESPACE CASA - BEGIN
  
  FTMachine::FTMachine() : image(0), uvwMachine_p(0), 
			   tangentSpecified_p(False), fixMovingSource_p(False),
			   distance_p(0.0), lastFieldId_p(-1),lastMSId_p(-1), 
			   useDoubleGrid_p(False), 
			   freqFrameValid_p(False), 
			   freqInterpMethod_p(InterpolateArray1D<Double,Complex>::nearestNeighbour), 
			   pointingDirCol_p("DIRECTION"),
			   cfStokes_p(), cfCache_p(), cfs_p(), cfwts_p(), canComputeResiduals_p(False)
  {
    spectralCoord_p=SpectralCoordinate();
    isIOnly=False;
    spwChanSelFlag_p=0;
    polInUse_p=0;
  }
  
  FTMachine::FTMachine(CountedPtr<CFCache>& cfcache,CountedPtr<ConvolutionFunction>& cf):
    image(0), uvwMachine_p(0), 
    tangentSpecified_p(False), fixMovingSource_p(False),
    distance_p(0.0), lastFieldId_p(-1),lastMSId_p(-1), 
    useDoubleGrid_p(False), 
    freqFrameValid_p(False), 
    freqInterpMethod_p(InterpolateArray1D<Double,Complex>::nearestNeighbour), 
    pointingDirCol_p("DIRECTION"),
    cfStokes_p(), cfCache_p(cfcache), cfs_p(), cfwts_p(),
    convFuncCtor_p(cf),canComputeResiduals_p(False)
  {
    spectralCoord_p=SpectralCoordinate();
    isIOnly=False;
    spwChanSelFlag_p=0;
    polInUse_p=0;
  }
  
  LogIO& FTMachine::logIO() {return logIO_p;};
  
  //---------------------------------------------------------------------- 
  FTMachine& FTMachine::operator=(const FTMachine& other)
  {
    if(this!=&other) {
      image=other.image;
      //generic selection stuff and state
      nAntenna_p=other.nAntenna_p;
      distance_p=other.distance_p;
      lastFieldId_p=other.lastFieldId_p;
      lastMSId_p=other.lastMSId_p;
      
      tangentSpecified_p=other.tangentSpecified_p;
      mTangent_p=other.mTangent_p;
      mImage_p=other.mImage_p;
      mFrame_p=other.mFrame_p;
      
      nx=other.nx;
      ny=other.ny;
      npol=other.npol;
      nchan=other.nchan;
      nvischan=other.nvischan;
      nvispol=other.nvispol;
      mLocation_p=other.mLocation_p;
      if(uvwMachine_p)
	delete uvwMachine_p;
      if(other.uvwMachine_p)
	uvwMachine_p=new UVWMachine(*other.uvwMachine_p);
      else
	uvwMachine_p=0;
      doUVWRotation_p=other.doUVWRotation_p;
      //Spectral and pol stuff 
      freqInterpMethod_p=other.freqInterpMethod_p;
      spwChanSelFlag_p.resize();
      spwChanSelFlag_p=other.spwChanSelFlag_p;
      freqFrameValid_p=other.freqFrameValid_p;
      selectedSpw_p.resize();
      selectedSpw_p=other.selectedSpw_p;
      imageFreq_p.resize();
      imageFreq_p=other.imageFreq_p;
      lsrFreq_p.resize();
      lsrFreq_p=other.lsrFreq_p;
      interpVisFreq_p.resize();
      interpVisFreq_p=other.interpVisFreq_p;
      multiChanMap_p=other.multiChanMap_p;
      chanMap.resize();
      chanMap=other.chanMap;
      polMap.resize();
      polMap=other.polMap;
      nVisChan_p.resize();
      nVisChan_p=other.nVisChan_p;
      spectralCoord_p=other.spectralCoord_p;
      doConversion_p.resize();
      doConversion_p=other.doConversion_p;
      pointingDirCol_p=other.pointingDirCol_p;
      //moving source stuff
      movingDir_p=other.movingDir_p;
      fixMovingSource_p=other.fixMovingSource_p;
      firstMovingDir_p=other.firstMovingDir_p;
      
      //Double precision gridding for those FTMachines that can do
      useDoubleGrid_p=other.useDoubleGrid_p;
      cfStokes_p = other.cfStokes_p;
      polInUse_p = other.polInUse_p;
      cfs_p = other.cfs_p;
      cfwts_p = other.cfwts_p;
      canComputeResiduals_p = other.canComputeResiduals_p;
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
  
  Bool FTMachine::doublePrecGrid(){
    return useDoubleGrid_p;
  }
  
  //----------------------------------------------------------------------
  void FTMachine::initPolInfo(const VisBuffer& vb)
  {
    //
    // Need to figure out where to compute the following arrays/ints
    // in the re-factored code.
    // ----------------------------------------------------------------
    {
      polInUse_p = 0;
      uInt N=0;
      for(uInt i=0;i<polMap.nelements();i++) if (polMap(i) > -1) polInUse_p++;
      cfStokes_p.resize(polInUse_p);
      for(uInt i=0;i<polMap.nelements();i++) 
	if (polMap(i) > -1) {cfStokes_p(N) = vb.corrType()(i);N++;}
    }
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
    
    // get the first position of moving source
    if(fixMovingSource_p){
      
      //First convert to HA-DEC or AZEL for parallax correction
      MDirection::Ref outref1(MDirection::AZEL, mFrame_p);
      MDirection tmphadec=MDirection::Convert(movingDir_p, outref1)();
      MDirection::Ref outref(directionCoord.directionType(), mFrame_p);
      firstMovingDir_p=MDirection::Convert(tmphadec, outref)();
      
    }
    
    
    // Now we need MDirection of the image phase center. This is
    // what we define it to be. So we define it to be the
    // center pixel. So we have to do the conversion here.
    // This is independent of padding since we just want to know 
    // what the world coordinates are for the phase center
    // pixel
    {
      Vector<Double> pixelPhaseCenter(2);
      pixelPhaseCenter(0) = Double( image->shape()(0) / 2 );
      pixelPhaseCenter(1) = Double( image->shape()(1) / 2 );
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
    if(observatory.contains("ATCA") || observatory.contains("DRAO")
       || observatory.contains("WSRT")){
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
    
    //Store the image/grid channels freq values
    {
      Int chanNumbre=image->shape()(3);
      Vector<Double> pixindex(chanNumbre);
      imageFreq_p.resize(chanNumbre);
      Vector<Double> tempStorFreq(chanNumbre);
      indgen(pixindex);
      //    pixindex=pixindex+1.0; 
      for (Int ll=0; ll< chanNumbre; ++ll){
	if( !spectralCoord_p.toWorld(tempStorFreq(ll), pixindex(ll))){
	  logIO() << "Cannot get imageFreq " << LogIO::EXCEPTION;
	  
	}
      }
      convertArray(imageFreq_p,tempStorFreq);
    }
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
    interpVisFreq_p.resize();
    interpVisFreq_p=vb.frequency();
    if(selectedSpw_p.nelements() < 1){
      Vector<Int> myspw(1);
      myspw[0]=vb.spectralWindow();
      setSpw(myspw, freqFrameValid_p);
    }
    
    matchAllSpwChans(vb);
    
    chanMap.resize();
    
    //  cout << "VBSPW " << vb.spectralWindow() << "  " << multiChanMap_p[vb.spectralWindow()] << endl;
    chanMap=multiChanMap_p[vb.spectralWindow()];
    if(chanMap.nelements() == 0)
      chanMap=Vector<Int>(vb.frequency().nelements(), -1);
    
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
    
    initPolInfo(vb);
    //cerr<<"spwchanselflag_p shape="<<spwChanSelFlag_p.shape()<<endl;
    /* STOKESDBG */ //cout << "FtMachine::initPolMaps - polMap : " << polMap << "   vispolmap : " << visPolMap << endl;
  }
  
  FTMachine::~FTMachine() 
  {
    if(uvwMachine_p) delete uvwMachine_p; uvwMachine_p=0;
  }

  FTMachine* FTMachine::clone() const
  {
    throw AipsError ("FTMachine::clone not implemented in derived class");
  }

  Bool FTMachine::interpolateFrequencyTogrid(const VisBuffer& vb,
					     const Matrix<Float>& wt,
					     Cube<Complex>& data, 
					     Cube<Int>& flags, 
					     Matrix<Float>& weight, 
					     FTMachine::Type type){
    Cube<Complex> origdata;
    Cube<Bool> modflagCube;
    Vector<Double> visFreq(vb.frequency().nelements());
    if(doConversion_p[vb.spectralWindow()]){
      visFreq.resize(lsrFreq_p.shape());
      convertArray(visFreq, lsrFreq_p);
    }
    else{      
      convertArray(visFreq, vb.frequency());
      lsrFreq_p.resize();
      lsrFreq_p=vb.frequency();
    }
    if(type==FTMachine::MODEL){
      origdata.reference(vb.modelVisCube());
    }
    else if(type==FTMachine::CORRECTED){
      origdata.reference(vb.correctedVisCube());
    }
    else if(type==FTMachine::OBSERVED){
      origdata.reference(vb.visCube());
    }
    else if(type==FTMachine::PSF){
      // make sure its a size 0 data ...psf
      //so avoid reading any data from disk 
      origdata.resize();
      
    }
    else{
      throw(AipsError("Don't know which column is being regridded"));
    }
    if((imageFreq_p.nelements()==1) || (freqInterpMethod_p== InterpolateArray1D<Double, Complex>::nearestNeighbour) || (vb.nChannel()==1)){
      data.reference(origdata);
      // do something here for apply flag based on spw chan sels
      // e.g. 
      // setSpecFlag(vb, chansels_p) -> newflag cube
      setSpectralFlag(vb,modflagCube);
      //flags.resize(vb.flagCube().shape());
      flags.resize(modflagCube.shape());
      flags=0;
      //flags(vb.flagCube())=True;
      flags(modflagCube)=True;
      weight.reference(wt);
      interpVisFreq_p.resize();
      interpVisFreq_p=lsrFreq_p;

      return False;
    }
    
    Cube<Bool>flag;
    
    //okay at this stage we have at least 2 channels
    Double width=fabs(imageFreq_p[1]-imageFreq_p[0])/fabs(visFreq[1]-visFreq[0]);
    //if width is smaller than number of points needed for interpolation ...do it directly
    //
    // If image chan width is more than twice the data chan width, make a new list of
    // data frequencies on which to interpolate. This new list is sync'd with the starting image chan
    // and have the same width as the data chans.
    if(((width >2.0) && (freqInterpMethod_p==InterpolateArray1D<Double, Complex>::linear)) || 
	   (width >4.0) && (freqInterpMethod_p !=InterpolateArray1D<Double, Complex>::linear)){
      Double minVF=min(visFreq);
      Double maxVF=max(visFreq);
      Double minIF=min(imageFreq_p);
      Double maxIF=max(imageFreq_p);
      if( (minIF > maxVF) ||   (maxIF < minVF)){
	//This function should not have been called with image 
	//being out of bound of data...but still
	interpVisFreq_p.resize(imageFreq_p.nelements());
	interpVisFreq_p=imageFreq_p;
	chanMap.resize(interpVisFreq_p.nelements());
	chanMap.set(-1);
      }
      else{ // Make a new list of frequencies.
	Bool found;
	uInt where=0;
	Double interpwidth=visFreq[1]-visFreq[0];
	if(minIF < minVF){ // Need to find the first image-channel with data in it
	  where=binarySearchBrackets(found, imageFreq_p, minVF, imageFreq_p.nelements());
	  if(where != imageFreq_p.nelements()){
	    minIF=imageFreq_p[where];
	  }
	}

	if(maxIF > maxVF){
	   where=binarySearchBrackets(found, imageFreq_p, maxVF, imageFreq_p.nelements());
	   if(where!= imageFreq_p.nelements()){
	    maxIF=imageFreq_p[where];
	   }
	  
	}

        // This new list of frequencies starts at the first image channel minus half image channel.
	// It ends at the last image channel plus half image channel.
	Int ninterpchan=(Int)ceil((maxIF-minIF+fabs(imageFreq_p[1]-imageFreq_p[0]))/fabs(interpwidth));
	chanMap.resize(ninterpchan);
	chanMap.set(-1);
	interpVisFreq_p.resize(ninterpchan);
	interpVisFreq_p[0]=(interpwidth > 0) ? minIF : maxIF;
	interpVisFreq_p[0] -= fabs(imageFreq_p[1]-imageFreq_p[0])/2.0;
	for (Int k=1; k < ninterpchan; ++k){
	  interpVisFreq_p[k] = interpVisFreq_p[k-1]+ interpwidth;
	}

	for (Int k=0; k < ninterpchan; ++k){
	  ///chanmap with width
	  Double nearestchanval = interpVisFreq_p[k]- (imageFreq_p[1]-imageFreq_p[0])/2.0;
	  where=binarySearchBrackets(found, imageFreq_p, nearestchanval, imageFreq_p.nelements());
	  if(where != imageFreq_p.nelements())
	    chanMap[k]=where;
	}

      }// By now, we have a new list of frequencies, synchronized with image channels, but with data chan widths.
    }// end of ' if (we have to make new frequencies) '
    else{
      // Interpolate directly onto output image frequencies.
      interpVisFreq_p.resize(imageFreq_p.nelements());
      convertArray(interpVisFreq_p, imageFreq_p);
      chanMap.resize(interpVisFreq_p.nelements());
      indgen(chanMap);
    }

    // Read flags from the vb.
    setSpectralFlag(vb,modflagCube);

    if(type != FTMachine::PSF){ // Interpolating the data
 	//Need to get  new interpolate functions that interpolate explicitly on the 2nd axis
	//2 swap of axes needed
	Cube<Complex> flipdata;
	Cube<Bool> flipflag;

        // Interpolate the data. 
        //      Input flags are from the previous step ( setSpectralFlag ). 
        //      Output flags contain info about channels that could not be interpolated 
        //                                   (for example, linear interp with only one data point)
	swapyz(flipflag,modflagCube);
	swapyz(flipdata,origdata);
	InterpolateArray1D<Double,Complex>::
	  interpolate(data,flag,interpVisFreq_p,visFreq,flipdata,flipflag,freqInterpMethod_p);
	flipdata.resize();
	swapyz(flipdata,data);
	data.resize();
	data.reference(flipdata);
	flipflag.resize();
	swapyz(flipflag,flag);
	flag.resize();     
	flag.reference(flipflag);
        // Note : 'flag' will get augmented with the flags coming out of weight interpolation
     }
    else
      { // get the flag array to the correct shape.
	// This will get filled at the end of weight-interpolation.
         flag.resize(vb.nCorr(), interpVisFreq_p.nelements(), vb.nRow());
         flag.set(False);
    }
      // Now, interpolate the weights also.
      //   (1) Read in the flags from the vb ( setSpectralFlags -> modflagCube )
      //   (2) Collapse the flags along the polarization dimension to match shape of weight.
       Matrix<Bool> chanflag(wt.shape());
       AlwaysAssert( chanflag.shape()[0]==modflagCube.shape()[1], AipsError);
       AlwaysAssert( chanflag.shape()[1]==modflagCube.shape()[2], AipsError);
       chanflag=False;
       for(uInt pol=0;pol<modflagCube.shape()[0];pol++)
	 chanflag = chanflag | modflagCube.yzPlane(pol);

       // (3) Interpolate the weights.
       //      Input flags are the collapsed vb flags : 'chanflag'
       //      Output flags are in tempoutputflag 
       //            - contains info about channels that couldn't be interpolated.
       Matrix<Float> flipweight;
       flipweight=transpose(wt);
       Matrix<Bool> flipchanflag;
       flipchanflag=transpose(chanflag);
       Matrix<Bool> tempoutputflag;
       InterpolateArray1D<Double,Float>::
	 interpolate(weight,tempoutputflag, interpVisFreq_p, visFreq,flipweight,flipchanflag,freqInterpMethod_p);
       flipweight.resize();
       flipweight=transpose(weight);    
       weight.resize();
       weight.reference(flipweight);
       flipchanflag.resize();
       flipchanflag=transpose(tempoutputflag);
       tempoutputflag.resize();
       tempoutputflag.reference(flipchanflag);

       // (4) Now, fill these flags back into the flag cube 
       //                 so that they get USED while gridding the PSF (and data)
       //      Taking the OR of the flags that came out of data-interpolation 
       //                         and weight-interpolation, in case they're different.
       //      Expanding flags across polarization.  This will destroy any 
       //                          pol-dependent flags for imaging, but msvis::VisImagingWeight 
       //                          uses the OR of flags across polarization anyway
       //                          so we don't lose anything.

       AlwaysAssert( tempoutputflag.shape()[0]==flag.shape()[1], AipsError);
       AlwaysAssert( tempoutputflag.shape()[1]==flag.shape()[2], AipsError);
       for(uInt pol=0;pol<flag.shape()[0];pol++)
	 flag.yzPlane(pol) = tempoutputflag | flag.yzPlane(pol);

       // Fill the output array of image-channel flags.
       flags.resize(flag.shape());
       flags=0;
       flags(flag)=True;

    return True;
  }
  
  void FTMachine::getInterpolateArrays(const VisBuffer& vb,
				       Cube<Complex>& data, Cube<Int>& flags){
    
    
    if((imageFreq_p.nelements()==1) || (freqInterpMethod_p== InterpolateArray1D<Double, Complex>::nearestNeighbour)||  (vb.nChannel()==1)){
      Cube<Bool> modflagCube;
      setSpectralFlag(vb,modflagCube);
      data.reference(vb.modelVisCube());
      //flags.resize(vb.flagCube().shape());
      flags.resize(modflagCube.shape());
      flags=0;
      //flags(vb.flagCube())=True;
      flags(modflagCube)=True;
      interpVisFreq_p.resize();
      interpVisFreq_p=vb.frequency();
      return;
    }
    
    data.resize(vb.nCorr(), imageFreq_p.nelements(), vb.nRow());
    flags.resize(vb.nCorr(), imageFreq_p.nelements(), vb.nRow());
    data.set(Complex(0.0,0.0));
    flags.set(0);
    //no need to degrid channels that does map over this vb
    Int maxchan=max(chanMap);
    for (uInt k =0 ; k < chanMap.nelements() ; ++k){
      if(chanMap(k)==-1)
	chanMap(k)=maxchan;
    }
    Int minchan=min(chanMap);
    if(minchan==maxchan)
      minchan=-1;
    
    
    for(Int k = 0; k < minchan; ++k)
      flags.xzPlane(k).set(1);
    
    for(uInt k = maxchan + 1; k < imageFreq_p.nelements(); ++k)
      flags.xzPlane(k).set(1);
    
    interpVisFreq_p.resize(imageFreq_p.nelements());
    convertArray(interpVisFreq_p, imageFreq_p);
    chanMap.resize(imageFreq_p.nelements());
    indgen(chanMap);
  }
  
  Bool FTMachine::interpolateFrequencyFromgrid(VisBuffer& vb, 
					       Cube<Complex>& data, 
					       FTMachine::Type type){
    
    Cube<Complex> *origdata;
    Vector<Double> visFreq(vb.frequency().nelements());
    
    if(doConversion_p[vb.spectralWindow()]){
      convertArray(visFreq, lsrFreq_p);
    }
    else{
      convertArray(visFreq, vb.frequency());
    }
    
    if(type==FTMachine::MODEL){
      origdata=&(vb.modelVisCube());
    }
    else if(type==FTMachine::CORRECTED){
      origdata=&(vb.correctedVisCube());
    }
    else{
      origdata=&(vb.visCube());
    }
    if((imageFreq_p.nelements()==1) || (freqInterpMethod_p== InterpolateArray1D<Double, Complex>::nearestNeighbour)){
      origdata->reference(data);
      return False;
    }
    
    //Need to get  new interpolate functions that interpolate explicitly on the 2nd axis
    //2 swap of axes needed
    Cube<Complex> flipgrid;
    flipgrid.resize();
    swapyz(flipgrid,data);
    
    Cube<Complex> flipdata((origdata->shape())(0),(origdata->shape())(2),
			   (origdata->shape())(1)) ;
    flipdata.set(Complex(0.0));
    InterpolateArray1D<Double,Complex>::
      interpolate(flipdata,visFreq, imageFreq_p, flipgrid,freqInterpMethod_p);
    swapyz(vb.modelVisCube(),flipdata);
    
    
    return True;
  }
  void FTMachine::rotateUVW(Matrix<Double>& uvw, Vector<Double>& dphase,
			    const VisBuffer& vb)
  {
    
    
    
    //the uvw rotation is done for common tangent reprojection or if the 
    //image center is different from the phasecenter
    // UVrotation is False only if field never changes
    if((vb.fieldId()!=lastFieldId_p) || (vb.msId()!=lastMSId_p))
      doUVWRotation_p=True;
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
	    //Tangent specified is being wrongly used...it should be for a 
	    //Use the safest way  for now.
	    uvwMachine_p=new UVWMachine(mImage_p, vb.phaseCenter(), mFrame_p,
					True, False);
	  }
	  else{
	    uvwMachine_p=new UVWMachine(mImage_p, vb.phaseCenter(), mFrame_p,
					False, True);
	  }
	}
	else {
	  if(observatory.contains("ATCA") || observatory.contains("WSRT")){
	    uvwMachine_p=new UVWMachine(mImage_p, vb.phaseCenter(), mFrame_p,
					True, False);
	  }
	  else{
	    uvwMachine_p=new UVWMachine(mImage_p, vb.phaseCenter(), mFrame_p, 
					False, True);
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
    
    if(withImage){
      Record imrec;
      if(image->toRecord(error, imrec))
	outRecord.defineRecord("image", imrec);
    }
    outRecord.define("nantenna", nAntenna_p);
    outRecord.define("distance", distance_p);
    outRecord.define("lastfieldid", lastFieldId_p);
    outRecord.define("lastmsid", lastMSId_p);
    outRecord.define("tangentspecified", tangentSpecified_p);
    {
      Record tmprec;
      MeasureHolder mh(mTangent_p);
      if(mh.toRecord(error, tmprec))
	outRecord.defineRecord("mtangent_rec", tmprec);
    }
    {
      Record tmprec;
      MeasureHolder mh(mImage_p);
      if(mh.toRecord(error, tmprec))
	outRecord.defineRecord("mimage_rec", tmprec);
    }
    
    //mFrame_p not necessary to save as it is generated from mLocation_p
    outRecord.define("nx", nx);
    outRecord.define("ny", ny);
    outRecord.define("npol", npol);
    outRecord.define("nchan", nchan);
    outRecord.define("nvischan", nvischan);
    outRecord.define("nvispol", nvispol);
    {
      Record tmprec;
      MeasureHolder mh(mLocation_p);
      if(mh.toRecord(error, tmprec))
	outRecord.defineRecord("mlocation_rec", tmprec);
    }


    // Set uvwMachine to NULL ..will force regeneration
    uvwMachine_p=0;
    outRecord.define("douvwrotation", doUVWRotation_p);
    outRecord.define("freqinterpmethod", static_cast<Int>(freqInterpMethod_p));
    outRecord.define("spwchanselflag", spwChanSelFlag_p);
    outRecord.define("freqframevalid", freqFrameValid_p);
    /*
      //Spectral and pol stuff 
      selectedSpw_p.resize();
      selectedSpw_p=other.selectedSpw_p;
      imageFreq_p.resize();
      imageFreq_p=other.imageFreq_p;
      lsrFreq_p.resize();
      lsrFreq_p=other.lsrFreq_p;
      interpVisFreq_p.resize();
      interpVisFreq_p=other.interpVisFreq_p;
      multiChanMap_p=other.multiChanMap_p;
      chanMap.resize();
      chanMap=other.chanMap;
      polMap.resize();
      polMap=other.polMap;
      nVisChan_p.resize();
      nVisChan_p=other.nVisChan_p;
      spectralCoord_p=other.spectralCoord_p;
      doConversion_p.resize();
      doConversion_p=other.doConversion_p;
      pointingDirCol_p=other.pointingDirCol_p;
      //moving source stuff
      movingDir_p=other.movingDir_p;
      fixMovingSource_p=other.fixMovingSource_p;
      firstMovingDir_p=other.firstMovingDir_p;
      
      //Double precision gridding for those FTMachines that can do
      useDoubleGrid_p=other.useDoubleGrid_p;
      cfStokes_p = other.cfStokes_p;
      polInUse_p = other.polInUse_p;
    */
    return False;
  };
  
  Bool FTMachine::fromRecord(String& error, const RecordInterface& inRecord) {
    // Restore an FTMachine from a Record; currently undefined for the 
    // base FTMachine class
    //
    uvwMachine_p=0; //when null it is reconstructed from mImage_p and mFrame_p
    //mFrame_p is not necessary as it is generated in initMaps from mLocation_p
    return False;
  };
  
  // Make a plain straightforward honest-to-FSM image. This returns
  // a complex image, without conversion to Stokes. The representation
  // is that required for the visibilities.
  //----------------------------------------------------------------------
  void FTMachine::makeImage(FTMachine::Type type, 
			    ROVisibilityIterator& vi,
			    ImageInterface<Complex>& theImage,
			    Matrix<Float>& weight) {
    
    
    logIO() << LogOrigin("FTMachine", "makeImage0") << LogIO::NORMAL;
    
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
    Bool useCorrected= !(vi.msColumns().correctedData().isNull());
    if((type==FTMachine::CORRECTED) && (!useCorrected))
      type=FTMachine::OBSERVED;
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
	  put(vb, -1, False, FTMachine::MODEL);
	  break;
	case FTMachine::CORRECTED:
	  put(vb, -1, False, FTMachine::CORRECTED);
	  break;
	case FTMachine::PSF:
	  vb.visCube()=Complex(1.0,0.0);
	  put(vb, -1, True);
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
    Bool useCorrected= !(vi.msColumns().correctedData().isNull());
    if((type==FTMachine::CORRECTED) && (!useCorrected))
      type=FTMachine::OBSERVED;
    
    
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
    Bool anyTopo=False;
    for (uInt k=0; k < selectedSpw_p.nelements(); ++k){ 
      Bool matchthis=matchChannel(selectedSpw_p[k], vb);
      anymatchChan= (anymatchChan || matchthis);
      anyTopo=anyTopo || ((MFrequency::castType(vb.msColumns().spectralWindow().measFreqRef()(selectedSpw_p[k]))==MFrequency::TOPO) && freqFrameValid_p);
    }
    // if TOPO and valid frame things may match later but not now  thus we'll go 
    // through the data 
    // hoping the user made the right choice
    if (!anymatchChan && !anyTopo){
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
      logIO() << " Spectral window " << spw 
	      << " does not seem to have been selected" << LogIO::EXCEPTION;
    nvischan  = nVisChan_p[spw];
    chanMap.resize(nvischan);
    chanMap.set(-1);
    Vector<Double> lsrFreq(0);
    Bool condoo=False;
    
    
    if(freqFrameValid_p){
      vb.lsrFrequency(spw, lsrFreq, condoo);
      doConversion_p[spw]=condoo;
    }
    else{
      lsrFreq=vb.frequency();
      doConversion_p[spw]=False;
    }
    if(lsrFreq.nelements() ==0){
      return False;
    }
    lsrFreq_p.resize(lsrFreq.nelements());
    lsrFreq_p=lsrFreq;
    Vector<Double> c(1);
    c=0.0;
    Vector<Double> f(1);
    Int nFound=0;
    
    
    //cout.precision(10);
    for (Int chan=0;chan<nvischan;chan++) {
      f(0)=lsrFreq[chan];
      if(spectralCoord_p.toPixel(c, f)) {
	Int pixel=Int(floor(c(0)+0.5));  // round to chan freq at chan center 
	//cout << "spw " << spw << " f " << f(0) << " pixel "<< c(0) << "  " << pixel << endl;
	/////////////
	//c(0)=pixel;
	//spectralCoord_p.toWorld(f, c);
	// cout << "f1 " << f(0) << " pixel "<< c(0) << "  " << pixel << endl;
	////////////////
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
	<< "number of pixels "
	<< nx << " on x axis is smaller that the gridding support "
	<< 2*convSupport   << " Please use a larger value " 
	<< LogIO::EXCEPTION;
    }
    
    if (ny <= 2*convSupport) {
      logIO_p 
	<< "number of pixels "
	<< ny << " on y axis is smaller that the gridding support "
	<< 2*convSupport   << " Please use a larger value " 
	<< LogIO::EXCEPTION;
    }
    
  }
  
  void FTMachine::setLocation(const MPosition& loc){
    
    mLocation_p=loc;
    
  }
  
  MPosition& FTMachine::getLocation(){
    
    return mLocation_p;
  }
  
  
  void FTMachine::setMovingSource(const String& sourcename){
    
    fixMovingSource_p=True;
    movingDir_p=MDirection(Quantity(0.0,"deg"), Quantity(90.0, "deg"));
    movingDir_p.setRefString(sourcename);
    
  }

  void FTMachine::setMovingSource(const MDirection& mdir){
    
    fixMovingSource_p=True;
    movingDir_p=mdir;
    
  }
  
  void FTMachine::setFreqInterpolation(const String& method){
    
    String meth=method;
    meth.downcase();
    if(meth.contains("linear")){
      freqInterpMethod_p=InterpolateArray1D<Double,Complex>::linear;
    }
    else if(meth.contains("splin")){
      freqInterpMethod_p=InterpolateArray1D<Double,Complex>::spline;  
    }	    
    else if(meth.contains("cub")){
      freqInterpMethod_p=InterpolateArray1D<Double,Complex>::cubic;
    }
    else{
      freqInterpMethod_p=InterpolateArray1D<Double,Complex>::nearestNeighbour;
    }
  
  }
  
  
  // helper function to swap the y and z axes of a Cube
  void FTMachine::swapyz(Cube<Complex>& out, const Cube<Complex>& in)
  {
    IPosition inShape=in.shape();
    uInt nxx=inShape(0),nyy=inShape(2),nzz=inShape(1);
    //resize breaks  references...so out better have the right shape 
    //if references is not to be broken
    if(out.nelements()==0)
      out.resize(nxx,nyy,nzz);
    Bool deleteIn,deleteOut;
    const Complex* pin = in.getStorage(deleteIn);
    Complex* pout = out.getStorage(deleteOut);
    uInt i=0, zOffset=0;
    for (uInt iz=0; iz<nzz; ++iz, zOffset+=nxx) {
      Int yOffset=zOffset;
      for (uInt iy=0; iy<nyy; ++iy, yOffset+=nxx*nzz) {
	for (uInt ix=0; ix<nxx; ++ix){ 
	  pout[i++] = pin[ix+yOffset];
	}
      }
    }
    out.putStorage(pout,deleteOut);
    in.freeStorage(pin,deleteIn);
  }
  
  // helper function to swap the y and z axes of a Cube
  void FTMachine::swapyz(Cube<Bool>& out, const Cube<Bool>& in)
  {
    IPosition inShape=in.shape();
    uInt nxx=inShape(0),nyy=inShape(2),nzz=inShape(1);
    if(out.nelements()==0)
      out.resize(nxx,nyy,nzz);
    Bool deleteIn,deleteOut;
    const Bool* pin = in.getStorage(deleteIn);
    Bool* pout = out.getStorage(deleteOut);
    uInt i=0, zOffset=0;
    for (uInt iz=0; iz<nzz; iz++, zOffset+=nxx) {
      Int yOffset=zOffset;
      for (uInt iy=0; iy<nyy; iy++, yOffset+=nxx*nzz) {
	for (uInt ix=0; ix<nxx; ix++) pout[i++] = pin[ix+yOffset];
      }
    }
    out.putStorage(pout,deleteOut);
    in.freeStorage(pin,deleteIn);
  }
  
  void FTMachine::setPointingDirColumn(const String& column){
    pointingDirCol_p=column;
    pointingDirCol_p.upcase();
    if( (pointingDirCol_p != "DIRECTION") &&(pointingDirCol_p != "TARGET") && (pointingDirCol_p != "ENCODER") && (pointingDirCol_p != "POINTING_OFFSET") && (pointingDirCol_p != "SOURCE_OFFSET")){
      
      //basically at this stage you don't know what you're doing...so you get the default
      
      pointingDirCol_p="DIRECTION";
      
    }    
  }
  
  String FTMachine::getPointingDirColumnInUse(){
    
    return pointingDirCol_p;
    
  }
  
  void FTMachine::setSpwChanSelection(const Cube<Int>& spwchansels) {
    spwChanSelFlag_p.resize();
    spwChanSelFlag_p=spwchansels;
  }
  
  void FTMachine::setSpectralFlag(const VisBuffer& vb, Cube<Bool>& modflagcube){
    
    modflagcube.resize(vb.flagCube().shape());
    modflagcube=vb.flagCube();
    uInt nchan = vb.nChannel();
    uInt msid = vb.msId();
    uInt selspw = vb.spectralWindow();
    Bool spwFlagIsSet=( (spwChanSelFlag_p.shape()(1) > selspw) && 
			(spwChanSelFlag_p.shape()(0) > msid) && 
			(spwChanSelFlag_p.shape()(2) >=nchan));
    for (uInt i=0;i<nchan;i++) {
      //Flag those channels that  did not get selected...
      //respect the flags from vb  if selected  or 
      //if spwChanSelFlag is wrong shape
      if ((spwFlagIsSet) && (spwChanSelFlag_p(msid,selspw,i)!=1)) {
	modflagcube.xzPlane(i).set(True);
      }
    }
  }
  
} //# NAMESPACE CASA - END

