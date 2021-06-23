/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMutableGraphHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkMutableGraphHelper
 * @brief   Helper class for building a directed or
 *   directed graph
 *
 *
 * svtkMutableGraphHelper has helper methods AddVertex and AddEdge which
 * add vertices/edges to the underlying mutable graph. This is helpful in
 * filters which need to (re)construct graphs which may be either directed
 * or undirected.
 *
 * @sa
 * svtkGraph svtkMutableDirectedGraph svtkMutableUndirectedGraph
 */

#ifndef svtkMutableGraphHelper_h
#define svtkMutableGraphHelper_h

#include "svtkGraph.h"             // For svtkEdgeType
#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkObject.h"

class svtkDataSetAttributes;
class svtkGraph;
class svtkGraphEdge;
class svtkMutableDirectedGraph;
class svtkMutableUndirectedGraph;

class SVTKINFOVISCORE_EXPORT svtkMutableGraphHelper : public svtkObject
{
public:
  static svtkMutableGraphHelper* New();
  svtkTypeMacro(svtkMutableGraphHelper, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the underlying graph that you want to modify with this helper.
   * The graph must be an instance of svtkMutableDirectedGraph or
   * svtkMutableUndirectedGraph.
   */
  void SetGraph(svtkGraph* g);
  svtkGraph* GetGraph();
  //@}

  /**
   * Add an edge to the underlying mutable graph.
   */
  svtkEdgeType AddEdge(svtkIdType u, svtkIdType v);

  svtkGraphEdge* AddGraphEdge(svtkIdType u, svtkIdType v);

  /**
   * Add a vertex to the underlying mutable graph.
   */
  svtkIdType AddVertex();

  /**
   * Remove a vertex from the underlying mutable graph.
   */
  void RemoveVertex(svtkIdType v);

  /**
   * Remove a collection of vertices from the underlying mutable graph.
   */
  void RemoveVertices(svtkIdTypeArray* verts);

  /**
   * Remove an edge from the underlying mutable graph.
   */
  void RemoveEdge(svtkIdType e);

  /**
   * Remove a collection of edges from the underlying mutable graph.
   */
  void RemoveEdges(svtkIdTypeArray* edges);

protected:
  svtkMutableGraphHelper();
  ~svtkMutableGraphHelper() override;

  svtkGetObjectMacro(InternalGraph, svtkGraph);
  void SetInternalGraph(svtkGraph* g);
  svtkGraph* InternalGraph;

  svtkGraphEdge* GraphEdge;

  svtkMutableDirectedGraph* DirectedGraph;
  svtkMutableUndirectedGraph* UndirectedGraph;

private:
  svtkMutableGraphHelper(const svtkMutableGraphHelper&) = delete;
  void operator=(const svtkMutableGraphHelper&) = delete;
};

#endif
