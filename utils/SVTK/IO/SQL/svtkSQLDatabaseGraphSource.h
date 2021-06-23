/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLDatabaseGraphSource.h

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
 * @class   svtkSQLDatabaseGraphSource
 * @brief   Generates a svtkGraph based on an SQL query.
 *
 *
 * This class combines svtkSQLDatabase, svtkSQLQuery, and svtkQueryToGraph to
 * provide a convenience class for generating graphs from databases.
 * Also this class can be easily wrapped and used within ParaView / OverView.
 */

#ifndef svtkSQLDatabaseGraphSource_h
#define svtkSQLDatabaseGraphSource_h

#include "svtkGraphAlgorithm.h"
#include "svtkIOSQLModule.h" // For export macro
#include "svtkStdString.h"

class svtkEventForwarderCommand;

class SVTKIOSQL_EXPORT svtkSQLDatabaseGraphSource : public svtkGraphAlgorithm
{
public:
  static svtkSQLDatabaseGraphSource* New();
  svtkTypeMacro(svtkSQLDatabaseGraphSource, svtkGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent);

  svtkStdString GetURL();
  void SetURL(const svtkStdString& url);

  void SetPassword(const svtkStdString& password);

  svtkStdString GetEdgeQuery();
  void SetEdgeQuery(const svtkStdString& query);

  svtkStdString GetVertexQuery();
  void SetVertexQuery(const svtkStdString& query);

  void AddLinkVertex(const char* column, const char* domain = 0, int hidden = 0);
  void ClearLinkVertices();
  void AddLinkEdge(const char* column1, const char* column2);
  void ClearLinkEdges();

  //@{
  /**
   * If on (default), generate edge pedigree ids.
   * If off, assign an array to be edge pedigree ids.
   */
  svtkGetMacro(GenerateEdgePedigreeIds, bool);
  svtkSetMacro(GenerateEdgePedigreeIds, bool);
  svtkBooleanMacro(GenerateEdgePedigreeIds, bool);
  //@}

  //@{
  /**
   * Use this array name for setting or generating edge pedigree ids.
   */
  svtkSetStringMacro(EdgePedigreeIdArrayName);
  svtkGetStringMacro(EdgePedigreeIdArrayName);
  //@}

  //@{
  /**
   * If on (default), generate a directed output graph.
   * If off, generate an undirected output graph.
   */
  svtkSetMacro(Directed, bool);
  svtkGetMacro(Directed, bool);
  svtkBooleanMacro(Directed, bool);
  //@}

protected:
  svtkSQLDatabaseGraphSource();
  ~svtkSQLDatabaseGraphSource() override;

  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  bool GenerateEdgePedigreeIds;
  char* EdgePedigreeIdArrayName;

private:
  svtkSQLDatabaseGraphSource(const svtkSQLDatabaseGraphSource&) = delete;
  void operator=(const svtkSQLDatabaseGraphSource&) = delete;

  /**
   * This intercepts events from the graph layout class
   * and re-emits them as if they came from this class.
   */
  svtkEventForwarderCommand* EventForwarder;

  class implementation;
  implementation* const Implementation;

  bool Directed;
};

#endif

// SVTK-HeaderTest-Exclude: svtkSQLDatabaseGraphSource.h
