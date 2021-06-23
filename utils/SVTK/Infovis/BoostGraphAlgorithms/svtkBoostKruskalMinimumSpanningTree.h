/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkBoostKruskalMinimumSpanningTree.h

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
 * @class   svtkBoostKruskalMinimumSpanningTree
 * @brief   Constructs a minimum spanning
 *    tree from a graph and the weighting array
 *
 *
 *
 * This svtk class uses the Boost Kruskal Minimum Spanning Tree
 * generic algorithm to perform a minimum spanning tree creation given
 * a weighting value for each of the edges in the input graph.
 *
 * @sa
 * svtkGraph svtkBoostGraphAdapter
 */

#ifndef svtkBoostKruskalMinimumSpanningTree_h
#define svtkBoostKruskalMinimumSpanningTree_h

#include "svtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "svtkStdString.h"                         // For string type
#include "svtkVariant.h"                           // For variant type

#include "svtkSelectionAlgorithm.h"

class SVTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT svtkBoostKruskalMinimumSpanningTree
  : public svtkSelectionAlgorithm
{
public:
  static svtkBoostKruskalMinimumSpanningTree* New();
  svtkTypeMacro(svtkBoostKruskalMinimumSpanningTree, svtkSelectionAlgorithm);
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

  //@{
  /**
   * Set the output selection type. The default is to use the
   * the set of minimum spanning tree edges "MINIMUM_SPANNING_TREE_EDGES". No
   * other options are defined.
   */
  svtkSetStringMacro(OutputSelectionType);
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
  svtkBoostKruskalMinimumSpanningTree();
  ~svtkBoostKruskalMinimumSpanningTree() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  char* EdgeWeightArrayName;
  char* OutputSelectionType;
  bool NegateEdgeWeights;
  float EdgeWeightMultiplier;

  svtkBoostKruskalMinimumSpanningTree(const svtkBoostKruskalMinimumSpanningTree&) = delete;
  void operator=(const svtkBoostKruskalMinimumSpanningTree&) = delete;
};

#endif
