//# SubMS.h: this defines SubMS which creates a subset of an MS with some
//# transformation
//# Copyright (C) 1997,1998,1999,2000,2001,2003
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
//# $Id$
#include <ms/MeasurementSets/MeasurementSet.h>
#include <ms/MeasurementSets/MSColumns.h>
#include <ms/MeasurementSets/MSMainEnums.h>
//#include <msvis/MSVis/VisIterator.h>
#include <casa/aips.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Vector.h>
//#include <casa/Utilities/CountedPtr.h>
#include <map>
#include <vector>
#include <scimath/Mathematics/InterpolateArray1D.h>


#ifndef MSVIS_SUBMS_H
namespace casa { //# NAMESPACE CASA - BEGIN

#define MSVIS_SUBMS_H

// <summary>
// SubMS provides functionalities to make a subset of an existing MS
// </summary>

// <visibility=export>

// <reviewed reviewer="" date="yyyy/mm/dd" tests="" demos="">
// </reviewed>

// <prerequisite>
//   <li> MeasurementSet
// </prerequisite>
//
// <etymology>
// SubMS ...from the SUBset of an MS
// </etymology>
//
// <synopsis>
// The order of operations (as in ms::split()) is:
//      ctor
//      setmsselect
//      selectTime
//      makeSubMS
// </synopsis>

// These forward declarations are so the corresponding .h files don't have to
// be included in this .h file, but it's only worth it if a lot of other files
// include this file.
class MSSelection; // #include <ms/MeasurementSets/MSSelection.h>

  // These typedefs are necessary because a<b::c> doesn't work.
  typedef std::vector<uInt> uivector;
  struct uIntCmp 
  {
    bool operator()(const uInt i1, const uInt i2) const 
    {
      return i1 < i2;
    }
  };
  typedef std::map<const uInt, uivector, uIntCmp> ui2vmap;

template<class T> class ROArrayColumn;
  Bool isAllColumns(const Vector<MS::PredefinedColumns>& colNames);

class SubMS
{

 public:

  enum RegriddingAlternatives {
    useFFTShift = -100,   // needs input and output grid to have the same number of channels and be equidistant in freq.
    useLinIntThenFFTShift // for the case the input grid is not equidistant in frequency but the output grid is
  };

  SubMS(String& theMS, Table::TableOption option = Table::Old);
  
  // construct from an MS
  SubMS(MeasurementSet& ms);

  

  virtual ~SubMS();
  
  // Change or Set the MS this MSSelector refers to.
  void setMS(MeasurementSet& ms);

  // Select spw and channels for each spw.
  // This is the version used by split.  It returns true on success and false
  // on failure.
  Bool selectSpw(const String& spwstr, const Vector<Int>& steps);

  // This older version is used by the older version of setmsselect().
  void selectSpw(Vector<Int> spw, Vector<Int> nchan, Vector<Int> start, 
                 Vector<Int> step);
  
  // Setup polarization selection (for now, only from available correlations -
  // no Stokes transformations.)
  Bool selectCorrelations(const String& corrstr);

  //select Time and time averaging or regridding
  //void selectTime();

  //select stuff using msselection syntax ...time is left out
  // call it separately with timebin
  // This version returns a success value, and does not need nchan, start, and
  // step.  It is used by split.
  Bool setmsselect(const String& spw="", const String& field="", 
		   const String& baseline="", const String& scan="",
                   const String& uvrange="", const String& taql="", 
		   const Vector<Int>& step=Vector<Int> (1,1),
		   const String& subarray="", const String& correlation="");

  // This older version does not return a success value, and does need nchan,
  // start, and step.  It is used elsewhere (i.e. ImagerMultiMS).
  void setmsselect(const String& spw,        const String& field, 
                   const String& baseline,   const String& scan,
                   const String& uvrange,    const String& taql,
                   const Vector<Int>& nchan, const Vector<Int>& start,
                   const Vector<Int>& step,  const String& subarray);

  // Select source or field
  Bool selectSource(const Vector<Int>& fieldid);
  
  // Select Antennas to split out  
  void selectAntenna(Vector<Int>& antennaids, Vector<String>& antennaSel);

