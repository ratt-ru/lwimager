//# CubeSkyEquation.cc: Implementation of Cube Optimized Sky Equation classes
//# Copyright (C) 2007
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

#include <casa/iostream.h>
#include <casa/Exceptions/Error.h>
#include <casa/Utilities/Assert.h>
#include <casa/BasicSL/Complex.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/MatrixMath.h>
#include <casa/OS/HostInfo.h>
#include <casa/System/ProgressMeter.h>
#include <casa/Utilities/CountedPtr.h>

#include <coordinates/Coordinates/CoordinateSystem.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/StokesCoordinate.h>
#include <lattices/Lattices/LatticeExpr.h>

#include <synthesis/MeasurementComponents/SkyModel.h>
#include <synthesis/MeasurementEquations/CubeSkyEquation.h>
#include <synthesis/MeasurementComponents/SkyJones.h>
#include <synthesis/MeasurementComponents/FTMachine.h>
#include <synthesis/MeasurementComponents/GridFT.h>
#include <synthesis/MeasurementComponents/MosaicFT.h>
#include <synthesis/MeasurementComponents/GridBoth.h>
#include <synthesis/MeasurementComponents/WProjectFT.h>
#include <synthesis/MeasurementComponents/nPBWProjectFT.h>
#include <synthesis/MeasurementComponents/PBMosaicFT.h>
#include <synthesis/MeasurementComponents/WPConvFunc.h>
#include <synthesis/MeasurementComponents/SimplePBConvFunc.h>
#include <synthesis/MeasurementComponents/ComponentFTMachine.h>
#include <synthesis/MeasurementEquations/StokesImageUtil.h>


#include <images/Images/ImageInterface.h>
#include <images/Images/SubImage.h>

#include <msvis/MSVis/StokesVector.h>
#include <msvis/MSVis/VisBufferUtil.h>
#include <msvis/MSVis/VisSet.h>
#include <msvis/MSVis/VisibilityIterator.h>
#include <msvis/MSVis/VisBuffer.h>




namespace casa { //# NAMESPACE CASA - BEGIN


CubeSkyEquation::CubeSkyEquation(SkyModel& sm, VisSet& vs, FTMachine& ft, ComponentFTMachine& cft, Bool noModelCol)
  : SkyEquation(sm, vs, ft, cft, noModelCol), internalChangesPut_p(False), 
    internalChangesGet_p(False), firstOneChangesPut_p(False), 
    firstOneChangesGet_p(False)
{
 
  init(ft);

}
  CubeSkyEquation::CubeSkyEquation(SkyModel& sm, ROVisibilityIterator& vi, FTMachine& ft, ComponentFTMachine& cft, Bool noModelCol) : SkyEquation(sm, vi, ft, cft, noModelCol), internalChangesPut_p(False),internalChangesGet_p(False), firstOneChangesPut_p(False),firstOneChangesGet_p(False){

    init(ft);
}

void CubeSkyEquation::init(FTMachine& ft){
  Int nmod=sm_->numberOfModels();
  
  //if(sm_->getAlgorithm()=="MSMFS") 
  if(sm_->numberOfTaylorTerms()>1) 
    {
      nmod = (sm_->numberOfModels()/sm_->numberOfTaylorTerms()) * (2 * sm_->numberOfTaylorTerms() - 1);
    }
  
  //case of component ft only
  if(nmod==0)
    nmod=1;
  
  ftm_p.resize(nmod, True);
  iftm_p.resize(nmod, True);
  
  //make a distinct ift_ as gridding and degridding can occur simultaneously
  if(ft.name() == "MosaicFT"){
    ft_=new MosaicFT(static_cast<MosaicFT &>(ft));
    ift_=new MosaicFT(static_cast<MosaicFT &>(ft));
    ftm_p[0]=ft_;
    iftm_p[0]=ift_;
    //For mosaic ...outlier fields get normal GridFT's
    
    MPosition loc=ift_->getLocation();
    for (Int k=1; k < (nmod); ++k){ 
      ftm_p[k]=new GridFT(1000000, 16, "SF", loc, 1.0, False);
      iftm_p[k]=new GridFT(1000000, 16, "SF", loc, 1.0, False);
    }
  }
  else if(ft.name()== "WProjectFT"){
    ft_=new WProjectFT(static_cast<WProjectFT &>(ft));
    ift_=new WProjectFT(static_cast<WProjectFT &>(ft));
    ftm_p[0]=ft_;
    iftm_p[0]=ift_;
    CountedPtr<WPConvFunc> sharedconvFunc= new WPConvFunc();
    static_cast<WProjectFT &>(ft).setConvFunc(sharedconvFunc);
    static_cast<WProjectFT &>(*ftm_p[0]).setConvFunc(sharedconvFunc);
    static_cast<WProjectFT &>(*iftm_p[0]).setConvFunc(sharedconvFunc);
    // For now have all the fields have WProjectFt machines....
    //but should be seperated between GridFT's for the outliers and 
    //WProject for the facets.
    for (Int k=1; k < (nmod); ++k){ 
      ftm_p[k]=new WProjectFT(static_cast<WProjectFT &>(*ft_));
      iftm_p[k]=new WProjectFT(static_cast<WProjectFT &>(*ift_));
      // Give each pair of FTMachine a convolution function set to share
      static_cast<WProjectFT &>(*ftm_p[k]).setConvFunc(sharedconvFunc);
      static_cast<WProjectFT &>(*iftm_p[k]).setConvFunc(sharedconvFunc);

    }
  }
  else if(ft.name()== "GridBoth"){
    ft_=new GridBoth(static_cast<GridBoth &>(ft));
    ift_=new GridBoth(static_cast<GridBoth &>(ft));
    ftm_p[0]=ft_;
    iftm_p[0]=ift_;
    if(nmod > 1){
      throw(AipsError("No multifield with joint gridding allowed"));
    }
  }
  else if(ft.name()== "PBWProjectFT"){
     ft_=new nPBWProjectFT(static_cast<nPBWProjectFT &>(ft));
     ift_=new nPBWProjectFT(static_cast<nPBWProjectFT &>(ft));
     ftm_p[0]=ft_;
     iftm_p[0]=ift_;
     if(nmod != (2 * sm_->numberOfTaylorTerms() - 1)) /* MFS */
       throw(AipsError("No multifield with pb-projection allowed"));
     for (Int k=1; k < (nmod); ++k){ 
      ftm_p[k]=new nPBWProjectFT(static_cast<nPBWProjectFT &>(*ft_));
      iftm_p[k]=new nPBWProjectFT(static_cast<nPBWProjectFT &>(*ift_));
    }
  }
  else if(ft.name()== "PBMosaicFT"){
     ft_=new PBMosaicFT(static_cast<PBMosaicFT &>(ft));
     ift_=new PBMosaicFT(static_cast<PBMosaicFT &>(ft));
     ftm_p[0]=ft_;
     iftm_p[0]=ift_;
     if(nmod != (2 * sm_->numberOfTaylorTerms() - 1)) /* MFS */
       throw(AipsError("No multifield with pb-mosaic allowed"));
     for (Int k=1; k < (nmod); ++k){ 
      ftm_p[k]=new PBMosaicFT(static_cast<PBMosaicFT &>(*ft_));
      iftm_p[k]=new PBMosaicFT(static_cast<PBMosaicFT &>(*ift_));
    }
  }
  else {
    //ft_=new GridFT(static_cast<GridFT &>(ft));
    ift_=new GridFT(static_cast<GridFT &>(ft));
    ftm_p[0]=CountedPtr<FTMachine>(ft_, False);
    iftm_p[0]=ift_;
    for (Int k=1; k < (nmod); ++k){ 
      ftm_p[k]=new GridFT(static_cast<GridFT &>(*ft_));
      iftm_p[k]=new GridFT(static_cast<GridFT &>(*ift_));
    }
  }

  imGetSlice_p.resize(nmod, True, False);
  imPutSlice_p.resize(nmod, True, False);
  weightSlice_p.resize(nmod, True, False);

}


CubeSkyEquation::~CubeSkyEquation(){
  //As we  make an explicit ift_ in the constructor we need 
  //to take care of it here...
  //if(ift_ && (ift_ != ft_))
  //  delete ift_;

}

void  CubeSkyEquation::predict(Bool incremental) {

  AlwaysAssert(cft_, AipsError);
  AlwaysAssert(sm_, AipsError);
  //AlwaysAssert(vs_, AipsError);
  if(sm_->numberOfModels()!= 0)  AlwaysAssert(ok(),AipsError);
  if(noModelCol_p)
        throw(AipsError("Cannot predict visibilities without using scratch columns yet"));
  // Initialize 
  VisIter& vi=*wvi_p;
  //Lets get the channel selection for later use
  vi.getChannelSelection(blockNumChanGroup_p, blockChanStart_p,
			 blockChanWidth_p, blockChanInc_p, blockSpw_p);
  checkVisIterNumRows(vi);
  VisBuffer vb(vi);
  Bool changedVI=False;
  // Reset the visibilities only if this is not an incremental
  // change to the model
  Bool initialized=False;
  predictComponents(incremental, initialized);
  //set to zero then loop over model...check for size...subimage then loop over  subimages


  Bool isEmpty=True;
  for (Int model=0; model < (sm_->numberOfModels());++model){
    isEmpty=isEmpty &&  (sm_->isEmpty(model));                

  }

 
  if( (sm_->numberOfModels() >0) && isEmpty  && !initialized && !incremental){ 
      // We are at the begining with an empty model as starting point
      for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
	for (vi.origin(); vi.more(); vi++) {
	  vb.setModelVisCube(Complex(0.0,0.0));
	  vi.setVis(vb.modelVisCube(),VisibilityIterator::Model);
	}
      }
  }

