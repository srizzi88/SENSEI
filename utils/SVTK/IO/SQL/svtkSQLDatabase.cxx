/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkSQLDatabase.cxx

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

#include "svtkSQLDatabase.h"
#include "svtkInformationObjectBaseKey.h"
#include "svtkSQLQuery.h"
#include "svtkToolkits.h"

#include "svtkSQLDatabaseSchema.h"

#include "svtkSQLiteDatabase.h"

#include "svtkCriticalSection.h"
#include "svtkObjectFactory.h"
#include "svtkStdString.h"

#include <sstream>
#include <svtksys/SystemTools.hxx>

class svtkSQLDatabase::svtkCallbackVector : public std::vector<svtkSQLDatabase::CreateFunction>
{
public:
  svtkSQLDatabase* CreateFromURL(const char* URL)
  {
    iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter)
    {
      svtkSQLDatabase* db = (*(*iter))(URL);
      if (db)
      {
        return db;
      }
    }
    return nullptr;
  }
};
svtkSQLDatabase::svtkCallbackVector* svtkSQLDatabase::Callbacks = nullptr;

// Ensures that there are no leaks when the application exits.
class svtkSQLDatabaseCleanup
{
public:
  inline void Use() {}
  ~svtkSQLDatabaseCleanup() { svtkSQLDatabase::UnRegisterAllCreateFromURLCallbacks(); }
};

// Used to clean up the Callbacks
static svtkSQLDatabaseCleanup svtkCleanupSQLDatabaseGlobal;

svtkInformationKeyMacro(svtkSQLDatabase, DATABASE, ObjectBase);

// ----------------------------------------------------------------------
svtkSQLDatabase::svtkSQLDatabase() = default;

// ----------------------------------------------------------------------
svtkSQLDatabase::~svtkSQLDatabase() = default;

// ----------------------------------------------------------------------
void svtkSQLDatabase::RegisterCreateFromURLCallback(svtkSQLDatabase::CreateFunction func)
{
  if (!svtkSQLDatabase::Callbacks)
  {
    svtkCleanupSQLDatabaseGlobal.Use();
    svtkSQLDatabase::Callbacks = new svtkCallbackVector();
  }
  svtkSQLDatabase::Callbacks->push_back(func);
}

// ----------------------------------------------------------------------
void svtkSQLDatabase::UnRegisterCreateFromURLCallback(svtkSQLDatabase::CreateFunction func)
{
  if (svtkSQLDatabase::Callbacks)
  {
    svtkSQLDatabase::svtkCallbackVector::iterator iter;
    for (iter = svtkSQLDatabase::Callbacks->begin(); iter != svtkSQLDatabase::Callbacks->end();
         ++iter)
    {
      if ((*iter) == func)
      {
        svtkSQLDatabase::Callbacks->erase(iter);
        break;
      }
    }
  }
}

// ----------------------------------------------------------------------
void svtkSQLDatabase::UnRegisterAllCreateFromURLCallbacks()
{
  delete svtkSQLDatabase::Callbacks;
  svtkSQLDatabase::Callbacks = nullptr;
}

