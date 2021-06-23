/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLiteQuery.cxx

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
#include "svtkSQLiteQuery.h"

#include "svtkObjectFactory.h"
#include "svtkSQLiteDatabase.h"
#include "svtkSQLiteDatabaseInternals.h"
#include "svtkStringArray.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

#include "svtk_sqlite.h"

#include <cassert>

#include <sstream>
#include <vector>

#define BEGIN_TRANSACTION "BEGIN TRANSACTION"
#define COMMIT_TRANSACTION "COMMIT"
#define ROLLBACK_TRANSACTION "ROLLBACK"

class svtkSQLiteQuery::Priv
{
public:
  sqlite3_stmt* Statement;
};

svtkStandardNewMacro(svtkSQLiteQuery);

// ----------------------------------------------------------------------
svtkSQLiteQuery::svtkSQLiteQuery()
{
  this->Private = new Priv;
  this->Private->Statement = nullptr;
  this->InitialFetch = true;
  this->InitialFetchResult = SQLITE_DONE;
  this->LastErrorText = nullptr;
  this->TransactionInProgress = false;
}

// ----------------------------------------------------------------------
svtkSQLiteQuery::~svtkSQLiteQuery()
{
  this->SetLastErrorText(nullptr);
  if (this->TransactionInProgress)
  {
    this->RollbackTransaction();
  }

  if (this->Private->Statement != nullptr)
  {
    if (this->Database != nullptr)
    {
      sqlite3_finalize(this->Private->Statement);
      this->Private->Statement = nullptr;
    }
  }
  delete this->Private;
}

// ----------------------------------------------------------------------
void svtkSQLiteQuery::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Statement: ";
  if (this->Private->Statement)
  {
    os << this->Private->Statement << "\n";
  }
  else
  {
    os << "(null)"
       << "\n";
  }
  os << indent << "InitialFetch: " << this->InitialFetch << "\n";
  os << indent << "InitialFetchResult: " << this->InitialFetchResult << "\n";
  os << indent << "TransactionInProgress: " << this->TransactionInProgress << "\n";
  os << indent << "LastErrorText: " << (this->LastErrorText ? this->LastErrorText : "(null)")
     << endl;
}

// ----------------------------------------------------------------------
bool svtkSQLiteQuery::SetQuery(const char* newQuery)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Query to "
                << (newQuery ? newQuery : "(null)"));

  if (this->Query == nullptr && newQuery == nullptr)
  {
    return true;
  }

  if (this->Query && newQuery && (!strcmp(this->Query, newQuery)))
  {
    return true; // we've already got that query
  }

  delete[] this->Query;

  if (newQuery)
  {
    // Keep a local copy of the query - this is from svtkSetGet.h
    size_t n = strlen(newQuery) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (newQuery);
    this->Query = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
  else
  {
    this->Query = nullptr;
  }

  // If we get to this point the query has changed.  We need to
  // finalize the already-prepared statement if one exists and then
  // prepare a new statement.
  if (this->Private->Statement)
  {
    svtkDebugMacro(<< "Finalizing old statement");
    int finalizeStatus = sqlite3_finalize(this->Private->Statement);
    if (finalizeStatus != SQLITE_OK)
    {
      svtkWarningMacro(<< "SetQuery(): Finalize returned unexpected code " << finalizeStatus);
    }
    this->Private->Statement = nullptr;
  }

  if (this->Query)
  {
    svtkSQLiteDatabase* dbContainer = svtkSQLiteDatabase::SafeDownCast(this->Database);

    if (dbContainer == nullptr)
    {
      svtkErrorMacro(<< "This should never happen: SetQuery() called when there is no underlying "
                       "database.  You probably instantiated svtkSQLiteQuery directly instead of "
                       "calling svtkSQLDatabase::GetInstance().  This also happens during "
                       "TestSetGet in the CDash testing.");
      return false;
    }

    sqlite3* db = dbContainer->Internal->SQLiteInstance;
    const char* unused_statement;

    int prepareStatus = sqlite3_prepare_v2(db, this->Query, static_cast<int>(strlen(this->Query)),
      &this->Private->Statement, &unused_statement);

    if (prepareStatus != SQLITE_OK)
    {
      this->SetLastErrorText(sqlite3_errmsg(db));
      svtkWarningMacro(<< "SetQuery(): sqlite3_prepare_v2() failed with error message "
                      << this->GetLastErrorText() << " on statement: '" << this->Query << "'");
      this->Active = false;
      return false;
    }
  } // Done preparing new statement

  this->Modified();
  return true;
}

