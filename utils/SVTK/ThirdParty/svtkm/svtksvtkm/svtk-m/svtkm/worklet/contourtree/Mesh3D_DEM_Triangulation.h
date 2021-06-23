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

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

//=======================================================================================
//
// COMMENTS:
//
//	Essentially, a vector of data values. BUT we will want them sorted to simplify
//	processing - i.e. it's the robust way of handling simulation of simplicity
//
//	On the other hand, once we have them sorted, we can discard the original data since
//	only the sort order matters
//
//	Since we've been running into memory issues, we'll start being more careful.
//	Clearly, we can eliminate the values if we sort, but in this iteration we are
//	deferring doing a full sort, so we need to keep the values.
//
//=======================================================================================

#ifndef svtkm_worklet_contourtree_mesh3d_dem_triangulation_h
#define svtkm_worklet_contourtree_mesh3d_dem_triangulation_h

#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandleZip.h>
#include <svtkm/worklet/DispatcherMapField.h>

#include <svtkm/worklet/contourtree/ChainGraph.h>
#include <svtkm/worklet/contourtree/LinkComponentCaseTable3D.h>
#include <svtkm/worklet/contourtree/Mesh3D_DEM_SaddleStarter.h>
#include <svtkm/worklet/contourtree/Mesh3D_DEM_VertexOutdegreeStarter.h>
#include <svtkm/worklet/contourtree/Mesh3D_DEM_VertexStarter.h>
#include <svtkm/worklet/contourtree/PrintVectors.h>
#include <svtkm/worklet/contourtree/Types.h>

//#define DEBUG_PRINT 1
//#define DEBUG_TIMING 1

namespace svtkm
{
namespace worklet
{
namespace contourtree
{

template <typename T, typename StorageType>
class Mesh3D_DEM_Triangulation
{
public:
  // original data array
  const svtkm::cont::ArrayHandle<T, StorageType>& values;

  // size of the mesh
  svtkm::Id nRows, nCols, nSlices, nVertices, nLogSteps;

  // array with neighbourhood masks
  svtkm::cont::ArrayHandle<svtkm::Id> neighbourhoodMask;

  // case table information for finding neighbours
  svtkm::cont::ArrayHandle<svtkm::IdComponent> neighbourOffsets3D;
  svtkm::cont::ArrayHandle<svtkm::UInt16> linkComponentCaseTable3D;

  // constructor
  Mesh3D_DEM_Triangulation(const svtkm::cont::ArrayHandle<T, StorageType>& Values,
                           svtkm::Id NRows,
                           svtkm::Id NCols,
                           svtkm::Id NSlices);

  // sets all vertices to point along an outgoing edge (except extrema)
  void SetStarts(svtkm::cont::ArrayHandle<svtkm::Id>& chains, bool descending);

