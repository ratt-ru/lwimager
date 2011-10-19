//# StandardVisCal.h: Declaration of standard (Solvable)VisCal types
//# Copyright (C) 1996,1997,2000,2001,2002,2003,2011
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

#ifndef SYNTHESIS_STANDARDVISCAL_H
#define SYNTHESIS_STANDARDVISCAL_H

#include <casa/aips.h>
#include <casa/Containers/Record.h>
#include <casa/BasicSL/Complex.h>
#include <synthesis/MeasurementComponents/VisCal.h>
#include <synthesis/MeasurementComponents/SolvableVisCal.h>
#include <synthesis/MeasurementComponents/CalCorruptor.h>

namespace casa { //# NAMESPACE CASA - BEGIN

// Forward declarations
class VisEquation;
class TJonesCorruptor;


// **********************************************************
//  PJones
//

class PJones : public VisJones {
public:

  // Constructor
  PJones(VisSet& vs);
  //   PJones(const Int& nAnt);  // NYI

  virtual ~PJones();

  // Return the type enum
  virtual Type type() { return VisCal::P; };

  // Return type name as string
  virtual String typeName()     { return "P Jones"; };
  virtual String longTypeName() { return "P Jones (parallactic angle phase)"; };

  // Type of Jones matrix according to nPar()
  Jones::JonesType jonesType() { return pjonestype_; };

protected:

  // P has one trivial Complex parameter
  virtual Int nPar() { return 1; };

  virtual Vector<Float>& pa() { return pa_; };

  // Jones matrix elements are NOT trivial
  virtual Bool trivialJonesElem() { return False; };

  // Calculate parallactic angle
  virtual void calcPar();

  // We need to locally form the Jones according to 
  //  the correlations we have
  virtual void syncJones(const Bool& doInv);

  // Calculate a single PJones matrix 
  virtual void calcOneJones(Vector<Complex>& mat, Vector<Bool>& mOk,
			    const Vector<Complex>& par, const Vector<Bool>& pOk );

private:

  Jones::JonesType pjonestype_;

  Vector<Float> pa_;
  
};











// **********************************************************
//  TJones
//

class TJones : public SolvableVisJones {
public:

  // Constructor
  TJones(VisSet& vs);
  TJones(const Int& nAnt);

  virtual ~TJones();

  // Return the type enum
  virtual Type type() { return VisCal::T; };

  // Return type name as string
  virtual String typeName()     { return "T Jones"; };
  virtual String longTypeName() { return "T Jones (polarization-independent troposphere"; };

  // Type of Jones matrix according to nPar()
  virtual Jones::JonesType jonesType() { return Jones::Scalar; };

  // This type is accumulatable
  virtual Bool accumulatable() { return True; };

  // This type is smoothable
  virtual Bool smoothable() { return True; };

  // Hazard a guess at parameters
  virtual void guessPar(VisBuffer& vb);

  // Set up corruptor
  virtual void createCorruptor(const VisIter& vi, const Record& simpar, const Int nSim);

protected:

  // T has one trivial complex parameter
  virtual Int nPar() { return 1; };

  // Jones matrix elements are trivial
  virtual Bool trivialJonesElem() { return True; };

  // Are diff'd Js trivial?
  virtual Bool trivialDJ() { return True; };

  // Initialize trivial dJs
  virtual void initTrivDJ();

private:

  // object that can simulate the corruption terms - internal to T;
  // public access is only to the CalCorruptor parts
  AtmosCorruptor *tcorruptor_p;

};










// **********************************************************
//  TfJones (freq-dep T)
//

class TfJones : public TJones {
public:

  // Constructor
  TfJones(VisSet& vs);
  TfJones(const Int& nAnt);

  virtual ~TfJones();

  // Return the type enum
  virtual Type type() { return VisCal::T; };

  // Return type name as string
  virtual String typeName()     { return "Tf Jones"; };
  virtual String longTypeName() { return "Tf Jones (frequency-dependent atmospheric complex gain"; };

  // This is the freq-dep version of T
  //   (this is the ONLY fundamental difference from T)
  virtual Bool freqDepPar() { return True; };

protected:

  // <nothing>

private:

  // <nothing>
  
};





// **********************************************************
//  GJones
//

class GJones : public SolvableVisJones {
public:

  // Constructor
  GJones(VisSet& vs);
  GJones(const Int& nAnt);