// ----------------------------------------------------------------------
bool svtkSQLiteQuery::Execute()
{

  if (this->Query == nullptr)
  {
    svtkErrorMacro(<< "Cannot execute before a query has been set.");
    return false;
  }

  if (this->Private->Statement == nullptr)
  {
    svtkErrorMacro(<< "Execute(): Query is not null but prepared statement is.  There may have been "
                     "an error during SetQuery().");
    this->Active = false;
    return false;
  }
  else
  {
    sqlite3_reset(this->Private->Statement);
  }

  svtkDebugMacro(<< "Execute(): Query ready to execute.");

  this->InitialFetch = true;
  int result = sqlite3_step(this->Private->Statement);
  this->InitialFetchResult = result;

  if (result == SQLITE_DONE)
  {
    this->SetLastErrorText(nullptr);
    this->Active = true;
    return true;
  }
  else if (result != SQLITE_ROW)
  {
    svtkSQLiteDatabase* dbContainer = svtkSQLiteDatabase::SafeDownCast(this->Database);
    assert(dbContainer != nullptr);

    sqlite3* db = dbContainer->Internal->SQLiteInstance;

    this->SetLastErrorText(sqlite3_errmsg(db));
    svtkDebugMacro(<< "Execute(): sqlite3_step() returned error message "
                  << this->GetLastErrorText());
    this->Active = false;
    return false;
  }

  this->SetLastErrorText(nullptr);
  this->Active = true;
  return true;
}

// ----------------------------------------------------------------------
int svtkSQLiteQuery::GetNumberOfFields()
{
  if (!this->Active)
  {
    svtkErrorMacro(<< "GetNumberOfFields(): Query is not active!");
    return 0;
  }
  else
  {
    return sqlite3_column_count(this->Private->Statement);
  }
}

// ----------------------------------------------------------------------
const char* svtkSQLiteQuery::GetFieldName(int column)
{
  if (!this->Active)
  {
    svtkErrorMacro(<< "GetFieldName(): Query is not active!");
    return nullptr;
  }
  else if (column < 0 || column >= this->GetNumberOfFields())
  {
    svtkErrorMacro(<< "GetFieldName(): Illegal field index " << column);
    return nullptr;
  }
  else
  {
    return sqlite3_column_name(this->Private->Statement, column);
  }
}

// ----------------------------------------------------------------------
int svtkSQLiteQuery::GetFieldType(int column)
{
  if (!this->Active)
  {
    svtkErrorMacro(<< "GetFieldType(): Query is not active!");
    return -1;
  }
  else if (column < 0 || column >= this->GetNumberOfFields())
  {
    svtkErrorMacro(<< "GetFieldType(): Illegal field index " << column);
    return -1;
  }
  else
  {
    switch (sqlite3_column_type(this->Private->Statement, column))
    {
      case SQLITE_INTEGER:
        return SVTK_INT;
      case SQLITE_FLOAT:
        return SVTK_FLOAT;
      case SQLITE_TEXT:
        return SVTK_STRING;
      case SQLITE_BLOB:
        return SVTK_STRING; // until we have a BLOB type of our own
      case SQLITE_NULL:
        return SVTK_VOID; // ??? what makes sense here?
      default:
      {
        svtkErrorMacro(<< "GetFieldType(): Unknown data type "
                      << sqlite3_column_type(this->Private->Statement, column) << " from SQLite.");
        return SVTK_VOID;
      }
    }
  }
}

// ----------------------------------------------------------------------
bool svtkSQLiteQuery::NextRow()
{
  if (!this->IsActive())
  {
    svtkErrorMacro(<< "NextRow(): Query is not active!");
    return false;
  }

  if (this->InitialFetch)
  {
    svtkDebugMacro(<< "NextRow(): Initial fetch being handled.");
    this->InitialFetch = false;
    if (this->InitialFetchResult == SQLITE_DONE)
    {
      return false;
    }
    else
    {
      return true;
    }
  }
  else
  {
    int result = sqlite3_step(this->Private->Statement);
    if (result == SQLITE_DONE)
    {
      return false;
    }
    else if (result == SQLITE_ROW)
    {
      return true;
    }
    else
    {
      svtkSQLiteDatabase* dbContainer = svtkSQLiteDatabase::SafeDownCast(this->Database);
      assert(dbContainer != nullptr);
      sqlite3* db = dbContainer->Internal->SQLiteInstance;
      this->SetLastErrorText(sqlite3_errmsg(db));
      svtkErrorMacro(<< "NextRow(): Database returned error code " << result
                    << " with the following message: " << this->GetLastErrorText());
      this->Active = false;
      return false;
    }
  }
}

