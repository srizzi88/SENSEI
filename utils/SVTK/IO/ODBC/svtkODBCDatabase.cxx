/*=========================================================================

  Program Toolkit
  Module:    svtkODBCDatabase.cxx

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
  ----------------------------------------------------------------------------
*/

/*
 * Microsoft's own version of sqltypes.h relies on some typedefs and
 * macros in windows.h.  This next fragment tells SVTK to include the
 * whole thing without any of its usual #defines to keep the size
 * manageable.  No WIN32_LEAN_AND_MEAN for us!
 */
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <svtkWindows.h>
#endif

#include "svtkSQLDatabaseSchema.h"

#include "svtkODBCDatabase.h"
#include "svtkODBCInternals.h"
#include "svtkODBCQuery.h"

#include "svtkObjectFactory.h"
#include "svtkStringArray.h"

#include <sstream>
#include <svtksys/SystemTools.hxx>

#include <cassert>
#include <cstring>
#include <vector>

#include <sql.h>
#include <sqlext.h>

// ----------------------------------------------------------------------------
svtkStandardNewMacro(svtkODBCDatabase);

// ----------------------------------------------------------------------------
static svtkStdString GetErrorMessage(SQLSMALLINT handleType, SQLHANDLE handle, int* code = 0)
{
  SQLINTEGER sqlNativeCode = 0;
  SQLSMALLINT messageLength = 0;
  SQLRETURN status;
  SQLCHAR state[SQL_SQLSTATE_SIZE + 1];
  SQLCHAR description[SQL_MAX_MESSAGE_LENGTH + 1];
  svtkStdString finalResult;
  int i = 1;

  // There may be several error messages queued up so we need to loop
  // until we've got everything.
  std::ostringstream messagebuf;
  do
  {
    status = SQLGetDiagRec(handleType, handle, i, state, &sqlNativeCode, description,
      SQL_MAX_MESSAGE_LENGTH, &messageLength);

    description[SQL_MAX_MESSAGE_LENGTH] = 0;
    if (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO)
    {
      if (code)
      {
        *code = sqlNativeCode;
      }
      if (i > 1)
      {
        messagebuf << ", ";
      }
      messagebuf << state << ' ' << description;
    }
    else if (status == SQL_ERROR || status == SQL_INVALID_HANDLE)
    {
      return svtkStdString(messagebuf.str());
    }
    ++i;
  } while (status != SQL_NO_DATA);

  return svtkStdString(messagebuf.str());
}

// ----------------------------------------------------------------------------
// COLUMN is zero-indexed but ODBC indexes from 1.  Sigh.  Aren't
// standards fun?
//
// Also, this will need to be updated when we start handling Unicode
// characters.

static svtkStdString odbcGetString(SQLHANDLE statement, int column, int columnSize)
{
  svtkStdString returnString;
  SQLRETURN status = SQL_ERROR;
  SQLLEN lengthIndicator;

  // Make sure we've got room to store the results but don't go past 64k
  if (columnSize <= 0)
  {
    columnSize = 1024;
  }
  else if (columnSize > 65536)
  {
    columnSize = 65536;
  }
  else
  {
    // make room for the null terminator
    ++columnSize;
  }

  std::vector<char> buffer(columnSize);
  while (true)
  {
    status = SQLGetData(statement, column + 1, SQL_C_CHAR, static_cast<SQLPOINTER>(buffer.data()),
      columnSize, &lengthIndicator);
    if (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO)
    {
      if (lengthIndicator == SQL_NULL_DATA || lengthIndicator == SQL_NO_TOTAL)
      {
        break;
      }
      int resultSize = 0;
      if (status == SQL_SUCCESS_WITH_INFO)
      {
        // SQL_SUCCESS_WITH_INFO means that there's more data to
        // retrieve so we have to do it in chunks -- hence the while
        // loop.
        resultSize = columnSize - 1;
      }
      else
      {
        resultSize = lengthIndicator;
      }
      buffer[resultSize] = 0;
      returnString += buffer.data();
    }
    else if (status == SQL_NO_DATA)
    {
      // we're done
      break;
    }
    else
    {
      cerr << "odbcGetString: Error " << status << " in SQLGetData\n";

      break;
    }
  }

  return returnString;
}

