//# WBCleanImageSkyModel.cc: Implementation of WBCleanImageSkyModel class
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
//# $Id$


#include <casa/Arrays/ArrayMath.h>
#include <synthesis/MeasurementComponents/WBCleanImageSkyModel.h>
#include <synthesis/MeasurementEquations/CubeSkyEquation.h>
#include <casa/OS/File.h>
#include <synthesis/MeasurementEquations/SkyEquation.h>
#include <synthesis/MeasurementEquations/StokesImageUtil.h>
#include <synthesis/MeasurementEquations/LatticeModel.h>
#include <synthesis/MeasurementEquations/LatConvEquation.h>
#include <casa/Exceptions/Error.h>
#include <casa/BasicSL/String.h>
#include <casa/Utilities/Assert.h>

#include <lattices/Lattices/LatticeCleaner.h>
#include <lattices/Lattices/SubLattice.h>
#include <lattices/Lattices/LCBox.h>
#include <lattices/Lattices/LatticeExprNode.h>
#include <lattices/Lattices/LatticeExpr.h>
#include <lattices/Lattices/TempLattice.h>
#include <lattices/Lattices/LatticeFFT.h>
#include <lattices/Lattices/TiledLineStepper.h> 
#include <lattices/Lattices/LatticeStepper.h> 
#include <lattices/Lattices/LatticeNavigator.h> 
#include <lattices/Lattices/LatticeIterator.h>

#include <scimath/Mathematics/MatrixMathLA.h>

#include <msvis/MSVis/VisSet.h>
#include <msvis/MSVis/VisSetUtil.h>

#include <ms/MeasurementSets/MSColumns.h>

#include <casa/sstream.h>

#include <casa/Logging/LogMessage.h>
#include <casa/Logging/LogSink.h>

#include <casa/OS/HostInfo.h>


