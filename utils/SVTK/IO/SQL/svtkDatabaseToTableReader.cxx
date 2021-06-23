/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDatabaseToTableReader.cxx

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

#include "svtkDatabaseToTableReader.h"

//----------------------------------------------------------------------------
svtkDatabaseToTableReader::svtkDatabaseToTableReader()
{
  this->Database = nullptr;
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkDatabaseToTableReader::~svtkDatabaseToTableReader() = default;

//----------------------------------------------------------------------------
bool svtkDatabaseToTableReader::SetDatabase(svtkSQLDatabase* db)
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
    return this->CheckIfTableExists();
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkDatabaseToTableReader::SetTableName(const char* name)
{
  std::string nameStr = name;
  this->TableName = nameStr;
  if (this->Database->IsOpen())
  {
    return this->CheckIfTableExists();
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkDatabaseToTableReader::CheckIfTableExists()
{
  if (!this->Database->IsOpen())
  {
    svtkErrorMacro(<< "CheckIfTableExists() called with no open database!");
    return false;
  }
  if (this->TableName.empty())
  {
    svtkErrorMacro(<< "CheckIfTableExists() called but no table name specified.");
    return false;
  }

  if (this->Database->GetTables()->LookupValue(this->TableName) == -1)
  {
    svtkErrorMacro(<< "Table " << this->TableName << " does not exist in the database!");
    this->TableName = "";
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
void svtkDatabaseToTableReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
