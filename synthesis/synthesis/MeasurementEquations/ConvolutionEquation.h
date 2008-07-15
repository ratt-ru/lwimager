//# ConvolutionEquation.h: this defines ConvolutionEquation
//# Copyright (C) 1996,1997,1999,2000,2003
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
//#
//# $Id: ConvolutionEquation.h,v 19.4 2004/11/30 17:50:59 ddebonis Exp $

#ifndef SYNTHESIS_CONVOLUTIONEQUATION_H
#define SYNTHESIS_CONVOLUTIONEQUATION_H


#include <casa/aips.h>
#include <synthesis/MeasurementEquations/LinearEquation.h>
#include <synthesis/MeasurementEquations/LinearModel.h>
#include <casa/Arrays/MaskedArray.h>
#include <scimath/Mathematics/Convolver.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/IPosition.h>

namespace casa { //# NAMESPACE CASA - BEGIN

// <summary> Implements the convolution equation </summary>

// <use visibility=local>

// <reviewed reviewer="" date="yyyy/mm/dd" tests="" demos="">
// </reviewed>

// <prerequisite> 
// <li> <linkto class="HogbomCleanModel">HogbomCleanModel</linkto> 
//       (or similar classes) 
// <li> LinearModel/LinearEquation Paradigm 
// </prerequisite>
//
// <etymology>
// This class implements convolution within the LinearEquation framework.
// </etymology>
//
// <synopsis>
// This class is used in conjunction with classes like HogbomCleanModel to
// implement deconvolution algorithms. This class contains the point spread
// function (psf) and the convolved data (dirty image), and is able to
// convolve a supplied model with the psf to produce a predicted output
// (using the evaluate() function), or to subtract the convolved data and
// produce a residual (using the residual() function).
//
// See the documentation for 
// <linkto class=HogbomCleanModel>HogbomCleanModel</linkto> 
// for an example of how this class can be used to perform deconvolution.
//
// This class also contains specialised functions (like the version of
// evaluate() for a point source model) that speed up the calculation of the
// convolution. This specialised version of evaluate() does not need to
// actually perform the convolution and instead returns a suitable part of
// the psf (zero padded if necessary). When this function is called this
// class will get the psf from the convolver and cache it, on the assumption
// that many evaluations of this function will be requested (as occurs in
// Clean algorithms). 
// 
// The size and shape of the psf and the supplied model may be different. The
// only restriction is that the dimension of the psf must be less than or
// equal to the dimension of the model. If the dimension of the
// model is larger than the dimension of the psf then the convolution
// will be repeated along the slowest moving (last) axis. The dirty image
// and the supplied model must be the same size and shape. 
//
// This class can also operate on MaskedArrays (and models representable by
// MaskedArrays). But the mask is currently discarded and the convolution
// performed on the entire supplied model. This may change in the future.
//
// </synopsis>
//
// <example>
// <srcblock>
// Matrix<Float> psf(4,4), dirty(20,20), model(20,20);
// .... put some meaningful values into these Arrays....
// // create a convolution equation, and an array model
// ConvolutionEquation convEqn(psf, dirty);
// ArrayModel<Float> myModel(model);
// // now calculate the convolution of the model and the psf
// Matrix<Float> prediction;
// convEqn.evaluate(myModel, prediction);
// // and calculate the difference between the predicted and actual convolution
// Matrix<Float> residual;
// convEqn.residual(mymodel, residual)
// </srcblock>
// </example>
//
// <motivation>
// This class was designed with deconvolution in mind. 
// </motivation>
//
// <todo asof="1990/05/03">
//   <li> Fix up the way this class works for other data types. 
//        It currently can handle Arrays of Floats only.
//        I do not intend to extend it to handle Double, Complex, & DComplex
//        until there is a demand.
//   <li> This class is not templated. If necessary I would use templating
//        to produce a Double Precision Version.
// </todo>

class ConvolutionEquation: 
  public LinearEquation< Array<Float>, Array<Float> >
{
public:
  // Construct the ConvolutionEquation. Until I write some functions for
  // setting the private data the default constructor is essentially useless
  ConvolutionEquation();

  // Construct the ConvolutionEquation setting the psf and measured data
  ConvolutionEquation(const Array<Float> & psf, 
		      const Array<Float> & dirtyImage);

  // Construct the ConvolutionEquation setting the psf and measured data
  // Even though a MaskedArray is used as an arguement the mask is discarded
  // internally and hence not used by residual().
  ConvolutionEquation(const Array<Float> & psf, 
		      const MaskedArray<Float> & dirtyImage);

  // Somewhere I read that a destructor should alway be defined even if it
  // does nothing (as this one does).
  ~ConvolutionEquation();
  
  // Do the convolution of the model supplied by the LinearModel class with
  // the internal psf. Return the answer in result .
  virtual Bool evaluate(Array<Float> & result, 
			const LinearModel< Array<Float> > & model);

  // Do the convolution of the model supplied by the LinearModel class with
  // the internal psf. Return the answer in result. This version 
  // uses Masked arrays. but the mask is currently discarded internally. 
  Bool evaluate(Array<Float> & result, 
		const LinearModel< MaskedArray<Float> > & model);

  // Do the convolution of the model supplied by the LinearModel class with
  // the internal psf. Return the answer in result. This version 
  // uses MaskedArrays,  but the mask is not currently used. However
  // the model mask is transfered to the result unchanged.
  Bool evaluate(MaskedArray<Float> & result, 
		const LinearModel< MaskedArray<Float> > & model);

  // Do the convolution of the a point source model at position 'position'
  // with amplitude 'amplitude' and the internal psf. Return the answer in
  // result. 
  Bool evaluate(Array<Float> & result, const IPosition & position, 
		const Float amplitude, 
		const IPosition & modelShape);

  // Calculate the convolution of the model (supplied by the LinearModel
  // class) and the psf and the difference between this and the supplied
  // (presumably measured) convolution.  
  virtual Bool residual(Array<Float> & result, 
			const LinearModel< Array<Float> > & model);


  // Calculate the convolution of the model (supplied by the LinearModel
  // class) and the psf and the difference between this and the supplied
  // (presumably measured) convolution.   Also return chisq.
  virtual Bool residual( Array<Float> & result, Float & chisq, 
                        const LinearModel< Array<Float> > & model);

  // Calculate the convolution of the model (supplied by the LinearModel
  // class) and the psf and the difference between this and the supplied
  // (presumably measured) convolution.   Also return chisq,
  // considering a mask image
  virtual Bool residual( Array<Float> & result, Float & chisq, 
			 Array<Float> & mask,
			 const LinearModel< Array<Float> > & model);

  // Calculate the convolution of the model (supplied by the LinearModel
  // class) and the psf and the difference between this and the supplied
  // (presumably measured) convolution. This version 
  // uses Masked arrays. but the mask is currently discarded internally.
  Bool residual(Array<Float> & result, 
		const LinearModel< MaskedArray<Float> > & model);

  // Calculate the convolution of the model (supplied by the LinearModel
  // class) and the psf and the difference between this and the supplied
  // (presumably measured) convolution. This version 
  // uses Masked arrays. but the mask is currently discarded in the
  // calculations and transfered unchanged from the model to the result.
  Bool residual(MaskedArray<Float> & result, 
		const LinearModel< MaskedArray<Float> > & model);

  // return the psf size used in the convolution. The returned size does not
  // include any zero padding  
  IPosition psfSize();
  // release the storage associated with the cached psf. The psf can 
  // however still be recovered from the Convolver object
  void flushPsf();

private:
  Array<Float> theMeas;
  Array<Float> thePsf;
  IPosition theRealPsfSize;
  IPosition thePsfOrigin;
  Convolver<Float> theConv;
};


} //# NAMESPACE CASA - END

#endif
