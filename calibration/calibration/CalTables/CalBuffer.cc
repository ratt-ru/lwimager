//# CalBuffer.cc: Implementation of CalBuffer.h
//# Copyright (C) 1996,1997,1998,2001,2003
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
//# $Id: CalBuffer.cc,v 19.3 2004/11/30 17:50:11 ddebonis Exp $
//----------------------------------------------------------------------------

#include <calibration/CalTables/CalBuffer.h>

namespace casa { //# NAMESPACE CASA - BEGIN

//----------------------------------------------------------------------------

CalBuffer::CalBuffer() :
  connectedToIter_p(False), calMainCol_p(NULL)
{
// Null constructor
// Output to private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    calMainCol_p       CalMainColumns*      Ptr to cal main col accessor
//    time_p             Vector<Double>&      Time
//    timeMeas_p         Vector<MEpoch>&      Time as a Measure
//    timeEP_p           Vector<Double>&      Extended precision time
//    interval_p         Vector<Double>&      Time interval
//    antenna1_p         Vector<Int>&         Antenna 1
//    feed1_p            Vector<Int>&         Feed 1
//    fieldId_p          Vector<Int>&         Field id.
//    arrayId_p          Vector<Int>&         Array id.
//    obsId_p            Vector<Int>&         Observation id.
//    scanNo_p           Vector<Int>&         Scan no.
//    processorId_p      Vector<Int>&         Processor id.
//    stateId_p          Vector<Int>&         State id.
//    phaseId_p          Vector<Int>&         Phase id.
//    pulsarBin_p        Vector<Int>&         Pulsar bin
//    pulsarGateId_p     Vector<Int>&         Pulsar gate id.
//    freqGrp_p          Vector<Int>&         Frequency group
//    freqGrpName_p      Vector<String>&      Frequency group name
//    fieldName_p        Vector<String>&      Field name
//    fieldCode_p        Vector<String>&      Field code
//    sourceName_p       Vector<String>&      Source name
//    sourceCode_p       Vector<String>&      Source code
//    calGrp_p           Vector<Int>&         Calibration group
//    gain_p             Array<Complex>&      Gain
//    refAnt_p           Array<Int>&          Reference antenna
//    refFeed_p          Array<Int>&          Reference feed
//    refReceptor_p      Array<Int>&          Reference receptor
//    refFreq_p          Array<Double>&       Reference frequency
//    measFreqRef_p      Vector<Int>&         Frequency measures ref.
//    refDir_p           Array<Double>&       Reference direction
//    measDirRef_p       Vector<Int>&         Direction measures ref.
//    calDescId_p        Vector<Int>&         CAL_DESC id.
//    calHistoryId_p     Vector<Int>&         CAL_HISTORY id.
//    timeOK_p           Bool                 Time cache ok
//    timeMeasOK_p       Bool                 Time as a Measure cache ok
//    timeEPOK_p         Bool                 Extended precision time cache ok
//    intervalOK_p       Bool                 Time interval cache ok
//    antenna1OK_p       Bool                 Antenna 1 cache ok
//    feed1OK_p          Bool                 Feed 1 cache ok
//    fieldIdOK_p        Bool                 Field id. cache ok
//    arrayIdOK_p        Bool                 Array id. cache ok
//    obsIdOK_p          Bool                 Observation id. cache ok
//    scanNoOK_p         Bool                 Scan no. cache ok
//    processorIdOK_p    Bool                 Processor id. cache ok
//    stateIdOK_p        Bool                 State id. cache ok
//    phaseIdOK_p        Bool                 Phase id. cache ok
//    pulsarBinOK_p      Bool                 Pulsar bin cache ok
//    pulsarGateIdOK_p   Bool                 Pulsar gate id. cache ok
//    freqGrpOK_p        Bool                 Frequency group cache ok
//    freqGrpNameOK_p    Bool                 Frequency group name cache ok
//    fieldNameOK_p      Bool                 Field name cache ok
//    fieldCodeOK_p      Bool                 Field code cache ok
//    sourceNameOK_p     Bool                 Source name cache ok
//    sourceCodeOK_p     Bool                 Source code cache ok
//    calGrpOK_p         Bool                 Calibration group cache ok
//    gainOK_p           Bool                 Gain cache ok
//    refAntOK_p         Bool                 Reference antenna cache ok
//    refFeedOK_p        Bool                 Reference feed cache ok
//    refReceptorOK_p    Bool                 Reference receptor cache ok
//    refFreqOK_p        Bool                 Reference frequency cache ok
//    measFreqRefOK_p    Bool                 Frequency measures ref. cache ok
//    refDirOK_p         Bool                 Reference direction cache ok
//    measDirRefOK_p     Bool                 Direction measures ref. cache ok
//    calDescIdOK_p      Bool                 CAL_DESC id. cache ok
//    calHistoryIdOK_p   Bool                 CAL_HISTORY id. cache ok
//
  // Invalidate cache 
  invalidate();
};

//----------------------------------------------------------------------------

CalBuffer::CalBuffer (CalIterBase& calIter) :
  connectedToIter_p(True), calMainCol_p(NULL)
{
// Construct from a calibration table iterator
// Input:
//    calIter            CalIterBase&         Calibration table iterator
// Output to private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    calMainCol_p       CalMainColumns*      Ptr to cal main col accessor
//    time_p             Vector<Double>&      Time
//    timeMeas_p         Vector<MEpoch>&      Time as a Measure
//    timeEP_p           Vector<Double>&      Extended precision time
//    interval_p         Vector<Double>&      Time interval
//    antenna1_p         Vector<Int>&         Antenna 1
//    feed1_p            Vector<Int>&         Feed 1
//    fieldId_p          Vector<Int>&         Field id.
//    arrayId_p          Vector<Int>&         Array id.
//    obsId_p            Vector<Int>&         Observation id.
//    scanNo_p           Vector<Int>&         Scan no.
//    processorId_p      Vector<Int>&         Processor id.
//    stateId_p          Vector<Int>&         State id.
//    phaseId_p          Vector<Int>&         Phase id.
//    pulsarBin_p        Vector<Int>&         Pulsar bin
//    pulsarGateId_p     Vector<Int>&         Pulsar gate id.
//    freqGrp_p          Vector<Int>&         Frequency group
//    freqGrpName_p      Vector<String>&      Frequency group name
//    fieldName_p        Vector<String>&      Field name
//    fieldCode_p        Vector<String>&      Field code
//    sourceName_p       Vector<String>&      Source name
//    sourceCode_p       Vector<String>&      Source code
//    calGrp_p           Vector<Int>&         Calibration group
//    gain_p             Array<Complex>&      Gain
//    refAnt_p           Array<Int>&          Reference antenna
//    refFeed_p          Array<Int>&          Reference feed
//    refReceptor_p      Array<Int>&          Reference receptor
//    refFreq_p          Array<Double>&       Reference frequency
//    measFreqRef_p      Vector<Int>&         Frequency measures ref.
//    refDir_p           Array<Double>&       Reference direction
//    measDirRef_p       Vector<Int>&         Direction measures ref.
//    calDescId_p        Vector<Int>&         CAL_DESC id.
//    calHistoryId_p     Vector<Int>&         CAL_HISTORY id.
//    timeOK_p           Bool                 Time cache ok
//    timeMeasOK_p       Bool                 Time as a Measure cache ok
//    timeEPOK_p         Bool                 Extended precision time cache ok
//    intervalOK_p       Bool                 Time interval cache ok
//    antenna1OK_p       Bool                 Antenna 1 cache ok
//    feed1OK_p          Bool                 Feed 1 cache ok
//    fieldIdOK_p        Bool                 Field id. cache ok
//    arrayIdOK_p        Bool                 Array id. cache ok
//    obsIdOK_p          Bool                 Observation id. cache ok
//    scanNoOK_p         Bool                 Scan no. cache ok
//    processorIdOK_p    Bool                 Processor id. cache ok
//    stateIdOK_p        Bool                 State id. cache ok
//    phaseIdOK_p        Bool                 Phase id. cache ok
//    pulsarBinOK_p      Bool                 Pulsar bin cache ok
//    pulsarGateIdOK_p   Bool                 Pulsar gate id. cache ok
//    freqGrpOK_p        Bool                 Frequency group cache ok
//    freqGrpNameOK_p    Bool                 Frequency group name cache ok
//    fieldNameOK_p      Bool                 Field name cache ok
//    fieldCodeOK_p      Bool                 Field code cache ok
//    sourceNameOK_p     Bool                 Source name cache ok
//    sourceCodeOK_p     Bool                 Source code cache ok
//    calGrpOK_p         Bool                 Calibration group cache ok
//    gainOK_p           Bool                 Gain cache ok
//    refAntOK_p         Bool                 Reference antenna cache ok
//    refFeedOK_p        Bool                 Reference feed cache ok
//    refReceptorOK_p    Bool                 Reference receptor cache ok
//    refFreqOK_p        Bool                 Reference frequency cache ok
//    measFreqRefOK_p    Bool                 Frequency measures ref. cache ok
//    refDirOK_p         Bool                 Reference direction cache ok
//    measDirRefOK_p     Bool                 Direction measures ref. cache ok
//    calDescIdOK_p      Bool                 CAL_DESC id. cache ok
//    calHistoryIdOK_p   Bool                 CAL_HISTORY id. cache ok
//
  // Invalidate cache
  invalidate();

  // Construct the cal main column accessor
  calMainCol_p = new CalMainColumns (calIter.subTable());
};

//----------------------------------------------------------------------------

CalBuffer::~CalBuffer()
{
// Destructor
// Output to private data:
//    calMainCol_p       CalMainColumns*      Ptr to cal main col accessor
//
  if (calMainCol_p) delete calMainCol_p;
};

//----------------------------------------------------------------------------

void CalBuffer::invalidate()
{
// Invalidate the current cache
// Output to private data:
//    timeOK_p           Bool                 Time cache ok
//    timeMeasOK_p       Bool                 Time as a Measure cache ok
//    timeEPOK_p         Bool                 Extended precision time cache ok
//    intervalOK_p       Bool                 Time interval cache ok
//    antenna1OK_p       Bool                 Antenna 1 cache ok
//    feed1OK_p          Bool                 Feed 1 cache ok
//    fieldIdOK_p        Bool                 Field id. cache ok
//    arrayIdOK_p        Bool                 Array id. cache ok
//    obsIdOK_p          Bool                 Observation id. cache ok
//    scanNoOK_p         Bool                 Scan no. cache ok
//    processorIdOK_p    Bool                 Processor id. cache ok
//    stateIdOK_p        Bool                 State id. cache ok
//    phaseIdOK_p        Bool                 Phase id. cache ok
//    pulsarBinOK_p      Bool                 Pulsar bin cache ok
//    pulsarGateIdOK_p   Bool                 Pulsar gate id. cache ok
//    freqGrpOK_p        Bool                 Frequency group cache ok
//    freqGrpNameOK_p    Bool                 Frequency group name cache ok
//    fieldNameOK_p      Bool                 Field name cache ok
//    fieldCodeOK_p      Bool                 Field code cache ok
//    sourceNameOK_p     Bool                 Source name cache ok
//    sourceCodeOK_p     Bool                 Source code cache ok
//    calGrpOK_p         Bool                 Calibration group cache ok
//    gainOK_p           Bool                 Gain cache ok
//    refAntOK_p         Bool                 Reference antenna cache ok
//    refFeedOK_p        Bool                 Reference feed cache ok
//    refReceptorOK_p    Bool                 Reference receptor cache ok
//    refFreqOK_p        Bool                 Reference frequency cache ok
//    measFreqRefOK_p    Bool                 Frequency measures ref. cache ok
//    refDirOK_p         Bool                 Reference direction cache ok
//    measDirRefOK_p     Bool                 Direction measures ref. cache ok
//    calDescIdOK_p      Bool                 CAL_DESC id. cache ok
//    calHistoryIdOK_p   Bool                 CAL_HISTORY id. cache ok
//
  // Set all cache flags to false
  timeOK_p = False;
  timeMeasOK_p = False;
  timeEPOK_p = False;
  intervalOK_p = False;
  antenna1OK_p = False;
  feed1OK_p = False;
  fieldIdOK_p = False;
  arrayIdOK_p = False;
  obsIdOK_p = False;
  scanNoOK_p = False;
  processorIdOK_p = False;
  stateIdOK_p = False;
  phaseIdOK_p = False;
  pulsarBinOK_p = False;
  pulsarGateIdOK_p = False;
  freqGrpOK_p = False;
  freqGrpNameOK_p = False;
  fieldNameOK_p = False;
  fieldCodeOK_p = False;
  sourceNameOK_p = False;
  sourceCodeOK_p = False;
  calGrpOK_p = False;
  gainOK_p = False;
  refAntOK_p = False;
  refFeedOK_p = False;
  refReceptorOK_p = False;
  refFreqOK_p = False;
  measFreqRefOK_p = False;
  refDirOK_p = False;
  measDirRefOK_p = False;
  calDescIdOK_p = False;
  calHistoryIdOK_p = False;
};

//----------------------------------------------------------------------------

Vector<Double>& CalBuffer::time()
{
// TIME data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    time_p             Vector<Double>&      Time
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!timeOK_p) {
      calMainCol_p->time().getColumn (time_p);
      timeOK_p = True;
    };
  };
  return time_p;
};

