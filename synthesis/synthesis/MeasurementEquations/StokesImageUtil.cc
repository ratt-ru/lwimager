//# StokesImageUtil.cc: Stokes Image Utilities
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
//# Correspondence concerning AIPS++ should be adressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//#
//# $Id: StokesImageUtil.cc,v 19.9 2004/12/01 16:42:18 ddebonis Exp $

#include <casa/aips.h>

#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/MatrixMath.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/Cube.h>
#include <casa/BasicMath/Math.h>
#include <casa/BasicSL/Complex.h>
#include <scimath/Mathematics/FFTServer.h>
#include <casa/BasicSL/Constants.h>
#include <casa/Utilities/Assert.h>
#include <lattices/Lattices/Lattice.h>
#include <lattices/Lattices/LatticeExpr.h>
#include <lattices/Lattices/LatticeIterator.h>
#include <lattices/Lattices/LatticeStepper.h>
#include <scimath/Fitting/NonLinearFitLM.h>
#include <scimath/Functionals/Gaussian2D.h>
#include <casa/Arrays/ArrayIO.h>

#include <ms/MeasurementSets/MSColumns.h>
#include <ms/MeasurementSets/MSDopplerUtil.h>

#include <msvis/MSVis/StokesVector.h>
#include <synthesis/MeasurementEquations/StokesImageUtil.h>
#include <images/Images/PagedImage.h>
#include <casa/OS/File.h>

#include <casa/Quanta/UnitMap.h>
#include <casa/Quanta/UnitVal.h>
#include <measures/Measures/Stokes.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/SpectralCoordinate.h>
#include <coordinates/Coordinates/StokesCoordinate.h>
#include <coordinates/Coordinates/Projection.h>
#include <casa/Logging/LogSink.h>
#include <casa/Logging/LogMessage.h>


#include <casa/iostream.h>