// ----------------------------------------------------------------------
svtkVariant svtkSQLiteQuery::DataValue(svtkIdType column)
{
  if (this->IsActive() == false)
  {
    svtkWarningMacro(<< "DataValue() called on inactive query");
    return svtkVariant();
  }
  else if (column < 0 || column >= this->GetNumberOfFields())
  {
    svtkWarningMacro(<< "DataValue() called with out-of-range column index " << column);
    return svtkVariant();
  }
  else
  {
    switch (sqlite3_column_type(this->Private->Statement, column))
    {
      case SQLITE_INTEGER:
        return svtkVariant(sqlite3_column_int(this->Private->Statement, column));

      case SQLITE_FLOAT:
        return svtkVariant(sqlite3_column_double(this->Private->Statement, column));

      case SQLITE_TEXT:
      {
        std::ostringstream str;
        str << sqlite3_column_text(this->Private->Statement, column);
        return svtkVariant(svtkStdString(str.str()));
      }

      case SQLITE_BLOB:
      {
        // This is a hack ... by passing the BLOB to svtkStdString with an explicit
        // byte count, we ensure that the string will store all of the BLOB's bytes,
        // even if there are nullptr values.

        return svtkVariant(svtkStdString(
          static_cast<const char*>(sqlite3_column_blob(this->Private->Statement, column)),
          sqlite3_column_bytes(this->Private->Statement, column)));
      }

      case SQLITE_NULL:
      default:
        return svtkVariant();
    }
  }
}

// ----------------------------------------------------------------------
const char* svtkSQLiteQuery::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------
bool svtkSQLiteQuery::HasError()
{
  return (this->GetLastErrorText() != nullptr);
}

// ----------------------------------------------------------------------
bool svtkSQLiteQuery::BeginTransaction()
{
  if (this->TransactionInProgress)
  {
    svtkErrorMacro(<< "Cannot start a transaction.  One is already in progress.");
    return false;
  }

  svtkSQLiteDatabase* dbContainer = svtkSQLiteDatabase::SafeDownCast(this->Database);
  assert(dbContainer != nullptr);

  sqlite3* db = dbContainer->Internal->SQLiteInstance;
  char* errorMessage = nullptr;
  int result = sqlite3_exec(db, BEGIN_TRANSACTION, nullptr, nullptr, &errorMessage);

  if (result == SQLITE_OK)
  {
    this->TransactionInProgress = true;
    this->SetLastErrorText(nullptr);
    svtkDebugMacro(<< "BeginTransaction() succeeded.");
    return true;
  }
  else
  {
    svtkErrorMacro(<< "BeginTransaction(): sqlite3_exec returned unexpected result code " << result);
    if (errorMessage)
    {
      svtkErrorMacro(<< " and error message " << errorMessage);
    }
    this->TransactionInProgress = false;
    return false;
  }
}

// ----------------------------------------------------------------------
bool svtkSQLiteQuery::CommitTransaction()
{
  if (this->Private->Statement)
  {
    sqlite3_finalize(this->Private->Statement);
    this->Private->Statement = nullptr;
  }

  if (!this->TransactionInProgress)
  {
    svtkErrorMacro(<< "Cannot commit.  There is no transaction in progress.");
    return false;
  }

  svtkSQLiteDatabase* dbContainer = svtkSQLiteDatabase::SafeDownCast(this->Database);
  assert(dbContainer != nullptr);
  sqlite3* db = dbContainer->Internal->SQLiteInstance;
  char* errorMessage = nullptr;
  int result = sqlite3_exec(db, COMMIT_TRANSACTION, nullptr, nullptr, &errorMessage);

  if (result == SQLITE_OK)
  {
    this->TransactionInProgress = false;
    this->SetLastErrorText(nullptr);
    svtkDebugMacro(<< "CommitTransaction() succeeded.");
    return true;
  }
  else
  {
    svtkErrorMacro(<< "CommitTransaction(): sqlite3_exec returned unexpected result code "
                  << result);
    if (errorMessage)
    {
      this->SetLastErrorText(errorMessage);
      svtkErrorMacro(<< " and error message " << errorMessage);
    }
    assert(1 == 0);
    return false;
  }
}