  virtual ~GJones();

  // Return the type enum
  virtual Type type() { return VisCal::G; };

  // Return type name as string
  virtual String typeName()     { return "G Jones"; };
  virtual String longTypeName() { return "G Jones (electronic Gain)"; };

  // Type of Jones matrix according to nPar()
  virtual Jones::JonesType jonesType() { return Jones::Diagonal; };

  // This type is accumulatable
  virtual Bool accumulatable() { return True; };

  // This type is smoothable
  virtual Bool smoothable() { return True; };

  // Nominally, we will only use parallel hands for now
  virtual Bool phandonly() { return True; };

  // Hazard a guess at parameters
  virtual void guessPar(VisBuffer& vb);

  virtual void createCorruptor(const VisIter& vi, const Record& simpar, const Int nSim);

protected:

  // G has two trivial Complex parameters
  virtual Int nPar() { return 2; };

  // Jones matrix elements are trivial
  virtual Bool trivialJonesElem() { return True; };

  // dG/dp are trivial
  virtual Bool trivialDJ() { return True; };

  // Initialize trivial dJs
  virtual void initTrivDJ();

private:

  GJonesCorruptor *gcorruptor_p;
  
};



// **********************************************************
//  BJones  (freq-dep GJones)
//

class BJones : public GJones {
public:

  // Constructor
  BJones(VisSet& vs);
  BJones(const Int& nAnt);

  virtual ~BJones();

  // Return the type enum
  virtual Type type() { return VisCal::B; };

  // Return type name as string
  virtual String typeName()     { return "B Jones"; };
  virtual String longTypeName() { return "B Jones (bandpass)"; };

  // Local setSolve
  using GJones::setSolve;
  virtual void setSolve(const Record& solve);

  // This is the freq-dep version of G 
  //   (this is the ONLY fundamental difference from G)
  virtual Bool freqDepPar() { return True; };

  virtual void normalize();

  // Specialize post solve operations (adds chan gap fill)
  virtual void globalPostSolveTinker();
  
  // Fill in bracketted channel gaps in solutions
  virtual void fillChanGaps();
  virtual void fillChanGapArray(Array<Complex>& sol,
				Array<Bool>& solOK);

protected:

  // <nothing>

private:

  // widest channel gap to fill by interpolation
  Int maxchangap_p;


};


// **********************************************************
//  DJones
//

class DJones : public SolvableVisJones {
public:

  // Constructor
  DJones(VisSet& vs);
  DJones(const Int& nAnt);

  virtual ~DJones();

  // Local setapply
  using SolvableVisJones::setApply;
  virtual void setApply(const Record& apply);

  // D-specific solve setup
  using SolvableVisJones::setSolve;
  void setSolve(const Record& solvepar);

  // Return the type enum
  virtual Type type() { return VisCal::D; };

  // Return type name as string
  virtual String typeName()     { return "Dgen Jones"; };
  virtual String longTypeName() { return "Dgen Jones (instrumental polarization"; };

  // Type of Jones matrix according to nPar()
  //   Do GENERAL matrix algebra
  virtual Jones::JonesType jonesType() { return Jones::General; };

  // We can solve for polarization with D
  virtual Int solvePol() { return solvePol_; };

  // Hazard a guess at parameters
  virtual void guessPar(VisBuffer& vb);

  // Update the parameters from solving
  //  (in linear approx, we always set the source update to zero, for now!)
  virtual void updatePar(const Vector<Complex> dCalPar,
			 const Vector<Complex> dSrcPar);

  // SNR is 1/err for D-terms (?)
  virtual void formSolveSNR();

  // D-specific post-solve stuff
  virtual void globalPostSolveTinker();

  // D-specific reReference
  // TBD: non-triv impl
  virtual void reReference() { cout << "reReference!" << endl;};

  virtual void applyRefAnt();

  // Method to list the D results
  virtual void logResults();

  virtual void createCorruptor(const VisIter& vi, 
			       const Record& simpar, 
			       const Int nSim);
protected:

  // D has two Complex parameters
  virtual Int nPar() { return 2; };

  // Jones matrix elements are trivial?
  //  True if GenLinear, False if General
  virtual Bool trivialJonesElem() { return (jonesType()==Jones::GenLinear); };  
  // dD/dp are trivial
  virtual Bool trivialDJ() { return True; };

