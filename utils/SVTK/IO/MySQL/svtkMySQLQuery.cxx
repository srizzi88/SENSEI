/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMySQLQuery.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMySQLQuery.h"
#include "svtkMySQLDatabase.h"
#include "svtkMySQLDatabasePrivate.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

#include <errmsg.h>
#include <mysql.h>

#if defined(_WIN32)
#include <locale.h>
#include <string.h>
#define LOWERCASE_COMPARE _stricmp
#else
#include <strings.h>
#define LOWERCASE_COMPARE strcasecmp
#endif

#include <cassert>

#include <sstream>
#include <vector>

/*
 * Bound Parameters and MySQL
 *
 * MySQL handles prepared statements using a MYSQL_STMT struct.
 * Parameters are bound to placeholders using a MYSQL_BIND struct.
 * The MYSQL_BIND contains a pointer to a data buffer.  That buffer
 * needs to be freed somehow when it's no longer needed.
 *
 * I'm going to handle this by using my own class
 * (svtkMySQLBoundParameter) to hold all the information the user
 * passes in.  At execution time, I'll take those parameters and
 * assemble the array of MYSQL_BIND objects that
 * mysql_stmt_bind_param() expects.  The svtkMySQLBoundParameter
 * instances will each own the buffers for their data.
 *
 * This is slightly inefficient in that it will generate
 * a few tiny little new[] requests.  If this ever becomes a problem,
 * we can allocate a fixed-size buffer (8 or 16 bytes) inside
 * svtkMySQLBoundParameter and use that for the data storage by
 * default.  That will still require special-case handling for blobs
 * and strings.
 *
 * The svtkMySQLQueryInternals class will handle the bookkeeping for
 * which parameters are and aren't bound at any given time.
 */

// ----------------------------------------------------------------------

class svtkMySQLBoundParameter
{
public:
  svtkMySQLBoundParameter()
    : IsNull(true)
    , Data(nullptr)
    , BufferSize(0)
    , DataLength(0)
    , HasError(false)
  {
  }

  ~svtkMySQLBoundParameter() { delete[] this->Data; }

  void SetData(char* data, unsigned long size)
  {
    delete[] this->Data;
    this->BufferSize = size;
    this->Data = new char[size];
    memcpy(this->Data, data, size);
  }

  MYSQL_BIND BuildParameterStruct()
  {
    MYSQL_BIND output;
    output.buffer_type = this->DataType;
    output.buffer = this->Data;
    output.buffer_length = this->BufferSize;
    output.length = &(this->DataLength);
    output.is_null = &(this->IsNull);
    output.is_unsigned = this->IsUnsigned;
    output.error = nullptr;
    return output;
  }

public:
  my_bool IsNull;                 // Is this parameter nullptr?
  my_bool IsUnsigned;             // For integer types, is it unsigned?
  char* Data;                     // Buffer holding actual data
  unsigned long BufferSize;       // Buffer size
  unsigned long DataLength;       // Size of the data in the buffer (must
                                  // be less than or equal to BufferSize)
  my_bool HasError;               // for the server to report truncation
  enum enum_field_types DataType; // MySQL data type for the contained data
};

// ----------------------------------------------------------------------

MYSQL_BIND BuildNullParameterStruct()
{
  MYSQL_BIND output;
  output.buffer_type = MYSQL_TYPE_NULL;
  return output;
}

// ----------------------------------------------------------------------

#define SVTK_MYSQL_TYPENAME_MACRO(type, return_type)                                                \
  enum enum_field_types svtkMySQLTypeName(type) { return return_type; }