namespace casa {

// <summary> 
// </summary>

// <reviewed reviewer="" date="" tests="tMEGI" demos="">

// <prerequisite>
// </prerequisite>
//
// <etymology>
// </etymology>
//
// <synopsis> 
// </synopsis> 
//
// <example>
// <srcblock>
// </srcblock>
// </example>
//
// <motivation>
// </motivation>
//
// <todo asof="">
// </todo>

// In-place convolve. image is 4 dimensional. RA and Dec first.
void StokesImageUtil::Convolve(ImageInterface<Float>& image,
			       ImageInterface<Float>& psf) {
  
  Vector<Int> map;
  AlwaysAssert(StokesMap(map, image.coordinates()), AipsError);
  
  Vector<Int> psfmap;
  AlwaysAssert(StokesMap(psfmap, psf.coordinates()), AipsError);
  
  Int nx = image.shape()(map(0));
  Int ny = image.shape()(map(1));
  
  // Make FFT machine
  FFTServer<Float,Complex> fft(IPosition(2, nx, ny));
  
  // Get the image into a Matrix for Fourier transformation
  Array<Complex> xfr;
  Matrix<Complex> cft;
  
  LatticeStepper ls(image.shape(), IPosition(4, nx, ny, 1, 1),
		    IPosition(4, map(0), map(1), map(2), map(3)));
  LatticeIterator<Float> li(image, ls);
  
  // Get the PSF into a Matrix for Fourier transformation
  LatticeStepper psfls(psf.shape(), IPosition(4, nx, ny, 1, 1),
		       IPosition(4, map(0), map(1), map(2), map(3)));
  RO_LatticeIterator<Float> psfli(psf, psfls);
  psfli.reset();
  //fft.fft(xfr, psfli.matrixCursor());
  fft.fft0(xfr, psfli.matrixCursor());
  
  // Loop over all planes
  for (li.reset();!li.atEnd();li++) {
    //fft.fft(cft, li.matrixCursor());
    fft.fft0(cft,li.matrixCursor());
    cft *= xfr;
    // fft.fft(li.rwMatrixCursor(), cft);
    fft.fft0(li.rwMatrixCursor(),cft,False);
    fft.flip(li.rwMatrixCursor(), False, False);
  }
};

void StokesImageUtil::Convolve(ImageInterface<Float>& image,
			       Quantity& bmaj, Quantity& bmin,
			       Quantity& bpa,
			       Bool normalizeVolume)
{
  Convolve(image, bmaj.get("arcsec").getValue(),
	   bmin.get("arcsec").getValue(),
	   bpa.get("deg").getValue(),
	   normalizeVolume);
}

void StokesImageUtil::Convolve(ImageInterface<Float>& image, Float bmaj,
			       Float bmin, Float bpa,
			       Bool normalizeVolume) {
  
  Vector<Float> beam(3);
  beam(0)=bmaj*C::arcsec;
  beam(1)=bmin*C::arcsec;
  beam(2)=(bpa+90.0)*C::degree;
  PagedImage<Float>
    cleanpsf(IPosition(4, image.shape()(0), image.shape()(1), 1, 1),
	     image.coordinates(),
	     File::newUniqueName("./","imagesolver::cleanpsf").originalName());
  cleanpsf.table().markForDelete();
  MakeGaussianPSF(cleanpsf, beam, normalizeVolume);
  Convolve(image, cleanpsf);
}

void StokesImageUtil::MakeGaussianPSF(ImageInterface<Float>& psf,
				      Quantity& bmaj, Quantity& bmin,
				      Quantity& bpa,
				      Bool normalizeVolume) 
{
  Vector<Float> beam(3);
  beam(0)=bmaj.get("arcsec").getValue();
  beam(1)=bmin.get("arcsec").getValue();
  beam(2)=bpa.get("deg").getValue();
  MakeGaussianPSF(psf, beam, normalizeVolume);
}

// Make an image into a Gaussian PSF
void StokesImageUtil::MakeGaussianPSF(ImageInterface<Float>& psf, Vector<Float>& beam,
				      Bool normalizeVolume) {
  
  Int nx=psf.shape()(0);
  Int ny=psf.shape()(1);
  Matrix<Float> ipsf(nx, ny);
  
  Double cospa=cos(beam(2));
  Double sinpa=sin(beam(2));
  Vector<Double> rp=psf.coordinates().referencePixel();
  Vector<Double> d=psf.coordinates().increment();
  Double refi=rp(0);
  Double refj=rp(1);
  AlwaysAssert(beam(0)>0.0,AipsError);
  AlwaysAssert(beam(1)>0.0,AipsError);
  Double sbmaj, sbmin;
  
  // Assumes that the cell sizes are the same in both directions!
  sbmaj=4.0*log(2.0)*square(d(0)/beam(0));
  sbmin=4.0*log(2.0)*square(d(0)/beam(1));
  Float volume=0.0;
  for (Int j=0;j<ny;j++) {
    for (Int i=0;i<nx;i++) {
      Double x =   cospa * (Double(i)-refi) + sinpa * (Double(j)-refj);
      Double y = - sinpa * (Double(i)-refi) + cospa * (Double(j)-refj);
      Double radius = sbmaj*square(x) + sbmin*square(y);
      if (radius<20.) {
	ipsf(i,j) = exp(-radius);
        volume+=ipsf(i,j);
      }
      else {
	ipsf(i,j)=0.;
      }
    }
  }
  if(normalizeVolume) ipsf/=volume;
  psf.putSlice(ipsf, IPosition(psf.ndim(),0), IPosition(psf.ndim(),1));
  
}

// Zero specified elements of a Stokes image
void StokesImageUtil::Zero(ImageInterface<Float>& image, Vector<Bool>& mask) {
  
  AlwaysAssert(image.ndim()==4,AipsError);
  AlwaysAssert(mask.shape()(0)==4,AipsError);
  
  Vector<Int> map;
  AlwaysAssert(StokesMap(map, image.coordinates()), AipsError);
  
  Int nx = image.shape()(map(0));
  Int ny = image.shape()(map(1));
  Int npol = image.shape()(map(2));
  Int nchan = image.shape()(map(3));
  
  LatticeStepper ls(image.shape(), IPosition(4, nx, 1, 1, 1),
		    IPosition(4, map(0), map(1), map(2), map(3)));
  LatticeIterator<Float> li(image, ls);
  
  // Loop over all planes
  li.reset();
  for (Int chan=0;chan<nchan;chan++) {
    for(Int pol=0;pol<npol;pol++) {
      for(Int iy=0;iy<ny;iy++,li++) {
	if(mask(pol)) {
	  li.woVectorCursor()=0.0;
	}
      }
    }
  }
  
}

void StokesImageUtil::MaskFrom(ImageInterface<Float>& mask,
			       ImageInterface<Float>& image,
			       const Quantity& threshold) 
{
  
  Double thres=threshold.get("Jy").getValue();
  
  AlwaysAssert(image.ndim()==4,AipsError);
  AlwaysAssert(mask.shape()==image.shape(),AipsError);
  
  Vector<Int> map;
  AlwaysAssert(StokesMap(map, image.coordinates()), AipsError);
  
  Int nx = image.shape()(map(0));
  Int npol = image.shape()(map(2));
  
  LatticeStepper ls(image.shape(), IPosition(4, nx, 1, npol, 1),
		    IPosition(4, map(0), map(1), map(2), map(3)));
  RO_LatticeIterator<Float> li(image, ls);
  LatticeIterator<Float> mli(mask, ls);
  
  // Loop over all planes
  for (li.reset(),mli.reset();!li.atEnd()&&!mli.atEnd();li++,mli++) {
    if(npol>1) {
      mli.rwMatrixCursor()=0.0;
      for (Int ix=0;ix<nx;ix++) {
	if(li.matrixCursor()(ix,0)>thres) {
	  for (Int pol=0;pol<npol;pol++) mli.rwMatrixCursor()(ix,pol)=1.0;
	}
      }
    }
    else {
      mli.rwVectorCursor()=0.0;
      for (Int ix=0;ix<nx;ix++) {
	if(li.vectorCursor()(ix)>thres) {
	  mli.rwVectorCursor()(ix)=1.0;
	}
      }
    }
  }
  
}

void StokesImageUtil::MaskOnStokesI(ImageInterface<Float>& image,
				    const Quantity& threshold) 
{
  
  Double thres=threshold.get("Jy").getValue();
  
  AlwaysAssert(image.ndim()==4,AipsError);
  
  Vector<Int> map;
  AlwaysAssert(StokesMap(map, image.coordinates()), AipsError);
  
  Int nx = image.shape()(map(0));
  Int npol = image.shape()(map(2));
  
  LatticeStepper ls(image.shape(), IPosition(4, nx, 1, npol, 1),
		    IPosition(4, map(0), map(1), map(2), map(3)));
  LatticeIterator<Float> li(image, ls);
  
  // Loop over all planes
  for (li.reset();!li.atEnd();li++) {
    for (Int ix=0;ix<nx;ix++) {
      if(npol>1) {
	if(li.matrixCursor()(ix,0)<thres) {
	  for (Int pol=0;pol<npol;pol++) li.rwMatrixCursor()(ix,pol)=0.0;
	}
      }
      else {
	if(li.vectorCursor()(ix)<thres) {
	  li.rwVectorCursor()(ix)=0.0;
	}
      }
    }
  }
  
}

// Constrain (a) Stokes I to be > 0 and (b) I > abs(V) and
// (c) fractional pol<1.0
void StokesImageUtil::Constrain(ImageInterface<Float>& image) {
  
  AlwaysAssert(image.ndim()==4,AipsError);
  
  Vector<Int> map;
  AlwaysAssert(StokesMap(map, image.coordinates()), AipsError);
  
  Int nx = image.shape()(map(0));
  Int npol = image.shape()(map(2));
  
  LatticeStepper ls(image.shape(), IPosition(4, nx, 1, npol, 1),
		    IPosition(4, map(0), map(2), map(1), map(3)));
  LatticeIterator<Float> li(image, ls);
  
  // Loop over all planes
  for (li.reset();!li.atEnd();li++) {
    
    // Make a copy of the cursor
    Matrix<Float>& limage=li.rwMatrixCursor();
    
    for (Int ix=0;ix<nx;ix++) {
      // I >=0
      Bool zero=False;
      if(limage(ix,0)<0.0) {
	zero=True;
      }
      // I >= abs(V)
      else if (npol==2) {
	if(limage(ix,0)<abs(limage(ix,1))) zero=True;
      }
      // fractional pol < 1.0
      else if (npol==4) {
	StokesVector sv(limage(ix,0),limage(ix,1),
			limage(ix,2),limage(ix,3));
	if(sv.minEigenValue()<0.0) zero=True;
      }
      if(zero) for (Int pol=0;pol<npol;pol++) limage(ix,pol)=0.0;
    }
  }
}

Bool StokesImageUtil::FitGaussianPSF(ImageInterface<Float>& psf, 
				     Quantity& bmaj, Quantity& bmin, Quantity& bpa) 
{
  Vector<Float> beam(3);
  beam=0.0;
  Bool status=True;
  if(!FitGaussianPSF(psf, beam)) status=False;
  bmaj=Quantity(abs(beam(0)),"arcsec");
  bmin=Quantity(abs(beam(1)),"arcsec");
  bpa=Quantity(beam(2),"deg");
  return status;
}

Bool StokesImageUtil::FitGaussianPSF(ImageInterface<Float>& psf, Vector<Float>& beam) {
  
  Vector<Double> deltas;
  deltas=psf.coordinates().increment(); 
  



  Vector<Int> map;
  AlwaysAssert(StokesMap(map, psf.coordinates()), AipsError);
  
  Int nx = psf.shape()(map(0));
  Int ny = psf.shape()(map(1));
  Int nchan = psf.shape()(map(3));
  Int px=0;
  Int py=0;
  Matrix<Float> lpsf;
  Float psfmax;

  Float bamp=0;


  IPosition oneplane(psf.ndim(), nx, ny, 1, 1);
  LatticeStepper psfls(psf.shape(), oneplane,
		       IPosition(4,map(0),map(1),map(3),map(2)));
  RO_LatticeIterator<Float> psfli(psf,psfls);
  while (nchan >= 1 && bamp < 0.9){
    lpsf=psfli.matrixCursor();
    
    IPosition psfposmax(lpsf.ndim());
    IPosition psfposmin(lpsf.ndim());
    Float psfmin;
    
    minMax(psfmin, psfmax, psfposmin, psfposmax, lpsf); 
    
    px=psfposmax(0);
    py=psfposmax(1);
    bamp=lpsf(px,py);

    ++psfli;
    --nchan;
  }

  LogIO os(LogOrigin("StokesImageUtil", "FitGaussianPSF()",WHERE));
  if((bamp > 1.1) || (bamp < 0.9)) // No warning if 10% error in PSF peak
    os << LogIO::WARN << "Peak of PSF is " << bamp << LogIO::POST;


try{  


  if(bamp==0.0) {
    beam(0)=2.5*abs(deltas(0))/C::arcsec;
    beam(1)=2.5*abs(deltas(0))/C::arcsec;
    beam(2)=0.0;
    os << LogIO::WARN << "Could not find peak " << LogIO::POST;
    return False;
  }
  
  lpsf/=psfmax;

  // The selection code attempts to avoid including any sidelobes, even
  // if they exceed the threshold, by starting from the center column and
  // working out, exiting the loop when it crosses the threshold.  It
  // assumes that the first time it finds a "good" point starting from
  // the center and working out that it's in the main lobe.  Narrow,
  // sharply ringing beams inclined at 45 degrees will confuse it, but 
  // that's even more pathological than most VLBI beams.
  
  Float amin=0.35;
  Int nrow=5;
  
  //we sample the central part of a, 2*nrow+1 by 2*nrow+1
  
  Int npoints=0;
  Int maxnpoints=(2*nrow+1)*(2*nrow+1);
  Matrix<Double> ix(maxnpoints,2);
  Vector<Double> iy(maxnpoints), isigma(maxnpoints);
  ix=0.0; iy=0.0; isigma=0.0;
  Bool converg=False;
  Vector<Double> solution;
  Int kounter=0;
 while(amin >0.1 && !converg && (kounter < 50)){
  amin*=bamp;
  kounter+=1;
  Int iflip = 1;
  Int jflip = 1;
  // loop through rows. Include both above and below in case
  // we are fitting an image feature
  for(Int jlo = 0;jlo<2;jlo++) {
    jflip*=-1;
    // loop from 0 to nrow then from 1 to nrow
    for(Int j = jlo;j<nrow;j++) { 
      // the current row under consideration
      Int jrow = py + j*jflip;
      // loop down row doing alternate halves. work our way 
      // out from the center until we cross threshold
      // don't include any sidelobes!
      for(Int ilo=0;ilo<2;ilo++) {
	iflip*=-1;
	// start at center row this may or may not be in the lobe,
	// if it's narrow and the pa is near 45 degrees
	Bool inlobe = lpsf(px,jrow)>amin;
	for(Int i = ilo;i<nrow;i++) {
	  Int irow = px + i*iflip;
	  // did we step out of the lobe?
	  if (inlobe&&(lpsf(irow,jrow)<amin)) break;
	  if (lpsf(irow,jrow)>amin) {
	    inlobe = True;
	    // the sign on the ra can cause problems.  we just fit 
	    // for what the beam "looks" like here, and worry about 
	    // it later.
	    ix(npoints,0) = (irow-px)*abs(deltas(0));
	    ix(npoints,1) = (jrow-py)*abs(deltas(1));
	    iy(npoints) = lpsf(irow,jrow);
	    isigma(npoints) = 1.0;
	    npoints++;
	    if(npoints>maxnpoints) {
	      inlobe=False;
	      break;
	    }
	  }
	}
      }
    }
  }
  

  Vector<Double> y(npoints), sigma(npoints);
  Matrix<Double> x(npoints,2);
  
  for (Int ip=0;ip<npoints;ip++) {
    x(ip,0)=ix(ip,0); x(ip,1)=ix(ip,1);
    y(ip)=iy(ip);
    sigma(ip)=isigma(ip);
    if(!(isigma(ip)>0.0)) break;
  }
  
  // Construct the function to be fitted
  Gaussian2D<AutoDiff<Double> > gauss2d;
  gauss2d[0] = 1.0;
  gauss2d[1] = 0.0;
  gauss2d[2] = 0.0;
  gauss2d[3] = 2.5*abs(deltas(0));
  gauss2d[4] = 0.5;
  gauss2d[5] = 1.0;
  
  // Fix height and center
  gauss2d.mask(0) = False;
  gauss2d.mask(1) = False;
  gauss2d.mask(2) = False;
  
  NonLinearFitLM<Double> fitter;
  // Set maximum number of iterations to 1000
  fitter.setMaxIter(1000);
  
  // Set converge criteria.  Default is 0.001
  fitter.setCriteria(0.0001);
  
  // Set the function and initial values
  fitter.setFunction(gauss2d);
  
  // The current parameter values are used as the initial guess.
  solution = fitter.fit(x, y, sigma);
  converg=fitter.converged();
  if (!fitter.converged()) {
    beam(0)=2.5*abs(deltas(0))/C::arcsec;
    beam(1)=2.5*abs(deltas(0))/C::arcsec;
    beam(2)=0.0;

    //fit did not coverge...reduce the minimum i.e expand the field a bit
    amin/=1.5;
  }
  
 }
 if (converg) {
   if (abs(solution(4))>1.0) {
     beam(0)=abs(solution(3)*solution(4))/C::arcsec;
     beam(1)=abs(solution(3))/C::arcsec;
     beam(2)=solution(5)/C::degree-90.0;
   } else {
     beam(0)=abs(solution(3))/C::arcsec;
     beam(1)=abs(solution(3)*solution(4))/C::arcsec;
     beam(2)=solution(5)/C::degree;
   }
   
   while (abs(beam(2)/180.0)> 1) {
     if (beam(2) > 180.0) beam(2)-=360.0;
     else beam(2)+=360.0;
   }
   return True;
 }
 else os << LogIO::WARN << "The fit did not coverge; check your PSF" <<
	LogIO::POST; 
 return False;


 } catch (AipsError x_error) {
    beam(0)=2.5*abs(deltas(0))/C::arcsec;
    beam(1)=2.5*abs(deltas(0))/C::arcsec;
    beam(2)=0.0;
    os << LogIO::SEVERE << "Caught Exception: "<< x_error.getMesg() <<
      LogIO::POST;
    return False;
   } 

}

void StokesImageUtil::To(ImageInterface<Float>& out, ImageInterface<Complex>& in) {
  
  AlwaysAssert(in.shape()(0)==out.shape()(0), AipsError);
  AlwaysAssert(in.shape()(1)==out.shape()(1), AipsError);
  AlwaysAssert(in.shape()(3)==out.shape()(3), AipsError);
  
  Vector<Int> map;
  AlwaysAssert(StokesMap(map, in.coordinates()), AipsError);
  Vector<Int> outmap;
  AlwaysAssert(StokesMap(outmap, out.coordinates()), AipsError);
  
  Int nx = in.shape()(map(0));
  Int innpol = in.shape()(map(2));
  Int outnpol = out.shape()(outmap(2));
  
  // Loop over all planes
  LatticeStepper inls(in.shape(), IPosition(4, nx, 1, innpol, 1),
		      IPosition(4, map(0), map(2), map(1), map(3)));
  LatticeStepper outls(out.shape(), IPosition(4, nx, 1, outnpol, 1),
		       IPosition(4, map(0), map(2), map(1), map(3)));
  RO_LatticeIterator<Complex> inli(in,  inls);
  LatticeIterator<Float>  outli(out, outls);
  
  Vector<Int> inMap(innpol), outMap(outnpol);
  SkyModel::PolRep polFrame=SkyModel::CIRCULAR;
  Int nStokesIn=CStokesPolMap(inMap, polFrame, in.coordinates());
  Int nStokesOut=StokesPolMap(outMap, out.coordinates());
  AlwaysAssert(nStokesOut, AipsError);
  // Try taking real part only (uses LatticeExpr)
  if(nStokesIn==0) {
    nStokesIn=StokesPolMap(inMap, in.coordinates());
    if(nStokesIn==nStokesOut) {
      out.copyData(LatticeExpr<Float>(real(in)));
      return;
    }
    throw(AipsError("Illegal conversion in ToStokesImage"));
  }
  
  Vector<Complex> cs(4);
  cs=Complex(0.0);
  CStokesVector csv(0.0, 0.0, 0.0, 0.0);
  StokesVector sv(0.0, 0.0, 0.0, 0.0);
  Int pol;
  if(nStokesIn==1) {
    for (inli.reset(), outli.reset(); !inli.atEnd() && !outli.atEnd();
	 inli++,outli++) {
      for (Int ix=0;ix<nx;ix++) {
        cs(inMap(0))=inli.vectorCursor()(ix);
	csv=CStokesVector(cs(0), cs(1), cs(2), cs(3));
	if(polFrame==SkyModel::LINEAR) {
	  applySlinInv(sv, csv);
	}
	else {
	  applyScircInv(sv, csv);
	}
        if(nStokesOut==1) {
	  outli.rwVectorCursor()(ix)=sv(outMap(0));
	}
	else {
	  for (pol=0;pol<outnpol;pol++)
	    outli.rwMatrixCursor()(ix,pol)=sv(outMap(pol));
	}
      }
    }
  }
  else {
    for (inli.reset(), outli.reset(); !inli.atEnd() && !outli.atEnd(); inli++,outli++) {
      for (Int ix=0;ix<nx;ix++) {
	cs=Complex(0.0);
        for (pol=0;pol<innpol;pol++) cs(inMap(pol))=inli.matrixCursor()(ix,pol);
	csv=CStokesVector(cs(0), cs(1), cs(2), cs(3));
	if(polFrame==SkyModel::LINEAR) {
	  applySlinInv(sv, csv);
	}
	else {
	  applyScircInv(sv, csv);
	}
        if(nStokesOut==1) {
	  outli.rwVectorCursor()(ix)=sv(outMap(0));
	}
	else {
	  for (pol=0;pol<outnpol;pol++) {
	    outli.rwMatrixCursor()(ix,pol)=sv(outMap(pol));
	  }
	}
      }
    }
  }
};

void StokesImageUtil::ToStokesPSF(ImageInterface<Float>& out, ImageInterface<Complex>& in) {
  
  AlwaysAssert(in.shape()(0)==out.shape()(0), AipsError);
  AlwaysAssert(in.shape()(1)==out.shape()(1), AipsError);
  AlwaysAssert(out.shape()(2)==out.shape()(3), AipsError);
  AlwaysAssert(in.shape()(3)==out.shape()(4), AipsError);
  
  Vector<Int> map;
  AlwaysAssert(StokesMap(map, in.coordinates()), AipsError);
  Vector<Int> outmap;
  AlwaysAssert(StokesMap(outmap, out.coordinates()), AipsError);
  
  Int nx = in.shape()(map(0));
  Int innpol = in.shape()(map(2));
  Int outnpol = out.shape()(outmap(2));
  
  // Loop over all planes. For the input we get a line of complex pixels
  // for the output we put a line of float matrices
  LatticeStepper inls(in.shape(), IPosition(4, nx, 1, innpol, 1),
		      IPosition(4, map(0), map(2), map(1), map(3)));
  LatticeStepper outls(out.shape(), IPosition(5, nx, 1, outnpol, outnpol, 1),
		       IPosition(5, map(0), map(3), map(1), map(2), 4));
  RO_LatticeIterator<Complex> inli(in,  inls);
  LatticeIterator<Float>  outli(out, outls);
  
  Vector<Int> inMap(innpol), outMap(outnpol);
  SkyModel::PolRep polFrame;
  Int nStokesIn=CStokesPolMap(inMap, polFrame, in.coordinates());
  AlwaysAssert(nStokesIn, AipsError);
  Int nStokesOut=StokesPolMap(outMap, out.coordinates());
  AlwaysAssert(nStokesOut, AipsError);
  
  Matrix<Complex> s(4,4), sAdjoint(4,4);
  s=0;
  if(polFrame==SkyModel::LINEAR) {
    s(0,0)=Complex(0.5);
    s(0,1)=Complex(0.5);
    s(1,2)=Complex(0.5);
    s(1,3)=Complex(0.0,0.5);
    s(2,2)=Complex(0.5);
    s(2,3)=Complex(0.0,-0.5);
    s(3,0)=Complex(0.5);
    s(3,1)=Complex(-0.5);
  }
  else {
    s(0,0)=Complex(0.5);
    s(0,3)=Complex(0.5);
    s(1,1)=Complex(0.5);
    s(1,2)=Complex(0.0,0.5);
    s(2,1)=Complex(0.5);
    s(2,2)=Complex(0.0,-0.5);
    s(3,0)=Complex(0.5);
    s(3,3)=Complex(-0.5);
  }
  sAdjoint=adjoint(s);
  
  Matrix<Complex> lambda(4,4);
  lambda=Complex(0.0);
  
  Matrix<Float> stokesPSF(4,4);
  Int i, j;
  for (inli.reset(), outli.reset(); !inli.atEnd() && !outli.atEnd(); inli++,outli++) {
    for (Int ix=0;ix<nx;ix++) {
      // Get the channel
      // Fill in the weight vector
      if(innpol==1) {
	lambda(inMap(0),inMap(0))=inli.vectorCursor()(ix);
      }
      else {
	for (i=0;i<innpol;i++) lambda(inMap(i),inMap(i))=
				 inli.matrixCursor()(ix,i);
      }
      stokesPSF=real(product(s,product(lambda, sAdjoint)));
      if(nStokesOut==1) {
	outli.rwVectorCursor()(ix)=stokesPSF(outMap(0),outMap(0));
      }
      else {
	for (j=0;j<outnpol;j++) {
	  for (i=0;i<outnpol;i++) {
	    outli.rwCubeCursor()(ix,i,j)=stokesPSF(outMap(i),outMap(j));
	  }
	}
      }
    }
  }
};


void StokesImageUtil::From(ImageInterface<Complex>& out,
			   ImageInterface<Float>& in) {
  
  AlwaysAssert(in.shape()(0)==out.shape()(0), AipsError);
  AlwaysAssert(in.shape()(1)==out.shape()(1), AipsError);
  AlwaysAssert(in.shape()(3)==out.shape()(3), AipsError);
  
  
  Vector<Int> map;
  AlwaysAssert(StokesMap(map, in.coordinates()), AipsError);
  Vector<Int> outmap;
  AlwaysAssert(StokesMap(outmap, out.coordinates()), AipsError);
  
  Int nx = in.shape()(map(0));
  Int innpol = in.shape()(map(2));
  Int outnpol = out.shape()(outmap(2));
  
  Vector<Int> inMap(innpol), outMap(outnpol);
  SkyModel::PolRep polFrame=SkyModel::LINEAR;
  Int nStokesIn=StokesPolMap(inMap, in.coordinates());
  AlwaysAssert(nStokesIn, AipsError);
  Int nStokesOut=CStokesPolMap(outMap, polFrame, out.coordinates());
  if(nStokesOut==0) {
    nStokesOut=StokesPolMap(outMap, out.coordinates());
    if(nStokesIn==nStokesOut) {
      out.copyData(LatticeExpr<Complex>(toComplex(in)));
      return;
    }
    throw(AipsError("Illegal conversion in FromStokesImage"));
  }
  
  // Loop over all planes
  LatticeStepper inls(in.shape(), IPosition(4, nx, 1, innpol, 1),
		      IPosition(4, map(0), map(2), map(1), map(3)));
  LatticeStepper outls(out.shape(), IPosition(4, nx, 1, outnpol, 1),
		       IPosition(4, map(0), map(2), map(1), map(3)));
  RO_LatticeIterator<Float> inli(in,  inls);
  LatticeIterator<Complex>  outli(out, outls);
  
  Vector<Float> s(4);
  CStokesVector csv(0.0, 0.0, 0.0, 0.0);
  StokesVector sv(0.0, 0.0, 0.0, 0.0);
  Int pol;
  if(nStokesIn==1) {
    for (inli.reset(), outli.reset(); !inli.atEnd() && !outli.atEnd(); inli++,outli++) {
      for (Int ix=0;ix<nx;ix++) {
	s=0.0;
	s(inMap(0))=inli.vectorCursor()(ix);
	sv=StokesVector(s(0), s(1), s(2), s(3));
	if(polFrame==SkyModel::LINEAR) {
	  applySlin(csv, sv);
	}
	else {
	  applyScirc(csv, sv);
	}
        if(nStokesOut==1) {
	  outli.rwVectorCursor()(ix)=csv(outMap(0));
	}
	else {
	  for (pol=0;pol<outnpol;pol++) {
	    outli.rwMatrixCursor()(ix,pol)=csv(outMap(pol));
	  }
	}
      }
    }
  }
  else {
    for (inli.reset(), outli.reset(); !inli.atEnd() && !outli.atEnd(); inli++,outli++) {
      for (Int ix=0;ix<nx;ix++) {
	s=0.0;
	for(pol=0;pol<innpol;pol++) s(inMap(pol))=inli.matrixCursor()(ix,pol);
	sv=StokesVector(s(0), s(1), s(2), s(3));
	if(polFrame==SkyModel::LINEAR) {
	  applySlin(csv, sv);
	}
	else {
	  applyScirc(csv, sv);
	}
        if(nStokesOut==1) {
	  outli.rwVectorCursor()(ix)=csv(outMap(0));
	}
	else {
	  for (pol=0;pol<outnpol;pol++) {
	    outli.rwMatrixCursor()(ix,pol)=csv(outMap(pol));
	  }
	}
      }
    }
  }
};

// map(axis) is the polarization in the sequence I,Q,U,V
// This is the sequence used by StokesVector
Int StokesImageUtil::StokesPolMap(Vector<Int>& map, const CoordinateSystem& coord) {
  
  map=-1;
  Int stokesIndex;
  stokesIndex= coord.findCoordinate(Coordinate::STOKES);
  StokesCoordinate stokesCoord=coord.stokesCoordinate(stokesIndex);
  Int pol, p;
  Bool Found= stokesCoord.toPixel(p, Stokes::I)||
    stokesCoord.toPixel(p, Stokes::Q)||
    stokesCoord.toPixel(p, Stokes::U)||
    stokesCoord.toPixel(p, Stokes::V);
  if(Found) {
    pol=0;
    Int found=0;
    if(stokesCoord.toPixel(p, Stokes::I)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::Q)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::U)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::V)) {map(p)=pol;found++;} pol++;
    return found;
  }
  return 0;
}

