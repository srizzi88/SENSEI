/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMySQLDatabase.cxx

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
#include "svtkMySQLDatabase.h"
#include "svtkMySQLDatabasePrivate.h"
#include "svtkMySQLQuery.h"

#include "svtkSQLDatabaseSchema.h"

#include "svtkObjectFactory.h"
#include "svtkStringArray.h"

#include <sstream>
#include <svtksys/SystemTools.hxx>

#include <cassert>

#define SVTK_MYSQL_DEFAULT_PORT 3306

svtkStandardNewMacro(svtkMySQLDatabase);

// ----------------------------------------------------------------------
svtkMySQLDatabase::svtkMySQLDatabase()
  : Private(new svtkMySQLDatabasePrivate())
{
  this->Tables = svtkStringArray::New();
  this->Tables->Register(this);
  this->Tables->Delete();

  // Initialize instance variables
  this->DatabaseType = 0;
  this->SetDatabaseType("mysql");
  this->HostName = 0;
  this->User = 0;
  this->Password = 0;
  this->DatabaseName = 0;
  this->Reconnect = 1;
  // Default: connect to local machine on standard port
  this->SetHostName("localhost");
  this->ServerPort = SVTK_MYSQL_DEFAULT_PORT;
}

// ----------------------------------------------------------------------
svtkMySQLDatabase::~svtkMySQLDatabase()
{
  if (this->IsOpen())
  {
    this->Close();
  }
  this->SetDatabaseType(0);
  this->SetHostName(0);
  this->SetUser(0);
  this->SetDatabaseName(0);
  this->SetPassword(0);

  this->Tables->UnRegister(this);

  delete this->Private;
}

// ----------------------------------------------------------------------
void svtkMySQLDatabase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DatabaseType: " << (this->DatabaseType ? this->DatabaseType : "nullptr") << endl;
  os << indent << "HostName: " << (this->HostName ? this->HostName : "nullptr") << endl;
  os << indent << "User: " << (this->User ? this->User : "nullptr") << endl;
  os << indent << "Password: " << (this->Password ? "(hidden)" : "(none)") << endl;
  os << indent << "DatabaseName: " << (this->DatabaseName ? this->DatabaseName : "nullptr") << endl;
  os << indent << "ServerPort: " << this->ServerPort << endl;
  os << indent << "Reconnect: " << (this->Reconnect ? "ON" : "OFF") << endl;
}

// ----------------------------------------------------------------------
bool svtkMySQLDatabase::IsSupported(int feature)
{
  switch (feature)
  {
    case SVTK_SQL_FEATURE_BATCH_OPERATIONS:
    case SVTK_SQL_FEATURE_NAMED_PLACEHOLDERS:
      return false;

    case SVTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS:
#if MYSQL_VERSION_ID >= 40108
      return true;
#else
      return false;
#endif

    case SVTK_SQL_FEATURE_PREPARED_QUERIES:
    {
      if (mysql_get_client_version() >= 40108 &&
        mysql_get_server_version(&this->Private->NullConnection) >= 40100)
      {
        return true;
      }
      else
      {
        return false;
      }
    };

    case SVTK_SQL_FEATURE_QUERY_SIZE:
    case SVTK_SQL_FEATURE_BLOB:
    case SVTK_SQL_FEATURE_LAST_INSERT_ID:
    case SVTK_SQL_FEATURE_UNICODE:
    case SVTK_SQL_FEATURE_TRANSACTIONS:
    case SVTK_SQL_FEATURE_TRIGGERS:
      return true;

    default:
    {
      svtkErrorMacro(<< "Unknown SQL feature code " << feature << "!  See "
                    << "svtkSQLDatabase.h for a list of possible features.");
      return false;
    };
  }
}

// ----------------------------------------------------------------------
bool svtkMySQLDatabase::Open(const char* password)
{
  if (this->IsOpen())
  {
    svtkGenericWarningMacro("Open(): Database is already open.");
    return true;
  }

  assert(this->Private->Connection == nullptr);

  if (this->Reconnect)
  {
    my_bool recon = true;
    mysql_options(&this->Private->NullConnection, MYSQL_OPT_RECONNECT, &recon);
  }

  this->Private->Connection =
    mysql_real_connect(&this->Private->NullConnection, this->GetHostName(), this->GetUser(),
      (password && strlen(password) ? password : this->Password), this->GetDatabaseName(),
      this->GetServerPort(), 0, 0);

  if (this->Private->Connection == nullptr)
  {
    svtkErrorMacro(<< "Open() failed with error: " << mysql_error(&this->Private->NullConnection));
    return false;
  }
  else
  {
    svtkDebugMacro(<< "Open() succeeded.");

    if (this->Password != password)
    {
      delete[] this->Password;
      this->Password = password ? svtksys::SystemTools::DuplicateString(password) : 0;
    }

    return true;
  }
}