  // Select array IDs to use.
  void selectArray(const String& subarray);

  //select time parameters
  void selectTime(Double timeBin=-1.0, String timerng="");

  //void selectSource(Vector<String> sourceid);

  //Method to set if a phase Center rotation is needed
  //void setPhaseCenter(Int fieldid, MDirection& newPhaseCenter);


  //Method to make the subMS
  //
  //TileShape of size 1 can have 2 values [0], and [1] ...these are used in to
  //determine the tileshape by using MSTileLayout. Otherwise it has to be a
  //vector size 3 e.g [4, 15, 351] => a tile shape of 4 stokes, 15 channels 351
  //rows.
  //
  // combine sets combine_p.  (Columns to ignore while time averaging.)
  //
  Bool makeSubMS(String& submsname, String& whichDataCol,
                 const Vector<Int>& tileShape=Vector<Int>(1, 0),
                 const String& combine="");

  //Method to make a scratch subMS and even in memory if posssible
  //Useful if temporary subselection/averaging is necessary
  // It'll be in memory if the basic output ms is less than half of 
  // memory reported by HostInfo unless forced to by user...
  virtual MeasurementSet* makeScratchSubMS(const Vector<MS::PredefinedColumns>& whichDataCols, 
				   const Bool forceInMemory=False);
  // In this form whichDataCol gets passed to parseColumnNames().
  virtual MeasurementSet* makeScratchSubMS(const String& whichDataCol, 
				   const Bool forceInMemory=False);

  // This sets up a default new ms
  // Declared static as it can be (and is) called directly outside of SubMS.
  // Therefore it is not dependent on any member variable.
  static MeasurementSet* setupMS(const String& msname, const Int nchan,
                                 const Int npol, const String& telescop,
                                 const Vector<MS::PredefinedColumns>& colNamesTok,
				 const Int obstype=0);

  // Same as above except allowing manual tileshapes
  static MeasurementSet* setupMS(const String& msname, const Int nchan,
                                 const Int npol,
                                 const Vector<MS::PredefinedColumns>& colNamesTok,
				 const Vector<Int>& tileShape=Vector<Int>(1,0));

  
  // Add optional columns to outTab if present in inTab and possColNames.
  // M must be derived from a Table.
  // beLazy should only be true if outTab is in its default state.
  // Returns the number of added columns.
  template<class M>
  static uInt addOptionalColumns(const M& inTab, M& outTab,
                                 const Bool beLazy=false);

  // Declared static because it's used in setupMS().  Therefore it can't use
  // any member variables.  It is also used in MSFixvis.cc.
  // colNameList is internally upcased, so it is not const or passed by reference.
  static const Vector<MS::PredefinedColumns>& parseColumnNames(String colNameList);
  // This version uses the MeasurementSet to check what columns are present,
  // i.e. it makes col=="all" smarter, and it is not necessary to call
  // verifyColumns() after calling this.  Unlike the other version, it knows
  // about FLOAT_DATA and LAG_DATA.  It throws an exception if a
  // _specifically_ requested column is absent.
  static const Vector<MS::PredefinedColumns>& parseColumnNames(String colNameList,
                                                    const MeasurementSet& ms);

  void verifyColumns(const MeasurementSet& ms, const Vector<MS::PredefinedColumns>& colNames);

  // The output MS must have (at least?) 1 of DATA, FLOAT_DATA, or LAG_DATA.
  // MODEL_DATA or CORRECTED_DATA will be converted to DATA if necessary.
  static Bool mustConvertToData(const uInt nTok,
                                const Vector<MS::PredefinedColumns>& datacols)
  {
    return (nTok == 1) && (datacols[0] != MS::FLOAT_DATA) &&
      (datacols[0] != MS::LAG_DATA);
  }

  static Bool sepFloat(const Vector<MS::PredefinedColumns>& anyDataCols,
                       Vector<MS::PredefinedColumns>& complexDataCols);