//----------------------------------------------------------------------------

Vector<Double>& CalBuffer::timeEP()
{
// TIME_EXTRA_PREC data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    timeEP_p           Vector<Double>&      Time EP
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!timeEPOK_p) {
      calMainCol_p->timeEP().getColumn (timeEP_p);
      timeEPOK_p = True;
    };
  };
  return timeEP_p;
};

//----------------------------------------------------------------------------

Vector<Double>& CalBuffer::interval()
{
// INTERVAL data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    interval_p         Vector<Double>&      Interval
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!intervalOK_p) {
      calMainCol_p->interval().getColumn (interval_p);
      intervalOK_p = True;
    };
  };
  return time_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::antenna1()
{
// ANTENNA1 data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    antenna1_p         Vector<Int>&         Antenna1
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!antenna1OK_p) {
      calMainCol_p->antenna1().getColumn (antenna1_p);
      antenna1OK_p = True;
    };
  };
  return antenna1_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::feed1()
{
// FEED1 data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    feed1_p            Vector<Int>&         Feed1
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!feed1OK_p) {
      calMainCol_p->feed1().getColumn (feed1_p);
      feed1OK_p = True;
    };
  };
  return feed1_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::fieldId()
{
// FIELD_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    fieldId_p          Vector<Int>&         Field id.
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!fieldIdOK_p) {
      calMainCol_p->fieldId().getColumn (fieldId_p);
      fieldIdOK_p = True;
    };
  };
  return fieldId_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::arrayId()
{
// ARRAY_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    arrayId_p          Vector<Int>&         Array id.
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!arrayIdOK_p) {
      calMainCol_p->arrayId().getColumn (arrayId_p);
      arrayIdOK_p = True;
    };
  };
  return arrayId_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::obsId()
{
// OBSERVATION_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    obsId_p            Vector<Int>&         Obs id.
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!obsIdOK_p) {
      calMainCol_p->obsId().getColumn (obsId_p);
      obsIdOK_p = True;
    };
  };
  return obsId_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::scanNo()
{
// SCAN_NUMBER data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    scanNo_p           Vector<Int>&         Scan no.
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!scanNoOK_p) {
      calMainCol_p->scanNo().getColumn (scanNo_p);
      scanNoOK_p = True;
    };
  };
  return scanNo_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::processorId()
{
// PROCESSOR_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    processorId_p      Vector<Int>&         Processor id.
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!processorIdOK_p) {
      calMainCol_p->processorId().getColumn (processorId_p);
      processorIdOK_p = True;
    };
  };
  return processorId_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::stateId()
{
// STATE_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    stateId_p          Vector<Int>&         State id.
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!stateIdOK_p) {
      calMainCol_p->stateId().getColumn (stateId_p);
      stateIdOK_p = True;
    };
  };
  return stateId_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::phaseId()
{
// PHASE_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    phaseId_p          Vector<Int>&         Phase id.
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!phaseIdOK_p) {
      calMainCol_p->phaseId().getColumn (phaseId_p);
      phaseIdOK_p = True;
    };
  };
  return phaseId_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::pulsarBin()
{
// PULSAR_BIN data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    pulsarBin_p        Vector<Int>&         Pulsar bin
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!pulsarBinOK_p) {
      calMainCol_p->pulsarBin().getColumn (pulsarBin_p);
      pulsarBinOK_p = True;
    };
  };
  return pulsarBin_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::pulsarGateId()
{
// PULSAR_GATE_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    pulsarGateId_p     Vector<Int>&         Pulsar gate id.
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!pulsarGateIdOK_p) {
      calMainCol_p->pulsarGateId().getColumn (pulsarGateId_p);
      pulsarGateIdOK_p = True;
    };
  };
  return pulsarGateId_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::freqGrp()
{
// FREQ_GROUP data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    freqGrp_p          Vector<Int>&         Frequency group
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!freqGrpOK_p) {
      calMainCol_p->freqGrp().getColumn (freqGrp_p);
      freqGrpOK_p = True;
    };
  };
  return freqGrp_p;
};

