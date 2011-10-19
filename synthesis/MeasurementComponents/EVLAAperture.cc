// -*- C++ -*-
//# EVLAAperture.cc: Implementation of the EVLAAperture class
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
//# $Id$
//
#include <synthesis/MeasurementComponents/Utils.h>
#include <synthesis/MeasurementComponents/EVLAAperture.h>
#include <synthesis/MeasurementComponents/SynthesisError.h>
#include <synthesis/MeasurementComponents/BeamCalc.h>
#include <synthesis/MeasurementComponents/WTerm.h>
#include <synthesis/MeasurementComponents/VLACalcIlluminationConvFunc.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/SpectralCoordinate.h>
#include <coordinates/Coordinates/StokesCoordinate.h>
//
//---------------------------------------------------------------------
//---------------------------------------------------------------------
// TEMPS
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
// #define CONVSIZE (1024*2)
// #define CONVWTSIZEFACTOR 1.0
// #define OVERSAMPLING 20
// #define THRESHOLD 1E-4

namespace casa{
  
  EVLAAperture& EVLAAperture::operator=(const EVLAAperture& other)
  {
    if(this!=&other) 
      {
	//	ConvolutionFunction::operator=(other);
	logIO_p = other.logIO_p;
	//	setParams(other.polMap_p_base, other.feedStokes_p);
	setPolMap(other.polMap_p_base);
	Diameter_p=other.Diameter_p;
	Nant_p=other.Nant_p;
	HPBW=other.HPBW;
	sigma=other.sigma;
      }
    return *this;
  }
  Int EVLAAperture::getVLABandID(Double& freq,String&telescopeName, const CoordinateSystem& skyCoord)
  {
    if (telescopeName=="VLA")
      {
	if      ((freq >=1.34E9) && (freq <=1.73E9))   return BeamCalc_VLA_L;
	else if ((freq >=4.5E9) && (freq <=5.0E9))     return BeamCalc_VLA_C;
	else if ((freq >=8.0E9) && (freq <=8.8E9))     return BeamCalc_VLA_X;
	else if ((freq >=14.4E9) && (freq <=15.4E9))   return BeamCalc_VLA_U;
	else if ((freq >=22.0E9) && (freq <=24.0E9))   return BeamCalc_VLA_K;
	else if ((freq >=40.0E9) && (freq <=50.0E9))   return BeamCalc_VLA_Q;
	else if ((freq >=100E6) && (freq <=300E6))     return BeamCalc_VLA_4;
      }
    else 
      if (telescopeName=="EVLA")
	{
	  Double refFreq = skyCoord.spectralCoordinate(skyCoord.findCoordinate(Coordinate::SPECTRAL)).referenceValue()(0);

	  if      ((refFreq >= 1.0E9) && (refFreq <= 2.0E9)) return BeamCalc_EVLA_L;
	  else if ((freq >=2.0E9) && (freq <=4.0E9))         return BeamCalc_EVLA_S;
	  else if ((freq >=4.0E9) && (freq <=8.0E9))         return BeamCalc_EVLA_C;
	  else if ((freq >=8.0E9) && (freq <=12.0E9))        return BeamCalc_EVLA_X;
	  else if ((freq >=12.0E9) && (freq <=18.0E9))       return BeamCalc_EVLA_U;
	  else if ((freq >=18.0E9) && (freq <=26.5E9))       return BeamCalc_EVLA_K;
	  else if ((freq >=26.5E9) && (freq <=40.8E9))       return BeamCalc_EVLA_A;
	  else if ((freq >=40.0E9) && (freq <=50.0E9))       return BeamCalc_EVLA_Q;
	}
    ostringstream mesg;
    mesg << telescopeName << "/" << freq << "(Hz) combination not recognized.";
    throw(SynthesisError(mesg.str()));
  }
  
  int EVLAAperture::getVisParams(const VisBuffer& vb,const CoordinateSystem& im)
  {
    Double Freq;

    Vector<String> telescopeNames=vb.msColumns().observation().telescopeName().getColumn();
    for(uInt nt=0;nt<telescopeNames.nelements();nt++)
      {
	if ((telescopeNames(nt) != "VLA") && (telescopeNames(nt) != "EVLA"))
	  {
	    String mesg="We can handle only (E)VLA antennas for now.\n";
	    mesg += "Erroneous telescope name = " + telescopeNames(nt) + ".";
	    SynthesisError err(mesg);
	    throw(err);
	  }
	if (telescopeNames(nt) != telescopeNames(0))
	  {
	    String mesg="We do not (yet) handle inhomogeneous arrays for A-Projection!\n";
	    mesg += "Not yet a \"priority\"!!";
	    SynthesisError err(mesg);
	    throw(err);
	  }
      }
    //    ROMSSpWindowColumns mssp(vb.msColumns().spectralWindow());
    Freq = vb.msColumns().spectralWindow().refFrequency()(0);
    Diameter_p=0;
    Nant_p     = vb.msColumns().antenna().nrow();
    for (Int i=0; i < Nant_p; i++)
      if (!vb.msColumns().antenna().flagRow()(i))
	{
	  Diameter_p = vb.msColumns().antenna().dishDiameter()(i);
	  break;
	}
    if (Diameter_p == 0)
      {
	logIO() << LogOrigin("EVLAAperture", "getVisParams")
		<< "No valid or finite sized antenna found in the antenna table. "
		<< "Assuming diameter = 25m."
		<< LogIO::WARN
		<< LogIO::POST;
	Diameter_p=25.0;
      }
    
    Double Lambda=C::c/Freq;
    HPBW = Lambda/(Diameter_p*sqrt(log(2.0)));
    sigma = 1.0/(HPBW*HPBW);
    //    awEij.setSigma(sigma);
    Int bandID = getVLABandID(Freq,telescopeNames(0),im);
    return bandID;
  }
  