  // Fills outToIn[pol] with a map from output correlation index to input
  // correlation index, for each input polID pol.
  // It does not yet check the appropriateness of the correlation selection
  // string, so ignore the return value for now.  outToIn[pol] defaults to
  // an empty Vector if no correlations are selected for pol.
  // That is not the same as the default "select everything in ms".
  static Bool getCorrMaps(MSSelection& mssel, const MeasurementSet& ms,
			  Vector<Vector<Int> >& outToIn,
			  const Bool areSelecting=false);
  
  // Replaces col[i] with mapper[col[i]] for each element of col.
  // Does NOT check whether mapper[col[i]] is defined.
  static void remap(Vector<Int>& col, const Vector<Int>& mapper);
  static void remap(Vector<Int>& col, const std::map<Int, Int>& mapper);

  // Transform spectral data to different reference frame,
  // optionally regrid the frequency channels 
  // return values: -1 = MS not modified, 1 = MS modified and OK, 
  // 0 = MS modified but not OK (i.e. MS is probably damaged) 
  Int regridSpw(String& message, // returns the MS history entry 
		const String& outframe="", // default = "keep the same"
		const String& regridQuant="chan",
		const Double regridVeloRestfrq=-3E30, // default = "not set" 
		const String& regridInterpMeth="LINEAR",
		const Double regridCenter=-3E30, // default = "not set" 
		const Double regridBandwidth=-1., // default = "not set" 
		const Double regridChanWidth=-1., // default = "not set" 
		const Bool doHanningSmooth=False,
		const Int phaseCenterFieldId=-2, // -2 = use pahse center from field table
		MDirection phaseCenter=MDirection(), // this direction is used if phaseCenterFieldId==-1
		const Bool centerIsStart=False, // if true, the parameter regridCenter specifies the start
		const Bool startIsEnd=False, // if true, and centerIsStart is true, regridCenter specifies the upper end in frequency
		const Int nchan=0, // if >0: used instead of regridBandwidth, ==
		const Int width=0, // if >0 and regridQuant=="freq": used instead of regridChanWidth
		const Int start=-1 // if >=0 and regridQuant=="freq": used instead of regridCenter
		);

  // the following inline convenience methods for regridSpw bypass the whole CASA measure system
  // because when they are used, they can assume that the frame stays the same and the units are OK
  static lDouble vrad(const lDouble freq, const lDouble rest){ return (C::c * (1. - freq/rest)); };
  static lDouble vopt(const lDouble freq, const lDouble rest){ return (C::c *(rest/freq - 1.)); };
  static lDouble lambda(const lDouble freq){ return (C::c/freq); };
  static lDouble freq_from_vrad(const lDouble vrad, const lDouble rest){ return (rest * (1. - vrad/C::c)); };
  static lDouble freq_from_vopt(const lDouble vopt, const lDouble rest){ return (rest / (1. + vopt/C::c)); };
  static lDouble freq_from_lambda(const lDouble lambda){ return (C::c/lambda); };
  
  // Support method for regridSpw():
  // results in the column oldName being renamed to newName, and a new column
  // which is an empty copy of oldName being created together with a
  // TileShapeStMan data manager and hypercolumn (name copied from the old
  // hypercolumn) with given dimension, the old hypercolumn of name
  // hypercolumnName is renamed to name+"B"
  Bool createPartnerColumn(TableDesc& modMSTD, const String& oldName,
                           const String& newName, const Int& hypercolumnDim,
                           const IPosition& tileShape);

  // Support method for regridSpw():
  // calculate the final new channel boundaries from the regridding parameters
  // and the old channel boundaries (already transformed to the desired
  // reference frame); returns False if input paramters were invalid and no
  // useful boundaries could be created
  static Bool regridChanBounds(Vector<Double>& newChanLoBound, 
			       Vector<Double>& newChanHiBound,
			       const Double regridCenter, 
			       const Double regridBandwidth,
			       const Double regridChanWidth,
			       const Double regridVeloRestfrq, 
			       const String regridQuant,
			       const Vector<Double>& transNewXin, 
			       const Vector<Double>& transCHAN_WIDTH,
			       String& message, // message to the user, epsecially in case of error 
			       const Bool centerIsStart=False, // if true, the parameter regridCenter specifies the start
			       const Bool startIsEnd=False, // if true, and centerIsStart is true, regridCenter specifies the upper end in frequency
			       const Int nchan=0, // if != 0 : used instead of regridBandwidth, -1 means use all channels
			       const Int width=0, // if >0 and regridQuant=="freq": used instead of regridChanWidth
			       const Int start=-1 // if >=0 and regridQuant=="freq": used instead of regridCenter
			       );