// map(axis) is the polarization in the sequence XX,XY,YX,YY or RR,RL,LR,LL.
// This is the sequence used by CStokesVector
Int StokesImageUtil::CStokesPolMap(Vector<Int>& map, SkyModel::PolRep& polFrame,
				   const CoordinateSystem& coord) {
  
  map=-1;
  Int stokesIndex;
  stokesIndex=coord.findCoordinate(Coordinate::STOKES);
  StokesCoordinate stokesCoord=coord.stokesCoordinate(stokesIndex);
  Int pol, p;
  Int found=0;
  Bool Linear= stokesCoord.toPixel(p, Stokes::XX)||
    stokesCoord.toPixel(p, Stokes::XY)||
    stokesCoord.toPixel(p, Stokes::YX)||
    stokesCoord.toPixel(p, Stokes::YY);
  if(Linear) {
    pol=0;
    if(stokesCoord.toPixel(p, Stokes::XX)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::XY)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::YX)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::YY)) {map(p)=pol;found++;} pol++;
    polFrame=SkyModel::LINEAR;
    return found;
  }
  Bool Circular= stokesCoord.toPixel(p, Stokes::LL)||
    stokesCoord.toPixel(p, Stokes::LR)||
    stokesCoord.toPixel(p, Stokes::RL)||
    stokesCoord.toPixel(p, Stokes::RR);
  if(Circular) {
    pol=0;
    if(stokesCoord.toPixel(p, Stokes::RR)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::RL)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::LR)) {map(p)=pol;found++;} pol++;
    if(stokesCoord.toPixel(p, Stokes::LL)) {map(p)=pol;found++;} pol++;
    polFrame=SkyModel::CIRCULAR;
    return found;
  }
  return 0;
}