// ----------------------------------------------------------------------
void svtkMySQLDatabase::Close()
{
  if (!this->IsOpen())
  {
    return; // not an error
  }
  else
  {
    mysql_close(this->Private->Connection);
    this->Private->Connection = nullptr;
  }
}

// ----------------------------------------------------------------------
bool svtkMySQLDatabase::IsOpen()
{
  return (this->Private->Connection != nullptr);
}

// ----------------------------------------------------------------------
svtkSQLQuery* svtkMySQLDatabase::GetQueryInstance()
{
  svtkMySQLQuery* query = svtkMySQLQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------
svtkStringArray* svtkMySQLDatabase::GetTables()
{
  this->Tables->Resize(0);
  if (!this->IsOpen())
  {
    svtkErrorMacro(<< "GetTables(): Database is closed!");
    return this->Tables;
  }
  else
  {
    MYSQL_RES* tableResult = mysql_list_tables(this->Private->Connection, nullptr);

    if (!tableResult)
    {
      svtkErrorMacro(<< "GetTables(): MySQL returned error: "
                    << mysql_error(this->Private->Connection));
      return this->Tables;
    }

    MYSQL_ROW row;
    int i = 0;

    while (tableResult)
    {
      mysql_data_seek(tableResult, i);
      row = mysql_fetch_row(tableResult);
      if (!row)
      {
        break;
      }

      this->Tables->InsertNextValue(row[0]);
      ++i;
    }
    // Done with processing so free it
    mysql_free_result(tableResult);

    return this->Tables;
  }
}

// ----------------------------------------------------------------------
svtkStringArray* svtkMySQLDatabase::GetRecord(const char* table)
{
  svtkStringArray* results = svtkStringArray::New();

  if (!this->IsOpen())
  {
    svtkErrorMacro(<< "GetRecord: Database is not open!");
    return results;
  }

  MYSQL_RES* record = mysql_list_fields(this->Private->Connection, table, 0);

  if (!record)
  {
    svtkErrorMacro(<< "GetRecord: MySQL returned error: " << mysql_error(this->Private->Connection));
    return results;
  }

  MYSQL_FIELD* field;
  while ((field = mysql_fetch_field(record)))
  {
    results->InsertNextValue(field->name);
  }

  mysql_free_result(record);
  return results;
}

bool svtkMySQLDatabase::HasError()
{
  if (this->Private->Connection)
  {
    return (mysql_errno(this->Private->Connection) != 0);
  }
  else
  {
    return (mysql_errno(&this->Private->NullConnection) != 0);
  }
}

const char* svtkMySQLDatabase::GetLastErrorText()
{
  if (this->Private->Connection)
  {
    return mysql_error(this->Private->Connection);
  }
  else if (this->HasError())
  {
    return mysql_error(&this->Private->NullConnection);
  }
  else
  {
    return 0;
  }
}

// ----------------------------------------------------------------------
svtkStdString svtkMySQLDatabase::GetURL()
{
  svtkStdString url;
  url = this->GetDatabaseType();
  url += "://";
  if (this->GetUser() && strlen(this->GetUser()))
  {
    url += this->GetUser();
    url += "@";
  }
  if (this->GetHostName() && strlen(this->GetHostName()))
  {
    url += this->GetHostName();
  }
  else
  {
    url += "localhost";
  }
  if (this->GetServerPort() >= 0 && this->GetServerPort() != SVTK_MYSQL_DEFAULT_PORT)
  {
    std::ostringstream stream;
    stream << ":" << this->GetServerPort();
    url += stream.str();
  }
  url += "/";
  if (this->GetDatabaseName() && strlen(this->GetDatabaseName()))
    url += this->GetDatabaseName();
  return url;
}

// ----------------------------------------------------------------------
bool svtkMySQLDatabase::ParseURL(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol;
  std::string username;
  std::string password;
  std::string hostname;
  std::string dataport;
  std::string database;

  if (!svtksys::SystemTools::ParseURL(
        urlstr, protocol, username, password, hostname, dataport, database))
  {
    svtkGenericWarningMacro("Invalid URL: \"" << urlstr.c_str() << "\"");
    return false;
  }

  if (protocol == "mysql")
  {
    if (username.size())
    {
      this->SetUser(username.c_str());
    }
    if (password.size())
    {
      this->SetPassword(password.c_str());
    }
    if (dataport.size())
    {
      this->SetServerPort(atoi(dataport.c_str()));
    }
    this->SetHostName(hostname.c_str());
    this->SetDatabaseName(database.c_str());
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------
svtkStdString svtkMySQLDatabase::GetColumnSpecification(
  svtkSQLDatabaseSchema* schema, int tblHandle, int colHandle)
{
  // With MySQL, the column name must be enclosed between backquotes
  std::ostringstream queryStr;
  queryStr << "`" << schema->GetColumnNameFromHandle(tblHandle, colHandle) << "` ";

  // Figure out column type
  int colType = schema->GetColumnTypeFromHandle(tblHandle, colHandle);
  svtkStdString colTypeStr;

  switch (static_cast<svtkSQLDatabaseSchema::DatabaseColumnType>(colType))
  {
    case svtkSQLDatabaseSchema::SERIAL:
      colTypeStr = "INT NOT nullptr AUTO_INCREMENT";
      break;
    case svtkSQLDatabaseSchema::SMALLINT:
      colTypeStr = "SMALLINT";
      break;
    case svtkSQLDatabaseSchema::INTEGER:
      colTypeStr = "INT";
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
      colTypeStr = "FLOAT";
      break;
    case svtkSQLDatabaseSchema::DOUBLE:
      colTypeStr = "DOUBLE PRECISION";
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
      break;
  }

  if (colTypeStr.size())
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
      colSizeType = 1;
      break;
    case svtkSQLDatabaseSchema::REAL:
      colSizeType = 0; // Eventually will make DB schemata handle (M,D) sizes
      break;
    case svtkSQLDatabaseSchema::DOUBLE:
      colSizeType = 0; // Eventually will make DB schemata handle (M,D) sizes
      break;
    case svtkSQLDatabaseSchema::BLOB:
      colSizeType = 1;
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
    if (colType == svtkSQLDatabaseSchema::BLOB)
    {
      if (colSize >= 1 << 24)
      {
        colTypeStr = "LONGBLOB";
      }
      else if (colSize >= 1 << 16)
      {
        colTypeStr = "MEDIUMBLOB";
      }
    }

    // At this point, we have either a valid size if required, or a possibly null valid size
    // if not required. Thus, skip sizing in the latter case.
    if (colSize > 0)
    {
      queryStr << "(" << colSize << ")";
    }
  }

  svtkStdString attStr = schema->GetColumnAttributesFromHandle(tblHandle, colHandle);
  if (attStr.size())
  {
    queryStr << " " << attStr;
  }

  return queryStr.str();
}

// ----------------------------------------------------------------------
svtkStdString svtkMySQLDatabase::GetIndexSpecification(
  svtkSQLDatabaseSchema* schema, int tblHandle, int idxHandle, bool& skipped)
{
  skipped = false;
  svtkStdString queryStr = ", ";
  bool mustUseName = true;

  int idxType = schema->GetIndexTypeFromHandle(tblHandle, idxHandle);
  switch (idxType)
  {
    case svtkSQLDatabaseSchema::PRIMARY_KEY:
      queryStr += "PRIMARY KEY ";
      mustUseName = false;
      break;
    case svtkSQLDatabaseSchema::UNIQUE:
      queryStr += "UNIQUE ";
      break;
    case svtkSQLDatabaseSchema::INDEX:
      queryStr += "INDEX ";
      break;
    default:
      return svtkStdString();
  }

  // No index_name for PRIMARY KEYs
  if (mustUseName)
  {
    queryStr += schema->GetIndexNameFromHandle(tblHandle, idxHandle);
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
    // With MySQL, the column name must be enclosed between backquotes
    queryStr += "`";
    queryStr += schema->GetIndexColumnNameFromHandle(tblHandle, idxHandle, cnmHandle);
    queryStr += "` ";
  }
  queryStr += ")";

  return queryStr;
}

bool svtkMySQLDatabase::CreateDatabase(const char* dbName, bool dropExisting = false)
{
  if (dropExisting)
  {
    this->DropDatabase(dbName);
  }
  svtkStdString queryStr;
  queryStr = "CREATE DATABASE ";
  queryStr += dbName;
  bool status = false;
  char* tmpName = this->DatabaseName;
  bool needToReopen = false;
  if (!strcmp(dbName, tmpName))
  {
    this->Close();
    this->DatabaseName = 0;
    needToReopen = true;
  }
  if (this->IsOpen() || this->Open(this->Password))
  {
    svtkSQLQuery* query = this->GetQueryInstance();
    query->SetQuery(queryStr.c_str());
    status = query->Execute();
    query->Delete();
  }
  if (needToReopen)
  {
    this->Close();
    this->DatabaseName = tmpName;
    this->Open(this->Password);
  }
  return status;
}

bool svtkMySQLDatabase::DropDatabase(const char* dbName)
{
  svtkStdString queryStr;
  queryStr = "DROP DATABASE IF EXISTS ";
  queryStr += dbName;
  bool status = false;
  char* tmpName = this->DatabaseName;
  bool dropSelf = false;
  if (!strcmp(dbName, tmpName))
  {
    this->Close();
    this->DatabaseName = 0;
    dropSelf = true;
  }
  if (this->IsOpen() || this->Open(this->Password))
  {
    svtkSQLQuery* query = this->GetQueryInstance();
    query->SetQuery(queryStr.c_str());
    status = query->Execute();
    query->Delete();
  }
  if (dropSelf)
  {
    this->Close();
    this->DatabaseName = tmpName;
  }
  return status;
}