  // a helper function for handling the gridding parameter user input
  static Bool convertGridPars(LogIO& os,
			      const String& mode, 
			      const int nchan, 
			      const String& start, 
			      const String& width,
			      const String& interp, 
			      const String& restfreq, 
			      const String& outframe,
			      const String& veltype,
			      String& t_mode,
			      String& t_outframe,
			      String& t_regridQuantity,
			      Double& t_restfreq,
			      String& t_regridInterpMeth,
			      Double& t_cstart, 
			      Double& t_bandwidth,
			      Double& t_cwidth,
			      Bool& t_centerIsStart, 
			      Bool& t_startIsEnd,			      
			      Int& t_nchan,
			      Int& t_width,
			      Int& t_start);

  // A wrapper for SubMS::regridChanBounds() which takes the user interface type gridding parameters
  // The ready-made grid is returned in newCHAN_FREQ and newCHAN_WIDTH
  static Bool calcChanFreqs(LogIO& os,
			    // output
			    Vector<Double>& newCHAN_FREQ,
			    Vector<Double>& newCHAN_WIDTH,
			    // input
			    const Vector<Double>& oldCHAN_FREQ, // the original grid
			    const Vector<Double>& oldCHAN_WIDTH, 
			    // the gridding parameters
			    const MDirection  phaseCenter,
			    const MFrequency::Types theOldRefFrame,
			    const MEpoch theObsTime,
			    const MPosition mObsPos,
			    const String& mode, 
			    const int nchan, 
			    const String& start, 
			    const String& width,
			    const String& restfreq, 
			    const String& outframe,
			    const String& veltype,
			    Bool verbose=False);

  // Support method for regridSpw():
  // if writeTables is False, the (const) input parameters are only verified, nothing is written;
  // return value is True if the parameters are OK.
  // if writeTables is True, the vectors are filled and the SPW, DD, and SOURCE tables are modified;
  // the return value in this case is True only if a successful modification (or none) took place
  Bool setRegridParameters(vector<Int>& oldSpwId,
			   vector<Int>& oldFieldId,
			   vector<Int>& newDataDescId,
			   vector<Bool>& regrid,
			   vector<Bool>& transform,
			   vector<MDirection>& theFieldDirV,
			   vector<MPosition>& mObsPosV,
			   vector<MFrequency::Types>& fromFrameTypeV,
			   vector<MFrequency::Ref>& outFrameV,
			   vector< Vector<Double> >& xold, 
			   vector< Vector<Double> >& xout, 
			   vector< Vector<Double> >& xin, 
			   vector< Int >& method, // interpolation method cast to Int
			   Bool& msMod,
			   const String& outframe,
			   const String& regridQuant,
			   const Double regridVeloRestfrq,
			   const String& regridInterpMeth,
			   const Double regridCenter, 
			   const Double regridBandwidth, 
			   const Double regridChanWidth,
			   const Int regridPhaseCenterFieldId, // -2 = take from field table, -1 = use 
			   const MDirection regridPhaseCenter, //    <- this value, >-1 = take from this field
			   const Bool writeTables,
			   LogIO& os,
			   String& regridMessage,
			   const Bool centerIsStart=False, // if true, the parameter regridCenter specifies the start
			   const Bool startIsEnd=False, // if true, and centerIsStart is true, regridCenter specifies the upper end in frequency
			   const Int nchan=0, // if >0: used instead of regridBandwidth
			   const Int width=0, // if >0 and regridQuant=="freq": used instead of regridChanWidth
			   const Int start=-1 // if >=0 and regridQuant=="freq": used instead of regridCenter
			   );

