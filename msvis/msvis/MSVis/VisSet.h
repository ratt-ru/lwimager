//# VisSet.h: VisSet definitions
//# Copyright (C) 1996,1997,1998,2001,2002
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

#ifndef MSVIS_VISSET_H
#define MSVIS_VISSET_H

#include <casa/aips.h>
#include <casa/BasicSL/Complex.h>
#include <casa/Arrays/Matrix.h>
#include <msvis/MSVis/StokesVector.h>
#include <msvis/MSVis/VisibilityIterator.h>

namespace casa { //# NAMESPACE CASA - BEGIN

// <summary> 
// The interface to the MeasurementSet for synthesis processing
// </summary>

// <reviewed reviewer="" date="" tests="t" demos="">

// <prerequisite>
//   <li> <linkto module="MeasurementSet">MeasurementSet</linkto>
// </prerequisite>
//
// <etymology>
// VisSet is the Set of Visibilities
// </etymology>
//
// <synopsis> 
// The VisSet is a class that simplifies access to the visibility data
// for the synthesis processing code. It holds a reference to an original
// MeasurementSet with observed data and creates two extra data
// columns for the storage of model visibilities and
// corrected visibilities. All access to the MeasurementSet is done via
// the <linkto class="VisibilityIterator">VisibilityIterator</linkto>
// and the <linkto class="VisBuffer">VisBuffer</linkto>
//
// The VisSet allows selection and sorting of the MeasurementSet to be applied.
// A number of columns can be specified to define the iteration order, a
// a time interval can be given to iterate in chunks of time and a channel
// selection can be made for each spectral window present in the data.
// </synopsis> 
//
// <example>
// <srcblock>
//    MeasurementSet ms("Example.MS",Table::Update);
//    cout << "Constructing VisSet"<<endl;
//    Block<Int> bi(2); 
//    bi[0]=MS::SPECTRAL_WINDOW_ID;
//    bi[1]=MS::TIME; 
//    Matrix<Int> chanSelection; // no channel selection
//    // iterate in 600s chunks within each SpectralWindow
//    Double interval=600.; 
//    VisSet vs(ms,bi,chanSelection,interval);
// </srcblock>
// </example>
//
// <motivation>
// This class provides an easy interface to the MS. 
// It keeps the iterator around
// for reuse, thus avoiding repeated sorting of the data.
// </motivation>
//
// <todo asof="">
// </todo>

typedef uInt Antenna;
typedef Double Frequency;
typedef RigidVector<Float,3> Position;
typedef RigidVector<Double,3> Direction;

typedef Vector<CStokesVector> vvCoh;

typedef ROVisibilityIterator ROVisIter;
typedef VisibilityIterator VisIter;


class VisSet {
public:
  // default constructor, only useful to assign to later.
  VisSet() {}

  // Construct from a MeasurementSet, with iteration order specified in
  // columns (giving the MS enum for the column) 
  // Specify channel selection as a Matrix(3,nSpw) where for each
  // spectral window the three values are start,nChannel and
  // spectral window number. Spectral windows without an entry will have 
  // all channels selected.
  // Specify a time interval for iterating in 'chunks' of time.
  // The default time interval of 0 groups all times together.
  // This constructor creates three new columns:
  // MODEL_DATA and CORRECTED_DATA and IMAGING_WEIGHT
  // If they already exist and have the
  // same channel selection applied, they are reused.
  // Note that the contents of these columns are NOT initialized,
  // you should fill them before trying to read the data.
  // The MS calibration scratch columns can be optionally compressed.
  VisSet(MeasurementSet & ms, const Block<Int>& columns, 
	 const Matrix<Int>& chanSelection, Double timeInterval=0,
	 Bool compress=False);

  // Same as above, but provide scratch column option
  VisSet(MeasurementSet& ms,const Block<Int>& columns, 
	 const Matrix<Int>& chanSelection, 
	 Bool addScratch,
	 Double timeInterval=0,Bool compress=False);

  // This is a constructor for multiple MS...but everything is same as the one 
  // above


  VisSet(Block<MeasurementSet>& mss, const Block<Int>& columns, 
         const Block< Matrix<Int> >& chanSelections, Bool addStratch=False, Double timeInterval=0,
         Bool compress=False);





  // This is a no frills constructor, no re-sorting, the default order is used,
  // no scratch columns is made even if they don't exist. So if you use
  // this constructor and plan to use the scratch columns make sure 
  // that they exist prior to constructing the VisSet this way.
  VisSet(MeasurementSet & ms, const Matrix<Int>& chanSelection, 
	 Double timeInterval=0);

  //Constructor from visibility iterator ....a temporary fix 
  //as EPJones as Imager stops using VisSet 
  VisSet(ROVisibilityIterator& vi);
  // Construct from an existing VisSet, this references the underlying
  // MeasurementSet(s) but allows a new iteration order and time interval
  // to be specified.
  VisSet(const VisSet & vs, const Block<Int>& columns, Double timeInterval=0);

  // Destructor, flushes the data to disk
  ~VisSet();
  // referencing assignment operator
  VisSet& operator=(const VisSet& other);

  // Re-initialize the VisibilityIterator (cf copy ctor)
  void resetVisIter(const Block<Int>& columns, Double timeInterval=0);

  // Initializes scratch columns
  void initCalSet(Int calSet=0);

  // Flushes the data to disk
  void flush();
  // Iterator access to the  data
  VisIter& iter();

  // Reset the channel selection. Only subsets of the original selection
  // (set in constructor) can be specified.
  // Note: this calls origin on the iterator.
  void selectChannel(Int nGroup,Int start, Int width, Int increment, 
		     Int spectralWindow);
  // call to VisIter origin optional:
  void selectChannel(Int nGroup,Int start, Int width, Int increment, 
		     Int spectralWindow, Bool callOrigin);

  // Collective selection via MSSelection channel selection Matrix
  void selectChannel(const Matrix<Int>& chansel);

  // Set nominal selection to ALL channels
  void selectAllChans();

  // number of antennas
  Int numberAnt();

  // number of fields
  Int numberFld();

  // number of spectral windows
  Int numberSpw();

  // number of channels in each spectral window
  Vector<Int> numberChan() const;

  // start channel of VisSet selection in each spectral window
  Vector<Int> startChan() const;

  // number of coherences
  Int numberCoh() const;

  // Lock and unlock the associated MS
  void lock() {ms_p.lock();};
  void unlock() {ms_p.unlock();};

  // Return the associated MS name
  String msName();
  
private:

  //Add the scratch columns
  void addScratchCols(MeasurementSet& ms, Bool compress=False);

  // Add a calibration set (comprising a set of CORRECTED_DATA, MODEL_DATA
  // and IMAGING_WEIGHT columns) to the MeasurementSet (MS). Optionally
  // compress these columns using the CompressComplex column engine.
  void addCalSet(MeasurementSet& ms, Bool compress=True);

  // Remove an existing cal set (a CORRECTED_DATA, MODEL_DATA 
  // and IMAGING_WEIGHT column set and, optionally, any associated
  // compression columns)
  void removeCalSet(MeasurementSet& ms);

  MeasurementSet ms_p;
  VisIter* iter_p;
  Matrix<Int> selection_p;
  Block<MeasurementSet> *blockOfMS_p;
  Bool multims_p;

};


} //# NAMESPACE CASA - END

#endif