// ----------------------------------------------------------------------
bool svtkSQLiteQuery::RollbackTransaction()
{
  if (!this->TransactionInProgress)
  {
    svtkErrorMacro(<< "Cannot rollback.  There is no transaction in progress.");
    return false;
  }

  svtkSQLiteDatabase* dbContainer = svtkSQLiteDatabase::SafeDownCast(this->Database);
  assert(dbContainer != nullptr);
  sqlite3* db = dbContainer->Internal->SQLiteInstance;
  char* errorMessage = nullptr;
  int result = sqlite3_exec(db, ROLLBACK_TRANSACTION, nullptr, nullptr, &errorMessage);

  if (result == SQLITE_OK)
  {
    this->TransactionInProgress = false;
    this->SetLastErrorText(nullptr);
    svtkDebugMacro(<< "RollbackTransaction() succeeded.");
    return true;
  }
  else
  {
    svtkErrorMacro(<< "RollbackTransaction(): sqlite3_exec returned unexpected result code "
                  << result);
    if (errorMessage)
    {
      this->SetLastErrorText(errorMessage);
      svtkErrorMacro(<< " and error message " << errorMessage);
    }
    return false;
  }
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, unsigned char value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, signed char value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, unsigned short value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, short value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, unsigned int value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, int value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, unsigned long value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, long value)
{
  return this->BindIntegerParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, unsigned long long value)
{
  return this->BindInt64Parameter(index, static_cast<svtkTypeInt64>(value));
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, long long value)
{
  return this->BindInt64Parameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, float value)
{
  return this->BindDoubleParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, double value)
{
  return this->BindDoubleParameter(index, value);
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, const char* value)
{
  return this->BindParameter(index, value, strlen(value));
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, const char* data, size_t length)
{
  return this->BindStringParameter(index, data, static_cast<int>(length));
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, const svtkStdString& value)
{
  return this->BindParameter(index, value.c_str());
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, const void* data, size_t length)
{
  return this->BindBlobParameter(index, data, static_cast<int>(length));
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindIntegerParameter(int index, int value)
{
  if (!this->Private->Statement)
  {
    svtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }
  int status = sqlite3_bind_int(this->Private->Statement, index + 1, value);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_int returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    svtkErrorMacro(<< errormessage.str().c_str());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindInt64Parameter(int index, svtkTypeInt64 value)
{
  if (!this->Private->Statement)
  {
    svtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }
  int status =
    sqlite3_bind_int(this->Private->Statement, index + 1, static_cast<sqlite_int64>(value));

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_int64 returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    svtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindDoubleParameter(int index, double value)
{
  if (!this->Private->Statement)
  {
    svtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }

  int status = sqlite3_bind_double(this->Private->Statement, index + 1, value);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_double returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    svtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindStringParameter(int index, const char* value, int length)
{
  if (!this->Private->Statement)
  {
    svtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }

  int status =
    sqlite3_bind_text(this->Private->Statement, index + 1, value, length, SQLITE_TRANSIENT);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_text returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    svtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindBlobParameter(int index, const void* data, int length)
{
  if (!this->Private->Statement)
  {
    svtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }

  int status =
    sqlite3_bind_blob(this->Private->Statement, index + 1, data, length, SQLITE_TRANSIENT);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_bind_blob returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    svtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::ClearParameterBindings()
{
  if (!this->Private->Statement)
  {
    svtkErrorMacro(<< "No statement available.  Did you forget to call SetQuery?");
    return false;
  }

  if (this->Active)
  {
    this->Active = false;
    sqlite3_reset(this->Private->Statement);
  }

  int status = sqlite3_clear_bindings(this->Private->Statement);

  if (status != SQLITE_OK)
  {
    std::ostringstream errormessage;
    errormessage << "sqlite_clear_bindings returned error: " << status;
    this->SetLastErrorText(errormessage.str().c_str());
    svtkErrorMacro(<< this->GetLastErrorText());
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------

bool svtkSQLiteQuery::BindParameter(int index, svtkVariant value)
{
  return this->Superclass::BindParameter(index, value);
}
