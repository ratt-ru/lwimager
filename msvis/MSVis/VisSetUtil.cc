//# VisSetUtil.cc: VisSet Utilities
//# Copyright (C) 1996,1997,1998,1999,2001,2002
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
//# $Id$

#include <casa/aips.h>

#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/MatrixMath.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/Cube.h>
#include <casa/BasicMath/Math.h>
#include <casa/BasicSL/Complex.h>
#include <casa/BasicSL/Constants.h>
#include <casa/Utilities/Assert.h>

#include <ms/MeasurementSets/MSColumns.h>

#include <msvis/MSVis/VisSet.h>
#include <msvis/MSVis/VisBuffer.h>
#include <msvis/MSVis/VisSetUtil.h>

#include <casa/Quanta/UnitMap.h>
#include <casa/Quanta/UnitVal.h>
#include <measures/Measures/Stokes.h>
#include <casa/Quanta/MVAngle.h>

#include <casa/Logging/LogIO.h>

#include <casa/iostream.h>

namespace casa { //# NAMESPACE CASA - BEGIN

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
void VisSetUtil::WeightNatural(VisSet& vs, Double& sumwt) {
  VisIter& vi(vs.iter());
  VisSetUtil::WeightNatural(vi,sumwt);

}
void VisSetUtil::WeightNatural(VisibilityIterator& vi, Double& sumwt) {
  
  LogIO os(LogOrigin("VisSetUtil", "WeightNatural()", WHERE));
  
  
  VisBuffer vb(vi);
  
  sumwt=0.0;

  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    for (vi.origin();vi.more();vi++) {
      Int nRow=vb.nRow();
      Int nChan=vb.nChannel();
      for (Int row=0; row<nRow; row++) {
	for (Int chn=0; chn<nChan; chn++) {
	  if( !vb.flag()(chn,row) ) {
	    vb.imagingWeight()(chn,row)=vb.weight()(row);
	    sumwt+=vb.imagingWeight()(chn,row);
	  }
	  else {
	    vb.imagingWeight()(chn,row)=0.0;
	  }
	}
      }
      vi.setImagingWeight(vb.imagingWeight());
    }
  }
  if(sumwt<=0.0) {
    os << LogIO::WARN << "Sum of weights is not positive: check that some data is unflagged and that the WEIGHT column is positive" << LogIO::POST;
  }
}
void VisSetUtil::WeightUniform(VisSet& vs,
			       const String& rmode, const Quantity& noise,
			       const Double robust, const Int nx, const Int ny,
			       const Quantity& cellx, const Quantity& celly,
			       Double& sumwt,
			       const Int uBox, const Int vBox) {
  VisIter& vi(vs.iter());
  VisSetUtil::WeightUniform(vi,rmode, noise,robust,nx, ny, cellx,celly,
			    sumwt,uBox, vBox);

}
void VisSetUtil::WeightUniform(VisibilityIterator& vi,
			       const String& rmode, const Quantity& noise,
			       const Double robust, const Int nx, const Int ny,
			       const Quantity& cellx, const Quantity& celly,
			       Double& sumwt,
			       const Int uBox, const Int vBox) {
  
  LogIO os(LogOrigin("VisSetUtil", "WeightUniform()", WHERE));
  
  sumwt=0.0;

  
  VisBuffer vb(vi);
  
  Float uscale, vscale;
  Int uorigin, vorigin;
  Vector<Double> deltas;
  uscale=(nx*cellx.get("rad").getValue())/2.0;
  vscale=(ny*celly.get("rad").getValue())/2.0;
  uorigin=nx/2;
  vorigin=ny/2;
  
  // Simply declare a big matrix 
  Matrix<Float> gwt(nx,ny);
  gwt=0.0;
  
  Float u, v;
  sumwt=0.0;
  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    for (vi.origin();vi.more();vi++) {
      Int nRow=vb.nRow();
      Int nChan=vb.nChannel();
      for (Int row=0; row<nRow; row++) {
	for (Int chn=0; chn<nChan; chn++) {
	  if(!vb.flag()(chn,row)) {
	    Float f=vb.frequency()(chn)/C::c;
	    u=vb.uvw()(row)(0)*f; 
	    v=vb.uvw()(row)(1)*f;
	    Int ucell=Int(uscale*u+uorigin);
	    Int vcell=Int(vscale*v+vorigin);
	    if(((ucell-uBox)>0)&&((ucell+uBox)<nx)&&((vcell-vBox)>0)&&((vcell+vBox)<ny)) {
	      for (Int iv=-vBox;iv<=vBox;iv++) {
		for (Int iu=-uBox;iu<=uBox;iu++) {
		  gwt(ucell+iu,vcell+iv)+=vb.weight()(row);
		  sumwt+=vb.weight()(row);
		}
	      }
	    }
	    ucell=Int(-uscale*u+uorigin);
	    vcell=Int(-vscale*v+vorigin);
	    if(((ucell-uBox)>0)&&((ucell+uBox)<nx)&&((vcell-vBox)>0)&&((vcell+vBox)<ny)) {
	      for (Int iv=-vBox;iv<=vBox;iv++) {
		for (Int iu=-uBox;iu<=uBox;iu++) {
		  gwt(ucell+iu,vcell+iv)+=vb.weight()(row);
		  sumwt+=vb.weight()(row);
		}
	      }
	    }
	  }
	}
      }
    }
  }
  
  // We use the approximation that all statistical weights are equal to
  // calculate the average summed weights (over visibilities, not bins!)
  // This is simply to try an ensure that the normalization of the robustness
  // parameter is similar to that of the ungridded case, but it doesn't have
  // to be exact, since any given case will require some experimentation.
  
  Float f2, d2;
  if (rmode=="norm") {
    os << "Normal robustness, robust = " << robust << LogIO::POST;
    Double sumlocwt = 0.;
    for(Int vgrid=0;vgrid<ny;vgrid++) {
      for(Int ugrid=0;ugrid<nx;ugrid++) {
	if(gwt(ugrid, vgrid)>0.0) sumlocwt+=square(gwt(ugrid,vgrid));
      }
    }
    f2 = square(5.0*pow(10.0,Double(-robust))) / (sumlocwt / sumwt);
    d2 = 1.0;
  }
  else if (rmode=="abs") {
    os << "Absolute robustness, robust = " << robust << ", noise = "
       << noise.get("Jy").getValue() << "Jy" << LogIO::POST;
    f2 = square(robust);
    d2 = 2.0 * square(noise.get("Jy").getValue());
  }
  else {
    f2 = 1.0;
    d2 = 0.0;
  }
  
  Int ndrop=0;
  sumwt=0.0;
  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    for (vi.origin();vi.more();vi++) {
      for (Int row=0; row<vb.nRow(); row++) {
	for (Int chn=0; chn<vb.nChannel(); chn++) {
	  if (!vb.flag()(chn,row)) {
	    Float f=vb.frequency()(chn)/C::c;
	    u=vb.uvw()(row)(0)*f;
	    v=vb.uvw()(row)(1)*f;
	    Int ucell=Int(uscale*u+uorigin);
	    Int vcell=Int(vscale*v+vorigin);
	    vb.imagingWeight()(chn,row)=vb.weight()(row);
	    if((ucell>0)&&(ucell<nx)&&(vcell>0)&&(vcell<ny)) {
	      if(gwt(ucell,vcell)>0.0) {
		vb.imagingWeight()(chn,row)/=gwt(ucell,vcell)*f2+d2;
		sumwt+=vb.imagingWeight()(chn,row);
	      }
	    }
	    else {
	      vb.imagingWeight()(chn,row)=0.0;
	      ndrop++;
	    }
	  }
	}
      }
      vi.setImagingWeight(vb.imagingWeight());
    }
  }
  if(sumwt<=0.0) {
    os << LogIO::WARN << "Sum of weights is not positive: check that some data is unflagged and that the WEIGHT column is positive" << LogIO::POST;
  }
}
void VisSetUtil::WeightRadial(VisSet& vs, Double& sumwt) {
  VisIter& vi(vs.iter());
  VisSetUtil::WeightRadial(vi, sumwt);
}
void VisSetUtil::WeightRadial(VisibilityIterator& vi, Double& sumwt) {
  
  LogIO os(LogOrigin("VisSetUtil", "WeightRadial()", WHERE));
  
  sumwt=0.0;

  
  VisBuffer vb(vi);
  
  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    for (vi.origin();vi.more();vi++) {
      for (Int row=0; row<vb.nRow(); row++) {
	for (Int chn=0; chn<vb.nChannel(); chn++) {
	  Float f=vb.frequency()(chn)/C::c;
	  if( !vb.flag()(chn,row) ) {
	    vb.imagingWeight()(chn,row)=
	      f*sqrt(square(vb.uvw()(row)(0))+square(vb.uvw()(row)(1)))
	      * vb.weight()(row);
	    sumwt+=vb.imagingWeight()(chn,row);
	  }
	  else {
	    vb.imagingWeight()(chn,row)=0.0;
	  }
	}
      }
      vi.setImagingWeight(vb.imagingWeight());
    }
  }
  if(sumwt<=0.0) {
    os << LogIO::WARN << "Sum of weights is not positive: check that some data is unflagged and that the WEIGHT column is positive" << LogIO::POST;
  }
}