  //If all model is zero...no need to continue
  if(isEmpty) 
    return;

  

  // Now do the images
  for (Int model=0; model < (sm_->numberOfModels());++model){ 
      // Change the model polarization frame
      if(vb.polFrame()==MSIter::Linear) {
	StokesImageUtil::changeCStokesRep(sm_->cImage(model),
					  SkyModel::LINEAR);
      }
      else {
	StokesImageUtil::changeCStokesRep(sm_->cImage(model),
					  SkyModel::CIRCULAR);
      }
      ft_=&(*ftm_p[model]);
      scaleImage(model, incremental);
  }
  ft_=&(*ftm_p[0]);
      // Reset the various SkyJones
  resetSkyJones();
  Int nCubeSlice=1;
  isLargeCube(sm_->cImage(0), nCubeSlice);
  for (Int cubeSlice=0; cubeSlice< nCubeSlice; ++cubeSlice){
    changedVI= getFreqRange(vi, sm_->cImage(0).coordinates(),
			    cubeSlice, nCubeSlice) || changedVI;
    vi.originChunks();
    vi.origin();
    vb.invalidate();    
    initializeGetSlice(vb, 0,incremental, cubeSlice, nCubeSlice);
    for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
      for (vi.origin(); vi.more(); vi++) {
	if(!incremental&&!initialized) {
	  vb.setModelVisCube(Complex(0.0,0.0));
	}
	// get the model visibility and write it to the model MS
	getSlice(vb,incremental, cubeSlice, nCubeSlice);
	vi.setVis(vb.modelVisCube(),VisibilityIterator::Model);
      }
    }
    finalizeGetSlice();
    if(!incremental&&!initialized) initialized=True;
  }

  for(Int model=0; model < sm_->numberOfModels(); ++model){
    //For now unscale test on name of ft_
    ft_=&(*ftm_p[model]);
    unScaleImage(model, incremental);
  }
  ft_=&(*ftm_p[0]);

  //lets return original selection back to iterator
  if(changedVI)
    vi.selectChannel(blockNumChanGroup_p, blockChanStart_p, 
		     blockChanWidth_p, blockChanInc_p, blockSpw_p); 

}

