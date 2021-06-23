/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLDatabaseTableSource.cxx

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

#include "svtkSQLDatabaseTableSource.h"

#include "svtkDataSetAttributes.h"
#include "svtkEventForwarderCommand.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkRowQueryToTable.h"
#include "svtkSQLDatabase.h"
#include "svtkSQLQuery.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

//---------------------------------------------------------------------------
class svtkSQLDatabaseTableSource::implementation
{
public:
  implementation()
    : Database(nullptr)
    , Query(nullptr)
    , Table(nullptr)
  {
  }

  ~implementation()
  {
    if (this->Table)
      this->Table->Delete();

    if (this->Query)
      this->Query->Delete();

    if (this->Database)
      this->Database->Delete();
  }

  svtkStdString URL;
  svtkStdString Password;
  svtkStdString QueryString;

  svtkSQLDatabase* Database;
  svtkSQLQuery* Query;
  svtkRowQueryToTable* Table;
};

svtkStandardNewMacro(svtkSQLDatabaseTableSource);

//---------------------------------------------------------------------------
svtkSQLDatabaseTableSource::svtkSQLDatabaseTableSource()
  : Implementation(new implementation())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->PedigreeIdArrayName = nullptr;
  this->SetPedigreeIdArrayName("id");
  this->GeneratePedigreeIds = true;

  // Set up eventforwarder
  this->EventForwarder = svtkEventForwarderCommand::New();
  this->EventForwarder->SetTarget(this);
}

//---------------------------------------------------------------------------
svtkSQLDatabaseTableSource::~svtkSQLDatabaseTableSource()
{
  delete this->Implementation;
  this->SetPedigreeIdArrayName(nullptr);
  this->EventForwarder->Delete();
}

//---------------------------------------------------------------------------
void svtkSQLDatabaseTableSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "URL: " << this->Implementation->URL << endl;
  os << indent << "Query: " << this->Implementation->QueryString << endl;
  os << indent << "GeneratePedigreeIds: " << this->GeneratePedigreeIds << endl;
  os << indent << "PedigreeIdArrayName: " << this->PedigreeIdArrayName << endl;
}

svtkStdString svtkSQLDatabaseTableSource::GetURL()
{
  return this->Implementation->URL;
}

void svtkSQLDatabaseTableSource::SetURL(const svtkStdString& url)
{
  if (url == this->Implementation->URL)
    return;

  if (this->Implementation->Query)
  {
    this->Implementation->Query->Delete();
    this->Implementation->Query = nullptr;
  }

  if (this->Implementation->Database)
  {
    this->Implementation->Database->Delete();
    this->Implementation->Database = nullptr;
  }

  this->Implementation->URL = url;

  this->Modified();
}

void svtkSQLDatabaseTableSource::SetPassword(const svtkStdString& password)
{
  if (password == this->Implementation->Password)
    return;

  if (this->Implementation->Query)
  {
    this->Implementation->Query->Delete();
    this->Implementation->Query = nullptr;
  }

  if (this->Implementation->Database)
  {
    this->Implementation->Database->Delete();
    this->Implementation->Database = nullptr;
  }

  this->Implementation->Password = password;

  this->Modified();
}

svtkStdString svtkSQLDatabaseTableSource::GetQuery()
{
  return this->Implementation->QueryString;
}

void svtkSQLDatabaseTableSource::SetQuery(const svtkStdString& query)
{
  if (query == this->Implementation->QueryString)
    return;

  this->Implementation->QueryString = query;
  this->Modified();
}

//---------------------------------------------------------------------------
int svtkSQLDatabaseTableSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  if (this->Implementation->URL.empty())
    return 1;

  if (this->Implementation->QueryString.empty())
    return 1;

  if (!this->PedigreeIdArrayName)
  {
    svtkErrorMacro(<< "You must specify a pedigree id array name.");
    return 0;
  }

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
      this->Implementation->Database = nullptr;

      svtkErrorMacro(<< "Error opening database: " << this->Implementation->URL.c_str());
      return 0;
    }
  }

  if (!this->Implementation->Query)
  {
    this->Implementation->Query = this->Implementation->Database->GetQueryInstance();
    if (!this->Implementation->Query)
    {
      svtkErrorMacro(<< "Internal error creating query instance.");
      return 0;
    }
  }

  // Set Progress Text
  this->SetProgressText("DatabaseTableSource");

  // I have a database: 5% progress
  this->UpdateProgress(.05);

  this->Implementation->Query->SetQuery(this->Implementation->QueryString.c_str());
  if (!this->Implementation->Query->Execute())
  {
    svtkErrorMacro(<< "Error executing query: " << this->Implementation->QueryString.c_str());
    return 0;
  }

  // Executed query: 33% progress
  this->UpdateProgress(.33);

  // Set Progress Text
  this->SetProgressText("DatabaseTableSource: RowQueryToTable");

  if (!this->Implementation->Table)
  {
    this->Implementation->Table = svtkRowQueryToTable::New();

    // Now forward progress events from the graph layout
    this->Implementation->Table->AddObserver(svtkCommand::ProgressEvent, this->EventForwarder);
  }
  this->Implementation->Table->SetQuery(this->Implementation->Query);
  this->Implementation->Table->Update();

  // Created Table: 66% progress
  this->SetProgressText("DatabaseTableSource");
  this->UpdateProgress(.66);

  svtkTable* const output = svtkTable::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(svtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(this->Implementation->Table->GetOutput());

  if (this->GeneratePedigreeIds)
  {
    svtkSmartPointer<svtkIdTypeArray> pedigreeIds = svtkSmartPointer<svtkIdTypeArray>::New();
    svtkIdType numRows = output->GetNumberOfRows();
    pedigreeIds->SetNumberOfTuples(numRows);
    pedigreeIds->SetName(this->PedigreeIdArrayName);
    for (svtkIdType i = 0; i < numRows; ++i)
    {
      pedigreeIds->InsertValue(i, i);
    }
    output->GetRowData()->SetPedigreeIds(pedigreeIds);
  }
  else
  {
    svtkAbstractArray* arr = output->GetColumnByName(this->PedigreeIdArrayName);
    if (arr)
    {
      output->GetRowData()->SetPedigreeIds(arr);
    }
    else
    {
      svtkErrorMacro(<< "Could find pedigree id array: " << this->PedigreeIdArrayName);
      return 0;
    }
  }

  // Done: 100% progress
  this->UpdateProgress(1);

  return 1;
}