// ----------------------------------------------------------------------------
svtkODBCDatabase::svtkODBCDatabase()
{
  this->Internals = new svtkODBCInternals;

  this->Tables = svtkStringArray::New();
  this->Tables->Register(this);
  this->Tables->Delete();
  this->LastErrorText = nullptr;

  this->Record = svtkStringArray::New();
  this->Record->Register(this);
  this->Record->Delete();

  this->UserName = nullptr;
  this->HostName = nullptr;
  this->DataSourceName = nullptr;
  this->DatabaseName = nullptr;
  this->Password = nullptr;

  this->ServerPort = -1; // use whatever the driver defaults to

  // Initialize instance variables
  this->DatabaseType = 0;
  this->SetDatabaseType("ODBC");
}

// ----------------------------------------------------------------------------
svtkODBCDatabase::~svtkODBCDatabase()
{
  if (this->IsOpen())
  {
    this->Close();
  }
  if (this->DatabaseType)
  {
    this->SetDatabaseType(0);
  }
  this->SetLastErrorText(nullptr);
  this->SetUserName(nullptr);
  this->SetHostName(nullptr);
  this->SetPassword(nullptr);
  this->SetDataSourceName(nullptr);
  this->SetDatabaseName(nullptr);
  delete this->Internals;

  this->Tables->UnRegister(this);
  this->Record->UnRegister(this);
}

// ----------------------------------------------------------------------------
bool svtkODBCDatabase::IsSupported(int feature)
{
  switch (feature)
  {
    case SVTK_SQL_FEATURE_BATCH_OPERATIONS:
    case SVTK_SQL_FEATURE_NAMED_PLACEHOLDERS:
      return false;

    case SVTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS:
      return true;

    case SVTK_SQL_FEATURE_PREPARED_QUERIES:
    {
      return true;
    }

    case SVTK_SQL_FEATURE_UNICODE:
      return false; // not until we have svtkStdWideString

    case SVTK_SQL_FEATURE_QUERY_SIZE:
    case SVTK_SQL_FEATURE_BLOB:
    case SVTK_SQL_FEATURE_LAST_INSERT_ID:
    case SVTK_SQL_FEATURE_TRANSACTIONS:
      return true;

    default:
    {
      svtkErrorMacro(<< "Unknown SQL feature code " << feature << "!  See "
                    << "svtkSQLDatabase.h for a list of possible features.");
      return false;
    };
  }
}

// ----------------------------------------------------------------------------
bool svtkODBCDatabase::Open(const char* password)
{
  if (!this->DataSourceName)
  {
    this->SetLastErrorText("Cannot open database because database ID is null.");
    svtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }

  if (this->IsOpen())
  {
    svtkGenericWarningMacro("Open(): Database is already open.");
    return true;
  }

  SQLRETURN status;
  status = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &(this->Internals->Environment));

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
  {
    // We don't actually have a valid SQL handle yet so I don't think
    // we can actually retrieve an error message.
    std::ostringstream sbuf;
    sbuf << "svtkODBCDatabase::Open: Unable to allocate environment handle.  "
         << "Return code " << status
         << ", error message: " << GetErrorMessage(SQL_HANDLE_ENV, this->Internals->Environment);
    this->SetLastErrorText(sbuf.str().c_str());
    return false;
  }
  else
  {
    svtkDebugMacro(<< "Successfully allocated environment handle.");
    status = SQLSetEnvAttr(this->Internals->Environment, SQL_ATTR_ODBC_VERSION,
      reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), SQL_IS_UINTEGER);
  }

  // Create the connection string itself
  svtkStdString connectionString;
  if (strstr(this->DataSourceName, ".dsn") != nullptr)
  {
    // the data source is a file of some sort
    connectionString = "FILEDSN=";
    connectionString += this->DataSourceName;
  }
  else if (strstr(this->DataSourceName, "DRIVER") != nullptr ||
    strstr(this->DataSourceName, "SERVER"))
  {
    connectionString = this->DataSourceName;
  }
  else
  {
    connectionString = "DSN=";
    connectionString += this->DataSourceName;
  }

  if (this->UserName != nullptr && strlen(this->UserName) > 0)
  {
    connectionString += ";UID=";
    connectionString += this->UserName;
  }
  if (password != nullptr)
  {
    connectionString += ";PWD=";
    connectionString += password;
  }
  if (this->DatabaseName != nullptr && strlen(this->DatabaseName) > 0)
  {
    connectionString += ";DATABASE=";
    connectionString += this->DatabaseName;
  }

  // Get a handle to connect with
  status =
    SQLAllocHandle(SQL_HANDLE_DBC, this->Internals->Environment, &(this->Internals->Connection));

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
  {
    std::ostringstream errbuf;
    errbuf << "Error allocating ODBC connection handle: "
           << GetErrorMessage(SQL_HANDLE_ENV, this->Internals->Environment);
    this->SetLastErrorText(errbuf.str().c_str());
    return false;
  }

  svtkDebugMacro(<< "ODBC connection handle successfully allocated");