void CubeSkyEquation::makeApproxPSF(PtrBlock<TempImage<Float> * >& psfs) 
{

  if(iftm_p[0]->name()=="MosaicFT")
    makeMosaicPSF(psfs);
  else
    makeSimplePSF(psfs);

}
void CubeSkyEquation::makeMosaicPSF(PtrBlock<TempImage<Float> * >& psfs){
  //lets try to make the psf directly
  LogIO os(LogOrigin("SkyEquation", "makeMosaicPSF"));
  makeSimplePSF(psfs);
  Int xpos;
  Int ypos;
  Matrix<Float> psfplane;
  Float peak;
  StokesImageUtil::locatePeakPSF(*(psfs[0]), xpos, ypos, peak, psfplane);
  Int nx=psfplane.shape()(0);
  Int ny=psfplane.shape()(1);
  Bool centered=True;
  // lets ignore  misers who made 10x10 pixel images
  centered=(abs(xpos-nx/2) <=5) && (abs(ypos-ny/2) <=5);

  /////////////////////
  //  cout << "nx " << nx << " ny " << ny << " xpos " << xpos << " ypos " << ypos << " peak " << peak << endl;
  //PagedImage<Float> thisScreen(psfs[0]->shape(), psfs[0]->coordinates(), "PSF__.psf");
  //thisScreen.copyData(*(psfs[0]));



  ///////////////////////////////
  if(centered){
    //for cubes some of the planes may not have a central peak
    Int nchana= (psfs[0])->shape()(3);
    if(nchana > 1){
      IPosition blc(4,nx, ny, 0, nchana);
      IPosition trc(4, nx, ny, 0, nchana);
      blc(0)=0; blc(1)=0; trc(0)=nx-1; trc(1)=ny-1;
      Array<Float> goodplane(psfplane.reform(IPosition(4, nx,ny,1,1)));
      for (Int k=0; k < nchana ; ++k){
	blc(3)=k; trc(3)=k;
	Slicer sl(blc, trc, Slicer::endIsLast);
	SubImage<Float> psfSub(*(psfs[0]), sl, True);
	Float planeMax;
	LatticeExprNode LEN = max( psfSub );
	planeMax =  LEN.getFloat();
	if( (planeMax >0.0) && (planeMax < 0.8 *peak)){
	  psfSub.put(goodplane);
	  
	}
      }
    }
    return;
  }
  //lets back up the ftmachines
  MosaicFT *ft_back= new MosaicFT(static_cast<MosaicFT &>(*ftm_p[0]));
  MosaicFT *ift_back = new MosaicFT(static_cast<MosaicFT &>(*iftm_p[0]));
  os << LogIO::WARN << "Mosaic psf is off. \nCould be no pointing in center of image \n"
     << "Will retry to make an approximate one without primary beam "
     << LogIO::POST;
  MPosition loc=iftm_p[0]->getLocation();
  ftm_p[0]=new GridFT(1000000, 16, "SF", loc, 1.0, False);
  iftm_p[0]=new GridFT(1000000, 16, "SF", loc, 1.0, False);
  ft_=&(*ftm_p[0]);
  ift_=&(*iftm_p[0]);
  // try again with simple ftmachines
  makeSimplePSF(psfs);
  //that 's the best psf you'd get
  //restore back MosaicFT machinas
  ftm_p[0]=ft_back;
  ft_=ft_back;
  iftm_p[0]=ift_back;
  ift_=ift_back;
}