CoordinateSystem StokesImageUtil::StokesCoordFromMS(const IPosition& shape, Vector<Double>& deltas,
						    MeasurementSet& ms) {
  Vector<Int> whichStokes(0);
  return StokesCoordFromMS(shape, deltas, ms, whichStokes);
}

CoordinateSystem StokesImageUtil::StokesCoordFromMS(const IPosition& shape, Vector<Double>& deltas,
						    MeasurementSet& ms, Vector<Int>& whichStokes,
						    Bool doCStokes, Int fieldID, Int SPWID, Int feedID) {
  
  MSColumns msc(ms);
  
  Int nx=shape(0);
  Int ny=shape(1);
  Int npol=shape(2);
  Int nchan=shape(3);
  
  Vector<Double> refCoord=msc.field().phaseDir()(fieldID); 
  Vector<Double> refPixel(2); 
  refPixel(0)=Double(nx/2);
  refPixel(1)=Double(ny/2);
  
  Matrix<Double> xform(2,2);
  xform=0.0;xform.diagonal()=1.0;
  DirectionCoordinate myRaDec(MDirection::J2000,
			      Projection::SIN,
			      refCoord(0), refCoord(1),
			      deltas(0), deltas(1),
			      xform,
			      refPixel(0), refPixel(1));
  
  // Frequency
  Vector<Double> chanFreq=msc.spectralWindow().chanFreq()(SPWID); 
  if(nchan==0) nchan=chanFreq.shape()(SPWID);
  Double refChan=0.0;
  Vector<Double> freqResolution=msc.spectralWindow().resolution()(SPWID); 

  // Retrieve the first rest frequency used with this SPW_ID (for now)
  MSDopplerUtil msdoppler(ms);
  Vector<Double> restFreqArray;
  Double restFreq;
  if (msdoppler.dopplerInfo(restFreqArray,SPWID,fieldID)) {
    restFreq=restFreqArray(0);
  } else {
    restFreq=1.0;
  };
  
  SpectralCoordinate mySpectral(MFrequency::TOPO,  chanFreq(0),
				freqResolution(0), refChan,
				restFreq);
  
  // Polarization: If the specified whichStokes are ok, we use them
  // otherwise we guess
  if(Int(whichStokes.nelements())!=npol) {
    Vector<String> polType=msc.feed().polarizationType()(feedID);
    whichStokes.resize(npol);
    // Polarization
    if(doCStokes) {
      if (polType(0)=="X" || polType(0)=="Y") {
	switch(npol) {
	case 1:
	  whichStokes.resize(1);
	  whichStokes(0)=Stokes::I;
	  break;
	case 2:
	  whichStokes.resize(2);
	  whichStokes(0)=Stokes::XX;
	  whichStokes(1)=Stokes::YY;
	  break;
	default:
	  whichStokes.resize(4);
	  whichStokes(0)=Stokes::XX;
	  whichStokes(1)=Stokes::XY;
	  whichStokes(2)=Stokes::YX;
	  whichStokes(3)=Stokes::YY;
	}
      }
      else {
	switch(npol) {
	case 1:
	  whichStokes.resize(1);
	  whichStokes(0)=Stokes::I;
	  break;
	case 2:
	  whichStokes.resize(2);
	  whichStokes(0)=Stokes::LL;
	  whichStokes(1)=Stokes::RR;
	  break;
	default:
	  whichStokes.resize(4);
	  whichStokes(0)=Stokes::LL;
	  whichStokes(1)=Stokes::LR;
	  whichStokes(2)=Stokes::RL;
	  whichStokes(3)=Stokes::RR;
	};
      }
    }
    else {
      switch(npol) {
      case 1:
	whichStokes.resize(1);
	whichStokes(0)=Stokes::I;
	break;
      case 2:
	whichStokes.resize(2);
	whichStokes(0)=Stokes::I;
	if (polType(0)=="X" || polType(0)=="Y") {
	  whichStokes(1)=Stokes::Q;
	}
	else {
	  whichStokes(1)=Stokes::V;
	}
	break;
      case 3:
	whichStokes.resize(3);
	whichStokes(0)=Stokes::I;
	whichStokes(1)=Stokes::Q;
	whichStokes(2)=Stokes::U;
	break;
      case 4:
	whichStokes.resize(4);
	whichStokes(0)=Stokes::I;
	whichStokes(1)=Stokes::Q;
	whichStokes(2)=Stokes::U;
	whichStokes(3)=Stokes::V;
	break;
      };
    }
  }
  StokesCoordinate myStokes(whichStokes);
  
  // Now set up coordinates for image. If the shape is
  // 5 dimensional then we add another StokesCoordinate
  CoordinateSystem coordInfo; 
  coordInfo.addCoordinate(myRaDec);
  coordInfo.addCoordinate(myStokes);
  if(shape.nelements()==5) coordInfo.addCoordinate(myStokes);
  coordInfo.addCoordinate(mySpectral);
  return coordInfo;
  
}