SVTK_MYSQL_TYPENAME_MACRO(signed char, MYSQL_TYPE_TINY);
SVTK_MYSQL_TYPENAME_MACRO(unsigned char, MYSQL_TYPE_TINY);
SVTK_MYSQL_TYPENAME_MACRO(signed short, MYSQL_TYPE_SHORT);
SVTK_MYSQL_TYPENAME_MACRO(unsigned short, MYSQL_TYPE_SHORT);
SVTK_MYSQL_TYPENAME_MACRO(signed int, MYSQL_TYPE_LONG);
SVTK_MYSQL_TYPENAME_MACRO(unsigned int, MYSQL_TYPE_LONG);
SVTK_MYSQL_TYPENAME_MACRO(signed long, MYSQL_TYPE_LONG);
SVTK_MYSQL_TYPENAME_MACRO(unsigned long, MYSQL_TYPE_LONG);
SVTK_MYSQL_TYPENAME_MACRO(float, MYSQL_TYPE_FLOAT);
SVTK_MYSQL_TYPENAME_MACRO(double, MYSQL_TYPE_DOUBLE);
SVTK_MYSQL_TYPENAME_MACRO(long long, MYSQL_TYPE_LONGLONG);
SVTK_MYSQL_TYPENAME_MACRO(unsigned long long, MYSQL_TYPE_LONGLONG);
SVTK_MYSQL_TYPENAME_MACRO(const char*, MYSQL_TYPE_STRING);
SVTK_MYSQL_TYPENAME_MACRO(char*, MYSQL_TYPE_STRING);
SVTK_MYSQL_TYPENAME_MACRO(void*, MYSQL_TYPE_BLOB);

template <typename T>
bool svtkMySQLIsTypeUnsigned(T)
{
  return false;
}

template <>
bool svtkMySQLIsTypeUnsigned<unsigned char>(unsigned char)
{
  return true;
}

template <>
bool svtkMySQLIsTypeUnsigned<unsigned short>(unsigned short)
{
  return true;
}

template <>
bool svtkMySQLIsTypeUnsigned<unsigned int>(unsigned int)
{
  return true;
}

template <>
bool svtkMySQLIsTypeUnsigned<unsigned long>(unsigned long)
{
  return true;
}

template <>
bool svtkMySQLIsTypeUnsigned<unsigned long long>(unsigned long long)
{
  return true;
}

// ----------------------------------------------------------------------

// Description:
// This function will build and populate a svtkMySQLBoundParameter
// struct.  The default implementation works for POD data types (char,
// int, long, etc).  I'll need to special-case strings and blobs.

template <typename T>
svtkMySQLBoundParameter* svtkBuildBoundParameter(T data_value)
{
  svtkMySQLBoundParameter* param = new svtkMySQLBoundParameter;

  param->IsNull = false;
  param->IsUnsigned = svtkMySQLIsTypeUnsigned(data_value);
  param->DataType = svtkMySQLTypeName(data_value);
  param->BufferSize = sizeof(T);
  param->DataLength = sizeof(T);
  param->SetData(new char[sizeof(T)], sizeof(T));
  memcpy(param->Data, &data_value, sizeof(T));

  return param;
}

// Description:
// Specialization of svtkBuildBoundParameter for nullptr-terminated
// strings (i.e. CHAR and VARCHAR fields)

template <>
svtkMySQLBoundParameter* svtkBuildBoundParameter<const char*>(const char* data_value)
{
  svtkMySQLBoundParameter* param = new svtkMySQLBoundParameter;

  param->IsNull = false;
  param->IsUnsigned = false;
  param->DataType = MYSQL_TYPE_STRING;
  param->BufferSize = strlen(data_value);
  param->DataLength = strlen(data_value);
  param->SetData(new char[param->BufferSize], param->BufferSize);
  memcpy(param->Data, data_value, param->BufferSize);

  return param;
}

// Description:
// Alternate signature for svtkBuildBoundParameter to handle blobs

svtkMySQLBoundParameter* svtkBuildBoundParameter(const char* data, unsigned long length, bool is_blob)
{
  svtkMySQLBoundParameter* param = new svtkMySQLBoundParameter;

  param->IsNull = false;
  param->IsUnsigned = false;
  param->BufferSize = length;
  param->DataLength = length;
  param->SetData(new char[length], length);
  memcpy(param->Data, data, param->BufferSize);

  if (is_blob)
  {
    param->DataType = MYSQL_TYPE_BLOB;
  }
  else
  {
    param->DataType = MYSQL_TYPE_STRING;
  }

  return param;
}