void CubeSkyEquation::makeSimplePSF(PtrBlock<TempImage<Float> * >& psfs) {
  

  Int nmodels=psfs.nelements();
  LogIO os(LogOrigin("CubeSkyEquation", "makeSimplePSF"));
  ft_->setNoPadding(noModelCol_p);
  isPSFWork_p= True; // avoid PB correction etc for PSF estimation
  Bool doPSF=True;
  Bool changedVI=False; 
 // Initialize the gradients
  sm_->initializeGradients();
  ROVisIter& vi(*rvi_p);
  //Lets get the channel selection for later use
  vi.getChannelSelection(blockNumChanGroup_p, blockChanStart_p,
			 blockChanWidth_p, blockChanInc_p, blockSpw_p);
  // Reset the various SkyJones
  resetSkyJones();
  checkVisIterNumRows(vi);
  // Loop over all visibilities and pixels
  VisBuffer vb(vi);
  vi.originChunks();
  vi.origin();
  // Change the model polarization frame
  for (Int model=0; model < nmodels; ++model){
    if(vb.polFrame()==MSIter::Linear) {
      StokesImageUtil::changeCStokesRep(sm_->cImage(model),
					SkyModel::LINEAR);
    }
    else {
      StokesImageUtil::changeCStokesRep(sm_->cImage(model),
					SkyModel::CIRCULAR);
    }
  }

  
  Int nCubeSlice=1;
  isLargeCube(sm_->cImage(0), nCubeSlice);
  for (Int cubeSlice=0; cubeSlice< nCubeSlice; ++cubeSlice){
    changedVI= getFreqRange(vi, sm_->cImage(0).coordinates(),
				  cubeSlice, nCubeSlice) || changedVI;
    vi.originChunks();
    vi.origin();
    vb.invalidate();
    Int cohDone=0;
    ProgressMeter pm(1.0, Double(vi.numberCoh()),
		     "Gridding weights for PSF",
		     "", "", "", True);
      
    initializePutSlice(vb, cubeSlice, nCubeSlice);
    
    for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
      for (vi.origin(); vi.more(); vi++) {
	if(noModelCol_p) {
          //This here forces the modelVisCube shape and prevents reading model column
	  vb.setModelVisCube(Complex(0.0,0.0));
	}
	putSlice(vb, doPSF, FTMachine::MODEL, cubeSlice, nCubeSlice);
	cohDone+=vb.nRow();
	pm.update(Double(cohDone));
	
      }
    }
    finalizePutSlice(vb, cubeSlice, nCubeSlice);
  }

  //lets return original selection back to iterator

  
  if(changedVI)
    vi.selectChannel(blockNumChanGroup_p, blockChanStart_p, 
		     blockChanWidth_p, blockChanInc_p, blockSpw_p); 
  sm_->finalizeGradients();
  fixImageScale();
  for(Int model=0; model < nmodels; ++model){
    {
      //Normalize the gS image
      Int nXX=sm_->ggS(model).shape()(0);
      Int nYY=sm_->ggS(model).shape()(1);
      Int npola= sm_->ggS(model).shape()(2);
      Int nchana= sm_->ggS(model).shape()(3);
      IPosition blc(4,nXX, nYY, npola, nchana);
      IPosition trc(4, nXX, nYY, npola, nchana);
      blc(0)=0; blc(1)=0; trc(0)=nXX-1; trc(1)=nYY-1; 
      //max weights per plane
      for (Int j=0; j < npola; ++j){
	for (Int k=0; k < nchana ; ++k){
	  
	  blc(2)=j; trc(2)=j;
	  blc(3)=k; trc(3)=k;
	  Slicer sl(blc, trc, Slicer::endIsLast);
	  SubImage<Float> gSSub(sm_->gS(model), sl, False);
	  SubImage<Float> ggSSub(sm_->ggS(model), sl, False);
	  SubImage<Float> psfSub(*(psfs[model]), sl, True);
	  Float planeMax;
	  LatticeExprNode LEN = max( ggSSub );
	  planeMax =  LEN.getFloat();
	  if(planeMax !=0){
	    psfSub.copyData( (LatticeExpr<Float>) 
			     (iif(ggSSub > (0.0), 
				  (gSSub/planeMax),0.0)));
	    
	  }
	}
      }
      
      //
    }
    LatticeExprNode maxPSF=max(*psfs[model]);
    Float maxpsf=maxPSF.getFloat();
    if(abs(maxpsf-1.0) > 1e-3) {
      os << "Maximum of approximate PSF for field " << model << " = "
	 << maxpsf << " : renormalizing to unity" <<  LogIO::POST;
    }
    if(maxpsf > 0.0 ){
      LatticeExpr<Float> len((*psfs[model])/maxpsf);
      psfs[model]->copyData(len);
    
    }
    else{
      if(sm_->numberOfTaylorTerms()>1) { /* MFS */
        os  << "PSF calculation resulted in a PSF with a peak  being 0 or lesser." << LogIO::POST;
      }
      else{
	throw(AipsError("SkyEquation:: PSF calculation resulted in a PSF with a peak  being 0 or lesser !"));
      }

    }
  }


  isPSFWork_p=False; // resseting this flag so that subsequent calculation uses
  // the right SkyJones correction;
  

}
void CubeSkyEquation::gradientsChiSquared(Bool incr, Bool commitModel){
  AlwaysAssert(cft_, AipsError);
  AlwaysAssert(sm_, AipsError);
  //AlwaysAssert(vs_, AipsError);
  Bool initialized=False;
  Bool changedVI=False;
  
  //For now we don't deal with incremental especially when having multi fields
  Bool incremental=False;
  
  predictComponents(incremental, initialized);
  Bool predictedComp=initialized;
  
  


  sm_->initializeGradients();
  // Initialize 
  //ROVisIter& vi=*rvi_p;
  //Lets get the channel selection for later use
  rvi_p->getChannelSelection(blockNumChanGroup_p, blockChanStart_p,
			 blockChanWidth_p, blockChanInc_p, blockSpw_p);
  checkVisIterNumRows(*rvi_p);
  VisBuffer vb(*rvi_p);
  /**** Do we need to do this
  if( (sm_->isEmpty(0))  && !initialized && !incremental){ 
    // We are at the begining with an empty model as starting point
    for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
      for (vi.origin(); vi.more(); vi++) {
	vb.setModelVisCube(Complex(0.0,0.0));
	vi.setVis(vb.modelVisCube(),VisibilityIterator::Model);
      }
    }
  }
  */
  Bool isEmpty=True;
  for (Int model=0; model < (sm_->numberOfModels());++model){
    isEmpty=isEmpty &&  sm_->isEmpty(model);                
    
  }

  // Now do the images
  for (Int model=0;model< (sm_->numberOfModels()); ++model) {  
    // Don't bother with empty images
    // Change the model polarization frame
    if(vb.polFrame()==MSIter::Linear) {
      StokesImageUtil::changeCStokesRep(sm_->cImage(model),
					SkyModel::LINEAR);
    }
    else {
      StokesImageUtil::changeCStokesRep(sm_->cImage(model),
					SkyModel::CIRCULAR);
    }
    //scaleImage(model, incremental);
    ft_=&(*ftm_p[model]);
    scaleImage(model);
    // Reset the various SkyJones
  }
  ft_=&(*ftm_p[0]);
  resetSkyJones();
  firstOneChangesPut_p=False;
  firstOneChangesGet_p=False;
    
  Int nCubeSlice=1;
   
  isLargeCube(sm_->cImage(0), nCubeSlice);
    
  
  for (Int cubeSlice=0; cubeSlice< nCubeSlice; ++cubeSlice){

    //      vi.originChunks();
    //      vi.origin();      
    //sliceCube(imGetSlice_p, model, cubeSlice, nCubeSlice, 1);
    //Redo the channel selection in case of chunked cube to match
    //data needed for gridding.
    changedVI= getFreqRange(*rvi_p, sm_->cImage(0).coordinates(),
			    cubeSlice, nCubeSlice) || changedVI;
      
    rvi_p->originChunks();
    rvi_p->origin();
    Bool useCorrected= !(rvi_p->msColumns().correctedData().isNull());

    vb.invalidate();
    if(!isEmpty){
      initializeGetSlice(vb, 0, False, cubeSlice, nCubeSlice);
    }
    initializePutSlice(vb, cubeSlice, nCubeSlice);
    Int cohDone=0;
    ProgressMeter pm(1.0, Double(rvi_p->numberCoh()),
		     "Gridding residual",
		     "", "", "", True);
    for (rvi_p->originChunks();rvi_p->moreChunks();rvi_p->nextChunk()) {
      for (rvi_p->origin(); rvi_p->more(); (*rvi_p)++) {
	if(!incremental && !predictedComp) {
          //This here forces the modelVisCube shape and prevents reading model column
	  vb.setModelVisCube(Complex(0.0,0.0));
	}
	// get the model visibility and write it to the model MS
	if(!isEmpty)
	  getSlice(vb, (predictedComp || incremental), cubeSlice, nCubeSlice);
	//saving the model for self-cal most probably
        if(commitModel && !noModelCol_p)
	  wvi_p->setVis(vb.modelVisCube(),VisibilityIterator::Model);
	// Now lets grid the -ve of residual
        // use visCube if there is no correctedData
        if(!useCorrected){
            vb.modelVisCube()-=vb.visCube();
        }
        else{
            vb.modelVisCube()-=vb.correctedVisCube();
        }
	putSlice(vb, False, FTMachine::MODEL, cubeSlice, nCubeSlice);
	cohDone+=vb.nRow();
	pm.update(Double(cohDone));
	
      }
    }

    finalizeGetSlice();
    if(!incremental&&!initialized) initialized=True;
    finalizePutSlice(vb, cubeSlice, nCubeSlice);


  }
  
  for (Int model=0;model<sm_->numberOfModels();model++) {
    //unScaleImage(model, incremental);
    ft_=&(*ftm_p[model]);
    unScaleImage(model);
  
  }
  ft_=&(*ftm_p[0]);

  this->fixImageScale();
  //lets return original selection back to iterator
  if(changedVI)
    rvi_p->selectChannel(blockNumChanGroup_p, blockChanStart_p, 
		     blockChanWidth_p, blockChanInc_p, blockSpw_p); 

  

}