//----------------------------------------------------------------------------

Vector<String>& CalBuffer::freqGrpName()
{
// FREQ_GROUP_NAME data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    freqGrpName_p      Vector<String>&      Frequency group name
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!freqGrpNameOK_p) {
      calMainCol_p->freqGrpName().getColumn (freqGrpName_p);
      freqGrpNameOK_p = True;
    };
  };
  return freqGrpName_p;
};

//----------------------------------------------------------------------------

Vector<String>& CalBuffer::fieldName()
{
// FIELD_NAME data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    fieldName_p        Vector<String>&      Field name
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!fieldNameOK_p) {
      calMainCol_p->fieldName().getColumn (fieldName_p);
      fieldNameOK_p = True;
    };
  };
  return fieldName_p;
};

//----------------------------------------------------------------------------

Vector<String>& CalBuffer::fieldCode()
{
// FIELD_CODE data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    fieldCode_p        Vector<String>&      Field code
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!fieldCodeOK_p) {
      calMainCol_p->fieldCode().getColumn (fieldCode_p);
      fieldCodeOK_p = True;
    };
  };
  return fieldCode_p;
};

//----------------------------------------------------------------------------

Vector<String>& CalBuffer::sourceCode()
{
// FIELD_CODE data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    sourceCode_p       Vector<String>&      Source code
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!sourceCodeOK_p) {
      calMainCol_p->sourceCode().getColumn (sourceCode_p);
      sourceCodeOK_p = True;
    };
  };
  return sourceCode_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::calGrp()
{
// CALIBRATION_GROUP data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    calGrp_p          Vector<Int>&          Calibration group
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!calGrpOK_p) {
      calMainCol_p->calGrp().getColumn (calGrp_p);
      calGrpOK_p = True;
    };
  };
  return calGrp_p;
};