  // combineSpws():
  // make one spectral window from all spws given by the spwids vector
  Bool combineSpws(const Vector<Int>& spwids,  // Vector<Int>(1,-1) means: use all SPWs
		   const Bool noModify,   // if True, the MS will not be modified
		   Vector<Double>& newCHAN_FREQ, // will return the grid of the resulting SPW
		   Vector<Double>& newCHAN_WIDTH,
		   Bool verbose=False
		   );

  Bool combineSpws(const Vector<Int>& spwids = Vector<Int>(1,-1)){  // Vector<Int>(1,-1) means: use all SPWs
    Vector<Double> temp1; 
    Vector<Double> temp2;
    return combineSpws(spwids, False, temp1, temp2, True);
  }

 protected:

  //method that returns the selected ms (?! - but it's Boolean - RR)
  Bool makeSelection();

  // (Sub)table fillers.
  Bool fillAllTables(const Vector<MS::PredefinedColumns>& colNames);
  Bool fillDDTables();		// Includes spw and pol.
  Bool fillFieldTable();
  Bool fillMainTable(const Vector<MS::PredefinedColumns>& colNames);
  Bool fillAverMainTable(const Vector<MS::PredefinedColumns>& colNames);
  Bool copyAntenna();
  Bool copyFeed();
  Bool copyFlag_Cmd();
  Bool copyHistory();
  Bool copyObservation();
  Bool copyPointing();
  Bool copyProcessor();
  Bool copySource();
  Bool copyState();
  Bool copySyscal();
  Bool copyWeather();
  Bool copyGenericSubtables();

  //  Bool writeDiffSpwShape(const Vector<MS::PredefinedColumns>& colNames);
  Bool fillAccessoryMainCols();

  // *** Private member functions ***
  Bool getDataColumn(ROArrayColumn<Complex>& data,
                     const MS::PredefinedColumns colName);
  Bool getDataColumn(ROArrayColumn<Float>& data,
                     const MS::PredefinedColumns colName);
  Bool putDataColumn(MSColumns& msc, ROArrayColumn<Complex>& data,
                     const MS::PredefinedColumns datacol,
                     const Bool writeToDataCol=False);
  Bool putDataColumn(MSColumns& msc, ROArrayColumn<Float>& data,
                     const MS::PredefinedColumns datacol,
                     const Bool writeToDataCol=False);

  // This method uses VisIter for efficient copy mode data transfer
  Bool copyDataFlagsWtSp(const Vector<MS::PredefinedColumns>& colNames,
                         const Bool writeToDataCol);

  // Helper function for parseColumnNames().  Converts col to a list of
  // MS::PredefinedColumnss, and returns the # of recognized data columns.
  // static because parseColumnNames() is static.
  static uInt dataColStrToEnums(const String& col,
                                Vector<MS::PredefinedColumns>& colvec);
    
  Bool doChannelMods(const Vector<MS::PredefinedColumns>& colNames);

  // return the number of unique antennas selected
  //Int numOfBaselines(Vector<Int>& ant1, Vector<Int>& ant2,
  //                   Bool includeAutoCorr=False);

  // Used in a couple of places to estimate how much memory to grab.
  Double n_bytes() {return mssel_p.nrow() * nchan_p[0] * ncorr_p[0] *
                           sizeof(Complex);}

  // Picks a reference to DATA, MODEL_DATA, CORRECTED_DATA, or LAG_DATA out
  // of ms_p.  FLOAT_DATA is not included because it is not natively complex. 
  const ROArrayColumn<Complex>& right_column(const ROMSColumns *ms_p,
                                             const MS::PredefinedColumns datacol);

  // The writable version of the above.
  ArrayColumn<Complex>& right_column(MSColumns *msclala,
				     const MS::PredefinedColumns col,
				     const Bool writeToDataCol);

  // Figures out the number, maximum, and index of the selected antennas.
  uInt fillAntIndexer(const ROMSColumns *msc, Vector<Int>& antIndexer);

  // Read the input, time average it to timeBin_p, and write the output.
  // The first version uses VisibilityIterator (much faster), but the second
  // supports correlation selection using VisIterator.  VisIterator should be
  // sped up soon!
  Bool doTimeAver(const Vector<MS::PredefinedColumns>& dataColNames);
  Bool doTimeAverVisIterator(const Vector<MS::PredefinedColumns>& dataColNames);