void  CubeSkyEquation::isLargeCube(ImageInterface<Complex>& theIm, 
				   Int& nslice) {

  //non-cube
  if(theIm.shape()[3]==1){
    nslice=1;
  }
  else{
    Long npix=theIm.shape().product();
    Long memtot=HostInfo::memoryTotal();
    //check for 32 bit OS and limit it to 2Gbyte
    if( sizeof(void*) == 4){
      if(memtot > 2000000)
	memtot=2000000;
    }

    Long pixInMem=(memtot/8)*1024;
    nslice=1;

    if(npix > (pixInMem/8)){
      //Lets slice it so grid is at most 1/6th of memory
      pixInMem=pixInMem/8;
      //One plane is
      npix=theIm.shape()(0)*theIm.shape()(1)*theIm.shape()(2);
      nchanPerSlice_p=Int(floor(pixInMem/npix));
      if (nchanPerSlice_p==0)
	nchanPerSlice_p=1;
      nslice=theIm.shape()(3)/nchanPerSlice_p;
      if( (theIm.shape()(3) % nchanPerSlice_p) > 0)
	++nslice;
    }
  }
}

void CubeSkyEquation::initializePutSlice(const VisBuffer& vb, 
					 Int cubeSlice, Int nCubeSlice) {
  AlwaysAssert(ok(),AipsError);
  for(Int model=0; model < (sm_->numberOfModels()) ; ++model){
    sliceCube(imPutSlice_p[model], model, cubeSlice, nCubeSlice, 0);
    weightSlice_p[model].resize();
    if(nCubeSlice>1){
      iftm_p[model]->reset();
    }
    iftm_p[model]->initializeToSky(*(imPutSlice_p[model]),weightSlice_p[model],
				   vb);
  }
  assertSkyJones(vb, -1);
  //vb_p is used to finalize things if vb has changed propoerties
  vb_p.assign(vb, False);
  vb_p.updateCoordInfo();
}


void CubeSkyEquation::getCoverageImage(Int model, ImageInterface<Float>& im){
  if ((sm_->doFluxScale(model)) && (ftm_p.nelements() > uInt(model))){
    ftm_p[model]->getFluxImage(im);
  }

}


void CubeSkyEquation::putSlice(const VisBuffer & vb, Bool dopsf, FTMachine::Type col, Int cubeSlice, Int nCubeSlice) {

  AlwaysAssert(ok(),AipsError);
  Int nRow=vb.nRow();
  internalChangesPut_p=False;  // Does this VB change inside itself?
  firstOneChangesPut_p=False;  // Has this VB changed from the previous one?
  if((ftm_p[0]->name() != "MosaicFT") && 
     (ftm_p[0]->name() != "PBWProjectFT")) {

    changedSkyJonesLogic(vb, firstOneChangesPut_p, internalChangesPut_p);
  }
  //First ft machine change should be indicative
  //anyways right now we are allowing only 1 ftmachine for GridBoth
  Bool IFTChanged=iftm_p[0]->changed(vb);
  



  // we might need to recompute the "sky" for every single row, but we
  // avoid this if possible.

    
  if(internalChangesPut_p || internalChangesGet_p) {
    if(internalChangesGet_p)
      internalChangesGet_p=False;
    // Yes there are changes: go row by row. 
    for (Int row=0; row<nRow; row++) {
      if(IFTChanged||changedSkyJones(vb,row)) {
	// Need to apply the SkyJones from the previous row
	// and finish off before starting with this row
	finalizePutSlice(vb_p, cubeSlice, nCubeSlice);  
	initializePutSlice(vb, cubeSlice, nCubeSlice);
      }
      for (Int model=0; model<sm_->numberOfModels(); ++model){
	//if(sm_->getAlgorithm()=="MSMFS"){ /* MFS */
        if(sm_->numberOfTaylorTerms()>1) { /* MFS */
	  calcVisWeights(vb,model);
	  iftm_p[model]->put(vb, row, dopsf, col, visweights_p);
	}
	else{
	  iftm_p[model]->put(vb, row, dopsf, col);
	}
      }
    }
  }
  else if (IFTChanged || firstOneChangesPut_p || firstOneChangesGet_p) {

    if(firstOneChangesGet_p)
      firstOneChangesGet_p=False;
     

    if(!isBeginingOfSkyJonesCache_p){
      finalizePutSlice(vb_p,  cubeSlice, nCubeSlice);      
    }
    initializePutSlice(vb, cubeSlice, nCubeSlice);
    isBeginingOfSkyJonesCache_p=False;
    for (Int model=0; model<sm_->numberOfModels(); ++model){
	//if(sm_->getAlgorithm()=="MSMFS"){ /* MFS */
        if(sm_->numberOfTaylorTerms()>1) { /* MFS */
	  calcVisWeights(vb,model);
	  iftm_p[model]->put(vb, -1, dopsf, col, visweights_p);
	}
	else{
	  iftm_p[model]->put(vb, -1, dopsf, col);
	}
    }
  }
  else {
    for (Int model=0; model<sm_->numberOfModels(); ++model){
	//if(sm_->getAlgorithm()=="MSMFS"){ /* MFS */
        if(sm_->numberOfTaylorTerms()>1) { /* MFS */
	  calcVisWeights(vb,model);
	  iftm_p[model]->put(vb, -1, dopsf, col, visweights_p);
	}
	else{
	  iftm_p[model]->put(vb, -1, dopsf, col);
	}
    }
  }
  
  isBeginingOfSkyJonesCache_p=False;
  
}



