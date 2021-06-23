/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLiteDatabase.cxx

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
#include "svtkSQLiteDatabase.h"
#include "svtkSQLiteDatabaseInternals.h"
#include "svtkSQLiteQuery.h"

#include "svtkSQLDatabaseSchema.h"

#include "svtkObjectFactory.h"
#include "svtkStringArray.h"

#include <fstream>
#include <sstream>
#include <svtksys/FStream.hxx>
#include <svtksys/SystemTools.hxx>

#include "svtk_sqlite.h"

svtkStandardNewMacro(svtkSQLiteDatabase);

// ----------------------------------------------------------------------
svtkSQLiteDatabase::svtkSQLiteDatabase()
{
  this->Internal = new svtkSQLiteDatabaseInternals;
  this->Internal->SQLiteInstance = nullptr;

  this->Tables = svtkStringArray::New();
  this->Tables->Register(this);
  this->Tables->Delete();

  // Initialize instance variables
  this->DatabaseType = nullptr;
  this->SetDatabaseType("sqlite");
  this->DatabaseFileName = nullptr;
}

// ----------------------------------------------------------------------
svtkSQLiteDatabase::~svtkSQLiteDatabase()
{
  if (this->IsOpen())
  {
    this->Close();
  }
  if (this->DatabaseType)
  {
    this->SetDatabaseType(nullptr);
  }
  if (this->DatabaseFileName)
  {
    this->SetDatabaseFileName(nullptr);
  }
  this->Tables->UnRegister(this);
  delete this->Internal;
}

// ----------------------------------------------------------------------
void svtkSQLiteDatabase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SQLiteInstance: ";
  if (this->Internal->SQLiteInstance)
  {
    os << this->Internal->SQLiteInstance << "\n";
  }
  else
  {
    os << "(null)"
       << "\n";
  }
  os << indent << "DatabaseType: " << (this->DatabaseType ? this->DatabaseType : "nullptr") << endl;
  os << indent
     << "DatabaseFileName: " << (this->DatabaseFileName ? this->DatabaseFileName : "nullptr")
     << endl;
}

// ----------------------------------------------------------------------
svtkStdString svtkSQLiteDatabase::GetColumnSpecification(
  svtkSQLDatabaseSchema* schema, int tblHandle, int colHandle)
{
  std::ostringstream queryStr;
  queryStr << schema->GetColumnNameFromHandle(tblHandle, colHandle);

  // Figure out column type
  int colType = schema->GetColumnTypeFromHandle(tblHandle, colHandle);
  svtkStdString colTypeStr;
  switch (static_cast<svtkSQLDatabaseSchema::DatabaseColumnType>(colType))
  {
    case svtkSQLDatabaseSchema::SERIAL:
      colTypeStr = "INTEGER NOT NULL";
      break;
    case svtkSQLDatabaseSchema::SMALLINT:
      colTypeStr = "SMALLINT";
      break;
    case svtkSQLDatabaseSchema::INTEGER:
      colTypeStr = "INTEGER";
      break;
    case svtkSQLDatabaseSchema::BIGINT:
      colTypeStr = "BIGINT";
      break;
    case svtkSQLDatabaseSchema::VARCHAR:
      colTypeStr = "VARCHAR";
      break;
    case svtkSQLDatabaseSchema::TEXT:
      colTypeStr = "TEXT";
      break;
    case svtkSQLDatabaseSchema::REAL:
      colTypeStr = "REAL";
      break;
    case svtkSQLDatabaseSchema::DOUBLE:
      colTypeStr = "DOUBLE";
      break;
    case svtkSQLDatabaseSchema::BLOB:
      colTypeStr = "BLOB";
      break;
    case svtkSQLDatabaseSchema::TIME:
      colTypeStr = "TIME";
      break;
    case svtkSQLDatabaseSchema::DATE:
      colTypeStr = "DATE";
      break;
    case svtkSQLDatabaseSchema::TIMESTAMP:
      colTypeStr = "TIMESTAMP";
  }

  if (!colTypeStr.empty())
  {
    queryStr << " " << colTypeStr;
  }
  else // if ( colTypeStr.size() )
  {
    svtkGenericWarningMacro("Unable to get column specification: unsupported data type " << colType);
    return svtkStdString();
  }

  // Decide whether size is allowed, required, or unused
  int colSizeType = 0;
  switch (static_cast<svtkSQLDatabaseSchema::DatabaseColumnType>(colType))
  {
    case svtkSQLDatabaseSchema::SERIAL:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::SMALLINT:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::INTEGER:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::BIGINT:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::VARCHAR:
      colSizeType = -1;
      break;
    case svtkSQLDatabaseSchema::TEXT:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::REAL:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::DOUBLE:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::BLOB:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::TIME:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::DATE:
      colSizeType = 0;
      break;
    case svtkSQLDatabaseSchema::TIMESTAMP:
      colSizeType = 0;
      break;
  }

  // Specify size if allowed or required
  if (colSizeType)
  {
    int colSize = schema->GetColumnSizeFromHandle(tblHandle, colHandle);
    // IF size is provided but absurd,
    // OR, if size is required but not provided OR absurd,
    // THEN assign the default size.
    if ((colSize < 0) || (colSizeType == -1 && colSize < 1))
    {
      colSize = SVTK_SQL_DEFAULT_COLUMN_SIZE;
    }

    // At this point, we have either a valid size if required, or a possibly null valid size
    // if not required. Thus, skip sizing in the latter case.
    if (colSize > 0)
    {
      queryStr << "(" << colSize << ")";
    }
  }

  svtkStdString attStr = schema->GetColumnAttributesFromHandle(tblHandle, colHandle);
  if (!attStr.empty())
  {
    queryStr << " " << attStr;
  }

  return queryStr.str();
}