  // Non-trivial Jones matrix calculation
  virtual void calcOneJones(Vector<Complex>& mat, Vector<Bool>& mOk,
			    const Vector<Complex>& par, const Vector<Bool>& pOk);

  // Initialize trivial dJs
  virtual void initTrivDJ();

private:

  Int solvePol_;
  DJonesCorruptor *dcorruptor_p;
  

  
};

// **********************************************************
//  DfJones (freq-dep D)  (general)
//

class DfJones : public DJones {
public:

  // Constructor
  DfJones(VisSet& vs);
  DfJones(const Int& nAnt);

  virtual ~DfJones();

  // Return type name as string
  virtual String typeName()     { return "Dfgen Jones"; };
  virtual String longTypeName() { return "Dfgen Jones (frequency-dependent instrumental polarization"; };

  // This is the freq-dep version of D 
  //   (this is the ONLY fundamental difference from D)
  virtual Bool freqDepPar() { return True; };
  
};



// **********************************************************
//  DlinJones   (linearized DJones)
//

class DlinJones : public DJones {
public:

  // Constructor
  DlinJones(VisSet& vs);
  DlinJones(const Int& nAnt);

  virtual ~DlinJones();

  // Return type name as string
  virtual String typeName()     { return "D Jones"; };
  virtual String longTypeName() { return "D Jones (instrumental polarization"; };

  // Type of Jones matrix according to nPar()
  //  Do linearized matrix algebra
  virtual Jones::JonesType jonesType() { return Jones::GenLinear; };

};

// **********************************************************
//  DflinJones (freq-dep, linearized DJones)
//

class DflinJones : public DlinJones {
public:

  // Constructor
  DflinJones(VisSet& vs);
  DflinJones(const Int& nAnt);

  virtual ~DflinJones();

  // Return type name as string
  virtual String typeName()     { return "Df Jones"; };
  virtual String longTypeName() { return "Df Jones (frequency-dependent instrumental polarization"; };

  // This is the freq-dep version of D 
  //   (this is the ONLY fundamental difference from D)
  virtual Bool freqDepPar() { return True; };

};


// **********************************************************
//  JJones
//

class JJones : public SolvableVisJones {
public:

  // Constructor
  JJones(VisSet& vs);
  JJones(const Int& nAnt);

  virtual ~JJones();

  // J-specific solve setup
  using SolvableVisJones::setSolve;
  void setSolve(const Record& solvepar);

  // Return the type enum
  virtual Type type() { return VisCal::J; };

  // Return type name as string
  virtual String typeName()     { return "J Jones"; };
  virtual String longTypeName() { return "J Jones (generic polarized gain)"; };

  // Type of Jones matrix according to nPar()
  virtual Jones::JonesType jonesType() { return Jones::General; };

  // Hazard a guess at parameters
  virtual void guessPar(VisBuffer& vb);

  // J-specific reReference
  // TBD: non-triv impl
  virtual void reReference() {};

protected:

  // J has four trivial Complex parameters
  virtual Int nPar() { return 4; };

  // Jones matrix elements are trivial
  virtual Bool trivialJonesElem() { return True; };

  // dJ/dp are trivial
  virtual Bool trivialDJ() { return True; };

  // Initialize trivial dJs
  virtual void initTrivDJ();

private:

  // <nothing>
  
};


// **********************************************************
//  M: baseline-based (closure) 
//


class MMueller : public SolvableVisMueller {
public:

  // Constructor
  MMueller(VisSet& vs);
  MMueller(const Int& nAnt);

  virtual ~MMueller();

  // Return the type enum
  virtual Type type() { return VisCal::M; };

  // Return type name as string
  virtual String typeName()     { return "M Mueller"; };
  virtual String longTypeName() { return "M Mueller (baseline-based)"; };

  // Type of Jones matrix according to nPar()
  virtual Mueller::MuellerType muellerType() { return Mueller::Diag2; };

  // Local setApply
  using SolvableVisCal::setApply;
  virtual void setApply(const Record& apply);

  // M gathers/solves for itself
  virtual Bool useGenericGatherForSolve() { return False; };

  // M solves for itself (by copying averaged data)
  virtual void selfGatherAndSolve(VisSet& vs, VisEquation& ve) { newselfSolve(vs,ve); };
  virtual void oldselfSolve(VisSet& vs, VisEquation& ve);  // old-fashioned iterator-driven
  virtual void newselfSolve(VisSet& vs, VisEquation& ve);  // new supports combine