void CubeSkyEquation::finalizePutSlice(const VisBuffer& vb,  
				       Int cubeSlice, Int nCubeSlice) {

  for (Int model=0; model < sm_->numberOfModels(); ++model){
    //the different apply...jones use ft_ and ift_
    ft_=&(*ftm_p[model]);
    ift_=&(*iftm_p[model]);
    // Actually do the transform. Update weights as we do so.
    iftm_p[model]->finalizeToSky();
    // 1. Now get the (unnormalized) image and add the 
    // weight to the summed weight
    Matrix<Float> delta;
    imPutSlice_p[model]->copyData(iftm_p[model]->getImage(delta, False));
  
    weightSlice_p[model]+=delta;

    // 2. Apply the SkyJones and add to grad chisquared
    SubImage<Float> *workSlice;
    SubImage<Float> *gSSlice;
    sliceCube(workSlice, sm_->work(model), cubeSlice, nCubeSlice);
    sliceCube(gSSlice, sm_->gS(model), cubeSlice, nCubeSlice);


    applySkyJonesInv(vb, -1, *(imPutSlice_p[model]), *workSlice,
		     *gSSlice);
    SubImage<Float> *ggSSlice;
    sliceCube(ggSSlice, sm_->ggS(model), cubeSlice, nCubeSlice);
  
    // 3. Apply the square of the SkyJones and add this to gradgrad chisquared
    applySkyJonesSquare(vb, -1, weightSlice_p[model], *workSlice,
			*ggSSlice);
  

    delete workSlice;
    delete gSSlice;
    delete ggSSlice;
  }
  ft_=&(*ftm_p[0]);
  ift_=&(*iftm_p[0]);
  // 4. Finally, we add the statistics
  sm_->addStatistics(sumwt, chisq);
}


void CubeSkyEquation::initializeGetSlice(const VisBuffer& vb, 
					   Int row, 
					   Bool incremental, Int cubeSlice, 
					   Int nCubeSlice){
  imGetSlice_p.resize(sm_->numberOfModels(), True, False);
  for(Int model=0; model < sm_->numberOfModels(); ++model){
     //the different apply...jones use ft_ and ift_
    ft_=&(*ftm_p[model]);
    ift_=&(*iftm_p[model]);
    if(cubeSlice==0){
      if(incremental) {
	applySkyJones(vb, row, sm_->deltaImage(model), sm_->cImage(model));
      }
      else {
	applySkyJones(vb, row, sm_->image(model), sm_->cImage(model));
      }
    }
    sliceCube(imGetSlice_p[model], model, cubeSlice, nCubeSlice, 1);
    ftm_p[model]->initializeToVis(*(imGetSlice_p[model]), vb);
  }
  ft_=&(*ftm_p[0]);
  ift_=&(*iftm_p[0]);
  

}

void CubeSkyEquation::sliceCube(CountedPtr<ImageInterface<Complex> >& slice,Int model, Int cubeSlice, 
				Int nCubeSlice, Int typeOfSlice){

  IPosition blc(4,0,0,0,0);
  IPosition trc(4,sm_->cImage(model).shape()(0)-1,
		 sm_->cImage(model).shape()(1)-1,sm_->cImage(model).shape()(2)-1,
		 0);
  Int beginChannel=cubeSlice*nchanPerSlice_p;
  Int endChannel=beginChannel+nchanPerSlice_p-1;
  if(cubeSlice==(nCubeSlice-1))
    endChannel=sm_->image(model).shape()(3)-1;
  blc(3)=beginChannel;
  trc(3)=endChannel;
  sl_p=Slicer (blc, trc, Slicer::endIsLast);
  SubImage<Complex>* sliceIm= new SubImage<Complex>(sm_->cImage(model), sl_p, False);
  if(typeOfSlice==0){    
    
    Double memoryMB=HostInfo::memoryTotal()/1024.0/(8.0*(sm_->numberOfModels()));
    slice=new TempImage<Complex> (sliceIm->shape(), sliceIm->coordinates(), memoryMB);
    //slice.copyData(sliceIm);
    slice->set(Complex(0.0, 0.0));
    //slice->setCoordinateInfo(sm_->image(model).coordinates());
    delete sliceIm;
  }
  else{
    slice=sliceIm;
  }

}


void CubeSkyEquation::sliceCube(SubImage<Float>*& slice,
				  ImageInterface<Float>& image, Int cubeSlice, 
				  Int nCubeSlice){

  IPosition blc(4,0,0,0,0);
  IPosition trc(4,image.shape()(0)-1,
		image.shape()(1)-1,image.shape()(2)-1,
		 0);
  Int beginChannel=cubeSlice*nchanPerSlice_p;
  Int endChannel=beginChannel+nchanPerSlice_p-1;
  if(cubeSlice==(nCubeSlice-1))
    endChannel=image.shape()(3)-1;
  blc(3)=beginChannel;
  trc(3)=endChannel;
  sl_p=Slicer(blc, trc, Slicer::endIsLast);
  //writeable if possible
  slice=  new SubImage<Float> (image, sl_p, True);
}