// ----------------------------------------------------------------------

class svtkMySQLQueryInternals
{
public:
  svtkMySQLQueryInternals();
  ~svtkMySQLQueryInternals();

  void FreeResult();
  void FreeStatement();
  void FreeUserParameterList();
  void FreeBoundParameters();
  bool SetQuery(const char* queryString, MYSQL* db, svtkStdString& error_message);
  bool SetBoundParameter(int index, svtkMySQLBoundParameter* param);
  bool BindParametersToStatement();

  // Description:
  // MySQL can only handle certain statements as prepared statements:
  // CALL, CREATE TABLE, DELETE, DO, INSERT, REPLACE, SELECT, SET,
  // UPDATE and some SHOW statements.  This function checks for those.
  // If we can't use a prepared statement then we have to do the query
  // the old-fashioned way.
  bool ValidPreparedStatementSQL(const char* query);

public:
  MYSQL_STMT* Statement;
  MYSQL_RES* Result;
  MYSQL_BIND* BoundParameters;
  MYSQL_ROW CurrentRow;
  unsigned long* CurrentLengths;

  typedef std::vector<svtkMySQLBoundParameter*> ParameterList;
  ParameterList UserParameterList;
};

// ----------------------------------------------------------------------

svtkMySQLQueryInternals::svtkMySQLQueryInternals()
  : Statement(nullptr)
  , Result(nullptr)
  , BoundParameters(nullptr)
  , CurrentLengths(nullptr)
{
}

// ----------------------------------------------------------------------

svtkMySQLQueryInternals::~svtkMySQLQueryInternals()
{
  this->FreeResult();
  this->FreeStatement();
  this->FreeUserParameterList();
  this->FreeBoundParameters();
}

// ----------------------------------------------------------------------

void svtkMySQLQueryInternals::FreeResult()
{
  if (this->Result)
  {
    mysql_free_result(this->Result);
    this->Result = nullptr;
  }
}

// ----------------------------------------------------------------------

void svtkMySQLQueryInternals::FreeStatement()
{
  if (this->Statement)
  {
    mysql_stmt_close(this->Statement);
    this->Statement = nullptr;
  }
}

// ----------------------------------------------------------------------

bool svtkMySQLQueryInternals::SetQuery(
  const char* queryString, MYSQL* db, svtkStdString& error_message)
{
  this->FreeStatement();
  this->FreeUserParameterList();
  this->FreeBoundParameters();

  if (this->ValidPreparedStatementSQL(queryString) == false)
  {
    return true; // we'll have to handle this query in immediate mode
  }

  this->Statement = mysql_stmt_init(db);
  if (this->Statement == nullptr)
  {
    error_message = svtkStdString("svtkMySQLQuery: mysql_stmt_init returned out of memory error");
    return false;
  }

  int status = mysql_stmt_prepare(this->Statement, queryString, strlen(queryString));

  if (status == 0)
  {
    this->UserParameterList.resize(mysql_stmt_param_count(this->Statement), nullptr);
    return true;
  }
  else
  {
    error_message = svtkStdString(mysql_stmt_error(this->Statement));
    return false;
  }
}

// ----------------------------------------------------------------------

void svtkMySQLQueryInternals::FreeUserParameterList()
{
  for (unsigned int i = 0; i < this->UserParameterList.size(); ++i)
  {
    delete this->UserParameterList[i];
    this->UserParameterList[i] = nullptr;
  }
  this->UserParameterList.clear();
}

// ----------------------------------------------------------------------

void svtkMySQLQueryInternals::FreeBoundParameters()
{
  delete[] this->BoundParameters;
}