#ifdef ODBC_DRIVER_IS_IODBC
  // Set the driver name so we know who to blame
  svtkStdString driverName("svtkODBCDatabase driver");
  status = SQLSetConnectAttr(
    this->Internals->Connection, SQL_APPLICATION_NAME, driverName.c_str(), driverName.size());
  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
  {
    std::ostringstream errbuf;
    errbuf << "Error setting driver name: "
           << GetErrorMessage(SQL_HANDLE_DBC, this->Internals->Connection);
    this->SetLastErrorText(errbuf.str().c_str());
    return false;
  }
  else
  {
    svtkDebugMacro(<< "Successfully set driver name on connect string.");
  }
#endif

  SQLSMALLINT cb;
  SQLTCHAR connectionOut[1024];
  status = SQLDriverConnect(this->Internals->Connection, nullptr,
    (SQLCHAR*)(const_cast<char*>(connectionString.c_str())),
    static_cast<SQLSMALLINT>(connectionString.size()), connectionOut, 1024, &cb,
    SQL_DRIVER_NOPROMPT);

  if (status != SQL_SUCCESS && status != SQL_SUCCESS_WITH_INFO)
  {
    std::ostringstream sbuf;
    sbuf << "svtkODBCDatabase::Open: Error during connection: "
         << GetErrorMessage(SQL_HANDLE_DBC, this->Internals->Connection);
    this->SetLastErrorText(sbuf.str().c_str());
    return false;
  }

  svtkDebugMacro(<< "Connection successful.");

  return true;
}

// ----------------------------------------------------------------------------
void svtkODBCDatabase::Close()
{
  if (!this->IsOpen())
  {
    return; // not an error
  }
  else
  {
    SQLRETURN status;

    if (this->Internals->Connection != SQL_NULL_HDBC)
    {
      status = SQLDisconnect(this->Internals->Connection);
      if (status != SQL_SUCCESS)
      {
        svtkWarningMacro(<< "ODBC Close: Unable to disconnect data source");
      }
      status = SQLFreeHandle(SQL_HANDLE_DBC, this->Internals->Connection);
      if (status != SQL_SUCCESS)
      {
        svtkWarningMacro(<< "ODBC Close: Unable to free connection handle");
      }
      this->Internals->Connection = nullptr;
    }

    if (this->Internals->Environment != SQL_NULL_HENV)
    {
      status = SQLFreeHandle(SQL_HANDLE_ENV, this->Internals->Environment);
      if (status != SQL_SUCCESS)
      {
        svtkWarningMacro(<< "ODBC Close: Unable to free environment handle");
      }
      this->Internals->Environment = nullptr;
    }
  }
}

// ----------------------------------------------------------------------------
bool svtkODBCDatabase::IsOpen()
{
  return (this->Internals->Connection != SQL_NULL_HDBC);
}

// ----------------------------------------------------------------------------
svtkSQLQuery* svtkODBCDatabase::GetQueryInstance()
{
  svtkODBCQuery* query = svtkODBCQuery::New();
  query->SetDatabase(this);
  return query;
}

