/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToSQLiteWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSQLiteDatabase.h"
#include "svtkSQLiteQuery.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkVariant.h"

#include "svtkTableToSQLiteWriter.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkTableToSQLiteWriter);

//----------------------------------------------------------------------------
svtkTableToSQLiteWriter::svtkTableToSQLiteWriter()
{
  this->Database = nullptr;
}

//----------------------------------------------------------------------------
svtkTableToSQLiteWriter::~svtkTableToSQLiteWriter() = default;

//----------------------------------------------------------------------------
void svtkTableToSQLiteWriter::WriteData()
{
  // Make sure we have all the information we need to create an SQLite table
  if (!this->Database)
  {
    svtkErrorMacro(<< "No open database connection");
    return;
  }
  if (!this->Database->IsA("svtkSQLiteDatabase"))
  {
    svtkErrorMacro(<< "Wrong type of database for this writer");
    return;
  }
  if (this->TableName.empty())
  {
    svtkErrorMacro(<< "No table name specified!");
    return;
  }

  // converting this table to SQLite will require two queries: one to create
  // the table, and another to populate its rows with data.
  std::string createTableQuery = "CREATE table ";
  createTableQuery += this->TableName;
  createTableQuery += "(";

  std::string insertPreamble = "INSERT into ";
  insertPreamble += this->TableName;
  insertPreamble += "(";

  // get the columns from the svtkTable to finish the query
  svtkIdType numColumns = this->GetInput()->GetNumberOfColumns();
  for (svtkIdType i = 0; i < numColumns; i++)
  {
    // get this column's name
    std::string columnName = this->GetInput()->GetColumn(i)->GetName();
    createTableQuery += columnName;
    insertPreamble += "'" + columnName + "'";

    // figure out what type of data is stored in this column
    std::string columnType = this->GetInput()->GetColumn(i)->GetClassName();

    if ((columnType.find("String") != std::string::npos) ||
      (columnType.find("Data") != std::string::npos) ||
      (columnType.find("Variant") != std::string::npos))
    {
      createTableQuery += " TEXT";
    }
    else if ((columnType.find("Double") != std::string::npos) ||
      (columnType.find("Float") != std::string::npos))
    {
      createTableQuery += " REAL";
    }
    else
    {
      createTableQuery += " INTEGER";
    }
    if (i == numColumns - 1)
    {
      createTableQuery += ");";
      insertPreamble += ") VALUES (";
    }
    else
    {
      createTableQuery += ", ";
      insertPreamble += ", ";
    }
  }

  // perform the create table query
  svtkSQLiteQuery* query = static_cast<svtkSQLiteQuery*>(this->Database->GetQueryInstance());

  query->SetQuery(createTableQuery.c_str());
  cout << "creating the table" << endl;
  if (!query->Execute())
  {
    svtkErrorMacro(<< "Error performing 'create table' query");
  }

  // iterate over the rows of the svtkTable to complete the insert query
  svtkIdType numRows = this->GetInput()->GetNumberOfRows();
  for (svtkIdType i = 0; i < numRows; i++)
  {
    std::string insertQuery = insertPreamble;
    for (svtkIdType j = 0; j < numColumns; j++)
    {
      insertQuery += "'" + this->GetInput()->GetValue(i, j).ToString() + "'";
      if (j < numColumns - 1)
      {
        insertQuery += ", ";
      }
    }
    insertQuery += ");";
    // perform the insert query for this row
    query->SetQuery(insertQuery.c_str());
    if (!query->Execute())
    {
      svtkErrorMacro(<< "Error performing 'insert' query");
    }
  }

  // cleanup and return
  query->Delete();
}

//----------------------------------------------------------------------------
int svtkTableToSQLiteWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//----------------------------------------------------------------------------
svtkTable* svtkTableToSQLiteWriter::GetInput()
{
  return svtkTable::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
svtkTable* svtkTableToSQLiteWriter::GetInput(int port)
{
  return svtkTable::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
void svtkTableToSQLiteWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
