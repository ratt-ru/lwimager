//# AMueller.cc: Implementation of AMueller
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

#include <calibration/CalTables/CalDescColumns2.h>
//#include <ms/MeasurementSets/MSColumns.h>
#include <msvis/MSVis/VisBuffer.h>
#include <msvis/MSVis/VisBuffGroupAcc.h>
#include <msvis/MSVis/VBContinuumSubtractor.h>
#include <synthesis/MeasurementComponents/CalCorruptor.h>
#include <synthesis/MeasurementComponents/AMueller.h>
#include <synthesis/MeasurementEquations/VisEquation.h>

namespace casa { //# NAMESPACE CASA - BEGIN


// **********************************************************
//  AMueller
//


AMueller::AMueller(VisSet& vs) :
  VisCal(vs),             // virtual base
  VisMueller(vs),         // virtual base
  MMueller(vs),            // immediate parent
  fitorder_p(0),
  doSub_p(true),
  nCorr_p(-1)
{
  if (prtlev()>2) cout << "A::A(vs)" << endl;

  init();
}

AMueller::AMueller(const Int& nAnt) :
  VisCal(nAnt),
  VisMueller(nAnt),
  MMueller(nAnt),            // immediate parent
  fitorder_p(0),
  doSub_p(true),
  nCorr_p(-1)
{
  if (prtlev()>2) cout << "A::A(nAnt)" << endl;

  init();
}

AMueller::~AMueller() {
  if (prtlev()>2) cout << "A::~A()" << endl;
}

void AMueller::init()
{
  lofreq_p.resize(nSpw());
  hifreq_p.resize(nSpw());
  totnumchan_p.resize(nSpw());
  spwApplied_p.resize(nSpw());
  
  lofreq_p = -1.0;
  hifreq_p = -1.0;
  totnumchan_p = 0;
  spwApplied_p = False;
}

void AMueller::setSolve(const Record& solvepar) {

  // Call parent
  MMueller::setSolve(solvepar);

  // Extract the AMueller-specific solve parameters
  if(solvepar.isDefined("fitorder"))
    fitorder_p = solvepar.asInt("fitorder");

  nChanParList() = fitorder_p + 1;  // Orders masquerade as output chans.

  // Override preavg 
  // (solver will fail if we don't average completely in each solint)
  preavg()=DBL_MAX;
}

void AMueller::setSolveChannelization(VisSet& vs)
{
  Vector<Int> startDatChan(vs.startChan());

  // If fitorder_p != 0, this is frequency dependent.
  if(fitorder_p){
    // AMueller keeps its polynomial orders where channels would normally go,
    // and typically (advisedly) the number of orders is << the number of data
    // channels.  *Otherwise* the overall par shape follows the data shape.
    nChanParList() = fitorder_p + 1;  // Deja vu from setSolve().
    startChanList() = startDatChan;

    // However, for solving, we will only consider one channel at a time:
    nChanMatList() = 1;
  }
  else {
    // Pars are not themselves channel-dependent
    nChanParList() = 1;

    // Check if matrices may still be freq-dep:
    if (freqDepMat()) {
      // cal is an explicit f(freq) (e.g., like delay)
      nChanMatList()  = vs.numberChan();
      startChanList() = startDatChan;
    } else {
      // cal has no freq dep at all
      nChanMatList()  = Vector<Int>(nSpw(),1);
      startChanList() = Vector<Int>(nSpw(),0);
    }
  }

  // At this point:
  //  1. nChanParList() represents the number of coefficients per polynomial,
  //     appropriate for shaping the CalSet.
  //  2. nChanMatList() represents the per-Spw matrix channel axis length to
  //     be used during the solve, independent of the parameter channel
  //     axis length.  In the solve context, nChanMat()>1 when there is
  //     more than one channel of data upon which the (single channel)
  //     solve parameters depend (e.g. polynomial order != 1)
}

Int AMueller::sizeUpSolve(VisSet& vs, Vector<Int>& nChunkPerSol)
{
  sortVisSet(vs);
  VisIter& vi(vs.iter());

  nCorr_p = fitorder_p ? vi.nCorr() : 2;

  // Would SolvableVisCal be better here?  After all the need for this
  // specialization started with MMueller::nPar()...
  return MMueller::sizeUpSolve(vs, nChunkPerSol);
}

void AMueller::selfSolveOne(VisBuffGroupAcc& vbga)
{
  // Solver for the polynomial continuum fit.  It is overkill for fitorder_p ==
  // 0, and not used in that case.

  LogIO os(LogOrigin("AMueller", "selfSolveOne()", WHERE));
  VBContinuumSubtractor vbcs;

  // Initialize
  if(lofreq_p[currSpw()] < 0.0){  // 1st time for this spw, so let vbga
    vbcs.initFromVBGA(vbga);      // provide the info.
    lofreq_p[currSpw()] = vbcs.getLowFreq();
    hifreq_p[currSpw()] = vbcs.getHighFreq();
    totnumchan_p[currSpw()] = vbcs.getTotNumChan();
  }
  else                            // Reuse the prev vals for consistency.
    vbcs.init(solveParOK().shape(), nAnt() - 1, totnumchan_p[currSpw()],
              lofreq_p[currSpw()], hifreq_p[currSpw()]);

  vbcs.fit(vbga, fitorder_p, MS::DATA, solveCPar(), solveParOK(),
           false, false, !append());
}

void AMueller::store()
{
  MMueller::store();
  if(fitorder_p != 0){    // Store lofreq_p[currSpw()] and hifreq_p[currSpw()]
    // Open the caltable
    CalTable2 calTable(calTableName(), Table::Update);
    CalDescColumns2 cd(calTable);

    // Why is this a Matrix instead of a Vector?
    Matrix<Double> lhfreqs(1, 2);

    // If combspw(), cd will only have one row.  Only store lhfreqs for spws
    // that have rows in cd.
    uInt nOutSpws = 0;

    for(Int cspw = 0; cspw < nSpw(); ++cspw){
      if(nSlots(cspw) > 0){
        lhfreqs(0, 0) = lofreq_p[cspw];
        lhfreqs(0, 1) = hifreq_p[cspw];

        // Storing lo and hifreq_p in chanFreq (suggested by George
        // Moellenbrock) is a hack, but it does not seem to be otherwise used,
        // and it avoids more serious mucking with the caltable.
        cd.chanFreq().put(nOutSpws, lhfreqs);
        ++nOutSpws;
      }
    }
  }
}

void AMueller::hurl(const String& origin, const String& msg)
{
  LogIO os(LogOrigin("AMueller", origin, WHERE));
  
  os << msg << LogIO::EXCEPTION;
}

void AMueller::setApply(const Record& applypar)
{
  LogIO os(LogOrigin("AMueller", "setApply()", WHERE));

  if(applypar.isDefined("table")){
    calTableName() = applypar.asString("table");
    verifyCalTable(calTableName());

    // Get nPar().
    CalTable2 caltab(calTableName());
    ROSolvableCalSetMCol<Complex> svjmcols(caltab);
    nCorr_p = svjmcols.gain().shape(0)[0];
  }
  else
    os << "AMueller::setApply(Record&) needs the record to have a table"
       << LogIO::EXCEPTION;

  MMueller::setApply(applypar);

  fitorder_p = nChanPar() - 1;

  if(fitorder_p != 0){
    // Open the caltable to get the scaling frequencies.
    CalTable2 calTable(calTableName());
    CalDescColumns2 cd(calTable);

    // if(cd.chanFreq()) is a valid column...
    if(!cd.chanFreq().isNull() && cd.chanFreq().isDefined(0) &&
       cd.chanFreq().shape(0)[0] > 0 && cd.chanFreq().shape(0)[1] > 0){
      // Why is this a Matrix instead of a Vector?
      Matrix<Double> lhfreqs(1, 2);
      uInt nrows = cd.chanFreq().nrow();

      lofreq_p.resize(nSpw());
      hifreq_p.resize(nSpw());

      if(nrows == 1){   // Use a single set of scaling frequencies for all
                        // spws.
        // -999 is the secret code for combspw() == True.
        // nSpw() != 1 && (applypar.asArrayInt("spwmap")[0] != -999)
        // seems to be flustered by overloaded overloading ambiguity.
        // Break it down.
        Vector<Int> spwmap(applypar.asArrayInt("spwmap"));
        if(nSpw() != 1 && spwmap[0] != -999)
          os << LogIO::WARN
             << "There is > 1 spw but only one set of scaling frequencies,"
             << "\nand applypar did not call for combining spws."
             << "The same scaling frequencies will be applied to all spws."
             << LogIO::POST;

        cd.chanFreq().get(0, lhfreqs);

        for(Int cspw = 0; cspw < nSpw(); ++cspw){
          lofreq_p[cspw] = lhfreqs(0, 0);
          hifreq_p[cspw] = lhfreqs(0, 1);
        }
      }
      else{
        Bool allvalidspws = True;  // I think this might be already assured.

        for(uInt row = 0; row < nrows; ++row){
          Int cspw = spwMap()[row];

          if(cspw > 0 && cspw < nSpw()){
            // Storing lo and hifreq_p in chanFreq (suggested by George
            // Moellenbrock) is a hack, but it does not seem to be otherwise
            // used, and it avoids more serious mucking with the caltable.
            cd.chanFreq().get(row, lhfreqs);
          
            lofreq_p[cspw] = lhfreqs(0, 0);
            hifreq_p[cspw] = lhfreqs(0, 1);
          }
          else
            allvalidspws = False;
        }
        if(!allvalidspws)
          os << LogIO::WARN
             << "The caltable pointed to some entries in the spwmap that do"
             << "\nnot point to spws in the MS."
             << LogIO::POST;
      }
    }
    else{
      os << LogIO::WARN
         << "CHAN_FREQ was not found in the caltable...setting fitorder to 0"
         << LogIO::POST;
      fitorder_p = 0;
    }
  }
}

// Apply this calibration to VisBuffer visibilities
void AMueller::applyCal(VisBuffer& vb, Cube<Complex>& Vout,
                        Bool avoidACs)
{
  LogIO os(LogOrigin("AMueller", "applyCal()", WHERE));

  if(fitorder_p == 0){
    VisMueller::applyCal(vb, Vout, avoidACs && false);
  }
  else{
    if(prtlev() > 3)
      os << "  AMueller::applyCal()" << LogIO::POST;

    Int cspw = currSpw();
    VBContinuumSubtractor vbcs;
    vbcs.init(currCPar().shape(), nAnt() - 1, totnumchan_p[cspw],
              lofreq_p[cspw], hifreq_p[cspw]);

    // correct() writes to vb.visCube(), not vb.correctedVisCube()!  I don't
    // get this...
    // MS::PredefinedColumns whichcol = MS::CORRECTED_DATA;
    MS::PredefinedColumns whichcol = MS::DATA;
   

    if(!vbcs.apply(vb, whichcol, currCPar(), currParOK(), doSub_p,
                   !spwApplied_p[cspw]))
      throw(AipsError("Could not place the continuum-subtracted data in "
                      + MS::columnName(whichcol)));
    spwApplied_p[cspw] = True;
  }
}

void AMueller::corrupt(VisBuffer& vb)
{
  LogIO os(LogOrigin("AMueller", "corrupt()", WHERE));

  if(prtlev() > 3)
    os << LogIO::NORMAL << "  A::corrupt()" << LogIO::POST;

  // Initialize model data to zero, so corruption contains
  //  only the AMueller solution
  //  TBD: may wish to make this user togglable.
  vb.setModelVisCube(Complex(0.0));

  if(fitorder_p == 0){
    // Call general version:
    VisMueller::corrupt(vb);
  }
  else{
    // Ensure weight calibration off internally for corrupt
    //   (corruption doesn't re-scale the data!)
    Bool userCalWt=calWt();
    calWt()=False;

    // Bring calibration up-to-date with the vb, with inversion turned OFF
    syncCal(vb,False);

    Int cspw = currSpw();
    VBContinuumSubtractor vbcs;
    vbcs.init(currCPar().shape(), nAnt() - 1, totnumchan_p[cspw],
              lofreq_p[cspw], hifreq_p[cspw]);

    MS::PredefinedColumns whichcol = MS::MODEL_DATA;
    
    if(!vbcs.apply(vb, whichcol, currCPar(), currParOK(), false,
                   !spwApplied_p[cspw]))
      throw(AipsError("Could not place the continuum estimate in "
                      + MS::columnName(whichcol)));
    spwApplied_p[cspw] = True;
    // Restore user's calWt()
    calWt()=userCalWt; 
  }

  // DEBUGGING
  //uInt nchan = vb.nChannel();
  uInt nvbrow = vb.nRow();
  for(uInt vbrow = 0; vbrow < nvbrow; ++vbrow){
    if(!vb.flagCube()(0, 0, vbrow) && !vb.flagRow()[vbrow] &&
       vb.modelVisCube()(0, 0, vbrow).real() > 3.95)
      os << LogIO::WARN
         << vbrow << ": " << vb.modelVisCube()(0, 0, vbrow)
         << LogIO::POST;
  }
}

void ANoise::createCorruptor(const VisIter& vi, const Record& simpar, const Int nSim)
{
  if (prtlev()>2) cout << " AN::createCorruptor()" << endl;
  AlwaysAssert((isSimulated()),AipsError);

  acorruptor_p = new ANoiseCorruptor();
  corruptor_p = acorruptor_p;

  // call generic parent to set corr,spw,etc info
  SolvableVisCal::createCorruptor(vi,simpar,nSim);

  Int Seed(1234);
  if (simpar.isDefined("seed")) {    
    Seed=simpar.asInt("seed");
  }

  Float Amp(1.0);
  if (simpar.isDefined("amplitude")) {    
    Amp=simpar.asFloat("amplitude");
  }

  acorruptor_p->initialize(Seed,Amp);

  String Mode("calc"); // calc means multiply by 1/sqrt(dnu dt)
  if (simpar.isDefined("mode")) {    
    Mode=simpar.asString("mode");
  }

  acorruptor_p->mode()=Mode;

  if (prtlev()>2) cout << " ~AN::createCorruptor()" << endl;

}


ANoise::ANoise(VisSet& vs) :
  VisCal(vs),             // virtual base
  VisMueller(vs),         // virtual base
  SolvableVisMueller(vs)  // immediate parent
{
  if (prtlev()>2) cout << "ANoise::ANoise(vs)" << endl;
}

ANoise::ANoise(const Int& nAnt) :
  VisCal(nAnt),
  VisMueller(nAnt),
  SolvableVisMueller(nAnt)
{
  if (prtlev()>2) cout << "ANoise::ANoise(nAnt)" << endl;
}

ANoise::~ANoise() {
  if (prtlev()>2) cout << "ANoise::~ANoise()" << endl;
}



void ANoise::calcOneMueller(Vector<Complex>& mat, Vector<Bool>& mOk,
			    const Vector<Complex>& par, const Vector<Bool>& pOk) {
  
  if(prtlev() > 10)
    cout << "        AN::calcOneMueller()\n"
         << "par: " << par                     // These are more to remove a compiler
         << "\npOk: " << pOk                   // warning (par & pOK unused)
         << endl;                              // than anything.

  // If Mueller matrix is trivial, shouldn't get here
  if (trivialMuellerElem()) 
    throw(AipsError("Trivial Mueller Matrix logic error."));
  else {
    Int len=0;
    mat.shape(len);
    for (Int i=0; i<len; i++) {
      mat[i]=acorruptor_p->simPar(); // single complex #
      mOk[i]=True;
    }    
  }
}



} //# NAMESPACE CASA - END
