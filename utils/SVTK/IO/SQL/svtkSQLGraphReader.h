/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLGraphReader.h

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
 * @class   svtkSQLGraphReader
 * @brief   read a svtkGraph from a database
 *
 *
 *
 * Creates a svtkGraph using one or two svtkSQLQuery's.  The first (required)
 * query must have one row for each arc in the graph.
 * The query must have two columns which represent the source and target
 * node ids.
 *
 * The second (optional) query has one row for each node in the graph.
 * The table must have a field whose values match those in the arc table.
 * If the node table is not given,
 * a node will be created for each unique source or target identifier
 * in the arc table.
 *
 * The source, target, and node ID fields must be of the same type,
 * and must be either svtkStringArray or a subclass of svtkDataArray.
 *
 * All columns in the queries, including the source, target, and node index
 * fields, are copied into the arc data and node data of the resulting
 * svtkGraph.  If the node query is not given, the node data will contain
 * a single "id" column with the same type as the source/target id arrays.
 *
 * If parallel arcs are collected, not all the arc data is not copied into
 * the output.  Only the source and target id arrays will be transferred.
 * An additional svtkIdTypeArray column called "weight" is created which
 * contains the number of times each arc appeared in the input.
 *
 * If the node query contains positional data, the user may specify the
 * names of these fields.
 * These arrays must be data arrays.  The z-coordinate array is optional,
 * and if not given the z-coordinates are set to zero.
 */

#ifndef svtkSQLGraphReader_h
#define svtkSQLGraphReader_h

#include "svtkGraphAlgorithm.h"
#include "svtkIOSQLModule.h" // For export macro

class svtkSQLQuery;

class SVTKIOSQL_EXPORT svtkSQLGraphReader : public svtkGraphAlgorithm
{
public:
  static svtkSQLGraphReader* New();
  svtkTypeMacro(svtkSQLGraphReader, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent);

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
   * The query that retrieves the node information.
   */
  virtual void SetVertexQuery(svtkSQLQuery* q);
  svtkGetObjectMacro(VertexQuery, svtkSQLQuery);
  //@}

  //@{
  /**
   * The query that retrieves the arc information.
   */
  virtual void SetEdgeQuery(svtkSQLQuery* q);
  svtkGetObjectMacro(EdgeQuery, svtkSQLQuery);
  //@}

  //@{
  /**
   * The name of the field in the arc query for the source node of each arc.
   */
  svtkSetStringMacro(SourceField);
  svtkGetStringMacro(SourceField);
  //@}

  //@{
  /**
   * The name of the field in the arc query for the target node of each arc.
   */
  svtkSetStringMacro(TargetField);
  svtkGetStringMacro(TargetField);
  //@}

  //@{
  /**
   * The name of the field in the node query for the node ID.
   */
  svtkSetStringMacro(VertexIdField);
  svtkGetStringMacro(VertexIdField);
  //@}

  //@{
  /**
   * The name of the field in the node query for the node's x coordinate.
   */
  svtkSetStringMacro(XField);
  svtkGetStringMacro(XField);
  //@}

  //@{
  /**
   * The name of the field in the node query for the node's y coordinate.
   */
  svtkSetStringMacro(YField);
  svtkGetStringMacro(YField);
  //@}

  //@{
  /**
   * The name of the field in the node query for the node's z coordinate.
   */
  svtkSetStringMacro(ZField);
  svtkGetStringMacro(ZField);
  //@}

  //@{
  /**
   * When set, creates a graph with no parallel arcs.
   * Parallel arcs are combined into one arc.
   * No cell fields are passed to the output, except the svtkGhostType array if
   * it exists, but a new field "weight" is created that holds the number of
   * duplicates of that arc in the input.
   */
  svtkSetMacro(CollapseEdges, bool);
  svtkGetMacro(CollapseEdges, bool);
  svtkBooleanMacro(CollapseEdges, bool);
  //@}

protected:
  svtkSQLGraphReader();
  ~svtkSQLGraphReader() override;

  bool Directed;
  bool CollapseEdges;
  svtkSQLQuery* EdgeQuery;
  svtkSQLQuery* VertexQuery;
  char* SourceField;
  char* TargetField;
  char* VertexIdField;
  char* XField;
  char* YField;
  char* ZField;

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  virtual int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

private:
  svtkSQLGraphReader(const svtkSQLGraphReader&) = delete;
  void operator=(const svtkSQLGraphReader&) = delete;
};

#endif