  // Local M version only supports normalization
  virtual void globalPostSolveTinker();

  // (At least?) one of the reasons it uses its own keep() is to store 0 to
  // nChanPar() - 1 of solveCPar() on the channel axis, as opposed to just
  // focusChan(). 
  virtual void keep(const Int& slot);

  virtual void createCorruptor(const VisIter& vi, const Record& simpar, const Int nSim);
protected:

  // M currently has just 2 complex parameters, i.e., both parallel hands
  virtual Int nPar() { return 2; };

  // Jones matrix elements are trivial
  virtual Bool trivialMuellerElem() { return True; };

private:
  AtmosCorruptor *atmcorruptor_p;

};






// **********************************************************
//  MfMueller (freq-dep MMueller)
//

class MfMueller : public MMueller {
public:

  // Constructor
  MfMueller(VisSet& vs);
  MfMueller(const Int& nAnt);

  virtual ~MfMueller();

  // Return the type enum
  virtual Type type() { return VisCal::M; };

  // Return type name as string
  virtual String typeName()     { return "Mf Mueller"; };
  virtual String longTypeName() { return "Mf Mueller (closure bandpass)"; };

  // This is the freq-dep version of M
  //   (this is the ONLY fundamental difference from M)
  virtual Bool freqDepPar() { return True; };

  // Normalize baseline spectra (after B)
  virtual void normalize();

protected:

  // <nothing>

private:

  // <nothing>

};




// **********************************************************
//  TOPac 
//

class TOpac : public TJones {
public:

  // Constructor
  TOpac(VisSet& vs);
  //  TOpac(const Int& nAnt);  // NYI

  virtual ~TOpac();

  // Return the type enum
  virtual Type type() { return VisCal::T; };

  // Return type name as string
  virtual String typeName()     { return "TOpac"; };
  virtual String longTypeName() { return "TOpac (Opacity corrections in amplitude)"; };

  // Type of Jones matrix according to nPar()
  Jones::JonesType jonesType() { return Jones::Scalar; };

  // Local specialization of setApply to extract opacity
  void setApply(const Record& applypar);

  // Report apply-related info
  String applyinfo();

protected:

  // TOpac has one Float par per ant
  virtual Int nPar() { return 1; };

  // Jones matrix elements are NOT trivial
  virtual Bool trivialJonesElem() { return False; };

  // Access to z.a. data
  Vector<Double>& za() { return za_; };

  // Calculate parameters (in this case, the z.a.)
  virtual void calcPar();

  // Calculate the TOpac matrix for all ants
  virtual void calcAllJones();

private:

  // avoid compiler warnings
  using TJones::setApply;  

  Float opacity_;

  Vector<Double> za_;
  
};


// **********************************************************
//  TfOpac (freq-dep TOpac)
//

class TfOpac : public TOpac {
public:

  // Constructor
  TfOpac(VisSet& vs);
  //  TfOpac(const Int& nAnt);

  virtual ~TfOpac();

  // Return the type enum
  virtual Type type() { return VisCal::T; };

  // Return type name as string
  virtual String typeName()     { return "TfOpac"; };
  virtual String longTypeName() { return "TfOpac (frequency-dependent opacity"; };

  // This is the freq-dep version of TOpac
  //   (this is the ONLY fundamental difference from TOpac)
  virtual Bool freqDepPar() { return True; };

protected:

  // <nothing>

private:

  // <nothing>
  
};






// **********************************************************
//  X: position angle calibration (for circulars!)
//    (rendered as a Mueller for now)

class XMueller : public SolvableVisMueller {
public:

  // Constructor
  XMueller(VisSet& vs);
  XMueller(const Int& nAnt);

  virtual ~XMueller();

  // Return the type enum
  virtual Type type() { return VisCal::X; };

  // Return type name as string
  virtual String typeName()     { return "X Mueller"; };
  virtual String longTypeName() { return "X Mueller (baseline-based)"; };

  // Type of Jones matrix according to nPar()
  virtual Mueller::MuellerType muellerType() { return Mueller::Diagonal; };

  // Local setApply
  using SolvableVisCal::setApply;
  virtual void setApply(const Record& apply);

