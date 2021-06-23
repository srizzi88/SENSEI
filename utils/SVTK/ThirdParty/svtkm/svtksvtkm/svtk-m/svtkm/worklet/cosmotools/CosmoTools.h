//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

#ifndef svtkm_worklet_cosmotools_cosmotools_h
#define svtkm_worklet_cosmotools_cosmotools_h

#include <svtkm/worklet/cosmotools/ComputeBinIndices.h>
#include <svtkm/worklet/cosmotools/ComputeBinRange.h>
#include <svtkm/worklet/cosmotools/ComputeBins.h>
#include <svtkm/worklet/cosmotools/ComputeNeighborBins.h>
#include <svtkm/worklet/cosmotools/GraftParticles.h>
#include <svtkm/worklet/cosmotools/IsStar.h>
#include <svtkm/worklet/cosmotools/MarkActiveNeighbors.h>
#include <svtkm/worklet/cosmotools/PointerJump.h>
#include <svtkm/worklet/cosmotools/ValidHalo.h>

#include <svtkm/worklet/cosmotools/ComputePotential.h>
#include <svtkm/worklet/cosmotools/ComputePotentialBin.h>
#include <svtkm/worklet/cosmotools/ComputePotentialMxN.h>
#include <svtkm/worklet/cosmotools/ComputePotentialNeighbors.h>
#include <svtkm/worklet/cosmotools/ComputePotentialNxN.h>
#include <svtkm/worklet/cosmotools/ComputePotentialOnCandidates.h>
#include <svtkm/worklet/cosmotools/EqualsMinimumPotential.h>
#include <svtkm/worklet/cosmotools/SetCandidateParticles.h>

#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandleReverse.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/worklet/ScatterCounting.h>

#include <svtkm/BinaryPredicates.h>
#include <svtkm/Math.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

//#define DEBUG_PRINT 1

namespace
{

///////////////////////////////////////////////////////////////////////////////
//
// Debug prints
//
///////////////////////////////////////////////////////////////////////////////
template <typename U>
void DebugPrint(const char* msg, svtkm::cont::ArrayHandle<U>& array)
{
  svtkm::Id count = 20;
  count = std::min(count, array.GetNumberOfValues());
  std::cout << std::setw(15) << msg << ": ";
  for (svtkm::Id i = 0; i < count; i++)
    std::cout << std::setprecision(3) << std::setw(5) << array.GetPortalConstControl().Get(i)
              << " ";
  std::cout << std::endl;
}

template <typename U>
void DebugPrint(const char* msg, svtkm::cont::ArrayHandleReverse<svtkm::cont::ArrayHandle<U>>& array)
{
  svtkm::Id count = 20;
  count = std::min(count, array.GetNumberOfValues());
  std::cout << std::setw(15) << msg << ": ";
  for (svtkm::Id i = 0; i < count; i++)
    std::cout << std::setw(5) << array.GetPortalConstControl().Get(i) << " ";
  std::cout << std::endl;
}
}

namespace svtkm
{
namespace worklet
{
namespace cosmotools
{


///////////////////////////////////////////////////////////////////////////////
//
// Scatter the result of a reduced array
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct ScatterWorklet : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inIndices, FieldOut outIndices);
  using ExecutionSignature = void(_1, _2);
  using ScatterType = svtkm::worklet::ScatterCounting;

  SVTKM_EXEC
  void operator()(T inputIndex, T& outputIndex) const { outputIndex = inputIndex; }
};

///////////////////////////////////////////////////////////////////////////////
//
// Scale or offset values of an array
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct ScaleBiasFunctor
{
  T Scale;
  T Bias;

  SVTKM_CONT
  ScaleBiasFunctor(T scale = T(1), T bias = T(0))
    : Scale(scale)
    , Bias(bias)
  {
  }

  SVTKM_EXEC_CONT
  T operator()(T value) const { return (Scale * value + Bias); }
};

template <typename T, typename StorageType>
class CosmoTools
{
public:
  using DeviceAlgorithm = svtkm::cont::Algorithm;
  const svtkm::Id NUM_NEIGHBORS = 9;

  // geometry of domain
  const svtkm::Id nParticles;
  const T particleMass;
  const svtkm::Id minPartPerHalo;
  const T linkLen;
  svtkm::Id numBinsX;
  svtkm::Id numBinsY;
  svtkm::Id numBinsZ;

  // particle locations within domain
  using LocationType = typename svtkm::cont::ArrayHandle<T, StorageType>;
  LocationType& xLoc;
  LocationType& yLoc;
  LocationType& zLoc;