// ----------------------------------------------------------------------

bool svtkMySQLQueryInternals::SetBoundParameter(int index, svtkMySQLBoundParameter* param)
{
  if (index >= static_cast<int>(this->UserParameterList.size()))
  {
    svtkGenericWarningMacro(<< "ERROR: Illegal parameter index " << index
                           << ".  Did you forget to set the query?");
    return false;
  }
  else
  {
    delete this->UserParameterList[index];
    this->UserParameterList[index] = param;
    return true;
  }
}

// ----------------------------------------------------------------------

bool svtkMySQLQueryInternals::BindParametersToStatement()
{
  if (this->Statement == nullptr)
  {
    svtkGenericWarningMacro(<< "BindParametersToStatement: No prepared statement available");
    return false;
  }

  this->FreeBoundParameters();
  unsigned long numParams = mysql_stmt_param_count(this->Statement);
  this->BoundParameters = new MYSQL_BIND[numParams];
  for (unsigned int i = 0; i < numParams; ++i)
  {
    if (this->UserParameterList[i])
    {
      this->BoundParameters[i] = this->UserParameterList[i]->BuildParameterStruct();
    }
    else
    {
      this->BoundParameters[i] = BuildNullParameterStruct();
    }
  }

  return mysql_stmt_bind_param(this->Statement, this->BoundParameters);
}

// ----------------------------------------------------------------------

bool svtkMySQLQueryInternals::ValidPreparedStatementSQL(const char* query)
{
  if (!query)
    return false;

  if (!LOWERCASE_COMPARE("call", query))
  {
    return true;
  }
  else if (!LOWERCASE_COMPARE("create table", query))
  {
    return true;
  }
  else if (!LOWERCASE_COMPARE("delete", query))
  {
    return true;
  }
  else if (!LOWERCASE_COMPARE("do", query))
  {
    return true;
  }
  else if (!LOWERCASE_COMPARE("insert", query))
  {
    return true;
  }
  else if (!LOWERCASE_COMPARE("replace", query))
  {
    return true;
  }
  else if (!LOWERCASE_COMPARE("select", query))
  {
    return true;
  }
  else if (!LOWERCASE_COMPARE("set", query))
  {
    return true;
  }
  else if (!LOWERCASE_COMPARE("update", query))
  {
    return true;
  }
  return false;
}

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkMySQLQuery);

// ----------------------------------------------------------------------

svtkMySQLQuery::svtkMySQLQuery()
{
  this->Internals = new svtkMySQLQueryInternals;
  this->InitialFetch = true;
  this->LastErrorText = nullptr;
}

// ----------------------------------------------------------------------

svtkMySQLQuery::~svtkMySQLQuery()
{
  this->SetLastErrorText(nullptr);
  delete this->Internals;
}

// ----------------------------------------------------------------------

