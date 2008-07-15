//# MFMSCleanImageSkyModel.cc: Implementation of MFMSCleanImageSkyModel class
//# Copyright (C) 1996,1997,1998,1999,2000,2001,2002,2003
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
//# $Id: MFMSCleanImageSkyModel.cc,v 19.13 2005/04/08 21:03:34 kgolap Exp $

#include <casa/Arrays/ArrayMath.h>
#include <synthesis/MeasurementComponents/MFMSCleanImageSkyModel.h>
#include <images/Images/PagedImage.h>
#include <casa/OS/File.h>
#include <lattices/Lattices/LatticeExpr.h>
#include <lattices/Lattices/LatticeExprNode.h>
#include <lattices/Lattices/SubLattice.h>
#include <casa/Arrays/IPosition.h>
#include <lattices/Lattices/LCBox.h>
#include <lattices/Lattices/LatticeCleaner.h>
#include <synthesis/MeasurementEquations/SkyEquation.h>
#include <casa/Exceptions/Error.h>
#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>
#include <casa/Quanta/Quantum.h>

#include <casa/sstream.h>
#include <casa/Logging/LogMessage.h>
#include <casa/Logging/LogSink.h>
#include <casa/Logging/LogIO.h>

#include <synthesis/MeasurementEquations/LatConvEquation.h>