// Filter the MeasurementSet
void VisSetUtil::Filter(VisSet& vs, const String& type, const Quantity& bmaj,
			const Quantity& bmin, const Quantity& bpa,
			Double& sumwt, Double& minfilter, Double& maxfilter)
{
  VisIter& vi(vs.iter());
  VisSetUtil::Filter(vi, type,bmaj,bmin, bpa, sumwt, minfilter, maxfilter);
}

void VisSetUtil::Filter(VisibilityIterator& vi, const String& type, const Quantity& bmaj,
			const Quantity& bmin, const Quantity& bpa,
			Double& sumwt, Double& minfilter, Double& maxfilter)
{

  LogIO os(LogOrigin("VisSetUtil", "filter()", WHERE));
  
  sumwt=0.0;
  maxfilter=0.0;
  minfilter=1.0;
  
  
  VisBuffer vb(vi);

  if (type=="gaussian") {
    
    Bool lambdafilt=False;
    Double rbmaj;
    Double rbmin;
    
    if( bmaj.getUnit().contains("lambda"))
      lambdafilt=True;
    if(lambdafilt){
      os << "Filtering for Gaussian of shape: " 
	 << bmaj.get("klambda").getValue() << " by " 
	 << bmin.get("klambda").getValue() << " (klambda) at p.a. "
	 << bpa.get("deg").getValue() << " (degrees)" << LogIO::POST;
      rbmaj=log(2.0)/square(bmaj.get("lambda").getValue());
      rbmin=log(2.0)/square(bmin.get("lambda").getValue());
    }
    else{
      os << "Filtering for Gaussian of shape: " 
	 << bmaj.get("arcsec").getValue() << " by " 
	 << bmin.get("arcsec").getValue() << " (arcsec) at p.a. "
	 << bpa.get("deg").getValue() << " (degrees)" << LogIO::POST;
    
      // Convert to values that we can use
      Double fact = 4.0*log(2.0);
      rbmaj = fact*square(bmaj.get("rad").getValue());
      rbmin = fact*square(bmin.get("rad").getValue());
    }
    Double rbpa  = MVAngle(bpa).get("rad").getValue();
    Double cospa = sin(rbpa);
    Double sinpa = cos(rbpa);
    
    // Now iterate through the data
    for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
      for (vi.origin();vi.more();vi++) {
	Int nRow=vb.nRow();
	Int nChan=vb.nChannel();
	for (Int row=0; row<nRow; row++) {
	  for (Int chn=0; chn<nChan; chn++) {
	    Double invLambdaC=vb.frequency()(chn)/C::c;
	    Double& u = vb.uvw()(row)(0);
	    Double& v = vb.uvw()(row)(1);
	    if(!vb.flag()(chn,row)&&vb.weight()(row)>0.0) {
	      Double ru = invLambdaC*(  cospa * u + sinpa * v);
	      Double rv = invLambdaC*(- sinpa * u + cospa * v);
	      Double filter = exp(-rbmaj*square(ru) - rbmin*square(rv));
	      vb.imagingWeight()(chn,row)*=filter;
	      if(filter>maxfilter) maxfilter=filter;
	      if(filter<minfilter) minfilter=filter;
	      sumwt+=vb.imagingWeight()(chn,row);
	    }
	    else {
	      vb.imagingWeight()(chn,row)=0.0;
	    }
	  }
	}
	vi.setImagingWeight(vb.imagingWeight());
      }
    }
  }
  else {
    os << "Unknown filtering " << type << LogIO::EXCEPTION;    
  }
  
}