CoordinateSystem StokesImageUtil::CStokesCoord(const IPosition& shape,
					       const CoordinateSystem& coord,
					       Vector<Int>& whichStokes,
					       SkyModel::PolRep polRep) {
  
  Int nx=shape(0);
  Int ny=shape(1);
  Int npol=shape(2);
  Int nchan=shape(3);
  AlwaysAssert(nx>0, AipsError);
  AlwaysAssert(ny>0, AipsError);
  AlwaysAssert(npol>0, AipsError);
  AlwaysAssert(nchan>0, AipsError);
  
  Int directionIndex=coord.findCoordinate(Coordinate::DIRECTION);
  DirectionCoordinate directionCoord=coord.directionCoordinate(directionIndex);
  
  Int spectralIndex=coord.findCoordinate(Coordinate::SPECTRAL);
  SpectralCoordinate spectralCoord=coord.spectralCoordinate(spectralIndex);
  
  // Polarization: If the specified whichStokes are ok, we use them
  // otherwise we guess
  if(Int(whichStokes.nelements())!=npol) {
    whichStokes.resize(npol);
    whichStokes=0;
    // Polarization
    if (polRep==SkyModel::LINEAR) {
      switch(npol) {
      case 1:
	whichStokes.resize(1);
	whichStokes(0)=Stokes::I;
	break;
      case 2:
	whichStokes.resize(2);
	whichStokes(1)=Stokes::XX;
	whichStokes(0)=Stokes::YY;
	break;
      default:
	whichStokes.resize(4);
	whichStokes(0)=Stokes::XX;
	whichStokes(1)=Stokes::XY;
	whichStokes(2)=Stokes::YX;
	whichStokes(3)=Stokes::YY;
        break;
      }
    }
    else {
      switch(npol) {
      case 1:
	whichStokes.resize(1);
	whichStokes(0)=Stokes::I;
	break;
      case 2:
	whichStokes.resize(2);
	whichStokes(0)=Stokes::LL;
	whichStokes(1)=Stokes::RR;
	break;
      default:
	whichStokes.resize(4);
	whichStokes(0)=Stokes::LL;
	whichStokes(1)=Stokes::LR;
	whichStokes(2)=Stokes::RL;
	whichStokes(3)=Stokes::RR;
	break;
      };
    }
  }
  AlwaysAssert(whichStokes.nelements(), AipsError);
  StokesCoordinate stokesCoord(whichStokes);
  
  // Now set up coordinates
  CoordinateSystem coordInfo; 
  coordInfo.addCoordinate(directionCoord);
  coordInfo.addCoordinate(stokesCoord);
  coordInfo.addCoordinate(spectralCoord);
  return coordInfo;

}