  // Local setSolve
  using SolvableVisCal::setSolve;
  void setSolve(const Record& solvepar);

  // Turn off normalization by model....
  virtual Bool normalizable() { return False; };

  // X gathers/solves for itself
  virtual Bool useGenericGatherForSolve() { return False; };

  // X gathers/solves for itself 
  virtual void selfGatherAndSolve(VisSet& vs, VisEquation& ve) { newselfSolve(vs,ve); };
  virtual void oldselfSolve(VisSet& vs, VisEquation& ve);  // old-fashioned iterator-driven
  virtual void newselfSolve(VisSet& vs, VisEquation& ve);  // new supports combine

  virtual void keep(const Int& slot);

protected:

  // X has just 1 complex parameter, storing a phase
  virtual Int nPar() { return 1; };

  // Jones matrix elements are trivial
  virtual Bool trivialMuellerElem() { return False; };

  // Calculate the X matrix for all ants
  virtual void calcAllMueller();

  // Solve in one VB for the position angle
  virtual void solveOneVB(const VisBuffer& vb);

private:

  // <nothing>

};



// **********************************************************
//  X: position angle calibration (for circulars!)
//
class XJones : public SolvableVisJones {
public:

  // Constructor
  XJones(VisSet& vs);
  XJones(const Int& nAnt);

  virtual ~XJones();

  // Return the type enum
  virtual Type type() { return VisCal::X; };

  // Return type name as string
  virtual String typeName()     { return "X Jones"; };
  virtual String longTypeName() { return "X Jones (antenna-based)"; };

  // Type of Jones matrix according to nPar()
  virtual Jones::JonesType jonesType() { return Jones::Diagonal; };

  // Local setApply
  using SolvableVisCal::setApply;
  virtual void setApply(const Record& apply);

  // Local setSolve
  using SolvableVisCal::setSolve;
  void setSolve(const Record& solvepar);

  // X is normalizable by the model
  virtual Bool normalizable() { return True; };

  // X gathers/solves for itself
  virtual Bool useGenericGatherForSolve() { return False; };

  // X gathers/solves for itself 
  virtual void selfGatherAndSolve(VisSet& vs, VisEquation& ve) { newselfSolve(vs,ve); };
  virtual void newselfSolve(VisSet& vs, VisEquation& ve);  // new supports combine

  virtual void keep(const Int& slot);

protected:

  // X has just 1 complex parameter, storing a phase
  virtual Int nPar() { return 1; };

  // Jones matrix elements are trivial
  virtual Bool trivialJonesElem() { return False; };

  // Calculate the X matrix for all ants
  virtual void calcAllJones();

  // Solve in one VB for the position angle
  virtual void solveOneVB(const VisBuffer& vb);

private:

  // <nothing>

};


// **********************************************************
//  Xf: position angle calibration (for circulars!)
//     (channel-dependent)
class XfJones : public XJones {
public:

  // Constructor
  XfJones(VisSet& vs);
  XfJones(const Int& nAnt);

  virtual ~XfJones();

  // Return the type enum
  virtual Type type() { return VisCal::X; };

  // Return type name as string
  virtual String typeName()     { return "Xf Jones"; };
  virtual String longTypeName() { return "Xf Jones (antenna-based)"; };

  // This is the freq-dep version of X 
  //   (this is the ONLY fundamental difference from X)
  virtual Bool freqDepPar() { return True; };

protected:

  // Use nchan>=1 shaping
  //  (TBD: this should be generalized!)
  void initSolvePar();


};



// X-Y phase 
class GlinXphJones : public GJones {
public:

  // Constructor
  GlinXphJones(VisSet& vs);
  GlinXphJones(const Int& nAnt);

  virtual ~GlinXphJones();

  // Return type name as string
  virtual String typeName()     { return "GlinXph Jones"; };
  virtual String longTypeName() { return "GlinXph Jones (X-Y phase)"; };

  // Though derived from GJones, this type actually uses the cross-hands
  virtual Bool phandonly() { return False; };

  // GlinXphJones gathers/solves for itself
  virtual Bool useGenericGatherForSolve() { return False; };
  virtual void selfGatherAndSolve(VisSet& vs, VisEquation& ve);

protected:

  // FFT solver for on VB, that collapses baselines and cross-hands first
  virtual void solveOneVB(const VisBuffer& vb);

};



} //# NAMESPACE CASA - END

#endif

