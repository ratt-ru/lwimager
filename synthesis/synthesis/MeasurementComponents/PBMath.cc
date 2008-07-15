//# PBMath.cc: Implementation for PBMath
//# Copyright (C) 1996,1997,1998,1999,2000,2002,2003
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
//# $Id: PBMath.cc,v 19.13 2006/04/20 03:19:29 mvoronko Exp $

#include <casa/aips.h>
#include <casa/BasicSL/Complex.h>
#include <synthesis/MeasurementComponents/PBMath.h>
#include <synthesis/MeasurementComponents/PBMath1DAiry.h>
#include <synthesis/MeasurementComponents/PBMath1DCosPoly.h>
#include <synthesis/MeasurementComponents/PBMath1DGauss.h>
#include <synthesis/MeasurementComponents/PBMath1DIPoly.h>
#include <synthesis/MeasurementComponents/PBMath1DNumeric.h>
#include <synthesis/MeasurementComponents/PBMath1DPoly.h>


#include <measures/Measures/Stokes.h>
#include <casa/BasicSL/Constants.h>

#include <components/ComponentModels/Flux.h>
#include <components/ComponentModels/ComponentShape.h>

#include <images/Images/PagedImage.h>
#include <images/Images/TempImage.h>
#include <images/Images/ImageInterface.h>
#include <images/Images/ImageRegion.h>

#include <lattices/Lattices/LatticeStepper.h>
#include <lattices/Lattices/LatticeIterator.h>
#include <lattices/Lattices/LatticeExpr.h>

#include <coordinates/Coordinates/CoordinateSystem.h>
#include <coordinates/Coordinates/DirectionCoordinate.h>
#include <coordinates/Coordinates/SpectralCoordinate.h>
#include <coordinates/Coordinates/StokesCoordinate.h>
#include <coordinates/Coordinates/Projection.h>
#include <casa/Logging/LogIO.h>

#include <casa/Utilities/Assert.h>
#include <casa/Containers/RecordInterface.h>
#include <casa/Quanta/QuantumHolder.h>
#include <measures/Measures/MeasureHolder.h>

