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

#ifndef svtkm_worklet_contourtree_augmented_activegraph_h
#define svtkm_worklet_contourtree_augmented_activegraph_h

#include <iomanip>
#include <numeric>

// local includes
#include <svtkm/worklet/contourtree_augmented/ArrayTransforms.h>
#include <svtkm/worklet/contourtree_augmented/MergeTree.h>
#include <svtkm/worklet/contourtree_augmented/MeshExtrema.h>
#include <svtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <svtkm/worklet/contourtree_augmented/Types.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/BuildChainsWorklet.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/BuildTrunkWorklet.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/CompactActiveEdgesComputeNewVertexOutdegree.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/CompactActiveEdgesTransferActiveEdges.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/EdgePeakComparator.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/FindGoverningSaddlesWorklet.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/FindSuperAndHyperNodesWorklet.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/HyperArcSuperNodeComparator.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/InitializeActiveEdges.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/InitializeActiveGraphVertices.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/InitializeEdgeFarFromActiveIndices.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/InitializeHyperarcsFromActiveIndices.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/InitializeNeighbourhoodMasksAndOutDegrees.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/SetArcsConnectNodes.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/SetArcsSetSuperAndHypernodeArcs.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/SetArcsSlideVertices.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/SetHyperArcsWorklet.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/SetSuperArcsSetTreeHyperparents.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/SetSuperArcsSetTreeSuperarcs.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/SuperArcNodeComparator.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/TransferRegularPointsWorklet.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/TransferSaddleStartsResetEdgeFar.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/TransferSaddleStartsSetNewOutdegreeForSaddles.h>
#include <svtkm/worklet/contourtree_augmented/activegraph/TransferSaddleStartsUpdateEdgeSorter.h>


//SVTKM includes
#include <svtkm/Types.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/Invoker.h>


namespace active_graph_inc_ns = svtkm::worklet::contourtree_augmented::active_graph_inc;


namespace svtkm
{
namespace worklet
{
namespace contourtree_augmented
{


class ActiveGraph
{ // class ActiveGraph

  template <typename T, typename S>
  static T GetLastValue(const svtkm::cont::ArrayHandle<T, S>& ah)
  {
    return svtkm::cont::ArrayGetValue(ah.GetNumberOfValues() - 1, ah);
  }

public:
  svtkm::cont::Invoker Invoke;

  // we also need the orientation of the edges (i.e. is it join or split)
  bool isJoinGraph;

  // we will store the number of iterations the computation took here
  svtkm::Id nIterations;

  // ARRAYS FOR NODES IN THE TOPOLOGY GRAPH

  // for each vertex, we need to know where it is in global sort order / mesh
  IdArrayType globalIndex;

  // the hyperarcs - i.e. the pseudoextremum defining the hyperarc the vertex is on
  IdArrayType hyperarcs;

  // the first edge for each vertex
  IdArrayType firstEdge;

  // the outdegree for each vertex
  IdArrayType outdegree;

  // ARRAYS FOR EDGES IN THE TOPOLOGY GRAPH

  // we will also need to keep track of both near and far ends of each edge
  IdArrayType edgeFar, edgeNear;

  // these now track the active nodes, edges, &c.:
  IdArrayType activeVertices;
  IdArrayType activeEdges;

  // and an array for sorting edges
  IdArrayType edgeSorter;

  // temporary arrays for super/hyper ID numbers
  IdArrayType superID, hyperID;

  // variables tracking size of super/hyper tree
  svtkm::Id nSupernodes, nHypernodes;

  // BASIC ROUTINES: CONSTRUCTOR, PRINT, &c.

  // constructor takes necessary references
  ActiveGraph(bool IsJoinGraph);

  // initialises the active graph
  template <class Mesh>
  void Initialise(Mesh& mesh, const MeshExtrema& meshExtrema);

  // routine that computes the merge tree from the active graph
  // was previously Compute()
  void MakeMergeTree(MergeTree& tree, MeshExtrema& meshExtrema);

  // sorts saddle starts to find governing saddles
  void FindGoverningSaddles();

  // marks now regular points for removal
  void TransferRegularPoints();

  // compacts the active vertex list
  void CompactActiveVertices();

  // compacts the active edge list
  void CompactActiveEdges();

  // builds the chains for the new active vertices
  void BuildChains();

  // suppresses non-saddles for the governing saddles pass
  void TransferSaddleStarts();

  // sets all remaining active vertices
  void BuildTrunk();

  // finds all super and hyper nodes, numbers them & sets up arrays for lookup
  void FindSuperAndHyperNodes(MergeTree& tree);

  // uses active graph to set superarcs & hyperparents in merge tree
  void SetSuperArcs(MergeTree& tree);

  // uses active graph to set hypernodes in merge tree
  void SetHyperArcs(MergeTree& tree);

  // uses active graph to set arcs in merge tree
  void SetArcs(MergeTree& tree, MeshExtrema& meshExtrema);

  // Allocate the vertex array
  void AllocateVertexArrays(svtkm::Id nElems);

