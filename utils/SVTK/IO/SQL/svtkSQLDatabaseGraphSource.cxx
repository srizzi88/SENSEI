/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLDatabaseGraphSource.cxx

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

#include "svtkSQLDatabaseGraphSource.h"

#include "svtkDataSetAttributes.h"
#include "svtkDirectedGraph.h"
#include "svtkEventForwarderCommand.h"
#include "svtkExecutive.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkRowQueryToTable.h"
#include "svtkSQLDatabase.h"
#include "svtkSQLQuery.h"
#include "svtkSmartPointer.h"
#include "svtkTableToGraph.h"
#include "svtkUndirectedGraph.h"

//---------------------------------------------------------------------------
class svtkSQLDatabaseGraphSource::implementation
{
public:
  implementation()
    : Database(0)
    , EdgeQuery(0)
    , EdgeTable(0)
    , VertexQuery(0)
    , VertexTable(0)
    , TableToGraph(svtkTableToGraph::New())
  {
  }

  ~implementation()
  {
    if (this->TableToGraph)
      this->TableToGraph->Delete();
    if (this->VertexTable)
      this->VertexTable->Delete();
    if (this->VertexQuery)
      this->VertexQuery->Delete();
    if (this->EdgeTable)
      this->EdgeTable->Delete();
    if (this->EdgeQuery)
      this->EdgeQuery->Delete();
    if (this->Database)
      this->Database->Delete();
  }

  svtkStdString URL;
  svtkStdString Password;
  svtkStdString EdgeQueryString;
  svtkStdString VertexQueryString;

  svtkSQLDatabase* Database;
  svtkSQLQuery* EdgeQuery;
  svtkRowQueryToTable* EdgeTable;
  svtkSQLQuery* VertexQuery;
  svtkRowQueryToTable* VertexTable;
  svtkTableToGraph* TableToGraph;
};

svtkStandardNewMacro(svtkSQLDatabaseGraphSource);

//---------------------------------------------------------------------------
svtkSQLDatabaseGraphSource::svtkSQLDatabaseGraphSource()
  : Implementation(new implementation())
  , Directed(true)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->GenerateEdgePedigreeIds = true;
  this->EdgePedigreeIdArrayName = 0;
  this->SetEdgePedigreeIdArrayName("id");

  // Set up eventforwarder
  this->EventForwarder = svtkEventForwarderCommand::New();
  this->EventForwarder->SetTarget(this);

  // Now forward progress events from the graph layout
  this->Implementation->TableToGraph->AddObserver(svtkCommand::ProgressEvent, this->EventForwarder);
}

//---------------------------------------------------------------------------
svtkSQLDatabaseGraphSource::~svtkSQLDatabaseGraphSource()
{
  delete this->Implementation;
  this->SetEdgePedigreeIdArrayName(0);
  this->EventForwarder->Delete();
}

//---------------------------------------------------------------------------
void svtkSQLDatabaseGraphSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "URL: " << this->Implementation->URL << endl;
  os << indent << "EdgeQuery: " << this->Implementation->EdgeQueryString << endl;
  os << indent << "VertexQuery: " << this->Implementation->VertexQueryString << endl;
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "GenerateEdgePedigreeIds: " << this->GenerateEdgePedigreeIds << endl;
  os << indent << "EdgePedigreeIdArrayName: "
     << (this->EdgePedigreeIdArrayName ? this->EdgePedigreeIdArrayName : "(null)") << endl;
}

svtkStdString svtkSQLDatabaseGraphSource::GetURL()
{
  return this->Implementation->URL;
}

void svtkSQLDatabaseGraphSource::SetURL(const svtkStdString& url)
{
  if (url == this->Implementation->URL)
    return;

  if (this->Implementation->EdgeQuery)
  {
    this->Implementation->EdgeQuery->Delete();
    this->Implementation->EdgeQuery = 0;
  }

  if (this->Implementation->VertexQuery)
  {
    this->Implementation->VertexQuery->Delete();
    this->Implementation->VertexQuery = 0;
  }

  if (this->Implementation->Database)
  {
    this->Implementation->Database->Delete();
    this->Implementation->Database = 0;
  }

  this->Implementation->URL = url;

  this->Modified();
}