  Int EVLAAperture::makePBPolnCoords(const VisBuffer&vb,
				     const Int& convSize,
				     const Int& convSampling,
				     const CoordinateSystem& skyCoord,
				     const Int& skyNx, const Int& skyNy,
				     CoordinateSystem& feedCoord)
  //				     Vector<Int>& cfStokes)
  {
    feedCoord = skyCoord;
    //
    // Make a two dimensional image to calculate auto-correlation of
    // the ideal illumination pattern. We want this on a fine grid in
    // the UV plane
    //
    Int directionIndex=skyCoord.findCoordinate(Coordinate::DIRECTION);
    AlwaysAssert(directionIndex>=0, AipsError);
    DirectionCoordinate dc=skyCoord.directionCoordinate(directionIndex);
    Vector<Double> sampling;
    sampling = dc.increment();
    sampling*=Double(convSampling);
    sampling*=Double(skyNx)/Double(convSize);
    dc.setIncrement(sampling);
    
    
    Vector<Double> unitVec(2);
    unitVec=convSize/2;
    dc.setReferencePixel(unitVec);
    
    // Set the reference value to that of the image
    feedCoord.replaceCoordinate(dc, directionIndex);

    //
    // Make an image with circular polarization axis.
    //
    Int NPol=0,M,N=0;
    M=polMap_p_base.nelements();
    for(Int i=0;i<M;i++) if (polMap_p_base(i) > -1) NPol++;
    Vector<Int> poln(NPol);
    
    Int index;
    Vector<Int> inStokes;
    index = feedCoord.findCoordinate(Coordinate::STOKES);
    inStokes = feedCoord.stokesCoordinate(index).stokes();
    N = 0;
    try
      {
	for(Int i=0;i<M;i++) if (polMap_p_base(i) > -1) {poln(N) = vb.corrType()(i);N++;}
	StokesCoordinate polnCoord(poln);
	Int StokesIndex = feedCoord.findCoordinate(Coordinate::STOKES);
	feedCoord.replaceCoordinate(polnCoord,StokesIndex);
	//	cfStokes = poln;
      }
    catch(AipsError& x)
      {
	throw(SynthesisFTMachineError("Likely cause: Discrepancy between the poln. "
				      "axis of the data and the image specifications."));
      }
    
    return NPol;
  }
  
  Bool EVLAAperture::findSupport(Array<Complex>& func, Float& threshold,Int& origin, Int& R)
  {
    Double NSteps;
    Int PixInc=1;
    Vector<Complex> vals;
    IPosition ndx(4,origin,0,0,0);
    Bool found=False;
    IPosition cfShape=func.shape();
    Int convSize = cfShape(0);
    for(R=convSize/4;R>1;R--)
      {
	NSteps = 90*R/PixInc; //Check every PixInc pixel along a
	//circle of radious R
	vals.resize((Int)(NSteps+0.5));
	vals=0;
	for(Int th=0;th<NSteps;th++)
	  {
	    ndx(0)=(int)(origin + R*sin(2.0*M_PI*th*PixInc/R));
	    ndx(1)=(int)(origin + R*cos(2.0*M_PI*th*PixInc/R));
	    
	    if ((ndx(0) < cfShape(0)) && (ndx(1) < cfShape(1)))
	      vals(th)=func(ndx);
	  }
	if (max(abs(vals)) > threshold)
	  {found=True;break;}
      }
    return found;
  }
  
  void EVLAAperture::applySky(ImageInterface<Complex>& outImages,
			      const VisBuffer& vb, 
			      const Bool doSquint,
			      const Int& cfKey)
  {
    VLACalcIlluminationConvFunc vlaPB;
    Long cachesize=(HostInfo::memoryTotal(true)/8)*1024;
    vlaPB.setMaximumCacheSize(cachesize);
    Int bandID=getVisParams(vb,outImages.coordinates());
    vlaPB.applyPB(outImages, vb, bandID, doSquint);
  }

  void EVLAAperture::applySky(ImageInterface<Float>& outImages,
			      const VisBuffer& vb, 
			      const Bool doSquint,
			      const Int& cfKey)
  {
    VLACalcIlluminationConvFunc vlaPB;
    Long cachesize=(HostInfo::memoryTotal(true)/8)*1024;
    vlaPB.setMaximumCacheSize(cachesize);
    Int bandID=getVisParams(vb,outImages.coordinates());
    vlaPB.applyPB(outImages, vb, bandID, doSquint);
  }
  
};
