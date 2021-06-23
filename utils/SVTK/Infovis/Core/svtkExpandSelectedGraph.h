/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExpandSelectedGraph.h

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
 * @class   svtkExpandSelectedGraph
 * @brief   expands a selection set of a svtkGraph
 *
 *
 * The first input is a svtkSelection containing the selected vertices.
 * The second input is a svtkGraph.
 * This filter 'grows' the selection set in one of the following ways
 * 1) SetBFSDistance controls how many 'hops' the selection is grown
 *    from each seed point in the selection set (defaults to 1)
 * 2) IncludeShortestPaths controls whether this filter tries to
 *    'connect' the vertices in the selection set by computing the
 *    shortest path between the vertices (if such a path exists)
 * Note: IncludeShortestPaths is currently non-functional
 */

#ifndef svtkExpandSelectedGraph_h
#define svtkExpandSelectedGraph_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkSelectionAlgorithm.h"

class svtkGraph;
class svtkIdTypeArray;

class SVTKINFOVISCORE_EXPORT svtkExpandSelectedGraph : public svtkSelectionAlgorithm
{
public:
  static svtkExpandSelectedGraph* New();
  svtkTypeMacro(svtkExpandSelectedGraph, svtkSelectionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * A convenience method for setting the second input (i.e. the graph).
   */
  void SetGraphConnection(svtkAlgorithmOutput* in);

  /**
   * Specify the first svtkSelection input and the second svtkGraph input.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  //@{
  /**
   * Set/Get BFSDistance which controls how many 'hops' the selection
   * is grown from each seed point in the selection set (defaults to 1)
   */
  svtkSetMacro(BFSDistance, int);
  svtkGetMacro(BFSDistance, int);
  //@}

  //@{
  /**
   * Set/Get IncludeShortestPaths controls whether this filter tries to
   * 'connect' the vertices in the selection set by computing the
   * shortest path between the vertices (if such a path exists)
   * Note: IncludeShortestPaths is currently non-functional
   */
  svtkSetMacro(IncludeShortestPaths, bool);
  svtkGetMacro(IncludeShortestPaths, bool);
  svtkBooleanMacro(IncludeShortestPaths, bool);
  //@}

  //@{
  /**
   * Set/Get the vertex domain to use in the expansion.
   */
  svtkSetStringMacro(Domain);
  svtkGetStringMacro(Domain);
  //@}

  //@{
  /**
   * Whether or not to use the domain when deciding to add a vertex to the
   * expansion. Defaults to false.
   */
  svtkSetMacro(UseDomain, bool);
  svtkGetMacro(UseDomain, bool);
  svtkBooleanMacro(UseDomain, bool);
  //@}

protected:
  svtkExpandSelectedGraph();
  ~svtkExpandSelectedGraph() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void Expand(svtkIdTypeArray*, svtkGraph*);

  int BFSDistance;
  bool IncludeShortestPaths;
  char* Domain;
  bool UseDomain;

private:
  svtkExpandSelectedGraph(const svtkExpandSelectedGraph&) = delete;
  void operator=(const svtkExpandSelectedGraph&) = delete;

  void BFSExpandSelection(svtkIdTypeArray* selection, svtkGraph* graph);
};

#endif