void svtkSQLDatabaseGraphSource::SetPassword(const svtkStdString& password)
{
  if (password == this->Implementation->Password)
    return;

  if (this->Implementation->EdgeQuery)
  {
    this->Implementation->EdgeQuery->Delete();
    this->Implementation->EdgeQuery = 0;
  }

  if (this->Implementation->VertexQuery)
  {
    this->Implementation->VertexQuery->Delete();
    this->Implementation->VertexQuery = 0;
  }

  if (this->Implementation->Database)
  {
    this->Implementation->Database->Delete();
    this->Implementation->Database = 0;
  }

  this->Implementation->Password = password;

  this->Modified();
}

svtkStdString svtkSQLDatabaseGraphSource::GetEdgeQuery()
{
  return this->Implementation->EdgeQueryString;
}

void svtkSQLDatabaseGraphSource::SetEdgeQuery(const svtkStdString& query)
{
  if (query == this->Implementation->EdgeQueryString)
    return;

  this->Implementation->EdgeQueryString = query;
  this->Modified();
}

svtkStdString svtkSQLDatabaseGraphSource::GetVertexQuery()
{
  return this->Implementation->VertexQueryString;
}

void svtkSQLDatabaseGraphSource::SetVertexQuery(const svtkStdString& query)
{
  if (query == this->Implementation->VertexQueryString)
    return;

  this->Implementation->VertexQueryString = query;
  this->Modified();
}

void svtkSQLDatabaseGraphSource::AddLinkVertex(const char* column, const char* domain, int hidden)
{
  this->Implementation->TableToGraph->AddLinkVertex(column, domain, hidden);
  this->Modified();
}

void svtkSQLDatabaseGraphSource::ClearLinkVertices()
{
  this->Implementation->TableToGraph->ClearLinkVertices();
  this->Modified();
}

void svtkSQLDatabaseGraphSource::AddLinkEdge(const char* column1, const char* column2)
{
  this->Implementation->TableToGraph->AddLinkEdge(column1, column2);
  this->Modified();
}

void svtkSQLDatabaseGraphSource::ClearLinkEdges()
{
  this->Implementation->TableToGraph->ClearLinkEdges();
  this->Modified();
}

//---------------------------------------------------------------------------
int svtkSQLDatabaseGraphSource::RequestDataObject(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  svtkGraph* output = 0;
  if (this->Directed)
  {
    output = svtkDirectedGraph::New();
  }
  else
  {
    output = svtkUndirectedGraph::New();
  }
  this->GetExecutive()->SetOutputData(0, output);
  output->Delete();

  return 1;
}