//----------------------------------------------------------------------------

Array<Complex>& CalBuffer::gain()
{
// GAIN data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    gain_p             Array<Complex>&      Calibration gain
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!gainOK_p) {
      calMainCol_p->gain().getColumn (gain_p);
      gainOK_p = True;
    };
  };
  return gain_p;
};

//----------------------------------------------------------------------------

Array<Int>& CalBuffer::refAnt()
{
// REF_ANT data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    refAnt_p           Array<Int>&          Reference antenna
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!refAntOK_p) {
      calMainCol_p->refAnt().getColumn (refAnt_p);
      refAntOK_p = True;
    };
  };
  return refAnt_p;
};

//----------------------------------------------------------------------------

Array<Int>& CalBuffer::refFeed()
{
// REF_FEED data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    refFeed_p          Array<Int>&          Reference feed
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!refFeedOK_p) {
      calMainCol_p->refFeed().getColumn (refFeed_p);
      refFeedOK_p = True;
    };
  };
  return refFeed_p;
};

//----------------------------------------------------------------------------

Array<Int>& CalBuffer::refReceptor()
{
// REF_FEED data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    refReceptor_p      Array<Int>&          Reference receptor
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!refReceptorOK_p) {
      calMainCol_p->refReceptor().getColumn (refReceptor_p);
      refReceptorOK_p = True;
    };
  };
  return refReceptor_p;
};