  // Fills mapper[ntok] with a map from dataColumn indices to ArrayColumns in
  // the output.  mapper must have ntok slots!
  void getDataColMap(ArrayColumn<Complex>* mapper, uInt ntok,
                     const Vector<MS::PredefinedColumns>& colEnums); 

  // Returns whether or not the numbers of correlations and channels
  // are independent of DATA_DESCRIPTION_ID, for both the input and output
  // MSes.
  Bool areDataShapesConstant();

  // Sets up sourceRelabel_p for mapping input SourceIDs (if any) to output
  // ones.  Must be called after fieldid_p is set and before calling
  // fillFieldTable() or copySource().
  void relabelSources();

  void relabelIDs();
  void remapColumn(const ROScalarColumn<Int>& incol, ScalarColumn<Int>& outcol);
  static void make_map(const Vector<Int>& mscol, Vector<Int>& mapper);
  static void make_map(const ROScalarColumn<Int>& mscol,
		       std::map<Int, Int>& mapper);
  uInt remapped(const Int ov, const Vector<Int>& mapper, uInt i);

  // *** Member variables ***

  // Initialized* by ctors.  (Maintain order both here and in ctors.)
  //  * not necessarily to anything useful.
  MeasurementSet ms_p, mssel_p;
  MSColumns * msc_p;		// columns of msOut_p
  ROMSColumns * mscIn_p;
  Bool keepShape_p,      	// Iff true, each output array has the
				// same shape as the corresponding input one.
       // sameShape_p,             // Iff true, the shapes of the arrays do not
       //  			// vary with row number.
       antennaSel_p;		// Selecting by antenna?
  Double timeBin_p;
  String scanString_p, uvrangeString_p, taqlString_p;
  String timeRange_p, arrayExpr_p, corrString_p;
  String combine_p;          // Should time averaging not split bins by
                             // scan #, observation, and/or state ID?
                             // Must be lowercase at all times.

  // Uninitialized by ctors.
  MeasurementSet msOut_p;
  Vector<Int> spw_p,      // The input spw corresponding to each output spw.
              spw_uniq_p, // Uniquified version of spw_p.
              spwind_to_min_spwind_p,
              nchan_p,    // The # of output channels for each range.
              totnchan_p, // The # of output channels for each output spw.
              chanStart_p,
              chanStep_p, // Increment between input chans, i.e. if 3, only every third
                          // input channel will be used. 
              widths_p,   // # of input chans per output chan for each range.
              ncorr_p,    // The # of output correlations for each DDID.
              inNumChan_p,    // The # of input channels for each spw.
              inNumCorr_p;    // The # of input correlations for each DDID.
  Vector<Int> fieldid_p;
  Vector<Int> spwRelabel_p, fieldRelabel_p, sourceRelabel_p;
  Vector<Int> oldDDSpwMatch_p;
  Vector<String> antennaSelStr_p;
  Vector<Int> antennaId_p;
  Vector<Int> antIndexer_p;
  Vector<Int> antNewIndex_p;

  Vector<Int> arrayId_p;
  Vector<Int> polID_p;	       // Map from input DDID to input polID, filled in fillDDTables(). 
  Vector<uInt> spw2ddid_p;

  // inCorrInd = outPolCorrToInCorrMap_p[polID_p[ddID]][outCorrInd]
  Vector<Vector<Int> > inPolOutCorrToInCorrMap_p;

  std::map<Int, Int> arrayRemapper_p, stateRemapper_p; 

  Vector<Vector<Slice> > chanSlices_p;  // Used by VisIterator::selectChannel()
  Vector<Slice> corrSlice_p;
  Vector<Vector<Slice> > corrSlices_p;  // Used by VisIterator::selectCorrelation()
  Matrix<Double> selTimeRanges_p;
};

} //# NAMESPACE CASA - END

#ifndef AIPS_NO_TEMPLATE_SRC
#include <msvis/MSVis/SubMS.tcc>
#endif //# AIPS_NO_TEMPLATE_SRC

#endif