//---------------------------------------------------------------------------
int svtkSQLDatabaseGraphSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  if (this->Implementation->URL.empty())
    return 1;

  if (this->Implementation->EdgeQueryString.empty())
    return 1;

  // Set Progress Text
  this->SetProgressText("DatabaseGraphSource");

  // I've started so 1% progress :)
  this->UpdateProgress(.01);

  // Setup the database if it doesn't already exist ...
  if (!this->Implementation->Database)
  {
    this->Implementation->Database = svtkSQLDatabase::CreateFromURL(this->Implementation->URL);
    if (!this->Implementation->Database)
    {
      svtkErrorMacro(<< "Error creating database using URL: " << this->Implementation->URL.c_str());
      return 0;
    }

    if (!this->Implementation->Database->Open(this->Implementation->Password))
    {
      this->Implementation->Database->Delete();
      this->Implementation->Database = 0;

      svtkErrorMacro(<< "Error opening database: " << this->Implementation->URL.c_str());
      return 0;
    }
  }

  // I have a database: 5% progress
  this->UpdateProgress(.05);

  // Setup the edge query if it doesn't already exist ...
  if (!this->Implementation->EdgeQuery)
  {
    this->Implementation->EdgeQuery = this->Implementation->Database->GetQueryInstance();
    if (!this->Implementation->EdgeQuery)
    {
      svtkErrorMacro(<< "Internal error creating edge query instance.");
      return 0;
    }
  }

  this->Implementation->EdgeQuery->SetQuery(this->Implementation->EdgeQueryString.c_str());
  if (!this->Implementation->EdgeQuery->Execute())
  {
    svtkErrorMacro(<< "Error executing edge query: "
                  << this->Implementation->EdgeQueryString.c_str());
    return 0;
  }

  // Executed edge query: 30% progress
  this->UpdateProgress(.3);

  if (!this->Implementation->EdgeTable)
  {
    this->Implementation->EdgeTable = svtkRowQueryToTable::New();
  }
  this->Implementation->EdgeTable->SetQuery(this->Implementation->EdgeQuery);

  this->Implementation->TableToGraph->SetInputConnection(
    0, this->Implementation->EdgeTable->GetOutputPort());

  // Setup the (optional) vertex query if it doesn't already exist ...
  if (this->Implementation->VertexQueryString.size())
  {
    if (!this->Implementation->VertexQuery)
    {
      this->Implementation->VertexQuery = this->Implementation->Database->GetQueryInstance();
      if (!this->Implementation->VertexQuery)
      {
        svtkErrorMacro(<< "Internal error creating vertex query instance.");
        return 0;
      }
    }

    this->Implementation->VertexQuery->SetQuery(this->Implementation->VertexQueryString.c_str());
    if (!this->Implementation->VertexQuery->Execute())
    {
      svtkErrorMacro(<< "Error executing vertex query: "
                    << this->Implementation->VertexQueryString.c_str());
      return 0;
    }

    // Executed vertex query: 50% progress
    this->UpdateProgress(.5);

    if (!this->Implementation->VertexTable)
    {
      this->Implementation->VertexTable = svtkRowQueryToTable::New();
    }
    this->Implementation->VertexTable->SetQuery(this->Implementation->VertexQuery);

    this->Implementation->TableToGraph->SetInputConnection(
      1, this->Implementation->VertexTable->GetOutputPort());
  }

  // Set Progress Text
  this->SetProgressText("DatabaseGraphSource:TableToGraph");

  // Get the graph output ...
  this->Implementation->TableToGraph->SetDirected(this->Directed);
  this->Implementation->TableToGraph->Update();

  // Set Progress Text
  this->SetProgressText("DatabaseGraphSource");

  // Finished table to graph: 90% progress
  this->UpdateProgress(.9);

  svtkGraph* const output = svtkGraph::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(svtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(this->Implementation->TableToGraph->GetOutput());

  if (this->GenerateEdgePedigreeIds)
  {
    svtkIdType numEdges = output->GetNumberOfEdges();
    svtkSmartPointer<svtkIdTypeArray> arr = svtkSmartPointer<svtkIdTypeArray>::New();
    arr->SetName(this->EdgePedigreeIdArrayName);
    arr->SetNumberOfTuples(numEdges);
    for (svtkIdType i = 0; i < numEdges; ++i)
    {
      arr->InsertValue(i, i);
    }
    output->GetEdgeData()->SetPedigreeIds(arr);
  }
  else
  {
    svtkAbstractArray* arr = output->GetEdgeData()->GetAbstractArray(this->EdgePedigreeIdArrayName);
    if (!arr)
    {
      svtkErrorMacro(<< "Could not find edge pedigree id array: " << this->EdgePedigreeIdArrayName);
      return 0;
    }
    output->GetEdgeData()->SetPedigreeIds(arr);
  }

  // Done: 100% progress
  this->UpdateProgress(1);

  return 1;
}