void svtkMySQLQuery::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------
bool svtkMySQLQuery::Execute()
{
  this->Active = false;

  if (this->Query == nullptr)
  {
    svtkErrorMacro(<< "Cannot execute before a query has been set.");
    return false;
  }

  this->Internals->FreeResult();

  svtkMySQLDatabase* dbContainer = static_cast<svtkMySQLDatabase*>(this->Database);
  assert(dbContainer != nullptr);

  if (!dbContainer->IsOpen())
  {
    svtkErrorMacro(<< "Cannot execute query.  Database is closed.");
    return false;
  }

  svtkDebugMacro(<< "Execute(): Query ready to execute.");

  if (this->Query != nullptr && this->Internals->Statement == nullptr)
  {
    MYSQL* db = dbContainer->Private->Connection;
    assert(db != nullptr);

    int result = mysql_query(db, this->Query);
    if (result == 0)
    {
      // The query probably succeeded.
      this->Internals->Result = mysql_store_result(db);

      // Statements like INSERT are supposed to return empty result sets,
      // but sometimes it is an error for mysql_store_result to return null.
      // If Result is null, but mysql_field_count is non-zero, it is an error.
      // See: http://dev.mysql.com/doc/refman/5.0/en/null-mysql-store-result.html
      if (this->Internals->Result || mysql_field_count(db) == 0)
      {
        // The query definitely succeeded.
        this->SetLastErrorText(nullptr);
        // mysql_field_count will return 0 for statements like INSERT.
        // set Active to false so that we don't call mysql_fetch_row on a nullptr
        // argument and segfault
        if (mysql_field_count(db) == 0)
        {
          this->Active = false;
        }
        else
        {
          this->Active = true;
        }
        return true;
      }
      else
      {
        // There was an error in mysql_query or mysql_store_result
        this->Active = false;
        this->SetLastErrorText(mysql_error(db));
        svtkErrorMacro(<< "Query returned an error: " << this->GetLastErrorText());
        return false;
      }
    }
    else
    {
      // result != 0; the query failed
      this->Active = false;
      this->SetLastErrorText(mysql_error(db));
      svtkErrorMacro(<< "Query returned an error: " << this->GetLastErrorText());
      return false;
    }
  }
  else
  {
    svtkDebugMacro(<< "Binding parameters immediately prior to execution.");
    bool bindStatus = this->Internals->BindParametersToStatement();
    if (!bindStatus)
    {
      this->SetLastErrorText(mysql_stmt_error(this->Internals->Statement));
      svtkErrorMacro(<< "Error binding parameters: " << this->GetLastErrorText());
      return false;
    }

    int result = mysql_stmt_execute(this->Internals->Statement);
    if (result == 0)
    {
      // The query succeeded.
      this->SetLastErrorText(nullptr);
      this->Active = true;
      this->Internals->Result = mysql_stmt_result_metadata(this->Internals->Statement);
      return true;
    }
    else
    {
      this->Active = false;
      this->SetLastErrorText(mysql_stmt_error(this->Internals->Statement));
      svtkErrorMacro(<< "Query returned an error: " << this->GetLastErrorText());
      return false;
    }
  }
}

// ----------------------------------------------------------------------
bool svtkMySQLQuery::BeginTransaction()
{
  this->SetQuery("START TRANSACTION");
  return this->Execute();
}

bool svtkMySQLQuery::CommitTransaction()
{
  this->SetQuery("COMMIT");
  return this->Execute();
}

bool svtkMySQLQuery::RollbackTransaction()
{
  this->SetQuery("ROLLBACK");
  return this->Execute();
}

// ----------------------------------------------------------------------

int svtkMySQLQuery::GetNumberOfFields()
{
  if (!this->Active)
  {
    svtkErrorMacro(<< "GetNumberOfFields(): Query is not active!");
    return 0;
  }
  else
  {
    return static_cast<int>(mysql_num_fields(this->Internals->Result));
  }
}

// ----------------------------------------------------------------------

const char* svtkMySQLQuery::GetFieldName(int column)
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
    MYSQL_FIELD* field = mysql_fetch_field_direct(this->Internals->Result, column);
    if (field == nullptr)
    {
      svtkErrorMacro(<< "GetFieldName(): MySQL returned null field for column " << column);
      return nullptr;
    }
    else
    {
      return field->name;
    }
  }
}

// ----------------------------------------------------------------------

