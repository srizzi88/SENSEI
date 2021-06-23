/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToGraph.h

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
 * @class   svtkTableToGraph
 * @brief   convert a svtkTable into a svtkGraph
 *
 *
 * svtkTableToGraph converts a table to a graph using an auxiliary
 * link graph.  The link graph specifies how each row in the table
 * should be converted to an edge, or a collection of edges.  It also
 * specifies which columns of the table should be considered part of
 * the same domain, and which columns should be hidden.
 *
 * A second, optional, table may be provided as the vertex table.
 * This vertex table must have one or more domain columns whose values
 * match values in the edge table.  The linked column name is specified in
 * the domain array in the link graph.  The output graph will only contain
 * vertices corresponding to a row in the vertex table.  For heterogeneous
 * graphs, you may want to use svtkMergeTables to create a single vertex table.
 *
 * The link graph contains the following arrays:
 *
 * (1) The "column" array has the names of the columns to connect in each table row.
 * This array is required.
 *
 * (2) The optional "domain" array provides user-defined domain names for each column.
 * Matching domains in multiple columns will merge vertices with the same
 * value from those columns.  By default, all columns are in the same domain.
 * If a vertex table is supplied, the domain indicates the column in the vertex
 * table that the edge table column associates with.  If the user provides a
 * vertex table but no domain names, the output will be an empty graph.
 * Hidden columns do not need valid domain names.
 *
 * (3) The optional "hidden" array is a bit array specifying whether the column should be
 * hidden.  The resulting graph will contain edges representing connections
 * "through" the hidden column, but the vertices for that column will not
 * be present.  By default, no columns are hidden.  Hiding a column
 * in a particular domain hides all columns in that domain.
 *
 * The output graph will contain three additional arrays in the vertex data.
 * The "domain" column is a string array containing the domain of each vertex.
 * The "label" column is a string version of the distinct value that, along
 * with the domain, defines that vertex. The "ids" column also contains
 * the distinguishing value, but as a svtkVariant holding the raw value instead
 * of being converted to a string. The "ids" column is set as the vertex pedigree
 * ID attribute.
 */

#ifndef svtkTableToGraph_h
#define svtkTableToGraph_h

#include "svtkGraphAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class svtkBitArray;
class svtkMutableDirectedGraph;
class svtkStringArray;
class svtkTable;

class SVTKINFOVISCORE_EXPORT svtkTableToGraph : public svtkGraphAlgorithm
{
public:
  static svtkTableToGraph* New();
  svtkTypeMacro(svtkTableToGraph, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a vertex to the link graph.  Specify the column name, the domain name
   * for the column, and whether the column is hidden.
   */
  void AddLinkVertex(const char* column, const char* domain = nullptr, int hidden = 0);

  /**
   * Clear the link graph vertices.  This also clears all edges.
   */
  void ClearLinkVertices();

  /**
   * Add an edge to the link graph.  Specify the names of the columns to link.
   */
  void AddLinkEdge(const char* column1, const char* column2);

  /**
   * Clear the link graph edges.  The graph vertices will remain.
   */
  void ClearLinkEdges();

  //@{
  /**
   * The graph describing how to link the columns in the table.
   */
  svtkGetObjectMacro(LinkGraph, svtkMutableDirectedGraph);
  void SetLinkGraph(svtkMutableDirectedGraph* g);
  //@}

  /**
   * Links the columns in a specific order.
   * This creates a simple path as the link graph.
   */
  void LinkColumnPath(
    svtkStringArray* column, svtkStringArray* domain = nullptr, svtkBitArray* hidden = nullptr);

  //@{
  /**
   * Specify the directedness of the output graph.
   */
  svtkSetMacro(Directed, bool);
  svtkGetMacro(Directed, bool);
  svtkBooleanMacro(Directed, bool);
  //@}

  /**
   * Get the current modified time.
   */
  svtkMTimeType GetMTime() override;

  /**
   * A convenience method for setting the vertex table input.  This
   * is mainly for the benefit of the SVTK client/server layer,
   * vanilla SVTK code should use e.g:

   * table_to_graph->SetInputConnection(1, vertex_table->output());
   */
  void SetVertexTableConnection(svtkAlgorithmOutput* in);

protected:
  svtkTableToGraph();
  ~svtkTableToGraph() override;

  /**
   * Validate that the link graph is in the appropriate format.
   */
  int ValidateLinkGraph();

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool Directed;
  svtkMutableDirectedGraph* LinkGraph;
  svtkStringArray* VertexTableDomains;

private:
  svtkTableToGraph(const svtkTableToGraph&) = delete;
  void operator=(const svtkTableToGraph&) = delete;
};

#endif