VisBuffer& CubeSkyEquation::getSlice(VisBuffer& result,  
				     Bool incremental,
				     Int cubeSlice, Int nCubeSlice) {

  Int nRow=result.nRow();

  result.modelVisCube(); // get the visibility so vb will have it
  VisBuffer vb(result);

  Int nmodels=sm_->numberOfModels();
  Bool FTChanged=ftm_p[0]->changed(vb);

  // we might need to recompute the "sky" for every single row, but we
  // avoid this if possible.
  internalChangesGet_p=False;  // Does this VB change inside itself?
  firstOneChangesGet_p=False;  // Has this VB changed from the previous one?
  if((ftm_p[0]->name()!="MosaicFT") &&
     (ftm_p[0]->name() != "PBWProjectFT")) {
    changedSkyJonesLogic(result, firstOneChangesGet_p, internalChangesGet_p);
  } 
  if(internalChangesGet_p || internalChangesPut_p) {
    if(internalChangesPut_p)
      internalChangesPut_p=False;
    // Yes there are changes within this buffer: go row by row.
    // This will automatically catch a change in the FTMachine so
    // we don't have to check for that.

    Matrix<Complex> refres;
    Matrix<Complex> refvb;
    for (Int row=0; row<nRow; row++) {
      finalizeGetSlice();
      initializeGetSlice(result, row, False, cubeSlice, 
			 nCubeSlice);
      if(incremental || (nmodels > 1)){
	for (Int model=0; model < nmodels; ++model){
	  ftm_p[model]->get(vb,row);
	  //if(sm_->getAlgorithm()=="MSMFS"){/* MFS */
          if(sm_->numberOfTaylorTerms()>1) { /* MFS */
	    modifySpecModelVis(vb,model);
	  }
	  refvb.reference(vb.modelVisCube().xyPlane(row));
	  refres.reference(result.modelVisCube().xyPlane(row));
	  refres += refvb;
	}
      }
      else
	ftm_p[0]->get(result, row);
    }
  }
  else if (FTChanged || firstOneChangesGet_p || firstOneChangesPut_p) {
    if(firstOneChangesPut_p)
      firstOneChangesPut_p=False;
    // This buffer has changed wrt the previous buffer, but
    // this buffer has no changes within it. Again we don't need to
    // check for the FTMachine changing.

    finalizeGetSlice();
    initializeGetSlice(result, 0, False, cubeSlice, nCubeSlice);
    if(incremental || (nmodels > 1)){
      for (Int model=0; model < nmodels; ++model){
	ftm_p[model]->get(vb);
	//if(sm_->getAlgorithm()=="MSMFS"){/* MFS */
        if(sm_->numberOfTaylorTerms()>1) { /* MFS */
	  modifySpecModelVis(vb,model);
	}
	result.modelVisCube()+=vb.modelVisCube();
      }
    }
    else
      ftm_p[0]->get(result);
  }
  else {
    if(incremental || (nmodels >1)){
      for (Int model=0; model < nmodels; ++model){
	ftm_p[model]->get(vb);
	//if(sm_->getAlgorithm()=="MSMFS")/* MFS */
        if(sm_->numberOfTaylorTerms()>1) { /* MFS */
	  modifySpecModelVis(vb,model);
	}
	result.modelVisCube()+=vb.modelVisCube();
      }
    }
    else
      ftm_p[0]->get(result);
  }
  return result;

}

  void CubeSkyEquation::finalizeGetSlice(){


  }

  Bool CubeSkyEquation::getFreqRange(ROVisibilityIterator& vi, 
				     const CoordinateSystem& coords, 
				     Int slice, Int nslice){

    //bypass this for now
    //    return False;

    // Only one slice lets keep what the user selected
    if(nslice==1)
      return False;

    Double start=0.0; 
    Double end=0.0;
    Double chanwidth=1.0;
    Int specIndex=coords.findCoordinate(Coordinate::SPECTRAL);
    SpectralCoordinate specCoord=coords.spectralCoordinate(specIndex);
    Vector<Int>spectralPixelAxis=coords.pixelAxes(specIndex);
    if(nchanPerSlice_p>0){
      specCoord.toWorld(start,Double(slice*nchanPerSlice_p)-0.5);
      specCoord.toWorld(end, Double(nchanPerSlice_p*(slice+1))-0.5);
      chanwidth=fabs(end-start)/Double(nchanPerSlice_p);
    }

    Block<Vector<Int> > spwb;
    Block<Vector<Int> > startb;
    Block<Vector<Int> > nchanb;
    Block<Vector<Int> > incrb=blockChanInc_p;
    vi.getSpwInFreqRange(spwb, startb, nchanb, start, end, chanwidth);
    if(spwb.nelements()==0)
      return False;

    

    //vi.selectChannel(1, startb[0][0], nchanb[0][0], 1, spwb[0][0]); 
    vi.selectChannel(blockNumChanGroup_p, startb, nchanb, incrb, spwb); 

    return True;
    
  }

  /* MFSCode */
  Bool CubeSkyEquation::calcVisWeights(const VisBuffer& vb, Int modelindex)
  { 
    Int taylor = sm_->getTaylorIndex(modelindex);

    // make sure shape of visweights_p is correct - same as vb.imagingWeight()
    if( taylor == 0 ) 
    {       
       if(visweights_p.shape() != vb.imagingWeight().shape())
	       visweights_p.resize(vb.imagingWeight().shape());
       visweights_p = vb.imagingWeight();
    }
    else
    {
       Float freq=0.0,mulfactor=1.0;
       Vector<Double> selfreqlist(vb.frequency());
       Double reffreq = sm_->getReferenceFrequency();
    
       for (Int row=0; row<vb.nRow(); row++) 
       for (Int chn=0; chn<vb.nChannel(); chn++) 
       {
	    freq = selfreqlist(IPosition(1,chn));
	    mulfactor = ((freq-reffreq)/reffreq);
	    
	    visweights_p(chn,row) = (vb.imagingWeight())(chn,row) * pow(mulfactor,taylor);
	    
       }
    }

    return True;
  }
	  
  /* MFSCode */
  Bool CubeSkyEquation::modifySpecModelVis(VisBuffer& vb, Int modelindex)
  { 
    Int taylor = sm_->getTaylorIndex(modelindex);
    
    if( taylor == 0 ){ return True; }
    else
    {
       Float freq=0.0,mulfactor=1.0;
       Vector<Double> selfreqlist(vb.frequency());
       Double reffreq = sm_->getReferenceFrequency();
//       Cube<Complex> modelvis = vb.modelVisCube();

       for (uInt pol=0; pol< uInt((vb.modelVisCube()).shape()[0]); pol++) 
	 for (uInt chn=0; chn< uInt(vb.nChannel()); chn++) 
	   for (uInt row=0; row< uInt(vb.nRow()); row++) 
       {
	    freq = selfreqlist(IPosition(1,chn));
	    mulfactor = ((freq-reffreq)/reffreq);
	    
//	    modelvis(pol,chn,row) *= pow(mulfactor,taylor);
	    (vb.modelVisCube())(pol,chn,row) *= pow(mulfactor,taylor);
       }
      // vb.modelVisCube() = modelvis;
    }

    return True;
  }