CoordinateSystem
StokesImageUtil::CStokesCoordFromImage(const ImageInterface<Complex>& image,
				       Vector<Int>& whichStokes,
				       SkyModel::PolRep polRep) {

  IPosition shape=image.shape();

  Int npol=shape(2);
  
  CoordinateSystem coord=image.coordinates();

  Int directionIndex=coord.findCoordinate(Coordinate::DIRECTION);
  DirectionCoordinate directionCoord=coord.directionCoordinate(directionIndex);

  Int spectralIndex=coord.findCoordinate(Coordinate::SPECTRAL);
  SpectralCoordinate spectralCoord=coord.spectralCoordinate(spectralIndex);

  // Polarization: If the specified whichStokes are ok, we use them
  // otherwise we guess
  if(Int(whichStokes.nelements())!=npol) {
    whichStokes.resize(npol);
    // Polarization
    if (polRep==SkyModel::LINEAR) {
      switch(npol) {
      case 1:
	whichStokes.resize(1);
	whichStokes(1)=Stokes::I;
	break;
      case 2:
	whichStokes.resize(2);
	whichStokes(1)=Stokes::XX;
	whichStokes(0)=Stokes::YY;
	break;
      default:
	whichStokes.resize(4);
	whichStokes(0)=Stokes::XX;
	whichStokes(1)=Stokes::XY;
	whichStokes(2)=Stokes::YX;
	whichStokes(3)=Stokes::YY;
      }
    }
    else {
      switch(npol) {
      case 1:
	whichStokes.resize(1);
	whichStokes(0)=Stokes::I;
	break;
      case 2:
	whichStokes.resize(2);
	whichStokes(0)=Stokes::LL;
	whichStokes(1)=Stokes::RR;
	break;
      default:
	whichStokes.resize(4);
	whichStokes(0)=Stokes::LL;
	whichStokes(1)=Stokes::LR;
	whichStokes(2)=Stokes::RL;
	whichStokes(3)=Stokes::RR;
      };
    }
  }
  StokesCoordinate stokesCoord(whichStokes);
  
  // Now set up coordinates for image.
  CoordinateSystem coordInfo; 
  coordInfo.addCoordinate(directionCoord);
  coordInfo.addCoordinate(stokesCoord);
  coordInfo.addCoordinate(spectralCoord);
  return coordInfo;

}

