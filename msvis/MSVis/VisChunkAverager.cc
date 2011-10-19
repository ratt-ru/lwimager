//# VisChunkAverager.cc: Implementation of VisChunkAverager.h
//# Copyright (C) 2010
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
//----------------------------------------------------------------------------

#include <msvis/MSVis/VisChunkAverager.h>

namespace casa { //# NAMESPACE CASA - BEGIN

//----------------------------------------------------------------------------
  
VisChunkAverager::VisChunkAverager(const Vector<MS::PredefinedColumns>& dataCols,
                                   const Bool doSpWeight) 
  : colEnums_p(dataCols),
    doSpWeight_p(doSpWeight),
    readyToHash_p(false),
    haveHashMap_p(false)
{
  reset();
}

//----------------------------------------------------------------------------

VisChunkAverager::~VisChunkAverager()
{
// Null default destructor
}

//----------------------------------------------------------------------------

void VisChunkAverager::reset()
{
  haveHashMap_p = false;
  sphash_to_inprows_p.clear();
  readyToHash_p = false;
}

// read all the columns into vb.
void VisChunkAverager::fill_vb(VisBuffer& vb)
{
  vb.time();
  // timeExtraPrec?
  vb.antenna1();
  vb.antenna2();
  // antenna3?
  vb.feed1();
  vb.feed2();
  // feed3?
  // ddid implicitly read during the implicit sort.
  // So shouldn't fieldId have been read as well?
  vb.processorId();
  // phaseId?
  vb.fieldId();
  vb.timeInterval();
  vb.exposure();
  vb.timeCentroid();
  // pulsar_bin?
  // pulsar_gate_id?
  vb.scan();
  vb.arrayId();
  vb.observationId();
  vb.stateId();
  // baseline_ref?
  vb.uvwMat();
  // vb.uvw2();
  for(Int i = colEnums_p.nelements(); i--;){
    if(colEnums_p[i] == MS::CORRECTED_DATA)
      vb.correctedVisCube();
    else if(colEnums_p[i] == MS::MODEL_DATA)
      vb.modelVisCube();
    // else if(colEnums_p[i] == MS::LAG_DATA)
    //  VisBuffer doesn't handle LAG_DATA.
    else if(colEnums_p[i] == MS::FLOAT_DATA)
      vb.floatDataCube();
    else if(colEnums_p[i] == MS::DATA)
      vb.visCube();
  }
  // video_point?
  vb.sigmaMat();
  vb.weightMat();
  if(doSpWeight_p)
    vb.weightSpectrum();
  vb.flagCube();
  vb.flagCategory();
  vb.flagRow();
}

Bool VisChunkAverager::setupHashFunction(ROVisibilityIterator& vi)
{
  if(readyToHash_p)                     // No-op.
    return true;

  VisBuffer vb(vi);

  maxant1p1_p = 1;
  maxant2p1_p = 1;
  maxfeed1p1_p = 1;
  maxfeed2p1_p = 1;
  // maxprocp1_p = max(vb.processorId()) + 1; Not needed

  for(vi.origin(); vi.more(); ++vi){
    maxant1p1_p = uIntMax(maxant1p1_p, 1 + max(vb.antenna1()));
    maxant2p1_p = uIntMax(maxant2p1_p, max(vb.antenna2()) + 1);
    maxfeed1p1_p = uIntMax(maxfeed1p1_p, max(vb.feed1()) + 1);
    maxfeed2p1_p = uIntMax(maxfeed2p1_p, max(vb.feed2()) + 1);
  }
  vi.origin();  // Go back to the start of the chunk.
  return true;
}

VisBuffer& VisChunkAverager::average(ROVisibilityIterator& vi)
{
  // Just in case findCollision() returned before doing it.
  // makeHashMap will return right away if it can.
  haveHashMap_p = makeHashMap(vi);

  VisBuffer vb(vi);
  uInt chunkletNum = 0;
  Bool firstValidOutRowInChunk = true;
  Double time, minTime, maxTime, firstinterval, lastinterval;
  Vector<Bool> firstrowinslot(sphash_to_inprows_p.size());
  firstrowinslot.set(true);

  initialize(vb);
  for(vi.origin(); vi.more(); ++vi){
    // Iterate through the current VisBuffer
    Int outrow = 0;
    Bool firstValidOutRowInIntegration = true;

    mapuIvIType::iterator sphend = sphash_to_inprows_p.end();
    for(mapuIvIType::iterator sphit = sphash_to_inprows_p.begin();
        sphit != sphend; ++sphit){
      Int inrow = sphit->second[chunkletNum];

      if(inrow >= 0){
        if(firstValidOutRowInIntegration){
          firstValidOutRowInIntegration = false;
          time = vb.time()[inrow];

          // Some initialization for the chunk
          if(firstValidOutRowInChunk){
            firstValidOutRowInChunk = false;
            minTime = time;
            maxTime = time;
            firstinterval = vb.timeInterval()[inrow];
            lastinterval = firstinterval;
          }
          
          if(time < minTime){
            minTime = time;
          }
          else if(time > maxTime){
            maxTime = time;
            lastinterval = vb.timeInterval()[inrow];
          }
        }

        // Add the VisBuffer row to the current accumulation
        if(firstrowinslot[outrow]){
          firstrowinslot[outrow] = false;
          avBuf_p.antenna1()[outrow] = vb.antenna1()[inrow];
          avBuf_p.antenna2()[outrow] = vb.antenna2()[inrow];
          avBuf_p.feed1()[outrow] = vb.feed1()[inrow];
          avBuf_p.feed2()[outrow] = vb.feed2()[inrow];
          avBuf_p.observationId()[outrow] = vb.observationId()[inrow];
          avBuf_p.processorId()[outrow] = vb.processorId()[inrow];
          avBuf_p.scan()[outrow] = vb.scan()[inrow];
          avBuf_p.stateId()[outrow] = vb.stateId()[inrow];
        }

        Vector<Float> wtM(vb.weightMat().column(inrow));

        // Accumulate the visibilities and weights, and set the flags.
        Double totwt = 0.0;                       // Total weight for inrow.
        for(Int cor = 0; cor < nCorr_p; ++cor){
          Double channellesswt = 0.0;
          Double constwtperchan = wtM[cor] / nChan_p;

          for(Int chn = 0; chn < nChan_p; ++chn){
            if(!vb.flagCube()(cor, chn, inrow)){
              Double wt = doSpWeight_p ? vb.weightSpectrum()(cor, chn, inrow) :
                constwtperchan;

              if(doSpWeight_p)
                avBuf_p.weightSpectrum()(cor, chn, outrow) += wt;
              channellesswt += wt;
              avBuf_p.flagCube()(cor, chn, outrow) = False;

              // FLAG_CATEGORY?
            
              for(Int i = colEnums_p.nelements(); i--;){
                if(colEnums_p[i] == MS::CORRECTED_DATA)
                  avBuf_p.correctedVisCube()(cor, chn, outrow) += 
                    (wt * vb.correctedVisCube()(cor, chn, inrow));
                else if(colEnums_p[i] == MS::MODEL_DATA)
                  avBuf_p.modelVisCube()(cor, chn, outrow) += 
                    (wt * vb.modelVisCube()(cor, chn, inrow));
                // else if(colEnums_p[i] == MS::LAG_DATA)
                //  VisBuffer doesn't handle LAG_DATA
                else if(colEnums_p[i] == MS::FLOAT_DATA)
                  avBuf_p.floatDataCube()(cor, chn, outrow) += 
                    (wt * vb.floatDataCube()(cor, chn, inrow));
                else if(colEnums_p[i] == MS::DATA)
                  avBuf_p.visCube()(cor, chn, outrow) += 
                    (wt * vb.visCube()(cor, chn, inrow));
              }
            }
          }
          avBuf_p.weightMat()(cor, outrow) += channellesswt;
          totwt += channellesswt;
        }

        Double totslotwt = 0.0;

        for(Int cor = 0; cor < nCorr_p; ++cor)
          totslotwt += avBuf_p.weightMat()(cor, outrow);

        // totwt > 0.0 implies totslotwt > 0.0, as required by
        // the running averages (mandatory for timeCentroid!).
        if(totwt > 0.0){
          Double leverage = totwt / totslotwt;

          // UVW (weighted only by weight for now)
      // // Iterate through slots.
      // for(ui2vmap::iterator slotit = bin_slots_p[tbn].begin();
      //     slotit != bin_slots_p[tbn].end(); ++slotit){
      //   for(uivector::iterator toikit = slotv.begin();
      //       toikit != slotv.end(); ++toikit){
      //       Double wv = 0.0;
      //       Array<Complex>::const_iterator dataEnd = data_toikit.end();
      //       for(Array<Complex>::const_iterator dit = data_toikit.begin();
      //           dit != dataEnd; ++dit)
      //         wv += fabs(*dit);
      //       if(wv > 0.0){
      //         swv += wv;
      //         outUVW += (wv / swv) * (inUVW(*toikit) - outUVW);
      //       }
      //   } // End of loop through the slot's rows.
          for(uInt ax = 0; ax < 3; ++ax)
            avBuf_p.uvwMat()(ax, outrow) += leverage * (vb.uvwMat()(ax, inrow) -
                                                        avBuf_p.uvwMat()(ax, outrow));

          avBuf_p.timeCentroid()[outrow] += leverage *
            (vb.timeCentroid()[inrow] - avBuf_p.timeCentroid()[outrow]);
          avBuf_p.exposure()[outrow] += totwt * vb.exposure()[inrow];
        }
        else if(totslotwt == 0.0){      // Put in a representative UVW.
          for(uInt ax = 0; ax < 3; ++ax)
            avBuf_p.uvwMat()(ax, outrow) = vb.uvwMat()(ax, inrow);
        }
      }
      ++outrow;
    }                   // End of loop over sphit for chunkletNum.
    ++chunkletNum;
  }             // End of loop over chunkletNums (integrations) in vi's current chunk.
  normalize(minTime, maxTime, firstinterval, lastinterval);

  return avBuf_p;
}

void VisChunkAverager::initialize(VisBuffer& vb)
{
// Initialize the averaging buffer.
// Output to private data:
//    avBuf_p          VisBuffer       Averaging buffer
//    readyToHash_p    Bool            sets to True
//    nChan_p          Int             # of channels
//    nCorr_p          Int             # of polarizations

  fill_vb(vb);

  // Assign to vb to establish a two-way connection to the underlying VisIter.
  // Assign main meta info only
  //    avBuf_p.assign(vb,True);  
  avBuf_p.assign(vb, False);  
  avBuf_p.updateCoordInfo();  // This is the (simplified) CalVisBuffer version!
  
  // Immutables:
  nChan_p = vb.nChannel();
  nCorr_p = vb.corrType().nelements();

  // More or less VisBuffAccumulator::initialize() up to
  // "// Fill in the antenna numbers for all rows"
  Int nRow = sphash_to_inprows_p.size();

  avBuf_p.nRow() = nRow;
  avBuf_p.nChannel() = nChan_p;

  // Resize and initialize the VisBuffer columns used here
  avBuf_p.antenna1().resize(nRow);
  avBuf_p.antenna2().resize(nRow);
  // avBuf_p.arrayId() is an Int, not a Vector.
  for(Int i = colEnums_p.nelements(); i--;){
    if(colEnums_p[i] == MS::CORRECTED_DATA)
      avBuf_p.correctedVisCube().resize(nCorr_p, nChan_p, nRow);
    else if(colEnums_p[i] == MS::MODEL_DATA)
      avBuf_p.modelVisCube().resize(nCorr_p, nChan_p, nRow);
    // else if(colEnums_p[i] == MS::LAG_DATA)
    //  VisBuffer doesn't handle LAG_DATA
    else if(colEnums_p[i] == MS::FLOAT_DATA)
      avBuf_p.floatDataCube().resize(nCorr_p, nChan_p, nRow);
    else if(colEnums_p[i] == MS::DATA)
      avBuf_p.visCube().resize(nCorr_p, nChan_p, nRow);
  }
  // avBuf_p.dataDescriptionId() is an Int, not a Vector.
  avBuf_p.exposure().resize(nRow);
  avBuf_p.feed1().resize(nRow);
  avBuf_p.feed2().resize(nRow);
  // avBuf_p.fieldId() is an Int, not a Vector.
  avBuf_p.flagCube().resize(nCorr_p, nChan_p, nRow);
  avBuf_p.flagRow().resize(nRow); 
  avBuf_p.observationId().resize(nRow);
  avBuf_p.processorId().resize(nRow);
  avBuf_p.scan().resize(nRow);

  // Use sigmaMat() throughout, not the correlation-averaged sigma().
  avBuf_p.sigmaMat().resize(nCorr_p, nRow); 

  avBuf_p.stateId().resize(nRow);
  avBuf_p.time().resize(nRow); 
  avBuf_p.timeCentroid().resize(nRow);
  avBuf_p.timeInterval().resize(nRow); 
  avBuf_p.uvwMat().resize(3, nRow); 

  // Use weightMat() throughout, not the correlation-averaged weight().
  avBuf_p.weightMat().resize(nCorr_p, nRow); 

  if(doSpWeight_p)
    avBuf_p.weightSpectrum().resize(nCorr_p, nChan_p, nRow);

  // Fill in the antenna numbers for all rows.
  // Do not assume a constant number of antennas like VisBuffAccumulator.

  // Initialize everything else
  const Complex czero(0.0);
  for(Int i = colEnums_p.nelements(); i--;){
    if(colEnums_p[i] == MS::CORRECTED_DATA)
      avBuf_p.correctedVisCube() = czero;
    else if(colEnums_p[i] == MS::MODEL_DATA)
      avBuf_p.modelVisCube() = czero;
    // else if(colEnums_p[i] == MS::LAG_DATA)
    //  VisBuffer doesn't handle LAG_DATA.
    else if(colEnums_p[i] == MS::FLOAT_DATA)
      avBuf_p.floatDataCube() = 0.0;
    else if(colEnums_p[i] == MS::DATA)
      avBuf_p.visCube() = czero;
  }
  avBuf_p.exposure() = 0.0;
  avBuf_p.flagCube() = True;
  avBuf_p.flagRow() = False;
  avBuf_p.scan() = 0;                   // This should be unnecessary.
  avBuf_p.time() = 0.0;                 // This should be unnecessary.
  avBuf_p.timeInterval() = 0.0;         // This should be unnecessary.
  avBuf_p.timeCentroid() = 0.0;
  avBuf_p.uvwMat() = 0.0;
  avBuf_p.weightMat() = 0.0;

  if(doSpWeight_p)
    avBuf_p.weightSpectrum() = 0.0;
}

//----------------------------------------------------------------------------

void VisChunkAverager::normalize(const Double minTime, const Double maxTime,
                                 const Double firstinterval,
                                 const Double lastinterval)
{
// Normalize the current accumulation timeBin
// Output to private data:
//    avBuf_p         VisBuffer&       Averaged buffer
//  

  // These columns are independently normalized:
  // DATA_DESC_ID, UVW, TIME_CENTROID, WEIGHT,
  // WEIGHT_SPECTRUM, FLAG

  // Things that are the same throughout avBuf_p.
  // A short running average (for numerical stability).
  avBuf_p.time() = minTime + 0.5 * (maxTime - minTime);

  avBuf_p.timeInterval() = maxTime - minTime +
                           0.5 * (firstinterval + lastinterval);

  // Already done by initialize()'s avBuf_p.assign(vb, False);
  // avBuf_p.dataDescriptionId() = vb.dataDescriptionId();

  uInt outrow = 0;
  mapuIvIType::iterator sphend = sphash_to_inprows_p.end();

  for(mapuIvIType::iterator sphit = sphash_to_inprows_p.begin();
      sphit != sphend; ++sphit){
    // Divide by the weights
    Vector<Float> wtM(avBuf_p.weightMat().column(outrow));
    Double wt = 0.0;  // total weight for the output row.

    for(Int cor = 0; cor < nCorr_p; ++cor){
      Float orw = wtM[cor];
        
      avBuf_p.sigmaMat()(cor, outrow) = orw > 0.0 ? sqrt(1.0 / orw)
                                        : -1.0; // Seems safer than 0.0.
      wt += orw;
    }

    if(wt == 0.0){
      avBuf_p.flagRow()[outrow] = True;

      // Looks better than 0 (-> 1858).
      avBuf_p.timeCentroid()[outrow] = avBuf_p.time()[outrow];
    }
    else{
      uInt nrowsinslot = 0;
      std::vector<Int>::iterator vend = sphit->second.end();

      for(std::vector<Int>::iterator vit = sphit->second.begin();
          vit != vend; ++vit){
        if(*vit >= 0)
          ++nrowsinslot;
      }

      avBuf_p.exposure()[outrow] *= nrowsinslot / wt;
    }

    if(!avBuf_p.flagRow()[outrow]){
      for(Int cor = 0; cor < nCorr_p; ++cor){
        Double constwtperchan = wtM[cor] / nChan_p;

        for(Int chn = 0; chn < avBuf_p.nChannel(); ++chn){
          if(!avBuf_p.flagCube()(cor, chn, outrow)){
            Double w = doSpWeight_p ? avBuf_p.weightSpectrum()(cor, chn, outrow)
              : constwtperchan;

            // The 2nd choice begs the question of why it isn't flagged.
            Double norm = w > 0.0 ? 1.0 / w : 0.0;      

            for(Int i = colEnums_p.nelements(); i--;){
              if(colEnums_p[i] == MS::CORRECTED_DATA)
                avBuf_p.correctedVisCube()(cor, chn, outrow) *= norm;
              else if(colEnums_p[i] == MS::MODEL_DATA)
                avBuf_p.modelVisCube()(cor, chn, outrow) *= norm;
              // else if(colEnums_p[i] == MS::LAG_DATA)
              //  VisBuffer doesn't handle LAG_DATA.
              else if(colEnums_p[i] == MS::FLOAT_DATA)
                avBuf_p.floatDataCube()(cor, chn, outrow) *= norm;
              else if(colEnums_p[i] == MS::DATA)
                avBuf_p.visCube()(cor, chn, outrow) *= norm;
            }
          }     // ends if(!avBuf_p.flagCube()(cor, chn, outrow))
        }       // ends chan loop
      }         // ends cor loop
    }           // ends if(!avBuf_p.flagRow()(outrow))
    ++outrow;
  }             // ends sphit loop
}

uInt VisChunkAverager::hashFunction(const Int ant1, const Int ant2,
                                   const Int feed1, const Int feed2,
                                   const Int procid) const
{
  return feed2 +
    maxfeed2p1_p * (feed1 +
                    maxfeed1p1_p * (ant2 +
                                    maxant2p1_p * (ant1 +
                                                   maxant1p1_p * procid)));
};

Bool VisChunkAverager::makeHashMap(ROVisibilityIterator& vi)
{
  if(haveHashMap_p)     // No-op
    return true;

  readyToHash_p = setupHashFunction(vi);
  if(!readyToHash_p)
    return false;

  sphash_to_inprows_p.clear();
  mapuIvIType::iterator poshint = sphash_to_inprows_p.begin(); 

  uInt nChunklets = vi.nSubInterval();  // The # of vb's in vi's current chunk.
  uInt chunkletNum = 0;
  VisBuffer vb(vi);

  for(vi.origin(); vi.more(); ++vi){
    uInt nrow = vb.nRow();

    for(uInt rn = 0; rn < nrow; ++rn){
      if(!vb.flagRow()[rn] && !allTrue(vb.flagCube().xyPlane(rn))){
        // sphash = sparse hash
        uInt sphash = hashFunction(vb.antenna1()[rn], vb.antenna2()[rn],
                                   vb.feed1()[rn], vb.feed2()[rn],
                                   vb.processorId()[rn]);

        if(sphash_to_inprows_p.count(sphash) < 1){
          sphash_to_inprows_p.insert(poshint,
                            std::pair<uInt, std::vector<Int> >(sphash,
                                                        std::vector<Int>()));
          ++poshint;

          // Fill the new vector of input rows with "the skip value".
          sphash_to_inprows_p[sphash].assign(nChunklets, -1);
        }
        sphash_to_inprows_p[sphash][chunkletNum] = rn;
      }
    }
    ++chunkletNum;
  }
  vi.origin();  // Go back to the start of the chunk.
  return true;
}

Bool VisChunkAverager::check_chunk(ROVisibilityIterator& vi, Double& time_to_break,
                                  const Bool watch_obs, const Bool watch_scan,
                                  const Bool watch_state)
{
  Bool foundbreak;
  Vector<MS::PredefinedColumns> dataCols;
  //VisChunkAverager vba(time_to_break, dataCols, false);
  VisChunkAverager vba(dataCols, false);

  foundbreak = vba.findCollision(vi, time_to_break, watch_obs,
                                 watch_scan, watch_state);
  return foundbreak;
}

Bool VisChunkAverager::findCollision(ROVisibilityIterator& vi, Double& time_to_break,
                                    const Bool watchObs, const Bool watchScan,
                                    const Bool watchState)
{
  if(!(watchScan || watchState || watchObs))
    return false;       // Nothing would count as a collision.

  if(vi.nRowChunk() < 2)        // It takes two to collide, so
    return false;               // return before accessing nonexistent rows.

  haveHashMap_p = makeHashMap(vi);

  if(!haveHashMap_p)
    throw(AipsError("VisChunkAverager::findCollision(): could not make a hash map"));

  Bool foundCollision = false;
  VisBuffer vb(vi);
  uInt chunkletNum = 0;
  uInt nslots = sphash_to_inprows_p.size();
  Vector<Int> firstScan;    // The first scan for each sphash
  Vector<Int> firstState;   // The first state for each sphash
  Vector<Int> firstObs;     // The first obsID for each sphash

  if(watchScan)
    firstScan.resize(nslots);
  if(watchState)
    firstState.resize(nslots);
  if(watchObs)
    firstObs.resize(nslots);

  for(vi.origin(); vi.more(); ++vi){
    if(foundCollision)                  // The last chunklet did it.
      break;

    // We want the soonest time_to_break in all the keys.  Default
    // time_to_break to the last time in the latest chunklet, assuming vb is
    // time sorted.
    time_to_break = vb.time()[vb.nRow() - 1];

    uInt slotnum = 0;
    for(mapuIvIType::const_iterator keyit = sphash_to_inprows_p.begin();
        keyit != sphash_to_inprows_p.end(); ++keyit){
      Int row = keyit->second[chunkletNum];

      if(row >= 0){     // If the chunklet has this sphash...
        // Don't collapse this into an || chain - time_to_break needs to be evaluated
        // even if a collision has already been found.
        if(watchScan)
          foundCollision |= checkForBreak(firstScan, vb.scan()[row], slotnum,
                                          chunkletNum, keyit->second);
        if(watchState)
          foundCollision |= checkForBreak(firstState, vb.stateId()[row], slotnum,
                                          chunkletNum, keyit->second);
        //if(watchArray)  arrayIDs are already separated by the chunking.
        if(watchObs)
          foundCollision |= checkForBreak(firstObs, vb.observationId()[row],
                                          slotnum, chunkletNum, keyit->second);
      
        if(foundCollision){
          time_to_break = vb.time()[row];
          break;
        }
      }
      ++slotnum;
    }
    ++chunkletNum;
  }
  vi.origin();                  // Go back to the start of the chunk.
  return foundCollision;
}

Bool VisChunkAverager::checkForBreak(Vector<Int>& firstVals, const Int i, 
                                     const uInt slotnum,
                                     const uInt chunkletNum,
                                     const std::vector<Int>& inrows_for_slot) const
{
  if(chunkletNum == 0 || inrows_for_slot[chunkletNum - 1] < 0)
    firstVals[slotnum] = i;
  return i != firstVals[slotnum];
}

} //# NAMESPACE CASA - END