  // sets outgoing paths for saddles
  void SetSaddleStarts(ChainGraph<T, StorageType>& mergeGraph, bool descending);
};

// creates input mesh
template <typename T, typename StorageType>
Mesh3D_DEM_Triangulation<T, StorageType>::Mesh3D_DEM_Triangulation(
  const svtkm::cont::ArrayHandle<T, StorageType>& Values,
  svtkm::Id NRows,
  svtkm::Id NCols,
  svtkm::Id NSlices)
  : values(Values)
  , nRows(NRows)
  , nCols(NCols)
  , nSlices(NSlices)
  , neighbourOffsets3D()
  , linkComponentCaseTable3D()
{
  nVertices = nRows * nCols * nSlices;

  // compute the number of log-jumping steps (i.e. lg_2 (nVertices))
  nLogSteps = 1;
  for (svtkm::Id shifter = nVertices; shifter > 0; shifter >>= 1)
    nLogSteps++;

  neighbourOffsets3D =
    svtkm::cont::make_ArrayHandle(svtkm::worklet::contourtree::neighbourOffsets3D, 42);
  linkComponentCaseTable3D =
    svtkm::cont::make_ArrayHandle(svtkm::worklet::contourtree::linkComponentCaseTable3D, 16384);
}

// sets outgoing paths for saddles
template <typename T, typename StorageType>
void Mesh3D_DEM_Triangulation<T, StorageType>::SetStarts(svtkm::cont::ArrayHandle<svtkm::Id>& chains,
                                                         bool ascending)
{
  // create the neighbourhood mask
  neighbourhoodMask.Allocate(nVertices);

  // For each vertex set the next vertex in the chain
  svtkm::cont::ArrayHandleIndex vertexIndexArray(nVertices);
  Mesh3D_DEM_VertexStarter<T> vertexStarter(nRows, nCols, nSlices, ascending);
  svtkm::worklet::DispatcherMapField<Mesh3D_DEM_VertexStarter<T>> vertexStarterDispatcher(
    vertexStarter);

  vertexStarterDispatcher.Invoke(vertexIndexArray,   // input
                                 values,             // input (whole array)
                                 chains,             // output
                                 neighbourhoodMask); // output
} // SetStarts()

// sets outgoing paths for saddles
template <typename T, typename StorageType>
void Mesh3D_DEM_Triangulation<T, StorageType>::SetSaddleStarts(
  ChainGraph<T, StorageType>& mergeGraph,
  bool ascending)
{
  // we need a temporary inverse index to change vertex IDs
  svtkm::cont::ArrayHandle<svtkm::Id> inverseIndex;
  svtkm::cont::ArrayHandle<svtkm::Id> isCritical;
  svtkm::cont::ArrayHandle<svtkm::Id> outdegree;

  svtkm::cont::ArrayHandleIndex vertexIndexArray(nVertices);
  Mesh3D_DEM_VertexOutdegreeStarter vertexOutdegreeStarter(nRows, nCols, nSlices, ascending);
  svtkm::worklet::DispatcherMapField<Mesh3D_DEM_VertexOutdegreeStarter>
    vertexOutdegreeStarterDispatcher(vertexOutdegreeStarter);

  vertexOutdegreeStarterDispatcher.Invoke(vertexIndexArray,         // input
                                          neighbourhoodMask,        // input
                                          mergeGraph.arcArray,      // input (whole array)
                                          neighbourOffsets3D,       // input (whole array)
                                          linkComponentCaseTable3D, // input (whole array)
                                          outdegree,                // output
                                          isCritical);              // output

  svtkm::cont::Algorithm::ScanExclusive(isCritical, inverseIndex);

  // now we can compute how many critical points we carry forward
  svtkm::Id nCriticalPoints = svtkm::cont::ArrayGetValue(nVertices - 1, inverseIndex) +
    svtkm::cont::ArrayGetValue(nVertices - 1, isCritical);

  // allocate space for the join graph vertex arrays
  mergeGraph.AllocateVertexArrays(nCriticalPoints);

  // compact the set of vertex indices to critical ones only
  svtkm::cont::Algorithm::CopyIf(vertexIndexArray, isCritical, mergeGraph.valueIndex);

  // we initialise the prunesTo array to "NONE"
  svtkm::cont::ArrayHandleConstant<svtkm::Id> notAssigned(NO_VERTEX_ASSIGNED, nCriticalPoints);
  svtkm::cont::Algorithm::Copy(notAssigned, mergeGraph.prunesTo);

  // copy the outdegree from our temporary array
  // : mergeGraph.outdegree[vID] <= outdegree[mergeGraph.valueIndex[vID]]
  svtkm::cont::Algorithm::CopyIf(outdegree, isCritical, mergeGraph.outdegree);

  // copy the chain maximum from arcArray
  // : mergeGraph.chainExtremum[vID] = inverseIndex[mergeGraph.arcArray[mergeGraph.valueIndex[vID]]]
  using IdArrayType = svtkm::cont::ArrayHandle<svtkm::Id>;
  using PermuteIndexType = svtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  svtkm::cont::ArrayHandle<svtkm::Id> tArray;
  tArray.Allocate(nCriticalPoints);
  svtkm::cont::Algorithm::CopyIf(mergeGraph.arcArray, isCritical, tArray);
  svtkm::cont::Algorithm::Copy(PermuteIndexType(tArray, inverseIndex), mergeGraph.chainExtremum);

  // and set up the active vertices - initially to identity
  svtkm::cont::ArrayHandleIndex criticalVertsIndexArray(nCriticalPoints);
  svtkm::cont::Algorithm::Copy(criticalVertsIndexArray, mergeGraph.activeVertices);

  // now we need to compute the firstEdge array from the outdegrees
  svtkm::cont::Algorithm::ScanExclusive(mergeGraph.outdegree, mergeGraph.firstEdge);

  svtkm::Id nCriticalEdges = svtkm::cont::ArrayGetValue(nCriticalPoints - 1, mergeGraph.firstEdge) +
    svtkm::cont::ArrayGetValue(nCriticalPoints - 1, mergeGraph.outdegree);

  // now we allocate the edge arrays
  mergeGraph.AllocateEdgeArrays(nCriticalEdges);

  // and we have to set them, so we go back to the vertices
  Mesh3D_DEM_SaddleStarter saddleStarter(nRows,      // input
                                         nCols,      // input
                                         nSlices,    // input
                                         ascending); // input
  svtkm::worklet::DispatcherMapField<Mesh3D_DEM_SaddleStarter> saddleStarterDispatcher(
    saddleStarter);

  svtkm::cont::ArrayHandleZip<svtkm::cont::ArrayHandle<svtkm::Id>, svtkm::cont::ArrayHandle<svtkm::Id>>
    outDegFirstEdge = svtkm::cont::make_ArrayHandleZip(mergeGraph.outdegree, mergeGraph.firstEdge);

  saddleStarterDispatcher.Invoke(criticalVertsIndexArray,  // input
                                 outDegFirstEdge,          // input (pair)
                                 mergeGraph.valueIndex,    // input
                                 neighbourhoodMask,        // input (whole array)
                                 mergeGraph.arcArray,      // input (whole array)
                                 inverseIndex,             // input (whole array)
                                 neighbourOffsets3D,       // input (whole array)
                                 linkComponentCaseTable3D, // input (whole array)
                                 mergeGraph.edgeNear,      // output (whole array)
                                 mergeGraph.edgeFar,       // output (whole array)
                                 mergeGraph.activeEdges);  // output (whole array)

  // finally, allocate and initialise the edgeSorter array
  svtkm::cont::ArrayCopy(mergeGraph.activeEdges, mergeGraph.edgeSorter);
} // SetSaddleStarts()
}
}
}

#endif