// ----------------------------------------------------------------------
void svtkSQLDatabase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------
svtkStdString svtkSQLDatabase::GetColumnSpecification(
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
      colTypeStr = "INTEGER";
      break;
    case svtkSQLDatabaseSchema::SMALLINT:
      colTypeStr = "INTEGER";
      break;
    case svtkSQLDatabaseSchema::INTEGER:
      colTypeStr = "INTEGER";
      break;
    case svtkSQLDatabaseSchema::BIGINT:
      colTypeStr = "INTEGER";
      break;
    case svtkSQLDatabaseSchema::VARCHAR:
      colTypeStr = "VARCHAR";
      break;
    case svtkSQLDatabaseSchema::TEXT:
      colTypeStr = "VARCHAR";
      break;
    case svtkSQLDatabaseSchema::REAL:
      colTypeStr = "FLOAT";
      break;
    case svtkSQLDatabaseSchema::DOUBLE:
      colTypeStr = "DOUBLE";
      break;
    case svtkSQLDatabaseSchema::BLOB:
      colTypeStr = "";
      break;
    case svtkSQLDatabaseSchema::TIME:
      colTypeStr = "TIME";
      break;
    case svtkSQLDatabaseSchema::DATE:
      colTypeStr = "DATE";
      break;
    case svtkSQLDatabaseSchema::TIMESTAMP:
      colTypeStr = "TIMESTAMP";
      break;
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
      colSizeType = 1;
      break;
    case svtkSQLDatabaseSchema::INTEGER:
      colSizeType = 1;
      break;
    case svtkSQLDatabaseSchema::BIGINT:
      colSizeType = 1;
      break;
    case svtkSQLDatabaseSchema::VARCHAR:
      colSizeType = -1;
      break;
    case svtkSQLDatabaseSchema::TEXT:
      colSizeType = -1;
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
svtkStdString svtkSQLDatabase::GetIndexSpecification(
  svtkSQLDatabaseSchema* schema, int tblHandle, int idxHandle, bool& skipped)
{
  svtkStdString queryStr;

  int idxType = schema->GetIndexTypeFromHandle(tblHandle, idxHandle);
  switch (idxType)
  {
    case svtkSQLDatabaseSchema::PRIMARY_KEY:
      queryStr = ", PRIMARY KEY ";
      skipped = false;
      break;
    case svtkSQLDatabaseSchema::UNIQUE:
      queryStr = ", UNIQUE ";
      skipped = false;
      break;
    case svtkSQLDatabaseSchema::INDEX:
      // Not supported within a CREATE TABLE statement by all SQL backends:
      // must be created later with a CREATE INDEX statement
      queryStr = "CREATE INDEX ";
      skipped = true;
      break;
    default:
      return svtkStdString();
  }

  // No index_name for PRIMARY KEYs nor UNIQUEs
  if (skipped)
  {
    queryStr += schema->GetIndexNameFromHandle(tblHandle, idxHandle);
  }

  // CREATE INDEX <index name> ON <table name> syntax
  if (skipped)
  {
    queryStr += " ON ";
    queryStr += schema->GetTableNameFromHandle(tblHandle);
  }

  queryStr += " (";

  // Loop over all column names of the index
  int numCnm = schema->GetNumberOfColumnNamesInIndex(tblHandle, idxHandle);
  if (numCnm < 0)
  {
    svtkGenericWarningMacro(
      "Unable to get index specification: index has incorrect number of columns " << numCnm);
    return svtkStdString();
  }

  bool firstCnm = true;
  for (int cnmHandle = 0; cnmHandle < numCnm; ++cnmHandle)
  {
    if (firstCnm)
    {
      firstCnm = false;
    }
    else
    {
      queryStr += ",";
    }
    queryStr += schema->GetIndexColumnNameFromHandle(tblHandle, idxHandle, cnmHandle);
  }
  queryStr += ")";

  return queryStr;
}

// ----------------------------------------------------------------------
svtkStdString svtkSQLDatabase::GetTriggerSpecification(
  svtkSQLDatabaseSchema* schema, int tblHandle, int trgHandle)
{
  svtkStdString queryStr = "CREATE TRIGGER ";
  queryStr += schema->GetTriggerNameFromHandle(tblHandle, trgHandle);

  int trgType = schema->GetTriggerTypeFromHandle(tblHandle, trgHandle);
  // odd types: AFTER, even types: BEFORE
  if (trgType % 2)
  {
    queryStr += " AFTER ";
  }
  else
  {
    queryStr += " BEFORE ";
  }
  // 0/1: INSERT, 2/3: UPDATE, 4/5: DELETE
  if (trgType > 1)
  {
    if (trgType > 3)
    {
      queryStr += "DELETE ON ";
    }
    else // if ( trgType > 3 )
    {
      queryStr += "UPDATE ON ";
    }
  }
  else // if ( trgType > 1 )
  {
    queryStr += "INSERT ON ";
  }

  queryStr += schema->GetTableNameFromHandle(tblHandle);
  queryStr += " ";
  queryStr += schema->GetTriggerActionFromHandle(tblHandle, trgHandle);

  return queryStr;
}

// ----------------------------------------------------------------------
svtkSQLDatabase* svtkSQLDatabase::CreateFromURL(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol;
  std::string username;
  std::string unused;
  std::string hostname;
  std::string dataport;
  std::string database;
  std::string dataglom;
  svtkSQLDatabase* db = nullptr;

  static svtkSimpleCriticalSection dbURLCritSec;
  dbURLCritSec.Lock();

  // SQLite is a bit special so lets get that out of the way :)
  if (!svtksys::SystemTools::ParseURLProtocol(urlstr, protocol, dataglom))
  {
    svtkGenericWarningMacro("Invalid URL (no protocol found): \"" << urlstr.c_str() << "\"");
    dbURLCritSec.Unlock();
    return nullptr;
  }
  if (protocol == "sqlite")
  {
    db = svtkSQLiteDatabase::New();
    db->ParseURL(URL);
    dbURLCritSec.Unlock();
    return db;
  }

  // Okay now for all the other database types get more detailed info
  if (!svtksys::SystemTools::ParseURL(
        urlstr, protocol, username, unused, hostname, dataport, database))
  {
    svtkGenericWarningMacro("Invalid URL (other components missing): \"" << urlstr.c_str() << "\"");
    dbURLCritSec.Unlock();
    return nullptr;
  }

  // Now try to look at registered callback to try and find someone who can
  // provide us with the required implementation.
  if (!db && svtkSQLDatabase::Callbacks)
  {
    db = svtkSQLDatabase::Callbacks->CreateFromURL(URL);
  }

  if (!db)
  {
    svtkGenericWarningMacro("Unsupported protocol: " << protocol.c_str());
  }
  dbURLCritSec.Unlock();
  return db;
}

// ----------------------------------------------------------------------
bool svtkSQLDatabase::EffectSchema(svtkSQLDatabaseSchema* schema, bool dropIfExists)
{
  if (!this->IsOpen())
  {
    svtkGenericWarningMacro("Unable to effect the schema: no database is open");
    return false;
  }

  // Instantiate an empty query and begin the transaction.
  svtkSQLQuery* query = this->GetQueryInstance();
  if (!query->BeginTransaction())
  {
    svtkGenericWarningMacro("Unable to effect the schema: unable to begin transaction");
    return false;
  }

  // Loop over preamble statements of the schema and execute them only if they are relevant
  int numPre = schema->GetNumberOfPreambles();
  for (int preHandle = 0; preHandle < numPre; ++preHandle)
  {
    // Don't execute if the statement is not for this backend
    const char* preBackend = schema->GetPreambleBackendFromHandle(preHandle);
    if (strcmp(preBackend, SVTK_SQL_ALLBACKENDS) && strcmp(preBackend, this->GetClassName()))
    {
      continue;
    }

    svtkStdString preStr = schema->GetPreambleActionFromHandle(preHandle);
    query->SetQuery(preStr);
    if (!query->Execute())
    {
      svtkGenericWarningMacro("Unable to effect the schema: unable to execute query.\nDetails: "
        << query->GetLastErrorText());
      query->RollbackTransaction();
      query->Delete();
      return false;
    }
  }

  // Loop over all tables of the schema and create them
  int numTbl = schema->GetNumberOfTables();
  for (int tblHandle = 0; tblHandle < numTbl; ++tblHandle)
  {
    // Construct the CREATE TABLE query for this table
    svtkStdString queryStr("CREATE TABLE ");
    queryStr += this->GetTablePreamble(dropIfExists);
    queryStr += schema->GetTableNameFromHandle(tblHandle);
    queryStr += " (";

    // Loop over all columns of the current table
    int numCol = schema->GetNumberOfColumnsInTable(tblHandle);
    if (numCol < 0)
    {
      query->RollbackTransaction();
      query->Delete();
      return false;
    }

    bool firstCol = true;
    for (int colHandle = 0; colHandle < numCol; ++colHandle)
    {
      if (!firstCol)
      {
        queryStr += ", ";
      }
      else // ( ! firstCol )
      {
        firstCol = false;
      }

      // Get column creation syntax (backend-dependent)
      svtkStdString colStr = this->GetColumnSpecification(schema, tblHandle, colHandle);
      if (!colStr.empty())
      {
        queryStr += colStr;
      }
      else // if ( colStr.size() )
      {
        query->RollbackTransaction();
        query->Delete();
        return false;
      }
    }

    // Check out number of indices
    int numIdx = schema->GetNumberOfIndicesInTable(tblHandle);
    if (numIdx < 0)
    {
      query->RollbackTransaction();
      query->Delete();
      return false;
    }

    // In case separate INDEX statements are needed (backend-specific)
    std::vector<svtkStdString> idxStatements;
    bool skipped = false;

    // Loop over all indices of the current table
    for (int idxHandle = 0; idxHandle < numIdx; ++idxHandle)
    {
      // Get index creation syntax (backend-dependent)
      svtkStdString idxStr = this->GetIndexSpecification(schema, tblHandle, idxHandle, skipped);
      if (!idxStr.empty())
      {
        if (skipped)
        {
          // Must create this index later
          idxStatements.push_back(idxStr);
          continue;
        }
        else // if ( skipped )
        {
          queryStr += idxStr;
        }
      }
      else // if ( idxStr.size() )
      {
        query->RollbackTransaction();
        query->Delete();
        return false;
      }
    }
    queryStr += ")";

    // Add options to the end of the CREATE TABLE statement
    int numOpt = schema->GetNumberOfOptionsInTable(tblHandle);
    if (numOpt < 0)
    {
      query->RollbackTransaction();
      query->Delete();
      return false;
    }
    for (int optHandle = 0; optHandle < numOpt; ++optHandle)
    {
      svtkStdString optBackend = schema->GetOptionBackendFromHandle(tblHandle, optHandle);
      if (strcmp(optBackend, SVTK_SQL_ALLBACKENDS) && strcmp(optBackend, this->GetClassName()))
      {
        continue;
      }
      queryStr += " ";
      queryStr += schema->GetOptionTextFromHandle(tblHandle, optHandle);
    }

    // Execute the CREATE TABLE query
    query->SetQuery(queryStr);
    if (!query->Execute())
    {
      svtkGenericWarningMacro("Unable to effect the schema: unable to execute query.\nDetails: "
        << query->GetLastErrorText());
      query->RollbackTransaction();
      query->Delete();
      return false;
    }

    // Execute separate CREATE INDEX statements if needed
    for (std::vector<svtkStdString>::iterator it = idxStatements.begin(); it != idxStatements.end();
         ++it)
    {
      query->SetQuery(*it);
      if (!query->Execute())
      {
        svtkGenericWarningMacro("Unable to effect the schema: unable to execute query.\nDetails: "
          << query->GetLastErrorText());
        query->RollbackTransaction();
        query->Delete();
        return false;
      }
    }

    // Check out number of triggers
    int numTrg = schema->GetNumberOfTriggersInTable(tblHandle);
    if (numTrg < 0)
    {
      query->RollbackTransaction();
      query->Delete();
      return false;
    }

    // Construct CREATE TRIGGER statements only if they are supported by the backend at hand
    if (numTrg && IsSupported(SVTK_SQL_FEATURE_TRIGGERS))
    {
      // Loop over all triggers of the current table
      for (int trgHandle = 0; trgHandle < numTrg; ++trgHandle)
      {
        // Don't execute if the trigger is not for this backend
        const char* trgBackend = schema->GetTriggerBackendFromHandle(tblHandle, trgHandle);
        if (strcmp(trgBackend, SVTK_SQL_ALLBACKENDS) && strcmp(trgBackend, this->GetClassName()))
        {
          continue;
        }

        // Get trigger creation syntax (backend-dependent)
        svtkStdString trgStr = this->GetTriggerSpecification(schema, tblHandle, trgHandle);

        // If not empty, execute query
        if (!trgStr.empty())
        {
          query->SetQuery(svtkStdString(trgStr));
          if (!query->Execute())
          {
            svtkGenericWarningMacro(
              "Unable to effect the schema: unable to execute query.\nDetails: "
              << query->GetLastErrorText());
            query->RollbackTransaction();
            query->Delete();
            return false;
          }
        }
        else // if ( trgStr.size() )
        {
          query->RollbackTransaction();
          query->Delete();
          return false;
        }
      }
    }

    // If triggers are specified but not supported, don't quit, but let the user know it
    else if (numTrg)
    {
      svtkGenericWarningMacro("Triggers are not supported by this SQL backend; ignoring them.");
    }
  } //  for ( int tblHandle = 0; tblHandle < numTbl; ++ tblHandle )

  // Commit the transaction.
  if (!query->CommitTransaction())
  {
    svtkGenericWarningMacro("Unable to effect the schema: unable to commit transaction.\nDetails: "
      << query->GetLastErrorText());
    query->Delete();
    return false;
  }

  query->Delete();
  return true;
}