// Return a map to RA, DEC, pol, chan. 0 is RA, 1 is Dec, 2 is
// polarization, and 3 is Channel
Bool StokesImageUtil::StokesMap(Vector<Int>& map, const CoordinateSystem& coord) {
  
  map.resize(4);
  map=-1;

  Int dirIndex=coord.findCoordinate(Coordinate::DIRECTION);
  if(dirIndex>-1) {
    Vector<Int> dirAxes=coord.pixelAxes(dirIndex);
    if(dirAxes.nelements()>2) {
      return False;
    }
    map(0)=dirAxes(0);
    map(1)=dirAxes(1);
  }
  else {
    return False;
  }

  Int stokesIndex=coord.findCoordinate(Coordinate::STOKES);
  if(stokesIndex>-1) {
    Vector<Int> stokesAxes=coord.pixelAxes(stokesIndex);
    if(stokesAxes.nelements()>1) {
      return False;
    }
    map(2)=stokesAxes(0);
  }
  else {
    return False;
  }

  Int spectralIndex=coord.findCoordinate(Coordinate::SPECTRAL);
  if(spectralIndex>-1) {
    Vector<Int> spectralAxes=coord.pixelAxes(spectralIndex);
    if(spectralAxes.nelements()>1) {
      return False;
    }
    map(3)=spectralAxes(0);
  }
  else {
    return False;
  }

  return True;
}