int svtkMySQLQuery::GetFieldType(int column)
{
  if (!this->Active)
  {
    svtkErrorMacro(<< "GetFieldType(): Query is not active!");
    return SVTK_VOID;
  }
  else if (column < 0 || column >= this->GetNumberOfFields())
  {
    svtkErrorMacro(<< "GetFieldType(): Illegal field index " << column);
    return SVTK_VOID;
  }
  else
  {
    svtkMySQLDatabase* dbContainer = static_cast<svtkMySQLDatabase*>(this->Database);
    assert(dbContainer != nullptr);
    if (!dbContainer->IsOpen())
    {
      svtkErrorMacro(<< "Cannot get field type.  Database is closed.");
      return SVTK_VOID;
    }

    MYSQL_FIELD* field = mysql_fetch_field_direct(this->Internals->Result, column);
    if (field == nullptr)
    {
      svtkErrorMacro(<< "GetFieldType(): MySQL returned null field for column " << column);
      return -1;
    }
    else
    {

      switch (field->type)
      {
        case MYSQL_TYPE_ENUM:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_YEAR:
          return SVTK_INT;

        case MYSQL_TYPE_SHORT:
          return SVTK_SHORT;

        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
          return SVTK_LONG;

        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_NEWDATE:
          return SVTK_STRING; // Just return the raw string.

#if MYSQL_VERSION_ID >= 50000
        case MYSQL_TYPE_BIT:
          return SVTK_BIT;
#endif

        case MYSQL_TYPE_FLOAT:
          return SVTK_FLOAT;

        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_DECIMAL:
#if MYSQL_VERSION_ID >= 50000
        case MYSQL_TYPE_NEWDECIMAL:
#endif
          return SVTK_DOUBLE;

        case MYSQL_TYPE_NULL:
          return SVTK_VOID;

        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
          return SVTK_STRING; // MySQL treats text fields as blobs

        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
#if MYSQL_VERSION_ID >= 50000
        case MYSQL_TYPE_VARCHAR:
#endif
          return SVTK_STRING;

        case MYSQL_TYPE_SET:
        case MYSQL_TYPE_GEOMETRY:
        default:
        {
          svtkErrorMacro(<< "GetFieldType(): Unknown data type " << field->type);
          return SVTK_VOID;
        }
      }
    }
  }
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::NextRow()
{
  if (!this->IsActive())
  {
    svtkErrorMacro(<< "NextRow(): Query is not active!");
    return false;
  }

  MYSQL_ROW row = mysql_fetch_row(this->Internals->Result);
  this->Internals->CurrentRow = row;
  this->Internals->CurrentLengths = mysql_fetch_lengths(this->Internals->Result);

  if (!row)
  {
    // A null row will come back in one of two situations.  The first
    // is when there's an error, in which case mysql_errno will return
    // some nonzero error code.  The second is when there are no more
    // rows to fetch.  Discriminate between the two by checking the
    // errno.
    this->Active = false;
    svtkMySQLDatabase* dbContainer = static_cast<svtkMySQLDatabase*>(this->Database);
    assert(dbContainer != nullptr);
    if (!dbContainer->IsOpen())
    {
      svtkErrorMacro(<< "Cannot get field type.  Database is closed.");
      this->SetLastErrorText("Database is closed.");
      return SVTK_VOID;
    }
    MYSQL* db = dbContainer->Private->Connection;
    assert(db != nullptr);

    if (mysql_errno(db) != 0)
    {
      this->SetLastErrorText(mysql_error(db));
      svtkErrorMacro(<< "NextRow(): MySQL returned error message " << this->GetLastErrorText());
    }
    else
    {
      // Nothing's wrong.  We're just out of results.
      this->SetLastErrorText(nullptr);
    }

    return false;
  }
  else
  {
    this->SetLastErrorText(nullptr);
    return true;
  }
}

// ----------------------------------------------------------------------

svtkVariant svtkMySQLQuery::DataValue(svtkIdType column)
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
    assert(this->Internals->CurrentRow);
    svtkVariant result;

    // Initialize base as a SVTK_VOID value... only populate with
    // data when a column value is non-nullptr.
    bool isNull = !this->Internals->CurrentRow[column];
    svtkVariant base;
    if (!isNull)
    {
      // Make a string holding the data, including possible embedded null characters.
      svtkStdString s(this->Internals->CurrentRow[column],
        static_cast<size_t>(this->Internals->CurrentLengths[column]));
      base = svtkVariant(s);
    }

    // It would be a royal pain to try to convert the string to each
    // different value in turn.  Fortunately, there is already code in
    // svtkVariant to do exactly that using the C++ standard library
    // sstream.  We'll exploit that.
    switch (this->GetFieldType(column))
    {
      case SVTK_INT:
      case SVTK_SHORT:
      case SVTK_BIT:
        return isNull ? base : svtkVariant(base.ToInt());

      case SVTK_LONG:
      case SVTK_UNSIGNED_LONG:
        return isNull ? base : svtkVariant(base.ToLong());

      case SVTK_FLOAT:
        return isNull ? base : svtkVariant(base.ToFloat());

      case SVTK_DOUBLE:
        return isNull ? base : svtkVariant(base.ToDouble());

      case SVTK_STRING:
        return base; // it's already a string

      case SVTK_VOID:
        return svtkVariant();

      default:
      {
        svtkWarningMacro(<< "Unhandled type " << this->GetFieldType(column) << " in DataValue().");
        return svtkVariant();
      }
    }
  }
}

