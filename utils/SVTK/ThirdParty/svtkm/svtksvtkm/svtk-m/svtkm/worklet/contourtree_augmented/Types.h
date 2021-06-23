//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================


#ifndef svtkm_worklet_contourtree_augmented_types_h
#define svtkm_worklet_contourtree_augmented_types_h

#include <svtkm/Types.h>
#include <svtkm/cont/ArrayHandle.h>

namespace svtkm
{
namespace worklet
{
namespace contourtree_augmented
{

// constexpr for bit flags
// clang-format off
constexpr svtkm::Id NO_SUCH_ELEMENT = std::numeric_limits<svtkm::Id>::min();
constexpr svtkm::Id TERMINAL_ELEMENT = std::numeric_limits<svtkm::Id>::max() / 2 + 1; //0x40000000 || 0x4000000000000000
constexpr svtkm::Id IS_SUPERNODE = std::numeric_limits<svtkm::Id>::max() / 4 + 1; //0x20000000 || 0x2000000000000000
constexpr svtkm::Id IS_HYPERNODE = std::numeric_limits<svtkm::Id>::max() / 8 + 1; //0x10000000 || 0x1000000000000000
constexpr svtkm::Id IS_ASCENDING = std::numeric_limits<svtkm::Id>::max() / 16 + 1; //0x08000000 || 0x0800000000000000
constexpr svtkm::Id INDEX_MASK = std::numeric_limits<svtkm::Id>::max() / 16; //0x07FFFFFF || 0x07FFFFFFFFFFFFFF
constexpr svtkm::Id CV_OTHER_FLAG = std::numeric_limits<svtkm::Id>::max() / 8 + 1; //0x10000000 || 0x1000000000000000
// clang-format on

using IdArrayType = svtkm::cont::ArrayHandle<svtkm::Id>;

using EdgePair = svtkm::Pair<svtkm::Id, svtkm::Id>; // here EdgePair.first=low and EdgePair.second=high
using EdgePairArray = svtkm::cont::ArrayHandle<EdgePair>; // Array of edge pairs

// inline functions for retrieving flags or index
SVTKM_EXEC_CONT
inline bool noSuchElement(svtkm::Id flaggedIndex)
{ // noSuchElement()
  return ((flaggedIndex & (svtkm::Id)NO_SUCH_ELEMENT) != 0);
} // noSuchElement()

SVTKM_EXEC_CONT
inline bool isTerminalElement(svtkm::Id flaggedIndex)
{ // isTerminalElement()
  return ((flaggedIndex & TERMINAL_ELEMENT) != 0);
} // isTerminalElement()

SVTKM_EXEC_CONT
inline bool isSupernode(svtkm::Id flaggedIndex)
{ // isSupernode()
  return ((flaggedIndex & IS_SUPERNODE) != 0);
} // isSupernode()

SVTKM_EXEC_CONT
inline bool isHypernode(svtkm::Id flaggedIndex)
{ // isHypernode()
  return ((flaggedIndex & IS_HYPERNODE) != 0);
} // isHypernode()

SVTKM_EXEC_CONT
inline bool isAscending(svtkm::Id flaggedIndex)
{ // isAscending()
  return ((flaggedIndex & IS_ASCENDING) != 0);
} // isAscending()

SVTKM_EXEC_CONT
inline svtkm::Id maskedIndex(svtkm::Id flaggedIndex)
{ // maskedIndex()
  return (flaggedIndex & INDEX_MASK);
} // maskedIndex()

// Used in the context of CombinedVector class used in ContourTreeMesh to merge the mesh of contour trees
SVTKM_EXEC_CONT
inline bool isThis(svtkm::Id flaggedIndex)
{ // isThis
  return ((flaggedIndex & CV_OTHER_FLAG) == 0);
} // isThis

template <typename T>
struct MaskedIndexFunctor
{
  SVTKM_EXEC_CONT

  MaskedIndexFunctor() {}

  SVTKM_EXEC_CONT
  svtkm::Id operator()(T x) const { return maskedIndex(x); }
};

inline std::string flagString(svtkm::Id flaggedIndex)
{ // flagString()
  std::string fString("");
  fString += (noSuchElement(flaggedIndex) ? "n" : ".");
  fString += (isTerminalElement(flaggedIndex) ? "t" : ".");
  fString += (isSupernode(flaggedIndex) ? "s" : ".");
  fString += (isHypernode(flaggedIndex) ? "h" : ".");
  fString += (isAscending(flaggedIndex) ? "a" : ".");
  return fString;
} // flagString()



} // namespace contourtree_augmented
} // worklet
} // svtkm

#endif