// ----------------------------------------------------------------------------
const char* svtkODBCDatabase::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------------
svtkStringArray* svtkODBCDatabase::GetTables()
{
  this->Tables->Resize(0);
  if (!this->IsOpen())
  {
    svtkErrorMacro(<< "GetTables(): Database is closed!");
    return this->Tables;
  }
  else
  {
    SQLHANDLE statement;
    SQLRETURN status = SQLAllocHandle(SQL_HANDLE_STMT, this->Internals->Connection, &statement);

    if (status != SQL_SUCCESS)
    {
      svtkErrorMacro(<< "svtkODBCDatabase::GetTables: Unable to allocate statement");

      return this->Tables;
    }

    status = SQLSetStmtAttr(statement, SQL_ATTR_CURSOR_TYPE,
      static_cast<SQLPOINTER>(SQL_CURSOR_FORWARD_ONLY), SQL_IS_UINTEGER);

    svtkStdString tableType("TABLE,");

    status = SQLTables(statement, nullptr, 0, nullptr, 0, nullptr, 0,
      (SQLCHAR*)(const_cast<char*>(tableType.c_str())), static_cast<SQLSMALLINT>(tableType.size()));

    if (status != SQL_SUCCESS)
    {
      svtkErrorMacro(<< "svtkODBCDatabase::GetTables: Unable to execute table list");
      return this->Tables;
    }

    status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
    while (status == SQL_SUCCESS)
    {
      svtkStdString fieldVal = odbcGetString(statement, 2, -1);
      this->Tables->InsertNextValue(fieldVal);
      status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
    }

    status = SQLFreeHandle(SQL_HANDLE_STMT, statement);
    if (status != SQL_SUCCESS)
    {
      svtkErrorMacro(<< "svtkODBCDatabase::GetTables: Unable to free statement handle.  Error "
                    << status);
    }
    return this->Tables;
  }
}

// ----------------------------------------------------------------------------
svtkStringArray* svtkODBCDatabase::GetRecord(const char* table)
{
  this->Record->Reset();
  this->Record->Allocate(20);

  if (!this->IsOpen())
  {
    svtkErrorMacro(<< "GetRecord: Database is not open!");
    return this->Record;
  }

  SQLHANDLE statement;
  SQLRETURN status = SQLAllocHandle(SQL_HANDLE_STMT, this->Internals->Connection, &statement);
  if (status != SQL_SUCCESS)
  {
    svtkErrorMacro(<< "svtkODBCDatabase: Unable to allocate statement: error " << status);
    return this->Record;
  }

  status = SQLSetStmtAttr(
    statement, SQL_ATTR_METADATA_ID, reinterpret_cast<SQLPOINTER>(SQL_TRUE), SQL_IS_INTEGER);

  if (status != SQL_SUCCESS)
  {
    svtkErrorMacro(<< "svtkODBCDatabase::GetRecord: Unable to set SQL_ATTR_METADATA_ID attribute on "
                     "query.  Return code: "
                  << status);
    return nullptr;
  }

  status = SQLSetStmtAttr(statement, SQL_ATTR_CURSOR_TYPE,
    static_cast<SQLPOINTER>(SQL_CURSOR_FORWARD_ONLY), SQL_IS_UINTEGER);

  status = SQLColumns(statement,
    nullptr, // catalog
    0,
    nullptr, // schema
    0, (SQLCHAR*)(const_cast<char*>(table)), static_cast<SQLSMALLINT>(strlen(table)),
    nullptr, // column
    0);

  if (status != SQL_SUCCESS && status != 0)
  {
    svtkStdString error = GetErrorMessage(SQL_HANDLE_STMT, statement);

    svtkErrorMacro(
      << "svtkODBCDatabase::GetRecord: Unable to retrieve column list (SQLColumns): error "
      << error.c_str());
    this->SetLastErrorText(error.c_str());
    SQLFreeHandle(SQL_HANDLE_STMT, statement);
    return this->Record;
  }

  status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
  if (status != SQL_SUCCESS)
  {
    svtkStdString error = GetErrorMessage(SQL_HANDLE_STMT, statement);
    svtkErrorMacro(
      << "svtkODBCDatabase::GetRecord: Unable to retrieve column list (SQLFetchScroll): error "
      << error.c_str());
    this->SetLastErrorText(error.c_str());
    SQLFreeHandle(SQL_HANDLE_STMT, statement);
    return this->Record;
  }
  while (status == SQL_SUCCESS)
  {
    svtkStdString fieldName = odbcGetString(statement, 3, -1);
    this->Record->InsertNextValue(fieldName);
    status = SQLFetchScroll(statement, SQL_FETCH_NEXT, 0);
  }

  status = SQLFreeHandle(SQL_HANDLE_STMT, statement);
  if (status != SQL_SUCCESS)
  {
    svtkErrorMacro("svtkODBCDatabase: Unable to free statement handle: error " << status);
  }

  return this->Record;
}