// ----------------------------------------------------------------------
bool svtkSQLiteDatabase::IsSupported(int feature)
{
  switch (feature)
  {
    case SVTK_SQL_FEATURE_BLOB:
    case SVTK_SQL_FEATURE_LAST_INSERT_ID:
    case SVTK_SQL_FEATURE_NAMED_PLACEHOLDERS:
    case SVTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS:
    case SVTK_SQL_FEATURE_PREPARED_QUERIES:
    case SVTK_SQL_FEATURE_TRANSACTIONS:
    case SVTK_SQL_FEATURE_UNICODE:
      return true;

    case SVTK_SQL_FEATURE_BATCH_OPERATIONS:
    case SVTK_SQL_FEATURE_QUERY_SIZE:
    case SVTK_SQL_FEATURE_TRIGGERS:
      return false;

    default:
    {
      svtkErrorMacro(<< "Unknown SQL feature code " << feature << "!  See "
                    << "svtkSQLDatabase.h for a list of possible features.");
      return false;
    };
  }
}

// ----------------------------------------------------------------------
bool svtkSQLiteDatabase::Open(const char* password)
{
  return this->Open(password, USE_EXISTING);
}

// ----------------------------------------------------------------------
bool svtkSQLiteDatabase::Open(const char* password, int mode)
{
  if (this->IsOpen())
  {
    svtkWarningMacro("Open(): Database is already open.");
    return true;
  }

  if (password && strlen(password))
  {
    svtkGenericWarningMacro("Password will be ignored by svtkSQLiteDatabase::Open().");
  }

  if (!this->DatabaseFileName)
  {
    svtkErrorMacro("Cannot open database because DatabaseFileName is not set.");
    return false;
  }

  if (this->IsOpen())
  {
    svtkGenericWarningMacro("Open(): Database is already open.");
    return true;
  }

  // Only do checks if it is not an in-memory database
  if (strcmp(":memory:", this->DatabaseFileName))
  {
    bool exists = svtksys::SystemTools::FileExists(this->DatabaseFileName);
    if (mode == USE_EXISTING && !exists)
    {
      svtkErrorMacro("You specified using an existing database but the file does not exist.\n"
                    "Use USE_EXISTING_OR_CREATE to allow database creation.");
      return false;
    }
    if (mode == CREATE && exists)
    {
      svtkErrorMacro("You specified creating a database but the file exists.\n"
                    "Use USE_EXISTING_OR_CREATE to allow using an existing database,\n"
                    "or CREATE_OR_CLEAR to clear any existing file.");
      return false;
    }
    if (mode == CREATE_OR_CLEAR && exists)
    {
      // Here we need to clear the file if it exists by opening it.
      svtksys::ofstream os;
      os.open(this->DatabaseFileName);
      if (!os.is_open())
      {
        svtkErrorMacro("Unable to create file " << this->DatabaseFileName << ".");
        return false;
      }
      os.close();
    }
  }

  int result = sqlite3_open(this->DatabaseFileName, &(this->Internal->SQLiteInstance));

  if (result != SQLITE_OK)
  {
    svtkDebugMacro(<< "SQLite open() failed.  Error code is " << result << " and message is "
                  << sqlite3_errmsg(this->Internal->SQLiteInstance));

    sqlite3_close(this->Internal->SQLiteInstance);
    return false;
  }
  else
  {
    svtkDebugMacro(<< "SQLite open() succeeded.");
    return true;
  }
}