  // Allocate the edge array
  void AllocateEdgeArrays(svtkm::Id nElems);

  // releases temporary arrays
  void ReleaseTemporaryArrays();

  // prints the contents of the active graph in a standard format
  void DebugPrint(const char* message, const char* fileName, long lineNum);

}; // class ActiveGraph


// constructor takes necessary references
ActiveGraph::ActiveGraph(bool IsJoinGraph)
  : Invoke()
  , isJoinGraph(IsJoinGraph)
{ // constructor
  nIterations = 0;
  nSupernodes = 0;
  nHypernodes = 0;
} // constructor



// initialises the active graph
template <class Mesh>
void ActiveGraph::Initialise(Mesh& mesh, const MeshExtrema& meshExtrema)
{ // InitialiseActiveGraph()
  // reference to the correct array in the extrema
  const IdArrayType& extrema = isJoinGraph ? meshExtrema.peaks : meshExtrema.pits;

  // For every vertex, work out whether it is critical
  // We do so by computing outdegree in the mesh & suppressing the vertex if outdegree is 1
  // All vertices of outdegree 0 must be extrema
  // Saddle points must be at least outdegree 2, so this is a correct test
  // BUT it is possible to overestimate the degree of a non-extremum,
  // The test is therefore necessary but not sufficient, and extra vertices are put in the active graph

  // Neighbourhood mask (one bit set per connected component in neighbourhood
  IdArrayType neighbourhoodMasks;
  neighbourhoodMasks.Allocate(mesh.nVertices);
  IdArrayType outDegrees; // TODO Should we change this to an unsigned type
  outDegrees.Allocate(mesh.nVertices);

  // Initialize the nerighborhoodMasks and outDegrees arrays
  mesh.setPrepareForExecutionBehavior(isJoinGraph);
  svtkm::cont::ArrayHandleIndex sortIndexArray(mesh.nVertices);
  active_graph_inc_ns::InitializeNeighbourhoodMasksAndOutDegrees initNeighMasksAndOutDegWorklet(
    isJoinGraph);

  this->Invoke(initNeighMasksAndOutDegWorklet,
               sortIndexArray,
               mesh,
               neighbourhoodMasks, // output
               outDegrees);        // output

  // next, we compute where each vertex lands in the new array
  // it needs to be one place offset, hence the +/- 1
  // this should automatically parallelise
  // The following commented code block is variant ported directly from PPP2 using std::partial_sum. This has been replaced here with svtkm's ScanExclusive.
  /*auto oneIfCritical = [](unsigned x) { return x!= 1 ? 1 : 0; };

    // we need a temporary inverse index to change vertex IDs
    IdArrayType inverseIndex;
    inverseIndex.Allocate(mesh.nVertices);
    inverseIndex.GetPortalControl().Set(0,0);

    std::partial_sum(
            boost::make_transform_iterator(svtkm::cont::ArrayPortalToIteratorBegin(outDegrees.GetPortalControl()), oneIfCritical),
            boost::make_transform_iterator(svtkm::cont::ArrayPortalToIteratorEnd(outDegrees.GetPortalControl())-1, oneIfCritical),
            svtkm::cont::ArrayPortalToIteratorBegin(inverseIndex.GetPortalControl()) + 1);
    */
  IdArrayType inverseIndex;
  onefIfCritical oneIfCriticalFunctor;
  auto oneIfCriticalArrayHandle =
    svtkm::cont::ArrayHandleTransform<IdArrayType, onefIfCritical>(outDegrees, oneIfCriticalFunctor);
  svtkm::cont::Algorithm::ScanExclusive(oneIfCriticalArrayHandle, inverseIndex);

  // now we can compute how many critical points we carry forward
  svtkm::Id nCriticalPoints =
    this->GetLastValue(inverseIndex) + oneIfCriticalFunctor(this->GetLastValue(outDegrees));

  // we need to keep track of what the index of each vertex is in the active graph
  // for most vertices, this should have the NO_SUCH_VERTEX flag set
  AllocateVertexArrays(
    nCriticalPoints); // allocates outdegree, globalIndex, hyperarcs, activeVertices

  // our processing now depends on the degree of the vertex
  // but basically, we want to set up the arrays for this vertex:
  // activeIndex gets the next available ID in the active graph (was called nearIndex before)
  // globalIndex stores the index in the join tree for later access
  IdArrayType activeIndices;
  activeIndices.Allocate(mesh.nVertices);
  svtkm::cont::ArrayHandleConstant<svtkm::Id> noSuchElementArray((svtkm::Id)NO_SUCH_ELEMENT,
                                                               mesh.nVertices);
  svtkm::cont::Algorithm::Copy(noSuchElementArray, activeIndices);

  active_graph_inc_ns::InitializeActiveGraphVertices initActiveGraphVerticesWorklet;
  this->Invoke(initActiveGraphVerticesWorklet,
               sortIndexArray,
               outDegrees,
               inverseIndex,
               extrema,
               activeIndices,
               globalIndex,
               outdegree,
               hyperarcs,
               activeVertices);

  // now we need to compute the firstEdge array from the outDegrees
  firstEdge.Allocate(nCriticalPoints);
  // STD Version of the prefix sum
  //firstEdge.GetPortalControl().Set(0, 0);
  //std::partial_sum(svtkm::cont::ArrayPortalToIteratorBegin(outdegree.GetPortalControl()),
  //                 svtkm::cont::ArrayPortalToIteratorEnd(outdegree.GetPortalControl()) - 1,
  //                 svtkm::cont::ArrayPortalToIteratorBegin(firstEdge.GetPortalControl()) + 1);
  // SVTKM Version of the prefix sum
  svtkm::cont::Algorithm::ScanExclusive(outdegree, firstEdge);
  // Compute the number of critical edges

  svtkm::Id nCriticalEdges = this->GetLastValue(firstEdge) + this->GetLastValue(outdegree);

  AllocateEdgeArrays(nCriticalEdges);

  active_graph_inc_ns::InitializeActiveEdges<Mesh> initActiveEdgesWorklet;
  this->Invoke(initActiveEdgesWorklet,
               outdegree,
               mesh,
               firstEdge,
               globalIndex,
               extrema,
               neighbourhoodMasks,
               edgeNear,
               edgeFar,
               activeEdges);

  // now we have to go through and set the far ends of the new edges using the
  // inverse index array
  active_graph_inc_ns::InitializeEdgeFarFromActiveIndices initEdgeFarWorklet;
  this->Invoke(initEdgeFarWorklet, edgeFar, extrema, activeIndices);

  DebugPrint("Active Graph Started", __FILE__, __LINE__);

  // then we loop through the active vertices to convert their indices to active graph indices
  active_graph_inc_ns::InitializeHyperarcsFromActiveIndices initHyperarcsWorklet;
  this->Invoke(initHyperarcsWorklet, hyperarcs, activeIndices);

  // finally, allocate and initialise the edgeSorter array
  edgeSorter.Allocate(activeEdges.GetNumberOfValues());
  svtkm::cont::Algorithm::Copy(activeEdges, edgeSorter);

  //DebugPrint("Active Graph Initialised", __FILE__, __LINE__);
} // InitialiseActiveGraph()


// routine that computes the merge tree from the active graph
// was previously Compute()
void ActiveGraph::MakeMergeTree(MergeTree& tree, MeshExtrema& meshExtrema)
{ // MakeMergeTree()
  DebugPrint("Active Graph Computation Starting", __FILE__, __LINE__);

  // loop until we run out of active edges
  nIterations = 0;
  while (true)
  { // main loop
    // choose the subset of edges for the governing saddles
    TransferSaddleStarts();

    // test whether there are any left (if not, we're on the trunk)
    if (edgeSorter.GetNumberOfValues() <= 0)
      break;

    // find & label the extrema with their governing saddles
    FindGoverningSaddles();

    // label the regular points
    TransferRegularPoints();

    // compact the active set of vertices & edges
    CompactActiveVertices();
    CompactActiveEdges();

    // rebuild the chains
    BuildChains();

    // increment the iteration count
    nIterations++;
  } // main loop

  // final pass to label the trunk vertices
  BuildTrunk();

  // transfer results to merge tree
  FindSuperAndHyperNodes(tree);
  SetSuperArcs(tree);
  SetHyperArcs(tree);
  SetArcs(tree, meshExtrema);

  // we can now release many of the arrays to free up space
  ReleaseTemporaryArrays();

  DebugPrint("Merge Tree Computed", __FILE__, __LINE__);
} // MakeMergeTree()


// suppresses non-saddles for the governing saddles pass
void ActiveGraph::TransferSaddleStarts()
{ // TransferSaddleStarts()
  // update all of the edges so that the far end resets to the result of the ascent in the previous step

  active_graph_inc_ns::TransferSaddleStartsResetEdgeFar transferSaddleResetWorklet;
  this->Invoke(transferSaddleResetWorklet, activeEdges, hyperarcs, edgeFar);

  // in parallel, we need to create a vector to count the first edge for each vertex
  IdArrayType newOutdegree;
  newOutdegree.Allocate(activeVertices.GetNumberOfValues());

  // this will be a stream compaction later, but for now we'll do it the serial way
  active_graph_inc_ns::TransferSaddleStartsSetNewOutdegreeForSaddles transferOutDegree;
  this->Invoke(transferOutDegree,
               activeVertices,
               firstEdge,
               outdegree,
               activeEdges,
               hyperarcs,
               edgeFar,
               newOutdegree);

  // now do a parallel prefix sum using the offset partial sum trick.
  IdArrayType newFirstEdge;
  newFirstEdge.Allocate(activeVertices.GetNumberOfValues());
  // STD version of the prefix sum
  // newFirstEdge.GetPortalControl().Set(0, 0);
  // std::partial_sum(svtkm::cont::ArrayPortalToIteratorBegin(newOutdegree.GetPortalControl()),
  //                 svtkm::cont::ArrayPortalToIteratorEnd(newOutdegree.GetPortalControl()) - 1,
  //                 svtkm::cont::ArrayPortalToIteratorBegin(newFirstEdge.GetPortalControl()) + 1);
  // SVTK:M version of the prefix sum
  svtkm::cont::Algorithm::ScanExclusive(newOutdegree, newFirstEdge);

  svtkm::Id nEdgesToSort = this->GetLastValue(newFirstEdge) + this->GetLastValue(newOutdegree);

  // now we write only the active saddle edges to the sorting array
  edgeSorter
    .ReleaseResources(); // TODO is there a single way to resize an array handle without calling ReleaseResources followed by Allocate
  edgeSorter.Allocate(nEdgesToSort);

  // this will be a stream compaction later, but for now we'll do it the serial way
  active_graph_inc_ns::TransferSaddleStartsUpdateEdgeSorter updateEdgeSorterWorklet;
  this->Invoke(updateEdgeSorterWorklet,
               activeVertices,
               activeEdges,
               firstEdge,
               newFirstEdge,
               newOutdegree,
               edgeSorter);

  DebugPrint("Saddle Starts Transferred", __FILE__, __LINE__);
} // TransferSaddleStarts()


// sorts saddle starts to find governing saddles
void ActiveGraph::FindGoverningSaddles()
{ // FindGoverningSaddles()
  // sort with the comparator
  svtkm::cont::Algorithm::Sort(
    edgeSorter, active_graph_inc_ns::EdgePeakComparator(edgeFar, edgeNear, isJoinGraph));

  // DebugPrint("After Sorting", __FILE__, __LINE__);

  // now loop through the edges to find the governing saddles
  active_graph_inc_ns::FindGoverningSaddlesWorklet findGovSaddlesWorklet;
  svtkm::cont::ArrayHandleIndex edgeIndexArray(edgeSorter.GetNumberOfValues());

  this->Invoke(
    findGovSaddlesWorklet, edgeIndexArray, edgeSorter, edgeFar, edgeNear, hyperarcs, outdegree);

  DebugPrint("Governing Saddles Set", __FILE__, __LINE__);
} // FindGoverningSaddles()


// marks now regular points for removal
void ActiveGraph::TransferRegularPoints()
{ // TransferRegularPointsWorklet
  // we need to label the regular points that have been identified
  active_graph_inc_ns::TransferRegularPointsWorklet transRegPtWorklet(isJoinGraph);
  this->Invoke(transRegPtWorklet, activeVertices, hyperarcs, outdegree);

  DebugPrint("Regular Points Should Now Be Labelled", __FILE__, __LINE__);
} // TransferRegularPointsWorklet()


// compacts the active vertex list
void ActiveGraph::CompactActiveVertices()
{ // CompactActiveVertices()
  using PermuteIndexType = svtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  // create a temporary array the same size
  svtkm::cont::ArrayHandle<svtkm::Id> newActiveVertices;

  // Use only the current activeVertices outdegree to match size on CopyIf
  svtkm::cont::ArrayHandle<svtkm::Id> outdegreeLookup;
  svtkm::cont::Algorithm::Copy(PermuteIndexType(activeVertices, outdegree), outdegreeLookup);

  // compact the activeVertices array to keep only the ones of interest
  svtkm::cont::Algorithm::CopyIf(activeVertices, outdegreeLookup, newActiveVertices);

  activeVertices.ReleaseResources();
  svtkm::cont::Algorithm::Copy(newActiveVertices, activeVertices);

  DebugPrint("Active Vertex List Compacted", __FILE__, __LINE__);
} // CompactActiveVertices()


// compacts the active edge list
void ActiveGraph::CompactActiveEdges()
{ // CompactActiveEdges()
  // grab the size of the array for easier reference
  svtkm::Id nActiveVertices = activeVertices.GetNumberOfValues();

  // first, we have to work out the first edge for each active vertex
  // we start with a temporary new outdegree
  IdArrayType newOutdegree;
  newOutdegree.Allocate(nActiveVertices);

  // Run workflet to compute newOutdegree for each vertex
  active_graph_inc_ns::CompactActiveEdgesComputeNewVertexOutdegree computeNewOutdegreeWorklet;
  this->Invoke(computeNewOutdegreeWorklet,
               activeVertices, // (input)
               activeEdges,    // (input)
               edgeFar,        // (input)
               firstEdge,      // (input)
               outdegree,      // (input)
               hyperarcs,      // (input/output)
               newOutdegree    // (output)
               );

  // now we do a reduction to compute the offsets of each vertex
  svtkm::cont::ArrayHandle<svtkm::Id> newPosition;
  // newPosition.Allocate(nActiveVertices);   // Not necessary. ScanExclusive takes care of this.
  svtkm::cont::Algorithm::ScanExclusive(newOutdegree, newPosition);

  svtkm::Id nNewEdges = svtkm::cont::ArrayGetValue(nActiveVertices - 1, newPosition) +
    svtkm::cont::ArrayGetValue(nActiveVertices - 1, newOutdegree);

  // create a temporary vector for copying
  IdArrayType newActiveEdges;
  newActiveEdges.Allocate(nNewEdges);
  // overwriting hyperarcs in parallel is safe, as the worst than can happen is
  // that another valid ascent is found; for comparison and validation purposes
  // however it makes sense to have a `canoical' computation. To achieve this
  // canonical computation, we need to write into a new array during computation
  // ensuring that we always use the same information. The following is left in
  // commented out for future debugging and validation

  //DebugPrint("Active Edges Counted", __FILE__, __LINE__);

  // now copy the relevant edges into the active edge array
  active_graph_inc_ns::CompactActiveEdgesTransferActiveEdges transferActiveEdgesWorklet;
  this->Invoke(transferActiveEdgesWorklet,
               activeVertices,
               newPosition,    // (input)
               newOutdegree,   // (input)
               activeEdges,    // (input)
               newActiveEdges, // (output)
               edgeFar,        // (input/output)
               firstEdge,      // (input/output)
               outdegree,      // (input/output)
               hyperarcs       // (input/output)
               );

  // resize the original array and recopy
  //svtkm::cont::Algorithm::::Copy(newActiveEdges, activeEdges);
  activeEdges.ReleaseResources();
  activeEdges =
    newActiveEdges; // svtkm ArrayHandles are smart, so we can just swap it in without having to copy

  // for canonical computation: swap in newly computed hyperarc array
  //      hyperarcs.swap(newHyperarcs);

  DebugPrint("Active Edges Now Compacted", __FILE__, __LINE__);
} // CompactActiveEdges()



// builds the chains for the new active vertices
void ActiveGraph::BuildChains()
{ // BuildChains()
  // 1. compute the number of log steps required in this pass
  svtkm::Id nLogSteps = 1;
  for (svtkm::Id shifter = activeVertices.GetNumberOfValues(); shifter != 0; shifter >>= 1)
    nLogSteps++;

  // 2.   Use path compression / step doubling to collect vertices along chains
  //              until every vertex has been assigned to *an* extremum
  for (svtkm::Id logStep = 0; logStep < nLogSteps; logStep++)
  { // per log step
    active_graph_inc_ns::BuildChainsWorklet buildChainsWorklet;
    this->Invoke(buildChainsWorklet, activeVertices, hyperarcs);
  } // per log step
  DebugPrint("Chains Built", __FILE__, __LINE__);
} // BuildChains()


// sets all remaining active vertices
void ActiveGraph::BuildTrunk()
{ //BuildTrunk
  // all remaining vertices belong to the trunk
  active_graph_inc_ns::BuildTrunkWorklet buildTrunkWorklet;
  this->Invoke(buildTrunkWorklet, activeVertices, hyperarcs);

  DebugPrint("Trunk Built", __FILE__, __LINE__);
} //BuildTrunk


// finds all super and hyper nodes, numbers them & sets up arrays for lookup
void ActiveGraph::FindSuperAndHyperNodes(MergeTree& tree)
{ // FindSuperAndHyperNodes()
  // allocate memory for nodes
  hyperID.ReleaseResources();
  hyperID.Allocate(globalIndex.GetNumberOfValues());

  // compute new node positions
  // The following commented code block is variant ported directly from PPP2 using std::partial_sum. This has been replaced here with svtkm's ScanExclusive.
  /*auto oneIfSupernode = [](svtkm::Id v) { return isSupernode(v) ? 1 : 0; };
    IdArrayType newSupernodePosition;
    newSupernodePosition.Allocate(hyperarcs.GetNumberOfValues());
    newSupernodePosition.GetPortalControl().Set(0, 0);

    std::partial_sum(
            boost::make_transform_iterator(svtkm::cont::ArrayPortalToIteratorBegin(hyperarcs.GetPortalControl()), oneIfSupernode),
            boost::make_transform_iterator(svtkm::cont::ArrayPortalToIteratorEnd(hyperarcs.GetPortalControl()) - 1, oneIfSupernode),
            svtkm::cont::ArrayPortalToIteratorBegin(newSupernodePosition.GetPortalControl()) + 1);*/

  IdArrayType newSupernodePosition;
  onefIfSupernode oneIfSupernodeFunctor;
  auto oneIfSupernodeArrayHandle = svtkm::cont::ArrayHandleTransform<IdArrayType, onefIfSupernode>(
    hyperarcs, oneIfSupernodeFunctor);
  svtkm::cont::Algorithm::ScanExclusive(oneIfSupernodeArrayHandle, newSupernodePosition);

  nSupernodes =
    this->GetLastValue(newSupernodePosition) + oneIfSupernodeFunctor(this->GetLastValue(hyperarcs));

  tree.supernodes.ReleaseResources();
  tree.supernodes.Allocate(nSupernodes);

  // The following commented code block is variant ported directly from PPP2 using std::partial_sum. This has been replaced here with svtkm's ScanExclusive.
  /*
    auto oneIfHypernode = [](svtkm::Id v) { return isHypernode(v) ? 1 : 0; };
    IdArrayType newHypernodePosition;
    newHypernodePosition.Allocate(hyperarcs.GetNumberOfValues());
    newHypernodePosition.GetPortalControl().Set(0, 0);
    std::partial_sum(
            boost::make_transform_iterator(svtkm::cont::ArrayPortalToIteratorBegin(hyperarcs.GetPortalControl()), oneIfHypernode),
            boost::make_transform_iterator(svtkm::cont::ArrayPortalToIteratorEnd(hyperarcs.GetPortalControl()) - 1, oneIfHypernode),
             svtkm::cont::ArrayPortalToIteratorBegin(newHypernodePosition.GetPortalControl()) + 1);
    */
  IdArrayType newHypernodePosition;
  onefIfHypernode oneIfHypernodeFunctor;
  auto oneIfHypernodeArrayHandle = svtkm::cont::ArrayHandleTransform<IdArrayType, onefIfHypernode>(
    hyperarcs, oneIfHypernodeFunctor);
  svtkm::cont::Algorithm::ScanExclusive(oneIfHypernodeArrayHandle, newHypernodePosition);

  nHypernodes =
    this->GetLastValue(newHypernodePosition) + oneIfHypernodeFunctor(this->GetLastValue(hyperarcs));

  tree.hypernodes.ReleaseResources();
  tree.hypernodes.Allocate(globalIndex.GetNumberOfValues());

  // perform stream compression
  active_graph_inc_ns::FindSuperAndHyperNodesWorklet findSuperAndHyperNodesWorklet;
  svtkm::cont::ArrayHandleIndex graphVertexIndex(globalIndex.GetNumberOfValues());
  this->Invoke(findSuperAndHyperNodesWorklet,
               graphVertexIndex,
               hyperarcs,
               newHypernodePosition,
               newSupernodePosition,
               hyperID,
               tree.hypernodes,
               tree.supernodes);

  DebugPrint("Super/Hypernodes Found", __FILE__, __LINE__);
  tree.DebugPrint("Super/Hypernodes Found", __FILE__, __LINE__);
} // FindSuperAndHyperNodes()


// uses active graph to set superarcs & hyperparents in merge tree
void ActiveGraph::SetSuperArcs(MergeTree& tree)
{ // SetSuperArcs()
  using PermutedIdArrayType = svtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  //      1.      set the hyperparents
  // allocate space for the hyperparents
  tree.hyperparents.ReleaseResources();
  tree.hyperparents.Allocate(nSupernodes);

  // execute the worklet to set the hyperparents
  active_graph_inc_ns::SetSuperArcsSetTreeHyperparents setTreeHyperparentsWorklet;
  this->Invoke(setTreeHyperparentsWorklet, tree.supernodes, hyperarcs, tree.hyperparents);

  tree.DebugPrint("Hyperparents Set", __FILE__, __LINE__);
  //      a.      And the super ID array needs setting up
  superID.ReleaseResources();
  svtkm::cont::Algorithm::Copy(
    svtkm::cont::make_ArrayHandleConstant(NO_SUCH_ELEMENT, globalIndex.GetNumberOfValues()),
    superID);
  svtkm::cont::ArrayHandleIndex supernodeIndex(nSupernodes);
  PermutedIdArrayType permutedSuperID(tree.supernodes, superID);
  svtkm::cont::Algorithm::Copy(supernodeIndex, permutedSuperID);

  //      2.      Sort the supernodes into segments according to hyperparent
  //              See comparator for details
  svtkm::cont::Algorithm::Sort(tree.supernodes,
                              active_graph_inc_ns::HyperArcSuperNodeComparator(
                                tree.hyperparents, this->superID, tree.isJoinTree));

  //      3.      Now update the other arrays to match
  IdArrayType hyperParentsTemp;
  hyperParentsTemp.Allocate(nSupernodes);
  auto permutedTreeHyperparents = svtkm::cont::make_ArrayHandlePermutation(
    svtkm::cont::make_ArrayHandlePermutation(tree.supernodes, superID), tree.hyperparents);

  svtkm::cont::Algorithm::Copy(permutedTreeHyperparents, hyperParentsTemp);
  svtkm::cont::Algorithm::Copy(hyperParentsTemp, tree.hyperparents);
  hyperParentsTemp.ReleaseResources();
  //      a.      And the super ID array needs setting up // TODO Check if we really need this?
  svtkm::cont::Algorithm::Copy(supernodeIndex, permutedSuperID);

  DebugPrint("Supernodes Sorted", __FILE__, __LINE__);
  tree.DebugPrint("Supernodes Sorted", __FILE__, __LINE__);

  //      4.      Allocate memory for superarcs
  tree.superarcs.ReleaseResources();
  tree.superarcs.Allocate(nSupernodes);
  tree.firstSuperchild.ReleaseResources();
  tree.firstSuperchild.Allocate(nHypernodes);

  //      5.      Each supernode points to its neighbour in the list, except at the end of segments
  // execute the worklet to set the tree.hyperparents and tree.firstSuperchild
  active_graph_inc_ns::SetSuperArcsSetTreeSuperarcs setTreeSuperarcsWorklet;
  this->Invoke(setTreeSuperarcsWorklet,
               tree.supernodes,     // (input)
               hyperarcs,           // (input)
               tree.hyperparents,   // (input)
               superID,             // (input)
               hyperID,             // (input)
               tree.superarcs,      // (output)
               tree.firstSuperchild // (output)
               );

  // 6.   Now we can reset the supernodes to mesh IDs
  PermutedIdArrayType permuteGlobalIndex(tree.supernodes, globalIndex);
  svtkm::cont::Algorithm::Copy(permuteGlobalIndex, tree.supernodes);

  // 7.   and the hyperparent to point to a hyperarc rather than a graph index
  PermutedIdArrayType permuteHyperID(tree.hyperparents, hyperID);
  svtkm::cont::Algorithm::Copy(permuteHyperID, tree.hyperparents);

  tree.DebugPrint("Superarcs Set", __FILE__, __LINE__);
} // SetSuperArcs()


// uses active graph to set hypernodes in merge tree
void ActiveGraph::SetHyperArcs(MergeTree& tree)
{ // SetHyperArcs()
  //      1.      Allocate memory for hypertree
  tree.hypernodes.Shrink(
    nHypernodes); // Has been allocated previously. The values are needed but the size may be too large.
  tree.hyperarcs.ReleaseResources();
  tree.hyperarcs.Allocate(nHypernodes); // Has not been allocated yet

  //      2.      Use the superIDs already set to fill in the hyperarcs array
  active_graph_inc_ns::SetHyperArcsWorklet setHyperArcsWorklet;
  this->Invoke(
    setHyperArcsWorklet, tree.hypernodes, tree.hyperarcs, this->hyperarcs, this->superID);

  // Debug output
  DebugPrint("Hyperarcs Set", __FILE__, __LINE__);
  tree.DebugPrint("Hyperarcs Set", __FILE__, __LINE__);
} // SetHyperArcs()


// uses active graph to set arcs in merge tree
void ActiveGraph::SetArcs(MergeTree& tree, MeshExtrema& meshExtrema)
{ // SetArcs()
  using PermuteIndexType = svtkm::cont::ArrayHandlePermutation<IdArrayType, IdArrayType>;

  // reference to the correct array in the extrema
  const IdArrayType& extrema = isJoinGraph ? meshExtrema.peaks : meshExtrema.pits;

  // 1.   Set the arcs for the super/hypernodes based on where they prune to
  active_graph_inc_ns::SetArcsSetSuperAndHypernodeArcs setSuperAndHypernodeArcsWorklet;
  this->Invoke(setSuperAndHypernodeArcsWorklet,
               this->globalIndex,
               this->hyperarcs,
               this->hyperID,
               tree.arcs,
               tree.superparents);

  DebugPrint("Sliding Arcs Set", __FILE__, __LINE__);
  tree.DebugPrint("Sliding Arcs Set", __FILE__, __LINE__);

  // 2.   Loop through all vertices to slide down hyperarcs
  active_graph_inc_ns::SetArcsSlideVertices slideVerticesWorklet(
    isJoinGraph, nSupernodes, nHypernodes);
  this->Invoke(slideVerticesWorklet,
               tree.arcs,            // (input)
               extrema,              // (input)  i.e,. meshExtrema.peaks or meshExtrema.pits
               tree.firstSuperchild, // (input)
               tree.supernodes,      // (input)
               tree.superparents);   // (input/output)

  tree.DebugPrint("Sliding Finished", __FILE__, __LINE__);

  // 3.   Now set the superparents correctly for the supernodes
  PermuteIndexType permuteTreeSuperparents(tree.supernodes, tree.superparents);
  svtkm::cont::ArrayHandleIndex supernodesIndex(nSupernodes);
  svtkm::cont::Algorithm::Copy(supernodesIndex, permuteTreeSuperparents);

  tree.DebugPrint("Superparents Set", __FILE__, __LINE__);

  // 4.   Finally, sort all of the vertices onto their superarcs
  IdArrayType nodes;
  svtkm::cont::ArrayHandleIndex nodesIndex(tree.arcs.GetNumberOfValues());
  svtkm::cont::Algorithm::Copy(nodesIndex, nodes);

  //  5.  Sort the nodes into segments according to superparent
  //      See comparator for details
  svtkm::cont::Algorithm::Sort(
    nodes, active_graph_inc_ns::SuperArcNodeComparator(tree.superparents, tree.isJoinTree));

  //  6. Connect the nodes to each other
  active_graph_inc_ns::SetArcsConnectNodes connectNodesWorklet;
  this->Invoke(connectNodesWorklet,
               tree.arcs,         // (input/output)
               nodes,             // (input)
               tree.superparents, // (input)
               tree.superarcs,    // (input)
               tree.supernodes);  // (input)

  tree.DebugPrint("Arcs Set", __FILE__, __LINE__);
} // SetArcs()


// Allocate the vertex array
void ActiveGraph::AllocateVertexArrays(svtkm::Id nElems)
{
  globalIndex.Allocate(nElems);
  outdegree.Allocate(nElems);
  hyperarcs.Allocate(nElems);
  activeVertices.Allocate(nElems);
}


// Allocate the edge array
void ActiveGraph::AllocateEdgeArrays(svtkm::Id nElems)
{
  activeEdges.Allocate(nElems);
  edgeNear.Allocate(nElems);
  edgeFar.Allocate(nElems);
}


// releases temporary arrays
void ActiveGraph::ReleaseTemporaryArrays()
{
  globalIndex.ReleaseResources();
  firstEdge.ReleaseResources();
  outdegree.ReleaseResources();
  edgeNear.ReleaseResources();
  edgeFar.ReleaseResources();
  activeEdges.ReleaseResources();
  activeVertices.ReleaseResources();
  edgeSorter.ReleaseResources();
  hyperarcs.ReleaseResources();
  hyperID.ReleaseResources();
  superID.ReleaseResources();
}


// prints the contents of the active graph in a standard format
void ActiveGraph::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "------------------------------------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Active Graph Contains:                                " << std::endl;
  std::cout << "------------------------------------------------------" << std::endl;

