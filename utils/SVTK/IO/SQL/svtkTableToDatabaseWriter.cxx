/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToDatabaseWriter.cxx

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
#include "svtkSQLDatabase.h"
#include "svtkSQLQuery.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

#include "svtkTableToDatabaseWriter.h"

//----------------------------------------------------------------------------
svtkTableToDatabaseWriter::svtkTableToDatabaseWriter()
{
  this->Database = nullptr;
}

//----------------------------------------------------------------------------
svtkTableToDatabaseWriter::~svtkTableToDatabaseWriter() = default;

//----------------------------------------------------------------------------
bool svtkTableToDatabaseWriter::SetDatabase(svtkSQLDatabase* db)
{
  if (!db)
  {
    return false;
  }
  this->Database = db;
  if (this->Database->IsOpen() == false)
  {
    svtkErrorMacro(<< "SetDatabase must be passed an open database connection");
    this->Database = nullptr;
    return false;
  }

  if (!this->TableName.empty())
  {
    return this->TableNameIsNew();
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkTableToDatabaseWriter::SetTableName(const char* name)
{
  std::string nameStr = name;
  this->TableName = nameStr;
  if (this->Database != nullptr)
  {
    return this->TableNameIsNew();
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkTableToDatabaseWriter::TableNameIsNew()
{
  if (this->Database == nullptr)
  {
    svtkErrorMacro(<< "TableNameIsNew() called with no open database!");
    return false;
  }

  if (this->TableName.empty())
  {
    svtkErrorMacro(<< "TableNameIsNew() called but no table name specified.");
    return false;
  }

  svtkStringArray* tableNames = this->Database->GetTables();
  if (tableNames->LookupValue(this->TableName) == -1)
  {
    return true;
  }

  svtkErrorMacro(<< "Table " << this->TableName << " already exists in the database.  "
                << "Please choose another name.");
  this->TableName = "";
  return false;
}

int svtkTableToDatabaseWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

svtkTable* svtkTableToDatabaseWriter::GetInput()
{
  return svtkTable::SafeDownCast(this->Superclass::GetInput());
}

svtkTable* svtkTableToDatabaseWriter::GetInput(int port)
{
  return svtkTable::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
void svtkTableToDatabaseWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