// ----------------------------------------------------------------------

const char* svtkMySQLQuery::GetLastErrorText()
{
  return this->LastErrorText;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::HasError()
{
  return (this->GetLastErrorText() != nullptr);
}

svtkStdString svtkMySQLQuery::EscapeString(svtkStdString src, bool addSurroundingQuotes)
{
  svtkStdString dst;
  svtkMySQLDatabase* dbContainer = static_cast<svtkMySQLDatabase*>(this->Database);
  assert(dbContainer != nullptr);

  MYSQL* db;
  if ((!dbContainer->IsOpen()) || !(db = dbContainer->Private->Connection))
  { // fall back to superclass implementation
    dst = this->Superclass::EscapeString(src, addSurroundingQuotes);
    return dst;
  }

  unsigned long ssz = src.size();
  char* dstarr = new char[2 * ssz + (addSurroundingQuotes ? 3 : 1)];
  char* end = dstarr;
  if (addSurroundingQuotes)
  {
    *(end++) = '\'';
  }
  end += mysql_real_escape_string(db, end, src.c_str(), ssz);
  if (addSurroundingQuotes)
  {
    *(end++) = '\'';
    *(end++) = '\0';
  }
  dst = dstarr;
  delete[] dstarr;
  return dst;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::SetQuery(const char* newQuery)
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
  this->Active = false;

  svtkMySQLDatabase* dbContainer = static_cast<svtkMySQLDatabase*>(this->Database);
  if (!dbContainer)
  {
    svtkErrorMacro(
      << "SetQuery: No database connection set!  This usually happens if you have instantiated "
         "svtkMySQLQuery directly.  Don't do that.  Call svtkSQLDatabase::GetQueryInstance instead.");
    return false;
  }

  MYSQL* db = dbContainer->Private->Connection;
  assert(db != nullptr);

  svtkStdString errorMessage;
  bool success = this->Internals->SetQuery(this->Query, db, errorMessage);
  if (!success)
  {
    this->SetLastErrorText(errorMessage.c_str());
    svtkErrorMacro(<< "SetQuery: Error while preparing statement: " << errorMessage.c_str());
  }
  return success;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, unsigned char value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, signed char value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, unsigned short value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, signed short value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, unsigned int value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, signed int value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, unsigned long value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, signed long value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, unsigned long long value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, long long value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, float value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, double value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, const char* value)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(value));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, const svtkStdString& value)
{
  return this->BindParameter(index, value.c_str());
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, const char* data, size_t length)
{
  this->Internals->SetBoundParameter(index, svtkBuildBoundParameter(data, length, false));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::BindParameter(int index, const void* data, size_t length)
{
  this->Internals->SetBoundParameter(
    index, svtkBuildBoundParameter(static_cast<const char*>(data), length, true));
  return true;
}

// ----------------------------------------------------------------------

bool svtkMySQLQuery::ClearParameterBindings()
{
  this->Internals->FreeBoundParameters();
  return true;
}