// Implement a uv range

void VisSetUtil::UVRange(VisSet &vs, const Double& uvmin, const Double& uvmax,
			 Double& sumwt)
{
  VisIter& vi(vs.iter());
  VisSetUtil::UVRange(vi, uvmin, uvmax,sumwt);
}
void VisSetUtil::UVRange(VisIter& vi, const Double& uvmin, const Double& uvmax,
			 Double& sumwt)
{
  LogIO os(LogOrigin("VisSetUtil", "UVRange()", WHERE));
  
  sumwt=0.0;
 
  VisBuffer vb(vi);

  if(uvmax<uvmin||uvmin<0.0) {
    os << "Invalid uvmin and uvmax: " << uvmin << ", " << uvmax
       << LogIO::EXCEPTION;
  }

  // Now iterate through the data
  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    for (vi.origin();vi.more();vi++) {
      Int nRow=vb.nRow();
      Int nChan=vb.nChannel();
      for (Int row=0; row<nRow; row++) {
	Double& u = vb.uvw()(row)(0);
	Double& v = vb.uvw()(row)(1);
	Double radius=sqrt(square(u)+square(v));
	for (Int chn=0; chn<nChan; chn++) {
	  if(!vb.flag()(chn,row)) {
	    Double radiusL=radius*vb.frequency()(chn)/C::c;
	    if(radiusL>uvmax||radiusL<uvmin) {
	      vb.imagingWeight()(chn,row)=0.0;
	    }
	  }
	}
      }
      sumwt+=sum(vb.imagingWeight());
      vi.setImagingWeight(vb.imagingWeight());
    }
  }
}

