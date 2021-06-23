/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRandomGraphSource.h

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
 * @class   svtkRandomGraphSource
 * @brief   a graph with random edges
 *
 *
 * Generates a graph with a specified number of vertices, with the density of
 * edges specified by either an exact number of edges or the probability of
 * an edge.  You may additionally specify whether to begin with a random
 * tree (which enforces graph connectivity).
 *
 */

#ifndef svtkRandomGraphSource_h
#define svtkRandomGraphSource_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class svtkGraph;
class svtkPVXMLElement;

class SVTKINFOVISCORE_EXPORT svtkRandomGraphSource : public svtkGraphAlgorithm
{
public:
  static svtkRandomGraphSource* New();
  svtkTypeMacro(svtkRandomGraphSource, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The number of vertices in the graph.
   */
  svtkGetMacro(NumberOfVertices, int);
  svtkSetClampMacro(NumberOfVertices, int, 0, SVTK_INT_MAX);
  //@}

  //@{
  /**
   * If UseEdgeProbability is off, creates a graph with the specified number
   * of edges.  Duplicate (parallel) edges are allowed.
   */
  svtkGetMacro(NumberOfEdges, int);
  svtkSetClampMacro(NumberOfEdges, int, 0, SVTK_INT_MAX);
  //@}

  //@{
  /**
   * If UseEdgeProbability is on, adds an edge with this probability between 0 and 1
   * for each pair of vertices in the graph.
   */
  svtkGetMacro(EdgeProbability, double);
  svtkSetClampMacro(EdgeProbability, double, 0.0, 1.0);
  //@}

  //@{
  /**
   * When set, includes edge weights in an array named "edge_weights".
   * Defaults to off.  Weights are random between 0 and 1.
   */
  svtkSetMacro(IncludeEdgeWeights, bool);
  svtkGetMacro(IncludeEdgeWeights, bool);
  svtkBooleanMacro(IncludeEdgeWeights, bool);
  //@}

  //@{
  /**
   * The name of the edge weight array. Default "edge weight".
   */
  svtkSetStringMacro(EdgeWeightArrayName);
  svtkGetStringMacro(EdgeWeightArrayName);
  //@}

  //@{
  /**
   * When set, creates a directed graph, as opposed to an undirected graph.
   */
  svtkSetMacro(Directed, bool);
  svtkGetMacro(Directed, bool);
  svtkBooleanMacro(Directed, bool);
  //@}

  //@{
  /**
   * When set, uses the EdgeProbability parameter to determine the density
   * of edges.  Otherwise, NumberOfEdges is used.
   */
  svtkSetMacro(UseEdgeProbability, bool);
  svtkGetMacro(UseEdgeProbability, bool);
  svtkBooleanMacro(UseEdgeProbability, bool);
  //@}

  //@{
  /**
   * When set, builds a random tree structure first, then adds additional
   * random edges.
   */
  svtkSetMacro(StartWithTree, bool);
  svtkGetMacro(StartWithTree, bool);
  svtkBooleanMacro(StartWithTree, bool);
  //@}

  //@{
  /**
   * If this flag is set to true, edges where the source and target
   * vertex are the same can be generated.  The default is to forbid
   * such loops.
   */
  svtkSetMacro(AllowSelfLoops, bool);
  svtkGetMacro(AllowSelfLoops, bool);
  svtkBooleanMacro(AllowSelfLoops, bool);
  //@}

  //@{
  /**
   * When set, multiple edges from a source to a target vertex are
   * allowed. The default is to forbid such loops.
   */
  svtkSetMacro(AllowParallelEdges, bool);
  svtkGetMacro(AllowParallelEdges, bool);
  svtkBooleanMacro(AllowParallelEdges, bool);
  //@}

  //@{
  /**
   * Add pedigree ids to vertex and edge data.
   */
  svtkSetMacro(GeneratePedigreeIds, bool);
  svtkGetMacro(GeneratePedigreeIds, bool);
  svtkBooleanMacro(GeneratePedigreeIds, bool);
  //@}

  //@{
  /**
   * The name of the vertex pedigree id array. Default "vertex id".
   */
  svtkSetStringMacro(VertexPedigreeIdArrayName);
  svtkGetStringMacro(VertexPedigreeIdArrayName);
  //@}

  //@{
  /**
   * The name of the edge pedigree id array. Default "edge id".
   */
  svtkSetStringMacro(EdgePedigreeIdArrayName);
  svtkGetStringMacro(EdgePedigreeIdArrayName);
  //@}

  //@{
  /**
   * Control the seed used for pseudo-random-number generation.
   * This ensures that svtkRandomGraphSource can produce repeatable
   * results.
   */
  svtkSetMacro(Seed, int);
  svtkGetMacro(Seed, int);
  //@}

protected:
  svtkRandomGraphSource();
  ~svtkRandomGraphSource() override;
  int NumberOfVertices;
  int NumberOfEdges;
  double EdgeProbability;
  bool Directed;
  bool UseEdgeProbability;
  bool StartWithTree;
  bool IncludeEdgeWeights;
  bool AllowSelfLoops;
  bool AllowParallelEdges;
  bool GeneratePedigreeIds;
  int Seed;
  char* EdgeWeightArrayName;
  char* VertexPedigreeIdArrayName;
  char* EdgePedigreeIdArrayName;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Creates directed or undirected output based on Directed flag.
   */
  int RequestDataObject(svtkInformation*, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkRandomGraphSource(const svtkRandomGraphSource&) = delete;
  void operator=(const svtkRandomGraphSource&) = delete;
};

#endif