// ----------------------------------------------------------------------
void svtkSQLiteDatabase::Close()
{
  if (this->Internal->SQLiteInstance == nullptr)
  {
    svtkDebugMacro(<< "Close(): Database is already closed.");
  }
  else
  {
    int result = sqlite3_close(this->Internal->SQLiteInstance);
    if (result != SQLITE_OK)
    {
      svtkWarningMacro(<< "Close(): SQLite returned result code " << result);
    }
    this->Internal->SQLiteInstance = nullptr;
  }
}

// ----------------------------------------------------------------------
bool svtkSQLiteDatabase::IsOpen()
{
  return (this->Internal->SQLiteInstance != nullptr);
}

// ----------------------------------------------------------------------
svtkSQLQuery* svtkSQLiteDatabase::GetQueryInstance()
{
  svtkSQLiteQuery* query = svtkSQLiteQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------
svtkStringArray* svtkSQLiteDatabase::GetTables()
{
  this->Tables->Resize(0);
  if (this->Internal->SQLiteInstance == nullptr)
  {
    svtkErrorMacro(<< "GetTables(): Database is not open!");
    return this->Tables;
  }

  svtkSQLQuery* query = this->GetQueryInstance();
  query->SetQuery("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
  bool status = query->Execute();

  if (!status)
  {
    svtkErrorMacro(<< "GetTables(): Database returned error: "
                  << sqlite3_errmsg(this->Internal->SQLiteInstance));
    query->Delete();
    return this->Tables;
  }
  else
  {
    svtkDebugMacro(<< "GetTables(): SQL query succeeded.");
    while (query->NextRow())
    {
      this->Tables->InsertNextValue(query->DataValue(0).ToString());
    }
    query->Delete();
    return this->Tables;
  }
}

// ----------------------------------------------------------------------
svtkStringArray* svtkSQLiteDatabase::GetRecord(const char* table)
{
  svtkSQLQuery* query = this->GetQueryInstance();
  svtkStdString text("PRAGMA table_info ('");
  text += table;
  text += "')";

  query->SetQuery(text.c_str());
  bool status = query->Execute();
  if (!status)
  {
    svtkErrorMacro(<< "GetRecord(" << table << "): Database returned error: "
                  << sqlite3_errmsg(this->Internal->SQLiteInstance));
    query->Delete();
    return nullptr;
  }
  else
  {
    // Each row in the results that come back from this query
    // describes a single column in the table.  The format of each row
    // is as follows:
    //
    // columnID columnName columnType ??? defaultValue nullForbidden
    //
    // (I don't know what the ??? column is.  It's probably maximum
    // length.)
    svtkStringArray* results = svtkStringArray::New();

    while (query->NextRow())
    {
      results->InsertNextValue(query->DataValue(1).ToString());
    }

    query->Delete();
    return results;
  }
}

// ----------------------------------------------------------------------
svtkStdString svtkSQLiteDatabase::GetURL()
{
  const char* fname = this->GetDatabaseFileName();
  this->TempURL = this->GetDatabaseType();
  this->TempURL += "://";
  if (fname)
  {
    this->TempURL += fname;
  }
  return this->TempURL;
}

// ----------------------------------------------------------------------
bool svtkSQLiteDatabase::ParseURL(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol;
  std::string dataglom;

  if (!svtksys::SystemTools::ParseURLProtocol(urlstr, protocol, dataglom))
  {
    svtkErrorMacro("Invalid URL: \"" << urlstr.c_str() << "\"");
    return false;
  }

  if (protocol == "sqlite")
  {
    this->SetDatabaseFileName(dataglom.c_str());
    return true;
  }

  return false;
}

// ----------------------------------------------------------------------
bool svtkSQLiteDatabase::HasError()
{
  return (sqlite3_errcode(this->Internal->SQLiteInstance) != SQLITE_OK);
}

const char* svtkSQLiteDatabase::GetLastErrorText()
{
  return sqlite3_errmsg(this->Internal->SQLiteInstance);
}