// ----------------------------------------------------------------------------
void svtkODBCDatabase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DataSourceName: ";
  if (this->DataSourceName == 0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->DataSourceName << endl;
  }

  os << indent << "DatabaseName: ";
  if (this->DatabaseName == 0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->DatabaseName << endl;
  }

  os << indent << "UserName: ";
  if (this->UserName == 0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->UserName << endl;
  }
  os << indent << "HostName: ";
  if (this->HostName == 0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->HostName << endl;
  }
  os << indent << "Password: ";
  if (this->Password == 0)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << "not displayed for security reason." << endl;
  }
  os << indent << "ServerPort: " << this->ServerPort << endl;

  os << indent << "DatabaseType: " << (this->DatabaseType ? this->DatabaseType : "nullptr") << endl;
}

// ----------------------------------------------------------------------------
bool svtkODBCDatabase::HasError()
{
  return this->LastErrorText != nullptr;
}

// ----------------------------------------------------------------------------
svtkStdString svtkODBCDatabase::GetURL()
{
  return svtkStdString("GetURL on ODBC databases is not yet implemented");
}

// ----------------------------------------------------------------------------
bool svtkODBCDatabase::ParseURL(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol;
  std::string username;
  std::string unused;
  std::string dsname;
  std::string dataport;
  std::string database;

  // Okay now for all the other database types get more detailed info
  if (!svtksys::SystemTools::ParseURL(
        urlstr, protocol, username, unused, dsname, dataport, database))
  {
    svtkErrorMacro("Invalid URL: \"" << urlstr.c_str() << "\"");
    return false;
  }

  if (protocol == "odbc")
  {
    this->SetUserName(username.c_str());
    this->SetServerPort(atoi(dataport.c_str()));
    this->SetDatabaseName(database.c_str());
    this->SetDataSourceName(dsname.c_str());
    return true;
  }

  return false;
}

// ----------------------------------------------------------------------------
svtkStdString svtkODBCDatabase::GetColumnSpecification(
  svtkSQLDatabaseSchema* schema, int tblHandle, int colHandle)
{
  std::ostringstream queryStr;
  queryStr << schema->GetColumnNameFromHandle(tblHandle, colHandle) << " ";

  // Figure out column type
  int colType = schema->GetColumnTypeFromHandle(tblHandle, colHandle);
  svtkStdString colTypeStr;

  switch (static_cast<svtkSQLDatabaseSchema::DatabaseColumnType>(colType))
  {
    case svtkSQLDatabaseSchema::SERIAL:
      colTypeStr = "INTEGER NOT nullptr";
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

// ----------------------------------------------------------------------------
svtkStdString svtkODBCDatabase::GetIndexSpecification(
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
    queryStr += schema->GetIndexColumnNameFromHandle(tblHandle, idxHandle, cnmHandle);
  }
  queryStr += ")";

  return queryStr;
}

// ----------------------------------------------------------------------------
bool svtkODBCDatabase::CreateDatabase(const char* dbName, bool dropExisting = false)
{
  if (dropExisting)
  {
    this->DropDatabase(dbName);
  }
  svtkStdString queryStr;
  queryStr = "CREATE DATABASE ";
  queryStr += dbName;
  svtkSQLQuery* query = this->GetQueryInstance();
  query->SetQuery(queryStr.c_str());
  bool status = query->Execute();
  query->Delete();
  // Close and re-open in case we deleted and recreated the current database
  this->Close();
  this->Open(this->Password);
  return status;
}

// ----------------------------------------------------------------------------
bool svtkODBCDatabase::DropDatabase(const char* dbName)
{
  svtkStdString queryStr;
  queryStr = "DROP DATABASE ";
  queryStr += dbName;
  svtkSQLQuery* query = this->GetQueryInstance();
  query->SetQuery(queryStr.c_str());
  bool status = query->Execute();
  query->Delete();
  return status;
}
