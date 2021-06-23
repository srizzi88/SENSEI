/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkBoostPrimMinimumSpanningTree.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkBoostPrimMinimumSpanningTree
 * @brief   Constructs a minimum spanning
 *    tree from a graph, start node, and the weighting array
 *
 *
 *
 * This svtk class uses the Boost Prim Minimum Spanning Tree
 * generic algorithm to perform a minimum spanning tree creation given
 * a weighting value for each of the edges in the input graph and a
 * a starting node for the tree.
 * A couple of caveats to be noted with the Prim implementation versus the
 * Kruskal implementation:
 *   1. The negate edge weights function cannot be utilized to obtain a
 * 'maximal' spanning tree (an exception is thrown when negated edge weights
 * exist), and
 *   2. the Boost implementation of the Prim algorithm returns a vertex
 * predecessor map which results in some ambiguity about which edge from
 * the original graph should be utilized if parallel edges between nodes
 * exist; therefore, the current SVTK implementation does not copy the edge
 * data from the graph to the new tree.
 *
 * @sa
 * svtkGraph svtkBoostGraphAdapter
 */

#ifndef svtkBoostPrimMinimumSpanningTree_h
#define svtkBoostPrimMinimumSpanningTree_h

#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "svtkStdString.h"                         // For string type
#include "svtkVariant.h"                           // For variant type

#include "svtkTreeAlgorithm.h"

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostPrimMinimumSpanningTree
  : public svtkTreeAlgorithm
{
public:
  static svtkBoostPrimMinimumSpanningTree* New();
  svtkTypeMacro(svtkBoostPrimMinimumSpanningTree, svtkTreeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the name of the edge-weight input array, which must name an
   * array that is part of the edge data of the input graph and
   * contains numeric data. If the edge-weight array is not of type
   * svtkDoubleArray, the array will be copied into a temporary
   * svtkDoubleArray.
   */
  svtkSetStringMacro(EdgeWeightArrayName);
  //@}

  /**
   * Set the index (into the vertex array) of the
   * minimum spanning tree 'origin' vertex.
   */
  void SetOriginVertex(svtkIdType index);

  /**
   * Set the minimum spanning tree 'origin' vertex.
   * This method is basically the same as above
   * but allows the application to simply specify
   * an array name and value, instead of having to
   * know the specific index of the vertex.
   */
  void SetOriginVertex(svtkStdString arrayName, svtkVariant value);

  //@{
  /**
   * Stores the graph vertex ids for the tree vertices in an array
   * named "GraphVertexId".  Default is off.
   */
  svtkSetMacro(CreateGraphVertexIdArray, bool);
  svtkGetMacro(CreateGraphVertexIdArray, bool);
  svtkBooleanMacro(CreateGraphVertexIdArray, bool);
  //@}

  //@{
  /**
   * Whether to negate the edge weights. By negating the edge
   * weights this algorithm will give you the 'maximal' spanning
   * tree (i.e. the algorithm will try to create a spanning tree
   * with the highest weighted edges). Defaulted to Off.
   * FIXME: put a real definition in...
   */
  void SetNegateEdgeWeights(bool value);
  svtkGetMacro(NegateEdgeWeights, bool);
  svtkBooleanMacro(NegateEdgeWeights, bool);
  //@}

protected:
  svtkBoostPrimMinimumSpanningTree();
  ~svtkBoostPrimMinimumSpanningTree() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  char* EdgeWeightArrayName;
  svtkIdType OriginVertexIndex;
  svtkVariant OriginValue;
  bool CreateGraphVertexIdArray;
  bool ArrayNameSet;
  char* ArrayName;
  bool NegateEdgeWeights;
  float EdgeWeightMultiplier;

  //@{
  /**
   * Using the convenience function internally
   */
  svtkSetStringMacro(ArrayName);
  //@}

  /**
   * This method is basically a helper function to find
   * the index of a specific value within a specific array
   */
  svtkIdType GetVertexIndex(svtkAbstractArray* abstract, svtkVariant value);

  svtkBoostPrimMinimumSpanningTree(const svtkBoostPrimMinimumSpanningTree&) = delete;
  void operator=(const svtkBoostPrimMinimumSpanningTree&) = delete;
};

#endif
