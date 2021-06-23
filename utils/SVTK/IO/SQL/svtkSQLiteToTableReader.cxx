/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLiteToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkSQLiteDatabase.h"
#include "svtkSQLiteQuery.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include "svtkSQLiteToTableReader.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkSQLiteToTableReader);

//----------------------------------------------------------------------------
svtkSQLiteToTableReader::svtkSQLiteToTableReader() = default;

//----------------------------------------------------------------------------
svtkSQLiteToTableReader::~svtkSQLiteToTableReader() = default;

//----------------------------------------------------------------------------
int svtkSQLiteToTableReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  // Make sure we have all the information we need to provide a svtkTable
  if (!this->Database)
  {
    svtkErrorMacro(<< "No open database connection");
    return 1;
  }
  if (!this->Database->IsA("svtkSQLiteDatabase"))
  {
    svtkErrorMacro(<< "Wrong type of database for this reader");
    return 1;
  }
  if (this->TableName.empty())
  {
    svtkErrorMacro(<< "No table selected");
    return 1;
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Return all data in the first piece ...
  if (outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    return 1;
  }

  svtkTable* const output = svtkTable::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // perform a query to get the names and types of the columns
  std::string queryStr = "pragma table_info(";
  queryStr += this->TableName;
  queryStr += ")";
  svtkSQLiteQuery* query = static_cast<svtkSQLiteQuery*>(this->Database->GetQueryInstance());
  query->SetQuery(queryStr.c_str());
  if (!query->Execute())
  {
    svtkErrorMacro(<< "Error performing 'pragma' query");
  }

  // use the results of the query to create columns of the proper name & type
  std::vector<std::string> columnTypes;
  while (query->NextRow())
  {
    std::string columnName = query->DataValue(1).ToString();
    std::string columnType = query->DataValue(2).ToString();
    columnTypes.push_back(columnType);
    if (columnType == "INTEGER")
    {
      svtkSmartPointer<svtkIntArray> column = svtkSmartPointer<svtkIntArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
    }
    else if (columnType == "REAL")
    {
      svtkSmartPointer<svtkDoubleArray> column = svtkSmartPointer<svtkDoubleArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
    }
    else
    {
      svtkSmartPointer<svtkStringArray> column = svtkSmartPointer<svtkStringArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
    }
  }

  // do a query to get the contents of the SQLite table
  queryStr = "SELECT * FROM ";
  queryStr += this->TableName;
  query->SetQuery(queryStr.c_str());
  if (!query->Execute())
  {
    svtkErrorMacro(<< "Error performing 'select all' query");
  }

  // use the results of the query to populate the columns
  while (query->NextRow())
  {
    for (int col = 0; col < query->GetNumberOfFields(); ++col)
    {
      if (columnTypes[col] == "INTEGER")
      {
        svtkIntArray* column = static_cast<svtkIntArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToInt());
      }
      else if (columnTypes[col] == "REAL")
      {
        svtkDoubleArray* column = static_cast<svtkDoubleArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToDouble());
      }
      else
      {
        svtkStringArray* column = static_cast<svtkStringArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToString());
      }
    }
  }

  query->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void svtkSQLiteToTableReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
