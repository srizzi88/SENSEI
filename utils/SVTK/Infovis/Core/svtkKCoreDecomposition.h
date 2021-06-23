/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkKCoreDecomposition.h

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
 * @class   svtkKCoreDecomposition
 * @brief   Compute the k-core decomposition of the input graph.
 *
 *
 * The k-core decomposition is a graph partitioning strategy that is useful for
 * analyzing the structure of large networks. A k-core of a graph G is a maximal
 * connected subgraph of G in which all vertices have degree at least k.  The k-core
 * membership for each vertex of the input graph is found on the vertex data of the
 * output graph as an array named 'KCoreDecompositionNumbers' by default.  The algorithm
 * used to find the k-cores has O(number of graph edges) running time, and is described
 * in the following reference paper.
 *
 * An O(m) Algorithm for Cores Decomposition of Networks
 *   V. Batagelj, M. Zaversnik, 2001
 *
 * @par Thanks:
 * Thanks to Thomas Otahal from Sandia National Laboratories for providing this
 * implementation.
 */

#ifndef svtkKCoreDecomposition_h
#define svtkKCoreDecomposition_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class svtkIntArray;

class SVTKINFOVISCORE_EXPORT svtkKCoreDecomposition : public svtkGraphAlgorithm
{
public:
  static svtkKCoreDecomposition* New();

  svtkTypeMacro(svtkKCoreDecomposition, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the output array name. If no output array name is
   * set then the name 'KCoreDecompositionNumbers' is used.
   */
  svtkSetStringMacro(OutputArrayName);
  //@}

  //@{
  /**
   * Directed graphs only.  Use only the in edges to
   * compute the vertex degree of a vertex.  The default
   * is to use both in and out edges to compute vertex
   * degree.
   */
  svtkSetMacro(UseInDegreeNeighbors, bool);
  svtkGetMacro(UseInDegreeNeighbors, bool);
  svtkBooleanMacro(UseInDegreeNeighbors, bool);
  //@}

  //@{
  /**
   * Directed graphs only.  Use only the out edges to
   * compute the vertex degree of a vertex.  The default
   * is to use both in and out edges to compute vertex
   * degree.
   */
  svtkSetMacro(UseOutDegreeNeighbors, bool);
  svtkGetMacro(UseOutDegreeNeighbors, bool);
  svtkBooleanMacro(UseOutDegreeNeighbors, bool);
  //@}

  //@{
  /**
   * Check the input graph for self loops and parallel
   * edges.  The k-core is not defined for graphs that
   * contain either of these.  Default is on.
   */
  svtkSetMacro(CheckInputGraph, bool);
  svtkGetMacro(CheckInputGraph, bool);
  svtkBooleanMacro(CheckInputGraph, bool);
  //@}

protected:
  svtkKCoreDecomposition();
  ~svtkKCoreDecomposition() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  char* OutputArrayName;

  bool UseInDegreeNeighbors;
  bool UseOutDegreeNeighbors;
  bool CheckInputGraph;

  // K-core partitioning implementation
  void Cores(svtkGraph* g, svtkIntArray* KCoreNumbers);

  svtkKCoreDecomposition(const svtkKCoreDecomposition&) = delete;
  void operator=(const svtkKCoreDecomposition&) = delete;
};

#endif