  // cosmo tools constructor for all particles
  CosmoTools(const svtkm::Id NParticles,                  // Number of particles
             const T mass,                               // Particle mass for potential
             const svtkm::Id pmin,                        // Minimum particles per halo
             const T bb,                                 // Linking length between particles
             svtkm::cont::ArrayHandle<T, StorageType>& X, // Physical location of each particle
             svtkm::cont::ArrayHandle<T, StorageType>& Y,
             svtkm::cont::ArrayHandle<T, StorageType>& Z);

  // cosmo tools constructor for particles in one halo
  CosmoTools(const svtkm::Id NParticles,                  // Number of particles
             const T mass,                               // Particle mass for potential
             svtkm::cont::ArrayHandle<T, StorageType>& X, // Physical location of each particle
             svtkm::cont::ArrayHandle<T, StorageType>& Y,
             svtkm::cont::ArrayHandle<T, StorageType>& Z);
  ~CosmoTools() {}

  // Halo finding and center finding on halos
  void HaloFinder(svtkm::cont::ArrayHandle<svtkm::Id>& resultHaloId,
                  svtkm::cont::ArrayHandle<svtkm::Id>& resultMBP,
                  svtkm::cont::ArrayHandle<T>& resultPot);
  void BinParticlesAll(svtkm::cont::ArrayHandle<svtkm::Id>& partId,
                       svtkm::cont::ArrayHandle<svtkm::Id>& binId,
                       svtkm::cont::ArrayHandle<svtkm::Id>& leftNeighbor,
                       svtkm::cont::ArrayHandle<svtkm::Id>& rightNeighbor);
  void MBPCenterFindingByHalo(svtkm::cont::ArrayHandle<svtkm::Id>& partId,
                              svtkm::cont::ArrayHandle<svtkm::Id>& haloId,
                              svtkm::cont::ArrayHandle<svtkm::Id>& mbpId,
                              svtkm::cont::ArrayHandle<T>& minPotential);

  // MBP Center finding on single halo using NxN algorithm
  svtkm::Id MBPCenterFinderNxN(T* nxnPotential);

  // MBP Center finding on single halo using MxN estimation
  svtkm::Id MBPCenterFinderMxN(T* mxnPotential);

  void BinParticlesHalo(svtkm::cont::ArrayHandle<svtkm::Id>& partId,
                        svtkm::cont::ArrayHandle<svtkm::Id>& binId,
                        svtkm::cont::ArrayHandle<svtkm::Id>& uniqueBins,
                        svtkm::cont::ArrayHandle<svtkm::Id>& partPerBin,
                        svtkm::cont::ArrayHandle<svtkm::Id>& particleOffset,
                        svtkm::cont::ArrayHandle<svtkm::Id>& binX,
                        svtkm::cont::ArrayHandle<svtkm::Id>& binY,
                        svtkm::cont::ArrayHandle<svtkm::Id>& binZ);
  void MBPCenterFindingByKey(svtkm::cont::ArrayHandle<svtkm::Id>& keyId,
                             svtkm::cont::ArrayHandle<svtkm::Id>& partId,
                             svtkm::cont::ArrayHandle<T>& minPotential);
};

///////////////////////////////////////////////////////////////////////////////
//
// Constructor for all particles in the system
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
CosmoTools<T, StorageType>::CosmoTools(const svtkm::Id NParticles,
                                       const T mass,
                                       const svtkm::Id pmin,
                                       const T bb,
                                       svtkm::cont::ArrayHandle<T, StorageType>& X,
                                       svtkm::cont::ArrayHandle<T, StorageType>& Y,
                                       svtkm::cont::ArrayHandle<T, StorageType>& Z)
  : nParticles(NParticles)
  , particleMass(mass)
  , minPartPerHalo(pmin)
  , linkLen(bb)
  , xLoc(X)
  , yLoc(Y)
  , zLoc(Z)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Constructor for particles in a single halo
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename StorageType>
CosmoTools<T, StorageType>::CosmoTools(const svtkm::Id NParticles,
                                       const T mass,
                                       svtkm::cont::ArrayHandle<T, StorageType>& X,
                                       svtkm::cont::ArrayHandle<T, StorageType>& Y,
                                       svtkm::cont::ArrayHandle<T, StorageType>& Z)
  : nParticles(NParticles)
  , particleMass(mass)
  , minPartPerHalo(10)
  , linkLen(0.2f)
  , xLoc(X)
  , yLoc(Y)
  , zLoc(Z)
{
}
}
}
}
#endif