namespace casa { //# NAMESPACE CASA - BEGIN

PBMath::PBMath()
{
  pb_pointer_p = 0;
};

PBMath::PBMath(const RecordInterface& rec)
{
  String name;
  const Int nameFieldNumber=rec.fieldNumber("name");
  if (nameFieldNumber!=-1)
      name = rec.asString(nameFieldNumber);

  if (name == "COMMONPB") {
    String commonpb;
    PBMath::CommonPB commonpbEnum;
    commonpb = rec.asString (rec.fieldNumber("commonpb"));
    PBMath::enumerateCommonPB(commonpb, commonpbEnum);
    PBMath  aPBMath(commonpbEnum);
    *this = aPBMath;
  } else if (name == "AIRY") {


    Quantity dishdiam;
    getQuantity(rec, "dishdiam", dishdiam);
    Quantity blockagediam;
    getQuantity(rec, "blockagediam", blockagediam);
    Quantity reffreq;
    getQuantity(rec, "reffreq", reffreq);
    Quantity maxrad;
    getQuantity(rec, "maxrad", maxrad);
    MDirection squintdir;
    getMDirection(rec, "squintdir", squintdir);
    Quantity squintreffreq;
    getQuantity(rec, "squintreffreq", squintreffreq);
    Bool usesymmetricbeam;
    usesymmetricbeam=rec.asBool( rec.fieldNumber("usesymmetricbeam"));

    pb_pointer_p = new PBMath1DAiry( dishdiam, blockagediam,
				     maxrad, reffreq,
				     BeamSquint(squintdir, squintreffreq),
				     usesymmetricbeam);

  } else if (name == "GAUSS") {


    Quantity halfwidth;
    getQuantity(rec, "halfwidth", halfwidth);
    Quantity reffreq;
    getQuantity(rec, "reffreq", reffreq);
    Quantity maxrad;
    getQuantity(rec, "maxrad", maxrad);
    MDirection squintdir;
    getMDirection(rec, "squintdir", squintdir);
    Quantity squintreffreq;
    getQuantity(rec, "squintreffreq", squintreffreq);
    Bool usesymmetricbeam;
    usesymmetricbeam=rec.asBool( rec.fieldNumber("usesymmetricbeam"));
    Bool isthisvp;
    isthisvp=rec.asBool( rec.fieldNumber("isthisvp"));

    pb_pointer_p = new PBMath1DGauss( halfwidth, maxrad, reffreq,
				      isthisvp,
				      BeamSquint(squintdir, squintreffreq),
				      usesymmetricbeam);

  } else if (name == "POLY") {


    Vector<Double> coeff;
    coeff=rec.asArrayDouble( rec.fieldNumber("coeff"));
    Quantity reffreq;
    getQuantity(rec, "reffreq", reffreq);
    Quantity maxrad;
    getQuantity(rec, "maxrad", maxrad);
    MDirection squintdir;
    getMDirection(rec, "squintdir", squintdir);
    Quantity squintreffreq;
    getQuantity(rec, "squintreffreq", squintreffreq);
    Bool usesymmetricbeam;
    usesymmetricbeam=rec.asBool( rec.fieldNumber("usesymmetricbeam"));
    Bool isthisvp;
    isthisvp=rec.asBool( rec.fieldNumber("isthisvp"));

    pb_pointer_p = new PBMath1DPoly( coeff, maxrad, reffreq,
				      isthisvp,
				      BeamSquint(squintdir, squintreffreq),
				      usesymmetricbeam);


  } else if (name == "IPOLY") {


    Vector<Double> coeff;
    coeff=rec.asArrayDouble( rec.fieldNumber("coeff"));
    Quantity reffreq;
    getQuantity(rec, "reffreq", reffreq);
    Quantity maxrad;
    getQuantity(rec, "maxrad", maxrad);
    MDirection squintdir;
    getMDirection(rec, "squintdir", squintdir);
    Quantity squintreffreq;
    getQuantity(rec, "squintreffreq", squintreffreq);
    Bool usesymmetricbeam;
    usesymmetricbeam=rec.asBool( rec.fieldNumber("usesymmetricbeam"));
    Bool isthisvp;
    isthisvp=rec.asBool( rec.fieldNumber("isthisvp"));

    pb_pointer_p = new PBMath1DIPoly( coeff, maxrad, reffreq,
				      isthisvp,
				      BeamSquint(squintdir, squintreffreq),
				      usesymmetricbeam);

  } else if (name == "COSPOLY") {


    Vector<Double> coeff;
    coeff=rec.asArrayDouble( rec.fieldNumber("coeff"));
    Vector<Double> scale;
    scale=rec.asArrayDouble( rec.fieldNumber("scale"));
    Quantity reffreq;
    getQuantity(rec, "reffreq", reffreq);
    Quantity maxrad;
    getQuantity(rec, "maxrad", maxrad);
    MDirection squintdir;
    getMDirection(rec, "squintdir", squintdir);
    Quantity squintreffreq;
    getQuantity(rec, "squintreffreq", squintreffreq);
    Bool usesymmetricbeam;
    usesymmetricbeam=rec.asBool( rec.fieldNumber("usesymmetricbeam"));
    Bool isthisvp;
    isthisvp=rec.asBool( rec.fieldNumber("isthisvp"));

    pb_pointer_p = new PBMath1DCosPoly( coeff, scale, maxrad, reffreq,
					isthisvp,
					BeamSquint(squintdir, squintreffreq),
					usesymmetricbeam);

  } else if (name == "NUMERIC") {
    Vector<Double> vect;
    vect=rec.asArrayDouble( rec.fieldNumber("vect"));
    Quantity reffreq;
    getQuantity(rec, "reffreq", reffreq);
    Quantity maxrad;
    getQuantity(rec, "maxrad", maxrad);
    MDirection squintdir;
    getMDirection(rec, "squintdir", squintdir);
    Quantity squintreffreq;
    getQuantity(rec, "squintreffreq", squintreffreq);
    Bool usesymmetricbeam;
    usesymmetricbeam=rec.asBool( rec.fieldNumber("usesymmetricbeam"));
    Bool isthisvp;
    isthisvp=rec.asBool( rec.fieldNumber("isthisvp"));

    Vector<Float> aTempVector;
    aTempVector.resize(vect.nelements());
    for (uInt i = 0; i< vect.nelements(); i++) {
      aTempVector(i) = (Float)vect(i);
    }

    pb_pointer_p = new PBMath1DNumeric( aTempVector, maxrad, reffreq,
					isthisvp,
					BeamSquint(squintdir, squintreffreq),
					usesymmetricbeam);
  } else if (name == "IMAGE") {

    String realImageName, imagImageName;
    realImageName=rec.asString(rec.fieldNumber("realimage"));
    imagImageName=rec.asString(rec.fieldNumber("imagimage"));

    PagedImage<Float> realImage(realImageName);
    if(Table::isReadable(imagImageName)) {
      PagedImage<Float> imagImage(imagImageName);
      pb_pointer_p = new PBMath2DImage(realImage, imagImage);
    }
    else {
      if(!Table::isReadable(imagImageName)) {
	pb_pointer_p = new PBMath2DImage(realImage);
      }
      else {
	throw(AipsError("Image "+ realImageName + " is not readable or does not exist"));
      }
    }

  } else 
    throw AipsError("Unrecognized PB name: " + name);
};

PBMath::PBMath(PBMath::CommonPB myPBType, Bool useSymmetricBeam)
{


  initByTelescope(myPBType, useSymmetricBeam);
 
};

PBMath::PBMath(String& telescopeName, Bool useSymmetricBeam, Quantity freq){
 
  String band;
  PBMath::CommonPB whichPB;
  String pbName;
  whichCommonPBtoUse(telescopeName, freq, band, whichPB, pbName);
  initByTelescope(whichPB, useSymmetricBeam, freq.getValue("Hz"));


}


// Explicitly call each letter class's constructor
// PBClass is for cases where we cannot distinquish the
// PB's to be made from the constructor arguments
PBMath::PBMath(PBMathInterface::PBClass theClass, Quantity halfWidth, 
	       Quantity maxRad, Quantity refFreq, 
	       Bool isThisVP,
	       BeamSquint squint, Bool useSymmetricBeam )
{
  LogIO os(LogOrigin("PBMath", "PBMath"));
  if (theClass == PBMathInterface::GAUSS) {
    pb_pointer_p = new  PBMath1DGauss(halfWidth, maxRad, refFreq,
				      isThisVP, squint, useSymmetricBeam);
  } else {
    os << "Unrecognized voltage pattern class type for this constructor:1" << LogIO::EXCEPTION;
  }
};


PBMath::PBMath(PBMathInterface::PBClass theClass, const Vector<Double>& avector, Quantity maxRad, 
	       Quantity refFreq, Bool isThisVP, BeamSquint squint, Bool useSymmetricBeam )
{
  LogIO os(LogOrigin("PBMath", "PBMath"));
  if (theClass == PBMathInterface::POLY) {
    pb_pointer_p = new  PBMath1DPoly(avector, maxRad, refFreq,
				     isThisVP, squint, useSymmetricBeam);
  } else if (theClass == PBMathInterface::IPOLY) {
    pb_pointer_p = new  PBMath1DIPoly(avector, maxRad, refFreq,
				     isThisVP, squint, useSymmetricBeam);
  } else if (theClass == PBMathInterface::NUMERIC) {
    Vector<Float> aTempVector;
    aTempVector.resize(avector.nelements());
    for (uInt i = 0; i< avector.nelements(); i++) {
      aTempVector(i) = (Float)avector(i);
    }
    pb_pointer_p = new  PBMath1DNumeric(aTempVector, maxRad, refFreq,
				     isThisVP, squint, useSymmetricBeam);
  } else {
    os << "Unrecognized voltage pattern class type for this constructor:2" << LogIO::EXCEPTION;
  }
};

PBMath::PBMath(PBMathInterface::PBClass theClass, const Vector<Float>& avector, Quantity maxRad, 
	       Quantity refFreq, Bool isThisVP, BeamSquint squint, Bool useSymmetricBeam )
{
  LogIO os(LogOrigin("PBMath", "PBMath"));
  if (theClass == PBMathInterface::NUMERIC) {
    pb_pointer_p = new  PBMath1DNumeric(avector, maxRad, refFreq,
				     isThisVP, squint, useSymmetricBeam);
  } else {
    os << "Unrecognized voltage pattern class type for this constructor:2" << LogIO::EXCEPTION;
  }
};

PBMath::PBMath(PBMathInterface::PBClass theClass,
	       ImageInterface<Float>& reJones)
{
  LogIO os(LogOrigin("PBMath", "PBMath"));
  if (theClass == PBMathInterface::IMAGE) {
    pb_pointer_p = new  PBMath2DImage(reJones);
  } else {
    os << "Unrecognized voltage pattern class type for this constructor:2" << LogIO::EXCEPTION;
  }
};

PBMath::PBMath(PBMathInterface::PBClass theClass,
	       ImageInterface<Float>& reJones,
	       ImageInterface<Float>& imJones)
{
  LogIO os(LogOrigin("PBMath", "PBMath"));
  if (theClass == PBMathInterface::IMAGE) {
    pb_pointer_p = new  PBMath2DImage(reJones, imJones);
  } else {
    os << "Unrecognized voltage pattern class type for this constructor:2" << LogIO::EXCEPTION;
  }
};


PBMath::PBMath(PBMathInterface::PBClass theClass, const Vector<Double>& coeff, 
	       const Vector<Double>& scales, Quantity maxRad, Quantity refFreq, 
	       Bool isThisVP,
	       BeamSquint squint, 
	       Bool useSymmetricBeam )
{
  LogIO os(LogOrigin("PBMath", "PBMath"));
  if (theClass == PBMathInterface::COSPOLY) {
    pb_pointer_p = new  PBMath1DCosPoly(coeff, scales, maxRad, refFreq,
					isThisVP, squint, useSymmetricBeam);
  } else {
    os << "Unrecognized voltage pattern class type for this constructor:3" << LogIO::EXCEPTION;
  }    
};

PBMath::PBMath(PBMathInterface::PBClass theClass, Quantity dishDiam, 
	       Quantity blockDiam, Quantity maxRad, 
	       Quantity refFreq, BeamSquint squint, 
	       Bool useSymmetricBeam )
{
  LogIO os(LogOrigin("PBMath", "PBMath"));
  if (theClass == PBMathInterface::AIRY) {
  pb_pointer_p = new  PBMath1DAiry(dishDiam, blockDiam, maxRad, 
				   refFreq, squint, useSymmetricBeam);
  } else {
    os << "Unrecognized voltage pattern class type for this constructor:4" << LogIO::EXCEPTION;
  }
};

PBMath::PBMath(const PBMath &other)
{
  pb_pointer_p = other.pb_pointer_p;
};


PBMath::~PBMath()
{
};

PBMath& PBMath::operator=(const PBMath& other)
{
  if (this != &other) {
    pb_pointer_p = other.pb_pointer_p;
  }
  return *this;
};

Bool PBMath::operator==(const PBMath& other) const
{
  return (pb_pointer_p == other.pb_pointer_p ? True : False);
};


Bool PBMath::operator!=(const PBMath& other) const
{
  return (pb_pointer_p != other.pb_pointer_p ? True : False);
};


ImageRegion*
PBMath::extent(const ImageInterface<Complex> & im, const MDirection& pointing,
	       const Int row, const Float fPad, const Int iChan,  
	       const SkyJones::SizeType sizeType)
{
 return (pb_pointer_p->extent(im, pointing, row, fPad, iChan, sizeType));
};

ImageRegion*
PBMath::extent(const ImageInterface<Float> & im, const MDirection& pointing,
	       const Int row, const Float fPad, const Int iChan,  
	       const SkyJones::SizeType sizeType)
{
 return (pb_pointer_p->extent(im, pointing, row, fPad, iChan, sizeType));
};




//Bool PBMath::flushToTable(Table& beamSubTable, Int iRow)
//{
//  pb_pointer_p->flushToTable(Table& beamSubTable, Int iRow);
//};

//PBMath PBMath::copy() const
//{
//  PBMath tmp (*this);
//  tmp.pb_pointer_p = pb_pointer_p->clone();
//  return tmp;
//};


void 
PBMath::summary(Int nValues)
{
  pb_pointer_p->summary(nValues);
};



ImageInterface<Complex>& 
PBMath::applyVP(const ImageInterface<Complex>& in,
		ImageInterface<Complex>& out,
		const MDirection& sp,
		const Quantity parAngle,
		const BeamSquint::SquintType doSquint,
		Bool inverse,
		Bool conjugate,
		Float cutoff,
		Bool forward)
{
  return pb_pointer_p->applyVP(in, out, sp, parAngle, doSquint, inverse,
			       conjugate, cutoff, forward);
};

ImageInterface<Complex>& 
PBMath::applyPB(const ImageInterface<Complex>& in,
		ImageInterface<Complex>& out,
		const MDirection& sp,
		const Quantity parAngle,
		const BeamSquint::SquintType doSquint,
		Bool inverse,
		Float cutoff,
		Bool forward)
{
  return pb_pointer_p->applyPB(in, out, sp, parAngle, doSquint, inverse, cutoff, forward);
};

ImageInterface<Float>& 
PBMath::applyPB2(const ImageInterface<Float>& in,
		 ImageInterface<Float>& out,
		 const MDirection& sp,
		 const Quantity parAngle,
		 const BeamSquint::SquintType doSquint,
		 Float cutoff)
{
  return pb_pointer_p->applyPB2(in, out, sp, parAngle, doSquint, cutoff);
};
SkyComponent& 
PBMath::applyVP(SkyComponent& in,
		SkyComponent& out,
		const MDirection& sp,
		const Quantity frequency,
		const Quantity parAngle,
		const BeamSquint::SquintType doSquint,
		Bool inverse,
		Bool conjugate,
		Float cutoff,
		Bool forward)
{
  return pb_pointer_p->applyVP(in, out, sp, frequency, parAngle, doSquint, inverse, conjugate,
			       cutoff, forward);
};

SkyComponent& 
PBMath::applyPB(SkyComponent& in,
		SkyComponent& out,
		const MDirection& sp,
		const Quantity frequency,
		const Quantity parAngle,
		const BeamSquint::SquintType doSquint,
		Bool inverse,
		Float cutoff,
		Bool forward)
{
  return pb_pointer_p->applyPB(in, out, sp, frequency, parAngle, doSquint, inverse, cutoff,
			       forward);
};

SkyComponent& 
PBMath::applyPB2(SkyComponent& in,
		 SkyComponent& out,
		 const MDirection& sp,
		 const Quantity frequency,
		 const Quantity parAngle,
		 const BeamSquint::SquintType doSquint)
{
  return pb_pointer_p->applyPB2(in, out, sp, frequency, parAngle, doSquint);
};



ImageInterface<Complex>& 
PBMath::apply(const ImageInterface<Complex>& in,
	      ImageInterface<Complex>& out,
	      const MDirection& sp,
	      const Quantity parAngle,	      
	      const BeamSquint::SquintType doSquint,
	      Bool inverse,
	      Bool conjugate, 
	      Int ipower, 
	      Float cutoff,
	      Bool forward)
{
  return pb_pointer_p->apply(in, out, sp, parAngle, doSquint, inverse,
			     conjugate, ipower, cutoff, forward); 
};


SkyComponent& 
PBMath::apply(SkyComponent& in,
	      SkyComponent& out,
	      const MDirection& sp,
	      const Quantity frequency,
	      const Quantity parAngle,
	      const BeamSquint::SquintType doSquint,
	      Bool inverse,
	      Bool conjugate,
	      Int ipower,  // ie, 1=VP, 2=PB, 4=PB^2
	      Float cutoff,
	      Bool forward)
{
  return pb_pointer_p->apply(in, out, sp, frequency, parAngle, doSquint, inverse,
			     conjugate, ipower, cutoff, forward); 
};



Bool PBMath::ok() const
{
  if (pb_pointer_p.null()) {
    return False;
  } else {
    return pb_pointer_p->ok();
  }
};


void
PBMath::whichCommonPBtoUse(String &telescope, Quantity &freq, 
			   String &band, PBMath::CommonPB &whichPB, String &PBName)
{

  // note:  these bands are fairly fast and loose,
  // and owe a lot to the fact that the band coverage is sparse!
  Double freqGHz = freq.getValue("GHz");
  if (telescope(0,3)=="VLA") {
    if (freqGHz > 35.0 && freqGHz < 55.0) {
      whichPB = PBMath::VLA_Q;
      band = "Q";
    } else if (freqGHz > 35.0 && freqGHz < 55.0) {
      whichPB = PBMath::VLA_Q;
      band = "Q";
    } else if (freqGHz > 19.0 && freqGHz < 35.0) {
      whichPB = PBMath::VLA_K;
      band = "K";
    } else if (freqGHz > 11.0 && freqGHz < 19.0) {
      whichPB = PBMath::VLA_U;
      band = "U";
    } else if (freqGHz > 7.0 && freqGHz < 11.0) {
      whichPB = PBMath::VLA_X;
      band = "X";
    } else if (freqGHz > 4.0 && freqGHz < 7.0) {
      whichPB = PBMath::VLA_C;
      band = "C";
    } else if (freqGHz > 1.0 && freqGHz < 2.0) {
      whichPB = PBMath::VLA_L;
      band = "L";
    } else if (freqGHz > 0.2 && freqGHz < 0.4) {
      whichPB = PBMath::VLA_P;
      band = "P";
    } else if (freqGHz < 0.1) {
      whichPB = PBMath::VLA_4;
      band = "4";
    } else {
      whichPB = PBMath::VLA_NVSS;
      band = "UNKNOWN";
    }
  } else if (telescope(0,4)=="WSRT") {
    if (freqGHz > 3.0 && freqGHz < 6.0) {
      whichPB = PBMath::WSRT;
      band = "C";
    } else if (freqGHz > 0.80 && freqGHz < 2.0) {
      whichPB = PBMath::WSRT;
      band = "L";
    } else if (freqGHz > 0.5 && freqGHz < 0.8) {
      whichPB = PBMath::WSRT_LOW;
      band = "49"; // Hey, I just don't know its name: 49cm
    } else if (freqGHz < 0.5 ) {
      whichPB = PBMath::WSRT_LOW;
      band = "P";
    } else {
      whichPB = PBMath::WSRT;
      band = "UNKNOWN";
    }
  } else if (telescope(0,4)=="ATCA") {
    if (freqGHz > 7.0 && freqGHz < 11.0) {
      whichPB = PBMath::ATCA_X;
      band = "X";
    } else if (freqGHz > 4.0 && freqGHz < 7.0) {
      whichPB = PBMath::ATCA_C;
      band = "C";
    } else if (freqGHz > 2.0 && freqGHz < 4.0) {
      whichPB = PBMath::ATCA_S;
      band = "S";
    } else if (freqGHz < 2.0 ) {
      whichPB = PBMath::ATCA_L1;
      band = "L1";
    } else {
      whichPB = PBMath::ATCA_L1;
      band = "UNKNOWN";
    }
  } else if (telescope(0,8)=="HATCREEK") {
    whichPB = PBMath::HATCREEK;
    band = "UNKNOWN";
  } else if (telescope(0,4)=="BIMA") {  // BIMA is a synonym for HATCREEK
    whichPB = PBMath::HATCREEK;
    band = "UNKNOWN";
  } else if (telescope(0,3)=="GBT") {
    whichPB = PBMath::GBT;
    band = "UNKNOWN";
  } else if (telescope(0,4)=="GMRT") {
    whichPB = PBMath::GMRT;
    band = "UNKNOWN";
  } else if (telescope(0,4)=="OVRO") {
    whichPB = PBMath::OVRO;
    band = "UNKNOWN";
  } else if (telescope(0,7)=="NRAO12M") {
    whichPB = PBMath::NRAO12M;
    band = "UNKNOWN";
  } else if (telescope(0,7)=="IRAMPDB") {
    whichPB = PBMath::IRAMPDB;
    band = "UNKNOWN";
  } else if (telescope(0,7)=="IRAM30M") {
    whichPB = PBMath::IRAM30M;
    band = "UNKNOWN";
  } else if (telescope(0,9)=="NRAO140FT") {
    whichPB = PBMath::NRAO140FT;
    band = "UNKNOWN";
  } else if (telescope(0,4)=="ALMA") {
    whichPB = PBMath::ALMA;
    band = "UNKNOWN";
  } else if (telescope(0,6)=="ALMASD") {
    whichPB = PBMath::ALMASD;
    band = "UNKNOWN";
  } else if (telescope(0,3)=="ACA") {
    whichPB = PBMath::ACA;
    band = "UNKNOWN";
  } else if (telescope(0,4)=="NONE") {
    whichPB = PBMath::NONE;
    band = "UNKNOWN";
  } else {
    whichPB = PBMath::UNKNOWN;
    band = "UNKNOWN";
  }
  
  nameCommonPB(whichPB, PBName);
};


// converts the enumrated type into a string
void PBMath::nameCommonPB(const PBMath::CommonPB iPB, String & str)
{

  switch (iPB) {
  case PBMath::UNKNOWN:
    str = "UNKNOWN";
    break;
  case PBMath::DEFAULT:
    str = "DEFAULT";
    break;
  case PBMath::ATCA_L1:
    str = "ATCA_L1";
    break;
  case PBMath::ATCA_L2:
    str = "ATCA_L2";
    break;
  case PBMath::ATCA_L3:
    str = "ATCA_L3";
    break;
  case PBMath::ATCA_S:
    str = "ATCA_S";
    break;
  case PBMath::ATCA_C:
    str = "ATCA_C";
    break;
  case PBMath::ATCA_X:
    str = "ATCA_X";
    break;
  case PBMath::GBT:
    str = "GBT";
    break;
  case PBMath::GMRT:
    str = "GMRT";
    break;
  case PBMath::HATCREEK:
    str = "HATCREEK";
    break;
  case PBMath::NRAO12M:
    str = "NRAO12M";
    break;
  case PBMath::IRAMPDB:
    str = "IRAMPDB";
    break;
  case PBMath::IRAM30M:
    str = "IRAM30M";
    break;
  case PBMath::OVRO:
    str = "OVRO";
    break;
  case PBMath::VLA:
    str = "VLA";
    break;
  case PBMath::VLA_INVERSE:
    str = "VLA_INVERSE";
    break;
  case PBMath::VLA_NVSS:
    str = "VLA_NVSS";
    break;
  case PBMath::VLA_2NULL:
    str = "VLA_2NULL";
    break;
  case PBMath::VLA_4:
    str = "VLA_4";
    break;
  case PBMath::VLA_P:
    str = "VLA_P";
    break;
  case PBMath::VLA_L:
    str = "VLA_L";
    break;
  case PBMath::VLA_C:
    str = "VLA_C";
    break;
  case PBMath::VLA_X:
    str = "VLA_X";
    break;
  case PBMath::VLA_U:
    str = "VLA_U";
    break;
  case PBMath::VLA_K:
    str = "VLA_K";
    break;
  case PBMath::VLA_Q:
    str = "VLA_Q";
    break;
  case PBMath::WSRT:
    str = "WSRT";
    break;
  case PBMath::WSRT_LOW:
    str = "WSRT_LOW";
    break;
  case PBMath::ALMA:
    str = "ALMA";
    break;
  case PBMath::ALMASD:
    str = "ALMASD";
    break;
  case PBMath::ACA:
    str = "ACA";
    break;
  case PBMath::NONE:
    str = "NONE";
    break;
  default:
    str = "UNKNOWN";
    break;
  }
};


void PBMath::enumerateCommonPB(const String & str, PBMath::CommonPB& ipb)
{
  if (str == "UNKNOWN") {
    ipb = PBMath::UNKNOWN;
  } else if (str == "DEFAULT") {
    ipb = PBMath::DEFAULT;
  } else if (str == "ATCA_L1") {
    ipb = PBMath::ATCA_L1;
  } else if (str == "ATCA_L2") {
    ipb = PBMath::ATCA_L2;
  } else if (str == "ATCA_L3") {
    ipb = PBMath::ATCA_L3;
  } else if (str == "ATCA_S") {
    ipb = PBMath::ATCA_S;
  } else if (str == "ATCA_C") {
    ipb = PBMath::ATCA_C;
  } else if (str == "ATCA_X") {
    ipb = PBMath::ATCA_X;
  } else if (str == "HATCREEK") {
    ipb = PBMath::HATCREEK;
  } else if (str == "BIMA") {  //  BIMA is a synonym for HATCREEK
    ipb = PBMath::HATCREEK;
  } else if (str == "VLA") {
    ipb = PBMath::VLA;
  } else if (str == "VLA_INVERSE") {
    ipb = PBMath::VLA_INVERSE;
  } else if (str == "VLA_NVSS") {
    ipb = PBMath::VLA_NVSS;
  } else if (str == "VLA_2NULL") {
    ipb = PBMath::VLA_2NULL;
  } else if (str == "VLA_4") {
    ipb = PBMath::VLA_4;
  } else if (str == "VLA_P") {
    ipb = PBMath::VLA_P;
  } else if (str == "VLA_L") {
    ipb = PBMath::VLA_L;
  } else if (str == "VLA_C") {
    ipb = PBMath::VLA_C;
  } else if (str == "VLA_X") {
    ipb = PBMath::VLA_X;
  } else if (str == "VLA_U") {
    ipb = PBMath::VLA_U;
  } else if (str == "VLA_K") {
    ipb = PBMath::VLA_K;
  } else if (str == "VLA_Q") {
    ipb = PBMath::VLA_Q;
  } else if (str == "WSRT") {
    ipb = PBMath::WSRT;
  } else if (str == "WSRT_LOW") {
    ipb = PBMath::WSRT_LOW;
  } else if (str == "OVRO") {
    ipb = PBMath::OVRO;
  } else if (str == "GBT") {
    ipb = PBMath::GBT;
  } else if (str == "GMRT") {
    ipb = PBMath::GMRT;
  } else if (str == "NRAO12M") {
    ipb = PBMath::NRAO12M;
  } else if (str == "IRAMPDB") {
    ipb = PBMath::IRAMPDB;
  } else if (str == "IRAM30M") {
    ipb = PBMath::IRAM30M;
  } else if (str == "NRAO140FT") {
    ipb = PBMath::NRAO140FT;
  } else if (str == "ALMA") {
    ipb = PBMath::ALMA;
  } else if (str == "ALMASD") {
    ipb = PBMath::ALMASD;
  } else if (str == "ACA") {
    ipb = PBMath::ACA;
  } else if (str == "NONE") {
    ipb = PBMath::NONE;
  } else {
    ipb = PBMath::UNKNOWN;
  }
};

Bool 
PBMath::getQuantity(const RecordInterface& rec, const String& item, 
		    Quantity& returnedQuantity) const
{
  String error;
  QuantumHolder h;
  const RecordInterface& quantRec = rec.asRecord(item);
  
  if (!h.fromRecord(error, quantRec)) {
    throw (AipsError ("PBMath::getQuantity - could not recover "+item+" because "+error));
  }
  if (!h.isQuantity()) {
    throw (AipsError ("PBMath::getQuantity - could not recover "+item+
		      " because it isnt a Quantity"));
  }
  returnedQuantity = h.asQuantumDouble();
  return True;
};

Bool 
PBMath::getMDirection(const RecordInterface& rec, const String& item, 
		      MDirection& returnedMDirection) const
{
  String error;
  MeasureHolder h;
  const RecordInterface& measureRec = rec.asRecord(item);

  if (!h.fromRecord(error, measureRec)) {
    throw (AipsError ("PBMath::getMDirection - could not recover "+item+" because "+error));
  }
  if (!h.isMDirection()) {
    throw (AipsError ("PBMath::getMDirection - could not recover "+item+
		      " because it isnt a MDirection"));
  }
  returnedMDirection = h.asMDirection();
  return True;
};

void PBMath::initByTelescope(PBMath::CommonPB myPBType, 
			     Bool useSymmetricBeam, 
			     Double frequency) 
{

 // Remember, these are fit parameters for the PB, not the PB

  
  LogIO os(LogOrigin("PBMath", "initByTelescope"));

  Vector<Double> vlacoef(4);
  vlacoef(0)= 1.0;
  vlacoef(1)= -1.300633e-03;
  vlacoef(2)= 6.480550e-07;
  vlacoef(3)= -1.267928e-10;

  // This attempts to reproduce the AIRY pattern VLA PB
  Vector<Float> vlanum(19);
  vlanum(0) = 1.000000;
  vlanum(1) = 0.997634;
  vlanum(2) = 0.972516;
  vlanum(3) = 0.913722;
  vlanum(4) = 0.837871;
  vlanum(5) = 0.750356;
  vlanum(6) = 0.651549;
  vlanum(7) = 0.549903;
  vlanum(8) = 0.449083;
  vlanum(9) = 0.352819;
  vlanum(10) = 0.266025;
  vlanum(11) = 0.190533;
  vlanum(12) = 0.128047;
  vlanum(13) = 0.0794855;
  vlanum(14) = 0.0438381;
  vlanum(15) = 0.0201386;
  vlanum(16) = 0.0065117;
  vlanum(17) = 0.000690041;
  vlanum(18) = 8.87288e-05;


  switch (myPBType) {
  case VLA:
    // No squint assumed here.
    // This beam has a peak of 1.00000 (unlike Napier and Rots; a refit to their data)
    {
      Vector<Double> coef(4);
      coef(0)= 1.0;
      coef(1)= -1.300633e-03;
      coef(2)= 6.480550e-07;
      coef(3)= -1.267928e-10;

      pb_pointer_p = new PBMath1DPoly( coef, Quantity(43.0,"'"), Quantity(1.0,"GHz"));
    }
  break;
  case VLA_INVERSE:
    // No squint assumed here
    // This beam does not have a peak of unity, kept for historical purposes
    // This is the beam model used by CLASSIC AIPS 
    {
      Vector<Double> coef(5);
      coef(0)= 0.9920378;
      coef(1)= 0.9956885e-3;
      coef(2)= 0.3814573e-5;
      coef(3)= -0.5311695e-8;
      coef(4)= 0.3980963e-11;

      pb_pointer_p = new PBMath1DIPoly( coef, Quantity(45.0,"'"), Quantity(1.0,"GHz"));
    }
  break;
  case VLA_NVSS:
    // No squint assumed here
    pb_pointer_p = new PBMath1DAiry( Quantity(24.5,"m"), Quantity(0.0,"m"),
				     Quantity(0.8564,"deg"), Quantity(1.0,"GHz"));
    break;
  case VLA_2NULL:
    // No squint assumed here
    pb_pointer_p = new PBMath1DAiry( Quantity(24.5,"m"), Quantity(0.0,"m"),
				     Quantity(1.566,"deg"), Quantity(1.0,"GHz"));
    break;


  case VLA_L:
    // Includes Squint.
    // "mag" is the equivalent magnitude of the "half-squint", ie, difference
    // between the nominal pointing center and the Stokes RR beam, but scaled
    // to a reference frequency of 1 GHz.
    // "ang" is calculated this way: look at the orientation of the feed 
    // looking DOWN on antenna (these are recorded in memos); flip to
    // BEHIND the antenna, lookup up at sky; add 90 degrees to the angle for
    // the squint orientation, which is perpendicular to the feed offset.
    //    The trick: the squint, measured CCW from +AZ, should be the same
    //    angle as the feed orientation measured CW from UP.
    //
    {
      Float mag = 1.21;  // half-squint magnitude in arcmin at 1 GHz)
      Float ang = -135.0*C::pi/180.0;    // squint orientation, rads, North of +AZ axis
      pb_pointer_p = new PBMath1DAiry( Quantity(25.0,"m"), Quantity(2.36,"m"),
				       Quantity(0.8564,"deg"), Quantity(1.0,"GHz"),
				       BeamSquint(MDirection(Quantity((mag*cos(ang)), "'"),
							     Quantity((mag*sin(ang)), "'"),
							     MDirection::Ref(MDirection::AZEL)),
						  Quantity(1.0, "GHz")),
				       useSymmetricBeam);
    }
    break;
  case VLA_C:
    {
      Float mag = 1.21;  // half-squint magnitude in arcmin at 1 GHz)
      Float ang = 25.0*C::pi/180.0;    // squint orientation, rads, North of +AZ axis
      pb_pointer_p = new PBMath1DAiry( Quantity(25.0,"m"), Quantity(2.36,"m"),
				       Quantity(0.8564,"deg"), Quantity(1.0,"GHz"),
				       BeamSquint(MDirection(Quantity((mag*cos(ang)), "'"),
							     Quantity((mag*sin(ang)), "'"),
							     MDirection::Ref(MDirection::AZEL)),
						  Quantity(1.0, "GHz")),
				       useSymmetricBeam);
    }
    break;
  case VLA_X:
    {
      // This is based on the OLD X feed position; upgrade in progress
      Float mag = 1.21;  // half-squint magnitude in arcmin at 1 GHz)
      Float ang = 82.0*C::pi/180.0;    // squint orientation, rads, North of +AZ axis
      pb_pointer_p = new PBMath1DAiry( Quantity(25.0,"m"), Quantity(2.36,"m"),
				       Quantity(0.8564,"deg"), Quantity(1.0,"GHz"),
				       BeamSquint(MDirection(Quantity((mag*cos(ang)), "'"),
							     Quantity((mag*sin(ang)), "'"),
							     MDirection::Ref(MDirection::AZEL)),
						  Quantity(1.0, "GHz")),
				       useSymmetricBeam);

    }
  break;
  case VLA_U:
    {
      Float mag = 1.21;  // half-squint magnitude in arcmin at 1 GHz)
      Float ang = -25.0*C::pi/180.0;    // squint orientation, rads, North of +AZ axis
      pb_pointer_p = new PBMath1DAiry( Quantity(25.0,"m"), Quantity(2.36,"m"),
				       Quantity(0.8564,"deg"), Quantity(1.0,"GHz"),
				       BeamSquint(MDirection(Quantity((mag*cos(ang)), "'"),
							     Quantity((mag*sin(ang)), "'"),
							     MDirection::Ref(MDirection::AZEL)),
						  Quantity(1.0, "GHz")),
				       useSymmetricBeam);
    }
  break;
  case VLA_K:
    {
      // This is based on the OLD K feed position; upgrade in progress
      Float mag = 1.21;  // half-squint magnitude in arcmin at 1 GHz)
      Float ang = -6.0*C::pi/180.0;    // squint orientation, rads, North of +AZ axis
      pb_pointer_p = new PBMath1DAiry( Quantity(25.0,"m"), Quantity(2.36,"m"),
				       Quantity(0.8564,"deg"), Quantity(1.0,"GHz"),
				       BeamSquint(MDirection(Quantity((mag*cos(ang)), "'"),
							     Quantity((mag*sin(ang)), "'"),
							     MDirection::Ref(MDirection::AZEL)),
						  Quantity(1.0, "GHz")),
				       useSymmetricBeam);
    }
  break;
  case VLA_Q:
    {
      // This is based on an approximate feed position; awaiting new numbers
      Float mag = 1.21;  // half-squint magnitude in arcmin at 1 GHz)
      Float ang = 0.0*C::pi/180.0;    // squint orientation, rads, North of +AZ axis
      pb_pointer_p = new PBMath1DAiry( Quantity(25.0,"m"), Quantity(2.36,"m"),
				       Quantity(0.8564,"deg"), Quantity(1.0,"GHz"),
				       BeamSquint(MDirection(Quantity((mag*cos(ang)), "'"),
							     Quantity((mag*sin(ang)), "'"),
							     MDirection::Ref(MDirection::AZEL)),
						  Quantity(1.0, "GHz")),
				       useSymmetricBeam);
    }
  break;
  case VLA_P:
    {
      // This is not based on any P band measurements; assume no squint
      pb_pointer_p = new PBMath1DAiry( Quantity(25.0,"m"), Quantity(2.36,"m"),
				       Quantity(0.8564,"deg"), Quantity(1.0,"GHz"));
    }
  break;
  case VLA_4:
    {
      // This is not based on any 4 band measurements; assume no squint
      pb_pointer_p = new PBMath1DAiry( Quantity(25.0,"m"), Quantity(2.36,"m"),
				       Quantity(0.8564,"deg"), Quantity(1.0,"GHz"));
    }
  break;

  // These 4 cases, OVRO, GMRT, NRAO12M, and NRAO140FT, do not represent
  // well determined models (though some of them have well determined models).
  // We are inserting them for completeness, though we should update some of them.
  // For now we just assume that they are AIRY disks (via the Numeric fix, as with
  // the VLA) of the approximate size (we fudge this by setting the scale-size
  // to 1.1998662' * 25M/DIAM

  case OVRO:
    {
      Double scalesize = 1.1998662 * 25.0/10.4;
      pb_pointer_p = new PBMath1DNumeric(vlanum, Quantity(scalesize,"'"), 
					 Quantity(43.0,"GHz"), False);
    }
    break;
  case GBT:
    {
      //Measured beam was at 340MHz and spanning 1 deg.
      //Data from Ron Maddalena 
      //If more data is available at other frequencies it should be cased 
      //out here otherwise this beam is going to be scaled for all frequencies
      Double scalesize = 60.0*3.4e8/1.0e9;
      Double factor=1.0e9/3.4e8; 
      Vector<Double> coeff(7);
      
      coeff[0]=1.00632057;
      coeff[1]=-0.00181068914; 
      coeff[2]=1.32948262e-06;
      coeff[3]=-4.72454392e-10;
      coeff[4]=7.11800547e-14;
      coeff[5]=-3.50646851e-20;
      coeff[6]=-7.40533706e-22;
      
      //These coefficient are at 340 MHz
      //For some peculiar reason PBMath1DPoly seem to scale things well only if
      //the ref refreq at 1 GHz..
      //hence converting coeeficients to the one at 1 GHz

      Double coeffac=1.0;
      for (uInt k=1; k < 7 ; ++k){
	coeffac=coeffac*factor*factor;
	coeff[k]=coeff[k]*coeffac;
      }
      
         
      pb_pointer_p = new PBMath1DPoly( coeff, Quantity(scalesize,"'"), 
				       Quantity(1.0e9,"Hz"),False);
    }
    break;
  case GMRT:
    {
      Double scalesize = 1.1998662 * 25.0/45.0;
      pb_pointer_p = new PBMath1DNumeric(vlanum, Quantity(scalesize,"'"), 
					 Quantity(43.0,"GHz"), False);
    }
    break;
  case NRAO12M:
    {
      Double scalesize = 1.1998662 * 25.0/12.0;
      pb_pointer_p = new PBMath1DNumeric(vlanum, Quantity(scalesize,"'"), 
					 Quantity(43.0,"GHz"), False);
    }
    break;
  case NRAO140FT:
    {
      Double scalesize = 1.1998662 * 25.0/43.0;
      pb_pointer_p = new PBMath1DNumeric(vlanum, Quantity(scalesize,"'"), 
					 Quantity(43.0,"GHz"), False);
    }
    break;



  case WSRT:
    {
      // WSRT for freq > 800 MHz
      Bool thisIsVP = True;
      Vector<Double> coef(4);
      Vector<Double> cosScale(4);
      coef(0)= 0.0;
      coef(1)= 0.0;
      coef(2)= 0.0;
      coef(3)= 1.0;
      cosScale(0)= 0.0;
      cosScale(1)= 0.0;
      cosScale(2)= 0.0;
      cosScale(3)= 0.01891;
      // 0.01891 = 0.0672 * 1000(MHz/GHz) /(60(arcm/deg)) * 2pi/180

      pb_pointer_p = new PBMath1DCosPoly(coef, cosScale, 
					 Quantity(1.0, "deg"), 
					 Quantity(1.0, "GHz"), thisIsVP);
    }
  break;
  case WSRT_LOW:
    {
      // WSRT for freq < 800 MHz
      Bool thisIsVP = True;
      Vector<Double> coef(4);
      Vector<Double> cosScale(4);
      coef(0)= 0.0;
      coef(1)= 0.0;
      coef(2)= 0.0;
      coef(3)= 1.0;
      cosScale(0)= 0.0;
      cosScale(1)= 0.0;
      cosScale(2)= 0.0;
      cosScale(3)= 0.01830;
      // 0.01830 = 0.065 * 1000(MHz/GHz) /(60(arcm/deg)) * 2pi/180

      pb_pointer_p = new PBMath1DCosPoly(coef, cosScale, Quantity(1.0, "deg"), 
					 Quantity(1.0, "GHz"), thisIsVP);
    }
    break;
  case ATCA_L1:    
    {
      Vector<Double> coef(5);
      coef(0)= 1.0;
      coef(1)= 8.99e-4;
      coef(2)= 2.15e-6;
      coef(3)= -2.23e-9;
      coef(4)= 1.56e-12;

      pb_pointer_p = new PBMath1DIPoly( coef, Quantity(53.0,"'"), Quantity(1.0,"GHz"));
    }
    break;
  case ATCA_L2:
    {
      Vector<Double> coef(7);
      coef(0)= 1.0;
      coef(1)= -1.0781341990755E-03;
      coef(2)= 4.6179146405726E-07;
      coef(3)= -1.0108079576125E-10;
      coef(4)= 1.2073518438662E-14;
      coef(5)= -7.5132629268134E-19;
      coef(6)= 1.9083641820123E-23;

      pb_pointer_p = new PBMath1DPoly( coef, Quantity(53.0,"'"), Quantity(1.0,"GHz"));
    }
    break;
  case ATCA_L3:
    os  << "ATCA_L3 not yet implemented" << LogIO::EXCEPTION;    
    break;
  case ATCA_S:
    {
      Vector<Double> coef(5);
      coef(0)= 1.0;
      coef(1)= 1.02e-3;
      coef(2)= 9.48e-7;
      coef(3)= -3.68e-10;
      coef(4)= 4.88e-13;

      pb_pointer_p = new PBMath1DIPoly( coef, Quantity(53.0,"'"), Quantity(1.0,"GHz"),
					False,
					BeamSquint(MDirection(Quantity(0.0, "'"),
							      Quantity(0.0, "'"),
							      MDirection::Ref(MDirection::AZEL)),
						   Quantity(1.0, "GHz")),
					False);
    }
    break;
  case ATCA_C:
    {
      Vector<Double> coef(5);
      coef(0)= 1.0;
      coef(1)= 1.08e-3;
      coef(2)= 1.31e-6;
      coef(3)= -1.17e-9;
      coef(4)= 1.07e-12;

      pb_pointer_p = new PBMath1DIPoly( coef, Quantity(53.0,"'"), Quantity(1.0,"GHz"),
					False,
					BeamSquint(MDirection(Quantity(0.0, "'"),
							      Quantity(0.0, "'"),
							      MDirection::Ref(MDirection::AZEL)),
						   Quantity(1.0, "GHz")),
					False);
    }
    break;
  case ATCA_X:
    {
      Vector<Double> coef(5);
      coef(0)= 1.0;
      coef(1)= 1.04e-3;
      coef(2)= 8.36e-7;
      coef(3)= -4.68e-10;
      coef(4)= 5.50e-13;

      pb_pointer_p = new PBMath1DIPoly( coef, Quantity(53.0,"'"), Quantity(1.0,"GHz"),
					False,
					BeamSquint(MDirection(Quantity(0.0, "'"),
							      Quantity(0.0, "'"),
							      MDirection::Ref(MDirection::AZEL)),
						   Quantity(1.0, "GHz")),
				        False);
     }
    break;
  case HATCREEK:
    pb_pointer_p = new PBMath1DGauss( Quantity((191.67/2.0),"'"),  // half width==> /2
				      Quantity(215.0, "'"),
				      Quantity(1.0, "GHz"),
				      False,
				      BeamSquint(MDirection(Quantity(0.0, "'"),
							    Quantity(0.0, "'"),
							    MDirection::Ref(MDirection::AZEL)),
						 Quantity(1.0, "GHz")),
				      False );
    break;
  case IRAMPDB:
      pb_pointer_p = new PBMath1DAiry( Quantity(15.0,"m"), Quantity(1.0,"m"),
				       Quantity(1.784,"deg"), Quantity(1.0,"GHz") );
    break;
  case IRAM30M:
      pb_pointer_p = new PBMath1DAiry( Quantity(15.0,"m"), Quantity(1.0,"m"),
				       Quantity(1.784,"deg"), Quantity(1.0,"GHz") );
    break;
  case ALMA:
      pb_pointer_p = new PBMath1DAiry( Quantity(12.0,"m"), Quantity(1.0,"m"),
				       Quantity(1.784,"deg"), Quantity(1.0,"GHz") );
    break;
  case ALMASD:
      pb_pointer_p = new PBMath1DAiry( Quantity(12.0,"m"), Quantity(1.0,"m"),
				       Quantity(1.784,"deg"), Quantity(1.0,"GHz") );
    break;
  case ACA:
      pb_pointer_p = new PBMath1DAiry( Quantity(6.0,"m"), Quantity(0.5,"m"),
				       Quantity(3.568,"deg"), Quantity(1.0,"GHz") );
    break;
  case NONE:
    {
      Vector<Double> coef(1);
      coef(0)= 1.0;

      pb_pointer_p = new PBMath1DIPoly( coef, Quantity(180,"deg"), Quantity(1.0,"GHz"),
					False,
					BeamSquint(MDirection(Quantity(0.0, "'"),
							      Quantity(0.0, "'"),
							      MDirection::Ref(MDirection::AZEL)),
						   Quantity(1.0, "GHz")),
				        False);
     }
    break;
  default:
    os << "Unrecognized CommonPB Type" << LogIO::EXCEPTION;
    break;
  }
  

};

} //# NAMESPACE CASA - END