// Calculate sensitivity
void VisSetUtil::Sensitivity(VisSet &vs, Quantity& pointsourcesens, 
			     Double& relativesens, Double& sumwt)
{
  ROVisIter& vi(vs.iter());
   VisSetUtil::Sensitivity(vi, pointsourcesens, relativesens,sumwt);

}
void VisSetUtil::Sensitivity(ROVisIter &vi, Quantity& pointsourcesens, 
			     Double& relativesens, Double& sumwt)
{
  LogIO os(LogOrigin("VisSetUtil", "Sensitivity()", WHERE));
  
  sumwt=0.0;
  Double sumwtsq=0.0;
  Double sumInverseVariance=0.0;
  
  VisBuffer vb(vi);

  // Now iterate through the data
  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    for (vi.origin();vi.more();vi++) {
      Int nRow=vb.nRow();
      Int nChan=vb.nChannel();
      for (Int row=0; row<nRow; row++) {
	// TBD: Should probably use weight() here, which updates with calibration
        Double variance=square(vb.sigma()(row));
	for (Int chn=0; chn<nChan; chn++) {
	  if(!vb.flag()(chn,row)&&variance>0.0) {
	    sumwt+=vb.imagingWeight()(chn,row);
	    sumwtsq+=square(vb.imagingWeight()(chn,row))*variance;
	    sumInverseVariance+=1.0/variance;
	  }
	}
      }
    }
  }

  if(sumwt==0.0) {
    os << "Cannot calculate sensitivity: sum of weights is zero" << endl
       << "Perhaps you need to weight the data" << LogIO::EXCEPTION;
  }
  if(sumInverseVariance==0.0) {
    os << "Cannot calculate sensitivity: sum of inverse variances is zero" << endl
       << "Perhaps you need to weight the data" << LogIO::EXCEPTION;
  }

  Double naturalsens=1.0/sqrt(sumInverseVariance);
  pointsourcesens=Quantity(sqrt(sumwtsq)/sumwt, "Jy");
  relativesens=sqrt(sumwtsq)/sumwt/naturalsens;
}
void VisSetUtil::HanningSmooth(VisSet &vs)
{
  VisIter& vi(vs.iter());
  VisSetUtil::HanningSmooth(vi);
}
void VisSetUtil::HanningSmooth(VisIter &vi)
{
  LogIO os(LogOrigin("VisSetUtil", "HanningSmooth()"));

  
  VisBuffer vb(vi);

  Int row, chn, pol;

  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    if (vi.existsWeightSpectrum()) {
      for (vi.origin();vi.more();vi++) {
	Cube<Complex>& vc= vb.correctedVisCube();
	Cube<Bool>& fc= vb.flagCube();
	Cube<Float>& wc= vb.weightSpectrum();

	Int nRow=vb.nRow();
	Int nChan=vb.nChannel();
	if (nChan < 3) break;
	Int nPol=vi.visibilityShape()(0);
	Cube<Complex> smoothedData(nPol,nChan,nRow);
	Cube<Bool> newFlag(nPol,nChan,nRow);
	Cube<Float> newWeight(nPol,nChan,nRow);
	for (row=0; row<nRow; row++) {
	  for (pol=0; pol<nPol; pol++) {
	    ///Handle first channel and flag it
	    smoothedData(pol,0,row) = vc(pol,0,row)*0.5 + vc(pol,1,row)*0.5;
	    newWeight(pol,0,row) = 0.0;
	    newFlag(pol,0,row) = True;
	    for (chn=1; chn<nChan-1; chn++) {
	      smoothedData(pol,chn,row) =
		vc(pol,chn-1,row)*0.25 + vc(pol,chn,row)*0.50 +
		vc(pol,chn+1,row)*0.25;
	      if (wc(pol,chn-1,row) != 0 && wc(pol,chn,row) != 0
		  && wc(pol,chn+1,row) != 0) {
		newWeight(pol,chn,row) = 1.0 /
		  (1.0/(wc(pol,chn-1,row)*16.0) + 1.0/(wc(pol,chn,row)*4.0)
		   + 1.0/(wc(pol,chn+1,row)*16.0));
	      } else {
		newWeight(pol,chn,row) = 0.0;
	      }
	      newFlag(pol,chn,row) =
		fc(pol,chn-1,row)||fc(pol,chn,row)||fc(pol,chn+1,row);
	    }
	    //Handle last channel and flag it
	    smoothedData(pol,nChan-1,row) =
	      vc(pol,nChan-2,row)*0.5+vc(pol,nChan-1,row)*0.5;
	    newWeight(pol,nChan-1,row) = 0.0;
	    newFlag(pol,nChan-1,row) = True;  // flag last channel
	  }
	}
	vi.setVisAndFlag(smoothedData,newFlag,VisibilityIterator::Corrected);
	vi.setWeightSpectrum(newWeight);
      }
    } else {
      for (vi.origin();vi.more();vi++) {
	Cube<Complex>& vc= vb.correctedVisCube();
	Cube<Bool>& fc= vb.flagCube();
	Matrix<Float>& wm = vb.weightMat();

	Int nRow=vb.nRow();
	Int nChan=vb.nChannel();
	if (nChan < 3) break;
	Int nPol=vi.visibilityShape()(0);
	Cube<Complex> smoothedData(nPol,nChan,nRow);
	Cube<Bool> newFlag(nPol,nChan,nRow);
	Matrix<Float> newWeight(nPol, nRow);
	for (row=0; row<nRow; row++) {
	  for (pol=0; pol<nPol; pol++) {
	    ///Handle first channel and flag it
	    smoothedData(pol,0,row) = vc(pol,0,row)*0.5 + vc(pol,1,row)*0.5;
	    newFlag(pol,0,row) = True;
	    ///Handle chan-independent weights
	    newWeight(pol, row) = 8.0*wm(pol, row)/3.0;
	    for (chn=1; chn<nChan-1; chn++) {
	      smoothedData(pol,chn,row) =
		vc(pol,chn-1,row)*0.25 + vc(pol,chn,row)*0.50 +
		vc(pol,chn+1,row)*0.25;
	      newFlag(pol,chn,row) =
		fc(pol,chn-1,row)||fc(pol,chn,row)||fc(pol,chn+1,row);
	    }
	    //Handle last channel and flag it
	    smoothedData(pol,nChan-1,row) =
	      vc(pol,nChan-2,row)*0.5+vc(pol,nChan-1,row)*0.5;
	    newFlag(pol,nChan-1,row) = True;  // flag last channel
	  }
	}
	vi.setVisAndFlag(smoothedData,newFlag,VisibilityIterator::Corrected);
	vi.setWeightMat(newWeight);
      }
    }
  }
}
void VisSetUtil::UVSub(VisSet &vs, Bool reverse)
{
  VisIter& vi(vs.iter());
  VisSetUtil::UVSub(vi, reverse);
}
void VisSetUtil::UVSub(VisIter &vi, Bool reverse)
{
  LogIO os(LogOrigin("VisSetUtil", "UVSub()"));


  VisBuffer vb(vi);

  Int row, chn, pol;

  for (vi.originChunks();vi.moreChunks();vi.nextChunk()) {
    for (vi.origin();vi.more();vi++) {

      Cube<Complex>& vc= vb.correctedVisCube();
      Cube<Complex>& mc= vb.modelVisCube();

      Int nRow=vb.nRow();
      Int nChan=vb.nChannel();
      Int nPol=vi.visibilityShape()(0);
      Cube<Complex> residualData(nPol,nChan,nRow);
      if (reverse) {
	for (row=0; row<nRow; row++) {
	  for (pol=0; pol<nPol; pol++) {
	    for (chn=0; chn<nChan; chn++) {
	      residualData(pol,chn,row) = vc(pol,chn,row)+mc(pol,chn ,row);
	    }
	  }
	}
      } else {
	for (row=0; row<nRow; row++) {
	  for (pol=0; pol<nPol; pol++) {
	    for (chn=0; chn<nChan; chn++) {
	      residualData(pol,chn,row) = vc(pol,chn,row)-mc(pol,chn ,row);
	    }
	  }
	}
      }
      vi.setVis(residualData,VisibilityIterator::Corrected);
    }
  }
}

} //# NAMESPACE CASA - END