//----------------------------------------------------------------------------

Array<Double>& CalBuffer::refFreq()
{
// REF_FREQUENCY data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    refFreq_p      Array<Double>&           Reference frequency
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!refFreqOK_p) {
      calMainCol_p->refFreq().getColumn (refFreq_p);
      refFreqOK_p = True;
    };
  };
  return refFreq_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::measFreqRef()
{
// REF_FEED data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    measFreqRef_p      Vector<Int>&         Reference frequency frame
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!measFreqRefOK_p) {
      calMainCol_p->measFreqRef().getColumn (measFreqRef_p);
      measFreqRefOK_p = True;
    };
  };
  return measFreqRef_p;
};

//----------------------------------------------------------------------------

Array<Double>& CalBuffer::refDir()
{
// REF_DIRECTION data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    refDir_p      Array<Double>&            Reference direction
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!refDirOK_p) {
      calMainCol_p->refDir().getColumn (refDir_p);
      refDirOK_p = True;
    };
  };
  return refDir_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::measDirRef()
{
// REF_DIRECTION data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    measDirRef_p      Vector<Int>&          Reference direction frame
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!measDirRefOK_p) {
      calMainCol_p->measDirRef().getColumn (measDirRef_p);
      measDirRefOK_p = True;
    };
  };
  return measDirRef_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::calDescId()
{
// CAL_DESC_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    calDescId_p        Vector<Int>&         Calibration format descriptor
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!calDescIdOK_p) {
      calMainCol_p->calDescId().getColumn (calDescId_p);
      calDescIdOK_p = True;
    };
  };
  return calDescId_p;
};

//----------------------------------------------------------------------------

Vector<Int>& CalBuffer::calHistoryId()
{
// CAL_HISTORY_ID data field accessor
// Input from private data:
//    connectedToIter_p  Bool                 True if connected to iterator
//    calHistoryId_p     Vector<Int>&         Calibration history identifier
//
  // Fill local cache for this column if cache not valid
  if (connectedToIter_p) {
    if (!calHistoryIdOK_p) {
      calMainCol_p->calHistoryId().getColumn (calHistoryId_p);
      calHistoryIdOK_p = True;
    };
  };
  return calHistoryId_p;
};

//----------------------------------------------------------------------------


} //# NAMESPACE CASA - END