  std::cout << "Is Join Graph? " << (isJoinGraph ? "T" : "F") << std::endl;
  std::cout << "nIterations    " << nIterations << std::endl;
  std::cout << "nSupernodes    " << nSupernodes << std::endl;
  std::cout << "nHypernodes    " << nHypernodes << std::endl;

  // Full Vertex Arrays
  std::cout << "Full Vertex Arrays - Size:  " << globalIndex.GetNumberOfValues() << std::endl;
  printHeader(globalIndex.GetNumberOfValues());
  printIndices("Global Index", globalIndex);
  printIndices("First Edge", firstEdge);
  printIndices("Outdegree", outdegree);
  printIndices("Hyperarc ID", hyperarcs);
  printIndices("Hypernode ID", hyperID);
  printIndices("Supernode ID", superID);
  std::cout << std::endl;

  // Active Vertex Arrays
  IdArrayType activeIndices;
  permuteArray<svtkm::Id>(globalIndex, activeVertices, activeIndices);
  IdArrayType activeFirst;
  permuteArray<svtkm::Id>(firstEdge, activeVertices, activeFirst);
  IdArrayType activeOutdegree;
  permuteArray<svtkm::Id>(outdegree, activeVertices, activeOutdegree);
  IdArrayType activeHyperarcs;
  permuteArray<svtkm::Id>(hyperarcs, activeVertices, activeHyperarcs);
  std::cout << "Active Vertex Arrays - Size: " << activeVertices.GetNumberOfValues() << std::endl;
  printHeader(activeVertices.GetNumberOfValues());
  printIndices("Active Vertices", activeVertices);
  printIndices("Active Indices", activeIndices);
  printIndices("Active First Edge", activeFirst);
  printIndices("Active Outdegree", activeOutdegree);
  printIndices("Active Hyperarc ID", activeHyperarcs);
  std::cout << std::endl;