namespace casa {


// Some constructors

MFMSCleanImageSkyModel::MFMSCleanImageSkyModel()
:  method_p(NSCALES), nscales_p(4), progress_p(0), 
   stopLargeNegatives_p(2), stopPointMode_p(-1)
{

  donePSF_p=False;
  modified_p=True;

};

MFMSCleanImageSkyModel::MFMSCleanImageSkyModel(const Int nscales, 
					       const Int sln,
					       const Int spm)
: method_p(NSCALES), nscales_p(nscales), progress_p(0),
  stopLargeNegatives_p(sln), stopPointMode_p(spm)
{


  donePSF_p=False;
  modified_p=True;


};

MFMSCleanImageSkyModel::MFMSCleanImageSkyModel(const Vector<Float>& userScaleSizes, 
					       const Int sln,
					       const Int spm)
: method_p(USERVECTOR), userScaleSizes_p(userScaleSizes), progress_p(0),
  stopLargeNegatives_p(sln), stopPointMode_p(spm)
{

  donePSF_p=False;
  modified_p=True;



};

MFMSCleanImageSkyModel::~MFMSCleanImageSkyModel()
{
  if (progress_p) {
    delete progress_p;
  }

  if(componentList_p) delete componentList_p; componentList_p=0;
  for (Int thismodel=0;thismodel<nmodels_p;thismodel++) {
    if(residualImage_p[thismodel]) delete residualImage_p[thismodel]; 
    residualImage_p[thismodel]=0;
    if(cimage_p[thismodel]) delete cimage_p[thismodel]; 
    cimage_p[thismodel]=0;
    for(Int numXFR=0;numXFR<maxNumXFR_p;numXFR++) {
      if(cxfr_p[thismodel*maxNumXFR_p+numXFR])
        delete cxfr_p[thismodel*maxNumXFR_p+numXFR];
      cxfr_p[thismodel*maxNumXFR_p+numXFR]=0;
    }
    if(gS_p[thismodel]) delete gS_p[thismodel]; gS_p[thismodel]=0;
    if(psf_p[thismodel]) delete psf_p[thismodel]; psf_p[thismodel]=0;
    if(ggS_p[thismodel]) delete ggS_p[thismodel]; ggS_p[thismodel]=0;
    if(fluxScale_p[thismodel]) delete fluxScale_p[thismodel]; 
    fluxScale_p[thismodel]=0;
    if(work_p[thismodel]) delete work_p[thismodel]; work_p[thismodel]=0;
    if(deltaimage_p[thismodel]) delete deltaimage_p[thismodel]; 
    deltaimage_p[thismodel]=0;
    if(weight_p[thismodel]) delete weight_p[thismodel]; weight_p[thismodel]=0;
    if(beam_p[thismodel]) delete beam_p[thismodel]; beam_p[thismodel]=0;
  }



};


// Clean solver
Bool MFMSCleanImageSkyModel::solve(SkyEquation& se) {

  LogIO os(LogOrigin("MFMSCleanImageSkyModel","solve"));
  
  //Make the PSFs, one per field
  if(!donePSF_p){
    os << "Making approximate PSFs" << LogIO::POST;
    makeApproxPSFs(se);
  }

  Bool converged=True;
  // Validate PSFs for each field
  Vector<Float> psfmax(numberOfModels()); psfmax=0.0;
  Vector<Float> psfmin(numberOfModels()); psfmin=1.0;
  Vector<Float> resmax(numberOfModels());
  Vector<Float> resmin(numberOfModels());

  Float maxSidelobe=0.0;
  Int model;
  for (model=0;model<numberOfModels();model++) {
    if(isSolveable(model)) {
      IPosition blc(PSF(model).shape().nelements(), 0);
      IPosition trc(PSF(model).shape()-1);
      blc(2) = 0;  trc(2) = 0;
      blc(3) = 0;  trc(3) = 0;

      SubLattice<Float> subPSF;
      Int k =0;
      Int numchan= PSF(model).shape()(3);
      //PSF of first non zero plane
      while(psfmax(model) < 0.1 && k< numchan){
        blc(3)= k;
	trc(3)=k;
	LCBox onePlane(blc, trc, PSF(model).shape());

	subPSF=SubLattice<Float> ( PSF(model), onePlane, True);
	{
	  LatticeExprNode node = max(subPSF);
	  psfmax(model) = node.getFloat();
	}
	++k;
      }
      {
	LatticeExprNode node = min(subPSF);
	psfmin(model) = node.getFloat();
      }
      os << "Model " << model+1 << ": max, min PSF = "
	 << psfmax(model) << ", " << psfmin(model) << endl;
      if(abs(psfmin(model))>maxSidelobe) maxSidelobe=abs(psfmin(model));
    }
  }
  os << LogIO::POST;
	
  Float absmax=threshold();
  Block< Vector<Int> > iterations(numberOfModels());
  Int maxIterations=0;

    
  // Loop over major cycles
  if (displayProgress_p) {
    if(progress_p) delete progress_p;
    progress_p=0;
    progress_p = new LatticeCleanProgress( pgplotter_p );
  }

  LatticeCleaner<Float>* cleaner;
  cleaner=0;

  Int cycle=0;
  Bool stop=False;
  while(absmax>=threshold()&&maxIterations<numberIterations()&&!stop) {

    os << "*** Starting major cycle " << cycle+1 << LogIO::POST;
    cycle++;

    // Make the residual images. We do an incremental update
    // for cycles after the first one. If we have only one
    // model then we use convolutions to speed the processing
    Bool incremental(cycle>1);
    if(modified_p){
      if (!incremental||(itsSubAlgorithm == "full")) {
	
	os << "Using visibility-subtraction for residual calculation"
	   << LogIO::POST;
	makeNewtonRaphsonStep(se, False);
	
      }
      else {
	os << "Using XFR-based shortcut for residual calculation"
	   << LogIO::POST;
	makeNewtonRaphsonStep(se, True);
      }
    
    }
    absmax=maxField(resmax, resmin);

    for (model=0;model<numberOfModels();model++) {
      os << "Model " << model+1 << ": max, min residuals = "
	 << resmax(model) << ", " << resmin(model) << endl;
    }
    os << LogIO::POST;

    // Can we stop?
    if(absmax<threshold()) {
      os << "Reached stopping peak point source residual = "
	 << absmax << LogIO::POST;
      stop=True;
    }
    else {
    
      // Calculate the threshold for this cycle. Add a safety factor
      Float fudge = cycleFactor_p*maxSidelobe;
      if (fudge > 0.8) fudge = 0.8;   // painfully slow!
      Quantity fThreshold(fudge*100, "%");
      Quantity aThreshold(threshold(), "Jy");
      os << "Maximum point source residual = " << absmax << LogIO::POST;
      os << "Cleaning scale flux down to maximum of " << fThreshold.getValue("%")
	 << " % and " << aThreshold.getValue("Jy") << " Jy" << LogIO::POST;
      
      for (model=0;model<numberOfModels();model++) {

	Int nchan=image(model).shape()(3);
	Int npol=image(model).shape()(2);
 
	IPosition blcDirty(image(model).shape().nelements(), 0);
	IPosition trcDirty(image(model).shape()-1);
  
	if(cycle==1) {
	  iterations[model].resize(nchan);
	  iterations[model]=0;
	}
	
	// Initialize delta image
	deltaImage(model).set(0.0);
	  
	// Only process solveable models
	if(isSolveable(model)) {
	  
	  os << "Processing model " << model+1 << LogIO::POST;
	  
	  // If mask exists, use it;
	  // If not, use the fluxScale image to figure out
	  Bool doMask = False;
	  Bool mustDeleteMask = False;
	  ImageInterface<Float> *maskPointer = 0;
	  
	  if (hasMask(model)) {
	    doMask = True;
	    maskPointer = &mask(model);
	  } else if (doFluxScale(model)) {
	    doMask = True;
	    mustDeleteMask = true;
	    
	    maskPointer = new TempImage<Float> ( fluxScale(model).shape(),
						 fluxScale(model).coordinates());
	    maskPointer->copyData( (LatticeExpr<Float>)
				   (iif( (fluxScale(model) > 0.0), 1.0, 0.0)) );
	  }
	  
	  // Now clean each channel and each pol
	  for (Int chan=0; chan<nchan; chan++) {
	    if(psfmax(model)>0.0) {
	      // We could keep a cleaner per channel but for the moment
	      // we simply make a new one for each channel
	      if(nchan>1) {
		os<<"Processing channel "<<chan+1<<" of "<<nchan<<LogIO::POST;
		if(cleaner) delete cleaner; cleaner=0;
	      }
	      
	      blcDirty(3) = chan;
	      trcDirty(3) = chan;
	      blcDirty(2) = 0; trcDirty(2) = 0;
	      LCBox firstPlane(blcDirty, trcDirty, image(model).shape());
	      
	      for (Int pol=0; pol<npol; pol++) {
		blcDirty(2) = pol; trcDirty(2) = pol;
		// The PSF should be the same for each polarization so we
		// can use the existing cleaner (unlike the spectral case)
		if(npol>1) {
		  os<<"Processing polarization "<<pol+1<<" of "<<npol<<LogIO::POST;
		}
		LCBox onePlane(blcDirty, trcDirty, image(model).shape());
		
		SubLattice<Float> subImage( image(model), onePlane, True);
		SubLattice<Float> subResid( residual(model), onePlane);
		SubLattice<Float> subPSF( PSF(model), firstPlane);
		SubLattice<Float> subDeltaImage( deltaImage(model), onePlane, True);
		SubLattice<Float> *subMaskPointer=0;
		Bool skipThisPlane=False;
		if (doMask) {
		  subMaskPointer = new SubLattice<Float> ( *maskPointer, onePlane, True);
		  if(max(*subMaskPointer).getFloat() <= 0.0){
		    skipThisPlane=True;
		  }
		}
		if(!skipThisPlane){
		  if(cleaner) {
		    os << "Updating multiscale cleaner with new residual images"
		       << LogIO::POST;
		    cleaner->update(subResid);
		  }
		  else {
		    os << "Creating multiscale cleaner with psf and residual images" << LogIO::POST;
		    cleaner=new LatticeCleaner<Float>(subPSF, subResid);
		    setScales(*cleaner);
		    if (doMask) {		  
		      cleaner->setMask(*subMaskPointer);
		    }
		  }
		  subDeltaImage.set(0.0);
		  
		  cleaner->setcontrol(CleanEnums::MULTISCALE, numberIterations(), gain(), 
				      aThreshold, fThreshold, True);
		  
		  if (cycleSpeedup_p > 1) {
		    os << "cycleSpeedup is " << cycleSpeedup_p << LogIO::POST;
		    cleaner->speedup(cycleSpeedup_p);
		  }
		
		  cleaner->startingIteration( iterations[model](chan) );
		  if (cycle <= stopLargeNegatives_p) {
		    cleaner->stopAtLargeScaleNegative();
		  }
		  cleaner->stopPointMode(stopPointMode_p);
		  cleaner->ignoreCenterBox(True);
		  cleaner->clean( subDeltaImage, progress_p );
		  
		
		  iterations[model](chan)=cleaner->numberIterations();
		  maxIterations=(iterations[model](chan)>maxIterations) ?
		    iterations[model](chan) : maxIterations;
		  os << "Clean used " << iterations[model](chan) << " iterations" 
		     << LogIO::POST;
		  modified_p=True;
		  
		  subImage.copyData( LatticeExpr<Float>( subImage + subDeltaImage));
		  
		  if (cleaner->queryStopPointMode()) {
		    stop = True;		
		    os << "MSClean terminating because we hit " 
		       << stopPointMode_p
		       << " consecutive compact sources" << LogIO::POST;
		  }
		  if (doMask) {		  
		    delete subMaskPointer;
		  }
		}
	      }
	    }
	  }
	  if (mustDeleteMask) {
	    delete maskPointer;
	  }
	}
      }
    }
  }

  if(modified_p) {
    os << "Finalizing residual images for all fields" << LogIO::POST;
    makeNewtonRaphsonStep(se, False, True);
    Float finalabsmax=maxField(resmax, resmin);
    converged=(finalabsmax < 1.05 * threshold());
      os << "Final maximum residual = " << finalabsmax << LogIO::POST;
      
    for (model=0;model<numberOfModels();model++) {
      os << "Model " << model+1 << ": max, min residuals = "
	 << resmax(model) << ", " << resmin(model) << endl;
    }
    os << LogIO::POST;
  }
  else {
    os << "Residual images for all fields are up-to-date" << LogIO::POST;
  }

  if(cleaner) delete cleaner; cleaner=0;

  return(converged);
};
  

void
MFMSCleanImageSkyModel::setScales(LatticeCleaner<Float>& cleaner)
{

  LogIO os(LogOrigin("MFCleanImageSkyModel","setScales"));
  if (method_p == USERVECTOR) {
    if (userScaleSizes_p.nelements() <= 0) {
      os << LogIO::SEVERE 
	 << "Need at least one scale for method uservector"
	 << LogIO::POST;
    }
    os << "Creating scales from uservector method: " << LogIO::POST;
    for(uInt scale=0; scale < userScaleSizes_p.nelements(); scale++) {
      os << "scale " << scale+1 << " = " << userScaleSizes_p(scale)
	 << " pixels" << LogIO::POST;
    }
    cleaner.setscales(userScaleSizes_p);   
  } else {
    if (nscales_p <= 0) nscales_p = 1;
    Vector<Float> scaleSizes(nscales_p);  
    os << "Creating " << nscales_p << 
      " scales from powerlaw nscales method" << LogIO::POST;
    scaleSizes(0) = 0.0;
    os << "scale 1 = 0.0 pixels " << LogIO::POST;
    Float scaleInc = 2.0;
    for (Int scale=1; scale<nscales_p;scale++) {
      scaleSizes(scale) =
	scaleInc * pow(10.0, (Float(scale)-2.0)/2.0);
      os << "scale " << scale+1 << " = " << scaleSizes(scale)
	 << " pixels" << LogIO::POST;
    }  
    cleaner.setscales(scaleSizes);   
  }
};

} //#End casa namespace