namespace casa { //# NAMESPACE CASA - BEGIN
#define TMR(a) "[User: " << a.user() << "] [System: " << a.system() << "] [Real: " << a.real() << "]"

#define MEMFACTOR (4.0)
	
#define MIN(a,b) ((a)<=(b) ? (a) : (b))
#define MAX(a,b) ((a)>=(b) ? (a) : (b))

/*************************************
 *          Constructor
 *************************************/
WBCleanImageSkyModel::WBCleanImageSkyModel()
{
  initVars();
//  if(adbg) cout << "CONSTRUCTOR - no args!!" << endl;
  nscales_p=4;
  ntaylor_p=2;
  refFrequency_p=1.42e+09;
  scaleSizes_p.resize(0);
};
WBCleanImageSkyModel::WBCleanImageSkyModel(const Int ntaylor,const Int nscales,const Double reffreq)
{
  initVars();
//  if(adbg) cout << "CONSTRUCTOR - with nscales !!" << endl;
  nscales_p=nscales;
  ntaylor_p=ntaylor;
  refFrequency_p=reffreq;
  scaleSizes_p.resize(0);
};
WBCleanImageSkyModel::WBCleanImageSkyModel(const Int ntaylor,const Vector<Float>& userScaleSizes,const Double reffreq)
{
  initVars();
//  if(adbg) cout << "CONSTRUCTOR with userscalevector !!" << endl;
  nscales_p=userScaleSizes.nelements();
  scaleSizes_p.resize(nscales_p);
  for(Int i=0;i<nscales_p;i++) scaleSizes_p[i] = userScaleSizes[i];
  ntaylor_p=ntaylor;
  refFrequency_p=reffreq;
};

void WBCleanImageSkyModel::initVars()
{
  adbg=1; 
  ddbg=1; // output per iteration
  tdbg=0;
  
  modified_p=True;
  memoryMB_p = Double(HostInfo::memoryTotal()/1024)/(2.0);
  donePSF_p=False;

  nfields_p=1;
  lc_p.resize(0);

  os = LogIO(LogOrigin("WBCleanImageSkyModel","solve"));

  setAlgorithm("MSMFS");
  
}

/*************************************
 *          Destructor
 *************************************/
WBCleanImageSkyModel::~WBCleanImageSkyModel()
{
//  if(adbg) cout << "WBCleanImageSkyModel DESTRUCTOR !!" << endl;

  for(uInt i=0;i<lc_p.nelements();i++)
	  if(lc_p[i] != NULL) delete lc_p[i];
  
//  if(adbg) cout << "... Successfully destroyed lattice-cleaners !!" << endl;
};

/*************************************
 *          Solver
 *************************************/
Bool WBCleanImageSkyModel::solve(SkyEquation& se) 
{
	if(adbg)os << "SOLVER for Multi-Frequency Synthesis deconvolution" << LogIO::POST;
	Int stopflag=0;
	static bool firstloop=True;
	Int nchan=0,npol=0;

	/* Gather shape information */
	nmodels_p = numberOfModels();

	if(nmodels_p % ntaylor_p != 0)
	{
	  os << "Incorrect number of input models " << LogIO::EXCEPTION;
	  os << "NModels = N_fields x N_taylor" << LogIO::EXCEPTION;
	  AlwaysAssert((nmodels_p % ntaylor_p == 0), AipsError);
	}

	/* Calc the number of fields */
	nfields_p = nmodels_p/ntaylor_p;

	for(Int model=0;model<nmodels_p;model++)
	{
	  nx = image(model).shape()(0);
	  ny = image(model).shape()(1);
	  npol=image(model).shape()(2);
	  nchan=image(model).shape()(3);
	  if(nchan > 1) os << "Cannot process more than one output channel" << LogIO::EXCEPTION;
	  if(npol > 1) os << "Cannot process more than one output polarization" << LogIO::EXCEPTION;
	  AlwaysAssert((nchan==1), AipsError);  
	  AlwaysAssert((npol==1), AipsError);  
    
	  AlwaysAssert(isSolveable(model), AipsError);
	}

	/* Initialize the MultiTermLatticeCleaners */
	if(firstloop)
	{
	  lc_p.resize(nfields_p);
	  for(Int thismodel=0;thismodel<nfields_p;thismodel++)
	  {
	    lc_p[thismodel] = new MultiTermLatticeCleaner<Float>();
	    lc_p[thismodel]->setcontrol(CleanEnums::MULTISCALE, numberIterations(), gain(), Quantity(threshold(),"Jy"), false);
	    //lc_p[thismodel]->ignoreCenterBox(true);
	    lc_p[thismodel]->setscales(scaleSizes_p);
	    lc_p[thismodel]->setntaylorterms(ntaylor_p);
	    nx = image(thismodel).shape()(0);
	    ny = image(thismodel).shape()(1);
	    lc_p[thismodel]->initialise(nx,ny); // allocates memory once....
	  }
	  firstloop=False;
	}

	/* Create the Point Spread Functions */
	if(!donePSF_p)
	{
	  /* Resize the work arrays to calculate extra PSFs */
	  Int original_nmodels = nmodels_p;
	  nmodels_p = original_nmodels + nfields_p * (ntaylor_p - 1);
	  resizeWorkArrays(nmodels_p);

	  /* Make the 2N-1 PSFs */
          makeSpectralPSFs(se);

	  /* Send all 2N-1 PSFs into the MultiTermLatticeCleaner */
	  for(Int thismodel=0;thismodel<nfields_p;thismodel++)
	  {
	    for (Int order=0;order<2*ntaylor_p-1;order++)
	    {
	      lc_p[thismodel]->setpsf(order,PSF(getModelIndex(thismodel,order))); //ARRAY
	    }
	  }
	  
	  /* Resize the work arrays to normal size - for residual comps, etc. */
	  nmodels_p = original_nmodels;
	  resizeWorkArrays(nmodels_p);
	   
	  donePSF_p=True;
	}

	/* Set up the Mask image */
	for(Int thismodel=0;thismodel<nfields_p;thismodel++)
	{
	  if(hasMask(thismodel)) 
	  {
	    os << "Sending in the mask" << LogIO::POST;
	    lc_p[thismodel]->setmask( mask(thismodel) );
	  }
	}

	/* Calculate the initial residual image for all models. */
	if(adbg)os << "Calc initial solveResiduals(se)..." << LogIO::POST;
	solveResiduals(se);

	/******************* START MAJOR CYCLE LOOP *****************/
	os << "Starting the solver major cycles" << LogIO::POST;
	Int index=0;
	for(Int itercountmaj=0;itercountmaj<100;itercountmaj++)
	{
	   for(Int thismodel=0;thismodel<nfields_p;thismodel++)
	   {
	     /* Send in the current model and residual */
	     for (Int order=0;order<ntaylor_p;order++)
	     {
	        index = getModelIndex(thismodel,order);
		lc_p[thismodel]->setresidual(order,residual(index)); //ARRAY
		lc_p[thismodel]->setmodel(order,image(index)); //ARRAY
	     }
		
	     /* Deconvolve */
	     stopflag = lc_p[thismodel]->mtclean();

	     /* Get out the updated model */
	     for (Int order=0;order<ntaylor_p;order++)
	     {
	        index = getModelIndex(thismodel,order);
	        lc_p[thismodel]->getmodel(order,image(index));
	     }
           
	   }// end of model loop

	   /* Do the prediction and residual computation for all models. */
	   if(adbg)os << "Calc residuals : solveResiduals(se)..." << LogIO::POST;
	   solveResiduals(se);
	   
	   if(abs(stopflag)) break;
	} 
	/******************* END MAJOR CYCLE LOOP *****************/
	
	/* Compute and write alpha,beta results to disk */
	writeResultsToDisk();
	
	if(stopflag>0) return(True);
	else return(False);
} // END OF SOLVE


/***********************************************************************/
/* Write alpha and beta to disk. Compute from model images. */
Int WBCleanImageSkyModel::writeResultsToDisk()
{
  if(ntaylor_p<=1) return 0;

  if(ntaylor_p==2) os << "Calculating Spectral Index" << LogIO::POST;
  if(ntaylor_p>2) os << "Calculating Spectral Index and Curvature" << LogIO::POST;

  for(Int model=0;model<nfields_p;model++)
  {

    String alphaname = (image(model)).name(False) +  String(".alpha");
    String betaname = (image(model)).name(False) +  String(".beta");
  
    PagedImage<Float> imalpha(image(model).shape(),image(model).coordinates(),alphaname); 
    imalpha.set(0.0);
    
    /* Create a mask */
    os << "Calculate spectral params for values greater than " << threshold()*2.0 << LogIO::POST;
    LatticeExpr<Float> mask1(iif(image(getModelIndex(model,0))>(threshold()*2.0),1.0,0.0));
    LatticeExpr<Float> mask0(iif(image(getModelIndex(model,0))>(threshold()*2.0),0.0,1.0));
	 
    LatticeExpr<Float> alphacalc( (image(getModelIndex(model,1))*mask1)/(image(getModelIndex(model,0))+(mask0)) );
    imalpha.copyData(alphacalc);
    imalpha.table().unmarkForDelete();

    if(ntaylor_p>2)
    {
      PagedImage<Float> imbeta(image(model).shape(),image(model).coordinates(),betaname); 
      imbeta.set(0.0);

      LatticeExpr<Float> betacalc( (image(getModelIndex(model,2))*mask1)/(image(getModelIndex(model,0))+(mask0))-0.5*(imalpha)*(imalpha-1.0) );
      imbeta.copyData(betacalc);
      
      imbeta.table().unmarkForDelete();
    }

  }// model loop
return 0;
}
/***********************************************************************/

/***********************************************************************/
#if 0
///// Write alpha and beta to disk. Calculate from smoothed model + residuals.

Int WBCleanImageSkyModel::writeResultsToDisk()
{
  if(ntaylor_p<=1) return 0;

  if(ntaylor_p==2) os << "Calculating Spectral Index" << LogIO::POST;
  if(ntaylor_p>2) os << "Calculating Spectral Index and Curvature" << LogIO::POST;
  
  PtrBlock<TempLattice<Float>* > smoothed;
  if(ntaylor_p>2) smoothed.resize(3);
  else if(ntaylor_p==2) smoothed.resize(2);

  Quantity bmaj=0.0,bmin=0.0,bpa=0.0;
  Int index=0;
  
  for(Int model=0;model<nfields_p;model++)
  {

    String alphaname = (image(model)).name(False) +  String(".alpha");
    String betaname = (image(model)).name(False) +  String(".beta");
  
    PagedImage<Float> imalpha(image(model).shape(),image(model).coordinates(),alphaname); 
    imalpha.set(0.0);
    

    StokesImageUtil::FitGaussianPSF(PSF(model), bmaj, bmin, bpa);
    IPosition gip(4,image(model).shape()[0],image(model).shape()[1],1,1);
    for(uInt i=0;i<smoothed.nelements();i++)
    {
	    smoothed[i] = new TempLattice<Float>(gip,memoryMB_p);
  
	    index = getModelIndex(model,i);
	    LatticeExpr<Float> cop(image(index));
	    imalpha.copyData(cop);
	    StokesImageUtil::Convolve(imalpha, bmaj, bmin, bpa);
	    LatticeExpr<Float> le(imalpha+(residual(index))); 
	    (*smoothed[i]).copyData(le);
    }


    /* Create a mask */
    os << "Calculate spectral params for values greater than " << threshold()*2.0 << LogIO::POST;
    LatticeExpr<Float> mask1(iif((*smoothed[0])>(threshold()*2.0),1.0,0.0));
    LatticeExpr<Float> mask0(iif((*smoothed[0])>(threshold()*2.0),0.0,1.0));
	 
    LatticeExpr<Float> alphacalc( ((*smoothed[1])*mask1)/((*smoothed[0])+(mask0)) );
    imalpha.copyData(alphacalc);
    
    ImageInfo ii = imalpha.imageInfo();
    ii.setRestoringBeam(bmaj, bmin, bpa);
    
    imalpha.setImageInfo(ii);
    //imalpha.setUnits(Unit("Spectral Index"));
    imalpha.table().unmarkForDelete();

    if(ntaylor_p>2)
    {
      PagedImage<Float> imbeta(image(model).shape(),image(model).coordinates(),betaname); 
      imbeta.set(0.0);

      LatticeExpr<Float> betacalc( ((*smoothed[2])*mask1)/((*smoothed[0])+(mask0))-0.5*(imalpha)*(imalpha-1.0) );
      imbeta.copyData(betacalc);
      
      imbeta.setImageInfo(ii);
      //imbeta.setUnits(Unit("Spectral Curvature"));
      imbeta.table().unmarkForDelete();
    }

    for(uInt i=0;i<smoothed.nelements();i++) if(smoothed[i]) delete smoothed[i];
  }// model loop
return 0;
}
#endif

/***********************************************************************/
#if 1
/*************************************
 *          Make Residuals and compute the current peak  
 *************************************/
Bool WBCleanImageSkyModel::solveResiduals(SkyEquation& se) 
{
        makeNewtonRaphsonStep(se,False);
	
	return True;
}
#endif
/***********************************************************************/

/*************************************
 *          Make Residuals 
 *************************************/
// Not sure if normalization needs to be done with ggS per taylor term
// Or if all need to be normalized with ggS from taylor0
// Currently - all normalized by ggS from taylor0.
// SAME WITH MAKE_SPECTRAL_PSFS

Bool WBCleanImageSkyModel::makeNewtonRaphsonStep(SkyEquation& se, Bool incremental, Bool modelToMS) 
{
  LogIO os(LogOrigin("WBCleanImageSkyModel", "makeNewtonRaphsonStep"));
  se.gradientsChiSquared(incremental, modelToMS);

  Int index=0;
  for(Int thismodel=0;thismodel<nfields_p;thismodel++) 
  {
    for(Int taylor=0;taylor<ntaylor_p;taylor++)
    {
      /* Normalize by the Taylor 0 weight image */
      index = getModelIndex(thismodel,taylor);
      //LatticeExpr<Float> le(iif(ggS(index)>(0.0), -gS(index)/ggS(index), 0.0));
      LatticeExpr<Float> le(iif(ggS(thismodel)>(0.0), -gS(index)/ggS(thismodel), 0.0));
      residual(index).copyData(le);
      
      //storeAsImg(String("TstRes.")+String::toString(thismodel)+String(".")+String::toString(taylor),residual(index));
    }
  }
  modified_p=False;
  return True;
}

/*************************************
 *          Make Approx PSFs. 
 *************************************/
// The normalization ignores that done in makeSimplePSFs in the Sky Eqn
// and recalculates it from gS and ggS.
Int WBCleanImageSkyModel::makeSpectralPSFs(SkyEquation& se) 
{
  LogIO os(LogOrigin("WBCleanImageSkyModel", "makeSpectralPSFs"));
  if(!donePSF_p)
  {
    //make sure the psf images are made
    for (Int thismodel=0;thismodel<nmodels_p;thismodel++) 
    {
      PSF(thismodel);
    }
  }

  se.makeApproxPSF(psf_p);  

#if 1
  Float normfactor=1.0;
  Int index=0;
  for (Int thismodel=0;thismodel<nfields_p;thismodel++) 
  {
     normfactor=1.0;
     for(Int taylor=0;taylor<2*ntaylor_p-1;taylor++)
     {
	/* Normalize by the Taylor 0 weight image */
 	index = getModelIndex(thismodel,taylor);
	//LatticeExpr<Float> le(iif(ggS(index)>(0.0), gS(index)/ggS(index), 0.0));
	LatticeExpr<Float> le(iif(ggS(thismodel)>(0.0), gS(index)/ggS(thismodel), 0.0));
	PSF(index).copyData(le);
	if(taylor==0)
	{ 
	  LatticeExprNode maxPSF=max(PSF(index));
	  normfactor = maxPSF.getFloat();
	  os << "Normalize PSFs for model " << thismodel << " by " << normfactor << LogIO::POST;
	}
	LatticeExpr<Float> lenorm(PSF(index)/normfactor);
	PSF(index).copyData(lenorm);
	LatticeExprNode maxPSF2=max(PSF(index));
        Float maxpsf=maxPSF2.getFloat();
	os << "Psf for Model " << thismodel << " and Taylor " << taylor << " has peak " << maxpsf << LogIO::POST;

	//storeAsImg(String("TstPsf.")+String::toString(thismodel)+String(".")+String::toString(taylor),PSF(index));
     }
     
     index = getModelIndex(thismodel,0);
     beam(thismodel)=0.0;
     if(!StokesImageUtil::FitGaussianPSF(PSF(index),beam(thismodel))) 
     os << "Beam fit failed: using default" << LogIO::POST;
  }
#endif
  os << "Made spectral PSFs." << LogIO::POST;
  return 0;
}



/***********************************************************************/

/*************************************
 *          Store a Templattice as an image
 *************************************/
Int WBCleanImageSkyModel::storeAsImg(String fileName, ImageInterface<Float>& theImg)
{
  PagedImage<Float> tmp(theImg.shape(), theImg.coordinates(), fileName);
  LatticeExpr<Float> le(theImg);
  tmp.copyData(le);
  return 0;
}

Int WBCleanImageSkyModel::storeTLAsImg(String fileName, TempLattice<Float> &TL, ImageInterface<Float>& theImg)
{
  PagedImage<Float> tmp(TL.shape(), theImg.coordinates(), fileName);
  LatticeExpr<Float> le(TL);
  tmp.copyData(le);
  return 0;
}

Int WBCleanImageSkyModel::storeTLAsImg(String fileName, TempLattice<Complex> &TL, ImageInterface<Float>& theImg)
{
  PagedImage<Complex> tmp(TL.shape(), theImg.coordinates(), fileName);
  LatticeExpr<Complex> le(TL);
  tmp.copyData(le);
  return 0;
}


/*************************************
 *         Find the max and position 
 *         - restrict this to within the inner quarter.
 *************************************/
Bool WBCleanImageSkyModel::findMaxAbsLattice(const TempLattice<Float>& masklat,const Lattice<Float>& lattice,Float& maxAbs,IPosition& posMaxAbs, Bool flip)
{

  AlwaysAssert(masklat.shape()==lattice.shape(), AipsError);

  Array<Float> msk;
  
  posMaxAbs = IPosition(lattice.shape().nelements(), 0);
  maxAbs=0.0;
  //maxAbs=-1.0e+10;
  const IPosition tileShape = lattice.niceCursorShape();
  TiledLineStepper ls(lattice.shape(), tileShape, 0);
  TiledLineStepper lsm(masklat.shape(), tileShape, 0);
  {
    RO_LatticeIterator<Float> li(lattice, ls);
    RO_LatticeIterator<Float> lim(masklat, lsm);
    for(li.reset(),lim.reset();!li.atEnd();li++,lim++) 
    {
      IPosition posMax=li.position();
      IPosition posMin=li.position();
      Float maxVal=0.0;
      Float minVal=0.0;
      
      msk = lim.cursor();
      if(flip) msk = (Float)1.0 - msk;
      
      //minMaxMasked(minVal, maxVal, posMin, posMax, li.cursor(),lim.cursor());
      minMaxMasked(minVal, maxVal, posMin, posMax, li.cursor(),msk);
      
      
      if((maxVal)>(maxAbs)) 
      {
        maxAbs=maxVal;
	posMaxAbs=li.position();
	posMaxAbs(0)=posMax(0);
      }
     
      /*
      if(abs(minVal)>abs(maxAbs)) {
        maxAbs=minVal;
	posMaxAbs=li.position();
	posMaxAbs(0)=posMin(0);
      }
      if(abs(maxVal)>abs(maxAbs)) {
        maxAbs=maxVal;
	posMaxAbs=li.position();
	posMaxAbs(0)=posMax(0);
      }
      */
    }
  }

  return True;
}

/*************************************
 *    Add two subLattices..      -- same code as in copyData.   
 *************************************/
Int WBCleanImageSkyModel::addTo(Lattice<Float>& to, const Lattice<Float>& add, Float multiplier)
{
  // Check the lattice is writable.
  // Check the shape conformance.
  AlwaysAssert (to.isWritable(), AipsError);
  const IPosition shapeIn  = add.shape();
  const IPosition shapeOut = to.shape();
  AlwaysAssert (shapeIn.isEqual (shapeOut), AipsError);
  IPosition cursorShape = to.niceCursorShape();
  LatticeStepper stepper (shapeOut, cursorShape, LatticeStepper::RESIZE);
  LatticeIterator<Float> toIter(to, stepper);
  RO_LatticeIterator<Float> addIter(add, stepper);
  for (addIter.reset(), toIter.reset(); !addIter.atEnd();addIter++, toIter++) 
  {
    toIter.rwCursor()+=addIter.cursor()*multiplier;
  }
  return 0;
}

/***************************************
 * copy a templattice to an imageinterface
 ****************************************/

Bool WBCleanImageSkyModel::copyLatToImInt(TempLattice<Float>& lat, ImageInterface<Float>& im)
{
  	IPosition blc2(4,0,0,0,0);
  	IPosition trc2(4,nx,ny,0,0);
  	IPosition inc2(4, 1);
  	
  	LCBox::verify(blc2,trc2,inc2,im.shape());
  	LCBox copybox(blc2,trc2,im.shape());
  	(SubLattice<Float>(im,copybox,True)).copyData(LatticeExpr<Float> (lat));
	
	return True;
}

/***************************************
 * copy an imageinterface to a lattice
 ****************************************/

Bool WBCleanImageSkyModel::copyImIntToLat(TempLattice<Float>& lat, ImageInterface<Float>& im)
{
  	IPosition blc2(4,0,0,0,0);
  	IPosition trc2(4,nx,ny,0,0);
  	IPosition inc2(4, 1);
  	
  	LCBox::verify(blc2,trc2,inc2,im.shape());
  	LCBox copybox(blc2,trc2,im.shape());
	lat.copyData(SubLattice<Float>(im,copybox,True));
  
	return True;
}

/**************************
  Resize Work Arrays to calculate extra PSFs.
  *************************/
Bool WBCleanImageSkyModel::resizeWorkArrays(Int length)
{
   Int originallength = gS_p.nelements();
  
   if(length < originallength) // Clean up extra arrays
   {
     for(Int i = length; i < originallength; ++i)
     {
	if(psf_p[i]){delete psf_p[i]; psf_p[i]=0;}
	if(image_p[i]){delete image_p[i]; image_p[i]=0;}
	if(cimage_p[i]){delete cimage_p[i]; cimage_p[i]=0;}
	if(gS_p[i]){delete gS_p[i]; gS_p[i]=0;}
	if(ggS_p[i]){delete ggS_p[i]; ggS_p[i]=0;}
	if(work_p[i]){delete work_p[i]; work_p[i]=0;}
	if(fluxScale_p[i]){delete fluxScale_p[i]; fluxScale_p[i]=0;}
     }
   }

   psf_p.resize(length,True);
   image_p.resize(length,True);
   cimage_p.resize(length,True);
   gS_p.resize(length,True);
   ggS_p.resize(length,True);
   work_p.resize(length,True);
   fluxScale_p.resize(length,True);
   
   if(length > originallength) // Add extra arrays
   {
     for(Int i = originallength; i < length; ++i)
     {
       psf_p[i]=0;gS_p[i]=0;ggS_p[i]=0;cimage_p[i]=0;work_p[i]=0;fluxScale_p[i]=0;

       Int ind = getFieldIndex(i);
       TempImage<Float>* imptr = 
	 new TempImage<Float> (IPosition(image_p[ind]->ndim(),
				 image_p[ind]->shape()(0),
				 image_p[ind]->shape()(1),
				 image_p[ind]->shape()(2),
				 image_p[ind]->shape()(3)),
			         image_p[ind]->coordinates(), memoryMB_p);
       AlwaysAssert(imptr, AipsError);
       image_p[i] = imptr;
     }
   }
   return True;
}



} //# NAMESPACE CASA - END