void StokesImageUtil::BoxMask(ImageInterface<Float>& mask,
			      const IPosition& blc,
			      const IPosition& trc, const Float value) 
{
  LatticeStepper ls(mask.shape(), IPosition(4, mask.shape()(0), 1, 1, 1),
		    IPosition(4, 0, 1, 2, 3));
  ls.subSection(blc, trc);
  LatticeIterator<Float> mli(mask, ls);
  
  // Loop over all planes
  for (mli.reset();!mli.atEnd();mli++) {
    mli.rwCursor()=value;
  }
  
}

// Change the representation used. The contents of the image
// are not changed!
void StokesImageUtil::changeCStokesRep(ImageInterface<Complex>& image,
				       SkyModel::PolRep polRep) {

  CoordinateSystem coords=image.coordinates();

  Int stokesIndex=coords.findCoordinate(Coordinate::STOKES);
  StokesCoordinate
    stokesCoord=coords.stokesCoordinate(stokesIndex);

  Int npol=stokesCoord.stokes().nelements();

  Vector<Int> whichStokes(npol);

  AlwaysAssert((npol==1)||(npol==2)||(npol==4), AipsError);

  // Polarization
  if (polRep==SkyModel::LINEAR) {
    
    switch(npol) {
    case 1:
      whichStokes(0)=Stokes::I;
      break;
    case 2:
      whichStokes(1)=Stokes::XX;
      whichStokes(0)=Stokes::YY;
      break;
    default:
      whichStokes(0)=Stokes::XX;
      whichStokes(1)=Stokes::XY;
      whichStokes(2)=Stokes::YX;
      whichStokes(3)=Stokes::YY;
    }
  }
  else {
    switch(npol) {
    case 1:
      whichStokes(0)=Stokes::I;
      break;
    case 2:
      whichStokes(0)=Stokes::LL;
      whichStokes(1)=Stokes::RR;
      break;
    default:
      whichStokes(0)=Stokes::LL;
      whichStokes(1)=Stokes::LR;
      whichStokes(2)=Stokes::RL;
      whichStokes(3)=Stokes::RR;
    };
  }
  StokesCoordinate newStokesCoord(whichStokes);
  coords.replaceCoordinate(newStokesCoord, stokesIndex);
  image.setCoordinateInfo(coords);
}

Bool StokesImageUtil::
standardImageCoordinates(const ImageInterface<Complex>& image) {
  return (standardImageCoordinates(image.coordinates()));
};

Bool StokesImageUtil::
standardImageCoordinates(const ImageInterface<Float>& image) {
  return (standardImageCoordinates(image.coordinates()));
};

Bool StokesImageUtil::
standardImageCoordinates(const CoordinateSystem& coords) {
  Bool isStandard = True;
  {
    Int ind=coords.findCoordinate(Coordinate::DIRECTION);
    if (ind != 0) isStandard = False;
    ind=coords.findCoordinate(Coordinate::STOKES);
    if (ind != 1) isStandard = False;
    ind=coords.findCoordinate(Coordinate::SPECTRAL);
    if (ind != 2) isStandard = False;
  }
  return isStandard;
};

} //#End casa namespace
