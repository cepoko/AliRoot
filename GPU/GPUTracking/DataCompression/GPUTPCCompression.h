//**************************************************************************\
//* This file is property of and copyright by the ALICE Project            *\
//* ALICE Experiment at CERN, All rights reserved.                         *\
//*                                                                        *\
//* Primary Authors: Matthias Richter <Matthias.Richter@ift.uib.no>        *\
//*                  for The ALICE HLT Project.                            *\
//*                                                                        *\
//* Permission to use, copy, modify and distribute this software and its   *\
//* documentation strictly for non-commercial purposes is hereby granted   *\
//* without fee, provided that the above copyright notice appears in all   *\
//* copies and that both the copyright notice and this permission notice   *\
//* appear in the supporting documentation. The authors make no claims     *\
//* about the suitability of this software for any purpose. It is          *\
//* provided "as is" without express or implied warranty.                  *\
//**************************************************************************

/// \file GPUTPCCompression.h
/// \author David Rohr

#ifndef GPUTPCCOMPRESSION_H
#define GPUTPCCOMPRESSION_H

#include "GPUDef.h"
#include "GPUProcessor.h"
#include "GPUCommonMath.h"
#include "GPUParam.h"

#ifdef HAVE_O2HEADERS
#include "DataFormatsTPC/CompressedClusters.h"
#else
namespace o2
{
namespace tpc
{
template <class T>
struct CompressedClustersPtrs_helper {
};
struct CompressedClustersCounters {
};
using CompressedClusters = CompressedClustersPtrs_helper<CompressedClustersCounters>;
struct CompressedClustersPtrsOnly {
};
} // namespace tpc
} // namespace o2
#endif

namespace GPUCA_NAMESPACE
{
namespace gpu
{
class GPUTPCGMMerger;

class GPUTPCCompression : public GPUProcessor
{
  friend class GPUTPCCompressionKernels;
  friend class GPUChainTracking;

 public:
#ifndef GPUCA_GPUCODE
  void InitializeProcessor();
  void RegisterMemoryAllocation();
  void SetMaxData();

  void* SetPointersOutputHost(void* mem);
  void* SetPointersOutput(void* mem);
  void* SetPointersScratch(void* mem);
  void* SetPointersMemory(void* mem);
#endif

  GPUd() static void truncateSignificantBitsCharge(unsigned short& charge, const GPUParam& param)
  {
    truncateSignificantBits(charge, param.rec.tpcSigBitsCharge);
  }
  GPUd() static void truncateSignificantBitsWidth(unsigned char& width, const GPUParam& param) { truncateSignificantBits(width, param.rec.tpcSigBitsWidth); }

 protected:
  struct memory {
    unsigned int nStoredTracks = 0;
    unsigned int nStoredAttachedClusters = 0;
    unsigned int nStoredUnattachedClusters = 0;
  };

  constexpr static unsigned int NSLICES = GPUCA_NSLICES;

  o2::tpc::CompressedClustersPtrsOnly mPtrs;
  o2::tpc::CompressedClusters mOutput;
  const GPUTPCGMMerger* mMerger = nullptr;

  memory* mMemory = nullptr;
  unsigned int* mAttachedClusterFirstIndex = nullptr;
  unsigned int* mClusterSortBuffer = nullptr;
  unsigned char* mClusterStatus = nullptr;

  unsigned int mMaxTracks = 0;
  unsigned int mMaxClusters = 0;
  unsigned int mMaxTrackClusters = 0;
  unsigned int mNMaxClusterSliceRow = 0;
  unsigned int mNGPUBlocks = 0;

  template <class T>
  void SetPointersCompressedClusters(void*& mem, T& c, unsigned int nClA, unsigned int nTr, unsigned int nClU, bool reducedClA);
  template <class T>
  GPUd() static void truncateSignificantBits(T& val, unsigned int nBits);

  short mMemoryResOutput = -1;
  short mMemoryResOutputHost = -1;
  short mMemoryResMemory = -1;
  short mMemoryResScratch = -1;
};

template <class T>
GPUdi() void GPUTPCCompression::truncateSignificantBits(T& v, unsigned int nBits)
{
  if (nBits == 0) {
    return;
  }

  unsigned int val = v;
  unsigned int ldz = sizeof(unsigned int) * 8 - CAMath::Clz(val);
  if (val && ldz > nBits) {
    if (val & (1 << (ldz - nBits - 1))) {
      val += (1 << (ldz - nBits - 1));
      ldz = sizeof(unsigned int) * 8 - CAMath::Clz(val);
    }
    val &= ((1 << ldz) - 1) ^ ((1 << (ldz - nBits)) - 1);
    // GPUInfo("CHANGING X %x --> %x", (unsigned int) v, val);
    v = val;
  }
}
} // namespace gpu
} // namespace GPUCA_NAMESPACE

#endif