  // Full Edge Arrays
  IdArrayType farIndices;
  permuteArray<svtkm::Id>(globalIndex, edgeFar, farIndices);
  IdArrayType nearIndices;
  permuteArray<svtkm::Id>(globalIndex, edgeNear, nearIndices);
  std::cout << "Full Edge Arrays - Size:     " << edgeNear.GetNumberOfValues() << std::endl;
  printHeader(edgeFar.GetNumberOfValues());
  printIndices("Near", edgeNear);
  printIndices("Far", edgeFar);
  printIndices("Near Index", nearIndices);
  printIndices("Far Index", farIndices);
  std::cout << std::endl;

  // Active Edge Arrays
  IdArrayType activeFarIndices;
  permuteArray<svtkm::Id>(edgeFar, activeEdges, activeFarIndices);
  IdArrayType activeNearIndices;
  permuteArray<svtkm::Id>(edgeNear, activeEdges, activeNearIndices);
  std::cout << "Active Edge Arrays - Size:   " << activeEdges.GetNumberOfValues() << std::endl;
  printHeader(activeEdges.GetNumberOfValues());
  printIndices("Active Edges", activeEdges);
  printIndices("Edge Near Index", activeNearIndices);
  printIndices("Edge Far Index", activeFarIndices);
  std::cout << std::endl;

  // Edge Sorter Array
  IdArrayType sortedFarIndices;
  permuteArray<svtkm::Id>(edgeFar, edgeSorter, sortedFarIndices);
  IdArrayType sortedNearIndices;
  permuteArray<svtkm::Id>(edgeNear, edgeSorter, sortedNearIndices);
  std::cout << "Edge Sorter - Size:          " << edgeSorter.GetNumberOfValues() << std::endl;
  printHeader(edgeSorter.GetNumberOfValues());
  printIndices("Edge Sorter", edgeSorter);
  printIndices("Sorted Near Index", sortedNearIndices);
  printIndices("Sorted Far Index", sortedFarIndices);
  std::cout << std::endl;

  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;
#else
  // Prevent unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()



} // namespace contourtree_augmented
} // worklet
} // svtkm

#endif