void CubeSkyEquation::fixImageScale()
{
  LogIO os(LogOrigin("CubeSkyEquation", "fixImageScale"));

  // make a minimum value to ggS
  // This has the same effect as Sault Weighting, but 
  // is implemented somewhat differently.
  // We also keep the fluxScale(mod) images around to
  // undo the weighting.
  Float ggSMax=0.0;
  for (Int model=0;model<sm_->numberOfModels();model++) {
    
    LatticeExprNode LEN = max( sm_->ggS(model) );
    ggSMax =  max(ggSMax,LEN.getFloat());
  }
  ggSMax_p=ggSMax;
  Float ggSMin1;
  Float ggSMin2;
  
  ggSMin1 = ggSMax * constPB_p * constPB_p;
  ggSMin2 = ggSMax * minPB_p * minPB_p;
    
  for (Int model=0;model<sm_->numberOfModels();model++) {
    if(ej_ || (ftm_p[model]->name() == "MosaicFT") ) {
      
      

    /*Don't print this for now
      if (scaleType_p == "SAULT") {
      os << "Using SAULT image plane weighting" << LogIO::POST;
    }
    else {
    os << "Using No image plane weighting" << LogIO::POST;
    }
    */
    sm_->fluxScale(model).removeRegion ("mask0", RegionHandler::Any, False);
    if ((ftm_p[model]->name()!="MosaicFT")) {
      if(scaleType_p=="SAULT"){
	
	  // Adjust flux scale to account for ggS being truncated at ggSMin1
	  // Below ggSMin2, set flux scale to 0.0
	  // FluxScale * image => true brightness distribution, but
	  // noise increases at edge.
	  // if ggS < ggSMin2, set to Zero;
	  // if ggS > ggSMin2 && < ggSMin1, set to ggSMin1/ggS
	  // if ggS > ggSMin1, set to 1.0
	  sm_->fluxScale(model).copyData( (LatticeExpr<Float>) 
					  (iif(sm_->ggS(model) < (ggSMin2), 0.0,
					       sqrt((sm_->ggS(model))/ggSMin1) )) );
	  sm_->fluxScale(model).copyData( (LatticeExpr<Float>) 
					  (iif(sm_->ggS(model) > (ggSMin1), 1.0,
					       (sm_->fluxScale(model)) )) );
	  // truncate ggS at ggSMin1
	  sm_->ggS(model).copyData( (LatticeExpr<Float>) 
				    (iif(sm_->ggS(model) < (ggSMin1), ggSMin1*(sm_->fluxScale(model)), 
					 sm_->ggS(model)) )
				    );
	}

	else{

	  sm_->fluxScale(model).copyData( (LatticeExpr<Float>) 
					  (iif(sm_->ggS(model) < (ggSMin2), 0.0,
					       sqrt((sm_->ggS(model))/ggSMax) )) );
	  sm_->ggS(model).copyData( (LatticeExpr<Float>) 
					  (iif(sm_->ggS(model) < (ggSMin2), 0.0,
					       sqrt((sm_->ggS(model))*ggSMax) )) );

	}

      } else {
	/*
	if(ft_->name() != "MosaicFT"){
	  sm_->fluxScale(model).copyData( (LatticeExpr<Float>) 1.0 );
	  sm_->ggS(model).copyData( (LatticeExpr<Float>) 
	  			    (iif(sm_->ggS(model) < (ggSMin2), 0.0, 
	  				 sm_->ggS(model)) ));
	 

	}
	else{

	*/
	 
	
	  Int nXX=sm_->ggS(model).shape()(0);
	  Int nYY=sm_->ggS(model).shape()(1);
	  Int npola= sm_->ggS(model).shape()(2);
	  Int nchana= sm_->ggS(model).shape()(3);
	  IPosition blc(4,nXX, nYY, npola, nchana);
	  IPosition trc(4, nXX, nYY, npola, nchana);
	  blc(0)=0; blc(1)=0; trc(0)=nXX-1; trc(1)=nYY-1; 


	  //Those damn weights per plane can be wildly different so 
	  //deal with it properly here
	  for (Int j=0; j < npola; ++j){
	    for (Int k=0; k < nchana ; ++k){
	      
	      blc(2)=j; trc(2)=j;
	      blc(3)=k; trc(3)=k;
	      Slicer sl(blc, trc, Slicer::endIsLast);
	      SubImage<Float> fscalesub(sm_->fluxScale(model), sl, True);
	      SubImage<Float> ggSSub(sm_->ggS(model), sl, True);
	      Float planeMax;
	      LatticeExprNode LEN = max( ggSSub );
	      planeMax =  LEN.getFloat();
	      ///As we chop the image later...the weight can vary per channel
	      ///lets be conservative and go to 10% of ggsmin2
	      if(planeMax !=0){
		fscalesub.copyData( (LatticeExpr<Float>) 
				    (iif(ggSSub < (ggSMin2/10.0), 
					 0.0, (ggSSub/planeMax))));
		ggSSub.copyData( (LatticeExpr<Float>) 
				 (iif(ggSSub < (ggSMin2/10.0), 0.0, 
				      (planeMax))));
	
	

	      }
	    }

	  }
	  /*
	    
	  ftm_p[model]->getFluxImage(sm_->fluxScale(model));
	  
	  sm_->fluxScale(model).copyData( (LatticeExpr<Float>) 
					  (iif(sm_->ggS(model) < (ggSMin2), 0.0,
					  (sm_->ggS(model)/ggSMax) )) );

	  */
	  //}	
      }
    
      //because for usual ft machines a applySJoneInv is done on the gS
      //in the finalizeput stage...need to understand if its necessary
      /*need to understand that square business
      if( (ft_->name() != "MosaicFT") && (!isPSFWork_p)){
	sm_->gS(model).copyData( (LatticeExpr<Float>) 
				 (iif(sm_->fluxScale(model) > 0.0, 
				      ((sm_->gS(model))/(sm_->fluxScale(model))), 0.0 )) );


      }
      */
      ///
    }

  }
}


} //# NAMESPACE CASA - END
