/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkSQLDatabaseSchema.cxx

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

#include "svtkSQLDatabaseSchema.h"
#include "svtkToolkits.h"

#include "svtkObjectFactory.h"
#include "svtkStdString.h"

#include <cstdarg> // va_list

#include <vector>

// ----------------------------------------------------------------------
svtkStandardNewMacro(svtkSQLDatabaseSchema);

// ----------------------------------------------------------------------
class svtkSQLDatabaseSchemaInternals
{
public: // NB: use of string instead of char* here to avoid leaks on destruction.
  struct Statement
  {
    svtkStdString Name;
    svtkStdString Action;  // may have backend-specific stuff
    svtkStdString Backend; // only active for this backend, if != ""
  };

  struct Column
  {
    svtkSQLDatabaseSchema::DatabaseColumnType Type;
    int Size; // used when required, ignored otherwise (e.g. varchar)
    svtkStdString Name;
    svtkStdString Attributes; // may have backend-specific stuff
  };

  struct Index
  {
    svtkSQLDatabaseSchema::DatabaseIndexType Type;
    svtkStdString Name;
    std::vector<svtkStdString> ColumnNames;
  };

  struct Trigger
  {
    svtkSQLDatabaseSchema::DatabaseTriggerType Type;
    svtkStdString Name;
    svtkStdString Action;  // may have backend-specific stuff
    svtkStdString Backend; // only active for this backend, if != ""
  };

  struct Option
  {
    svtkStdString Text;
    svtkStdString Backend;
  };

  struct Table
  {
    svtkStdString Name;
    std::vector<Column> Columns;
    std::vector<Index> Indices;
    std::vector<Trigger> Triggers;
    std::vector<Option> Options;
  };

  std::vector<Statement> Preambles;
  std::vector<Table> Tables;
};

// ----------------------------------------------------------------------
svtkSQLDatabaseSchema::svtkSQLDatabaseSchema()
{
  this->Name = nullptr;
  this->Internals = new svtkSQLDatabaseSchemaInternals;
}

// ----------------------------------------------------------------------
svtkSQLDatabaseSchema::~svtkSQLDatabaseSchema()
{
  this->SetName(nullptr);
  delete this->Internals;
}

// ----------------------------------------------------------------------
void svtkSQLDatabaseSchema::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: ";
  if (this->Name)
  {
    os << this->Name << "\n";
  }
  else
  {
    os << "(null)"
       << "\n";
  }
  os << indent << "Internals: " << this->Internals << "\n";
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::AddPreamble(
  const char* preName, const char* preAction, const char* preBackend)
{
  if (!preName)
  {
    svtkErrorMacro("Cannot add preamble with empty name");
    return -1;
  }

  svtkSQLDatabaseSchemaInternals::Statement newPre;
  int preHandle = static_cast<int>(this->Internals->Preambles.size());
  newPre.Name = preName;
  newPre.Action = preAction;
  newPre.Backend = preBackend;
  this->Internals->Preambles.push_back(newPre);
  return preHandle;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::AddTable(const char* tblName)
{
  if (!tblName)
  {
    svtkErrorMacro("Cannot add table with empty name");
    return -1;
  }

  svtkSQLDatabaseSchemaInternals::Table newTbl;
  int tblHandle = static_cast<int>(this->Internals->Tables.size());
  newTbl.Name = tblName;
  this->Internals->Tables.push_back(newTbl);
  return tblHandle;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::AddColumnToIndex(int tblHandle, int idxHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot add column to index of non-existent table " << tblHandle);
    return -1;
  }

  svtkSQLDatabaseSchemaInternals::Table* table = &this->Internals->Tables[tblHandle];
  if (colHandle < 0 || colHandle >= static_cast<int>(table->Columns.size()))
  {
    svtkErrorMacro("Cannot add non-existent column " << colHandle << " in table " << tblHandle);
    return -1;
  }

  if (idxHandle < 0 || idxHandle >= static_cast<int>(table->Indices.size()))
  {
    svtkErrorMacro(
      "Cannot add column to non-existent index " << idxHandle << " of table " << tblHandle);
    return -1;
  }

  table->Indices[idxHandle].ColumnNames.push_back(table->Columns[colHandle].Name);
  return static_cast<int>(table->Indices[idxHandle].ColumnNames.size() - 1);
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::AddColumnToTable(
  int tblHandle, int colType, const char* colName, int colSize, const char* colOpts)
{
  if (!colName)
  {
    svtkErrorMacro("Cannot add column with empty name to table " << tblHandle);
    return -1;
  }

  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot add column to non-existent table " << tblHandle);
    return -1;
  }

  // DCT: This trick avoids copying a Column structure the way push_back would:
  int colHandle = static_cast<int>(this->Internals->Tables[tblHandle].Columns.size());
  this->Internals->Tables[tblHandle].Columns.resize(colHandle + 1);
  svtkSQLDatabaseSchemaInternals::Column* column =
    &this->Internals->Tables[tblHandle].Columns[colHandle];
  column->Type = static_cast<DatabaseColumnType>(colType);
  column->Size = colSize;
  column->Name = colName;
  column->Attributes = colOpts;
  return colHandle;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::AddIndexToTable(int tblHandle, int idxType, const char* idxName)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot add index to non-existent table " << tblHandle);
    return -1;
  }

  int idxHandle = static_cast<int>(this->Internals->Tables[tblHandle].Indices.size());
  this->Internals->Tables[tblHandle].Indices.resize(idxHandle + 1);
  svtkSQLDatabaseSchemaInternals::Index* index =
    &this->Internals->Tables[tblHandle].Indices[idxHandle];
  index->Type = static_cast<DatabaseIndexType>(idxType);
  index->Name = idxName;
  return idxHandle;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::AddTriggerToTable(
  int tblHandle, int trgType, const char* trgName, const char* trgAction, const char* trgBackend)
{
  if (!trgName)
  {
    svtkErrorMacro("Cannot add trigger with empty name to table " << tblHandle);
    return -1;
  }

  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot add trigger to non-existent table " << tblHandle);
    return -1;
  }

  int trgHandle = static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size());
  this->Internals->Tables[tblHandle].Triggers.resize(trgHandle + 1);
  svtkSQLDatabaseSchemaInternals::Trigger* trigger =
    &this->Internals->Tables[tblHandle].Triggers[trgHandle];
  trigger->Type = static_cast<DatabaseTriggerType>(trgType);
  trigger->Name = trgName;
  trigger->Action = trgAction;
  trigger->Backend = trgBackend;
  return trgHandle;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::AddOptionToTable(
  int tblHandle, const char* optText, const char* optBackend)
{
  if (!optText)
  {
    svtkErrorMacro("Cannot add nullptr option to table " << tblHandle);
    return -1;
  }

  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot add option to non-existent table " << tblHandle);
    return -1;
  }

  int optHandle = static_cast<int>(this->Internals->Tables[tblHandle].Options.size());
  this->Internals->Tables[tblHandle].Options.resize(optHandle + 1);
  svtkSQLDatabaseSchemaInternals::Option* optn =
    &this->Internals->Tables[tblHandle].Options[optHandle];
  optn->Text = optText;
  optn->Backend = optBackend ? optBackend : SVTK_SQL_ALLBACKENDS;
  return optHandle;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetPreambleHandleFromName(const char* preName)
{
  int i;
  int ntab = static_cast<int>(this->Internals->Preambles.size());
  svtkStdString preNameStr(preName);
  for (i = 0; i < ntab; ++i)
  {
    if (this->Internals->Preambles[i].Name == preNameStr)
    {
      return i;
    }
  }
  return -1;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetPreambleNameFromHandle(int preHandle)
{
  if (preHandle < 0 || preHandle >= this->GetNumberOfPreambles())
  {
    svtkErrorMacro("Cannot get name of non-existent preamble " << preHandle);
    return nullptr;
  }

  return this->Internals->Preambles[preHandle].Name;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetPreambleActionFromHandle(int preHandle)
{
  if (preHandle < 0 || preHandle >= this->GetNumberOfPreambles())
  {
    svtkErrorMacro("Cannot get action of non-existent preamble " << preHandle);
    return nullptr;
  }

  return this->Internals->Preambles[preHandle].Action;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetPreambleBackendFromHandle(int preHandle)
{
  if (preHandle < 0 || preHandle >= this->GetNumberOfPreambles())
  {
    svtkErrorMacro("Cannot get backend of non-existent preamble " << preHandle);
    return nullptr;
  }

  return this->Internals->Preambles[preHandle].Backend;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetTableHandleFromName(const char* tblName)
{
  int i;
  int ntab = static_cast<int>(this->Internals->Tables.size());
  svtkStdString tblNameStr(tblName);
  for (i = 0; i < ntab; ++i)
  {
    if (this->Internals->Tables[i].Name == tblNameStr)
    {
      return i;
    }
  }
  return -1;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetTableNameFromHandle(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get name of non-existent table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Name;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetIndexHandleFromName(const char* tblName, const char* idxName)
{
  int tblHandle = this->GetTableHandleFromName(tblName);
  if (tblHandle < 0)
  {
    return -1;
  }

  int i;
  int nidx = static_cast<int>(this->Internals->Tables[tblHandle].Indices.size());
  svtkStdString idxNameStr(idxName);
  for (i = 0; i < nidx; ++i)
  {
    if (this->Internals->Tables[tblHandle].Indices[i].Name == idxNameStr)
    {
      return i;
    }
  }
  return -1;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetIndexNameFromHandle(int tblHandle, int idxHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get name of an index in non-existent table " << tblHandle);
    return nullptr;
  }

  if (idxHandle < 0 ||
    idxHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Indices.size()))
  {
    svtkErrorMacro(
      "Cannot get name of non-existent index " << idxHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Indices[idxHandle].Name;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetIndexTypeFromHandle(int tblHandle, int idxHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get type of an index in non-existent table " << tblHandle);
    return -1;
  }

  if (idxHandle < 0 ||
    idxHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Indices.size()))
  {
    svtkErrorMacro(
      "Cannot get type of non-existent index " << idxHandle << " in table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Indices[idxHandle].Type);
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetIndexColumnNameFromHandle(
  int tblHandle, int idxHandle, int cnmHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get column name of an index in non-existent table " << tblHandle);
    return nullptr;
  }

  if (idxHandle < 0 ||
    idxHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Indices.size()))
  {
    svtkErrorMacro(
      "Cannot get column name of non-existent index " << idxHandle << " in table " << tblHandle);
    return nullptr;
  }

  if (cnmHandle < 0 ||
    cnmHandle >=
      static_cast<int>(this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames.size()))
  {
    svtkErrorMacro("Cannot get column name of non-existent column "
      << cnmHandle << " of index " << idxHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames[cnmHandle];
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetColumnHandleFromName(const char* tblName, const char* colName)
{
  int tblHandle = this->GetTableHandleFromName(tblName);
  if (tblHandle < 0)
  {
    return -1;
  }

  int i;
  int ncol = static_cast<int>(this->Internals->Tables[tblHandle].Columns.size());
  svtkStdString colNameStr(colName);
  for (i = 0; i < ncol; ++i)
  {
    if (this->Internals->Tables[tblHandle].Columns[i].Name == colNameStr)
    {
      return i;
    }
  }
  return -1;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetColumnNameFromHandle(int tblHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get name of a column in non-existent table " << tblHandle);
    return nullptr;
  }

  if (colHandle < 0 ||
    colHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Columns.size()))
  {
    svtkErrorMacro(
      "Cannot get name of non-existent column " << colHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Columns[colHandle].Name;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetColumnTypeFromHandle(int tblHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get type of a column in non-existent table " << tblHandle);
    return -1;
  }

  if (colHandle < 0 ||
    colHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Columns.size()))
  {
    svtkErrorMacro(
      "Cannot get type of non-existent column " << colHandle << " in table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Columns[colHandle].Type);
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetColumnSizeFromHandle(int tblHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get size of a column in non-existent table " << tblHandle);
    return -1;
  }

  if (colHandle < 0 ||
    colHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Columns.size()))
  {
    svtkErrorMacro(
      "Cannot get size of non-existent column " << colHandle << " in table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Columns[colHandle].Size);
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetColumnAttributesFromHandle(int tblHandle, int colHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get attributes of a column in non-existent table " << tblHandle);
    return nullptr;
  }

  if (colHandle < 0 ||
    colHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Columns.size()))
  {
    svtkErrorMacro(
      "Cannot get attributes of non-existent column " << colHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Columns[colHandle].Attributes;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetTriggerHandleFromName(const char* tblName, const char* trgName)
{
  int tblHandle = this->GetTableHandleFromName(tblName);
  if (tblHandle < 0)
  {
    return -1;
  }

  int i;
  int ntrg = static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size());
  svtkStdString trgNameStr(trgName);
  for (i = 0; i < ntrg; ++i)
  {
    if (this->Internals->Tables[tblHandle].Triggers[i].Name == trgNameStr)
    {
      return i;
    }
  }
  return -1;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetTriggerNameFromHandle(int tblHandle, int trgHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get name of a trigger in non-existent table " << tblHandle);
    return nullptr;
  }

  if (trgHandle < 0 ||
    trgHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size()))
  {
    svtkErrorMacro(
      "Cannot get name of non-existent trigger " << trgHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Triggers[trgHandle].Name;
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetTriggerTypeFromHandle(int tblHandle, int trgHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get type of a trigger in non-existent table " << tblHandle);
    return -1;
  }

  if (trgHandle < 0 ||
    trgHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size()))
  {
    svtkErrorMacro(
      "Cannot get type of non-existent trigger " << trgHandle << " in table " << tblHandle);
    return -1;
  }

  return this->Internals->Tables[tblHandle].Triggers[trgHandle].Type;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetTriggerActionFromHandle(int tblHandle, int trgHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get action of a trigger in non-existent table " << tblHandle);
    return nullptr;
  }

  if (trgHandle < 0 ||
    trgHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size()))
  {
    svtkErrorMacro(
      "Cannot get action of non-existent trigger " << trgHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Triggers[trgHandle].Action;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetTriggerBackendFromHandle(int tblHandle, int trgHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get backend of a trigger in non-existent table " << tblHandle);
    return nullptr;
  }

  if (trgHandle < 0 ||
    trgHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size()))
  {
    svtkErrorMacro(
      "Cannot get backend of non-existent trigger " << trgHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Triggers[trgHandle].Backend;
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetOptionTextFromHandle(int tblHandle, int optHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get text of an option in non-existent table " << tblHandle);
    return nullptr;
  }

  if (optHandle < 0 ||
    optHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Options.size()))
  {
    svtkErrorMacro(
      "Cannot get text of non-existent option " << optHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Options[optHandle].Text.c_str();
}

// ----------------------------------------------------------------------
const char* svtkSQLDatabaseSchema::GetOptionBackendFromHandle(int tblHandle, int optHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get backend of an option in non-existent table " << tblHandle);
    return nullptr;
  }

  if (optHandle < 0 ||
    optHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Options.size()))
  {
    svtkErrorMacro(
      "Cannot get backend of non-existent option " << optHandle << " in table " << tblHandle);
    return nullptr;
  }

  return this->Internals->Tables[tblHandle].Options[optHandle].Backend.c_str();
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::AddTableMultipleArguments(const char* tblName, ...)
{
  int tblHandle = this->AddTable(tblName);
  int token;
  int dtyp;
  int size;
  int curIndexHandle;
  const char* name;
  const char* attr;
  const char* bcke;

  va_list args;
  va_start(args, tblName);
  while ((token = va_arg(args, int)) != END_TABLE_TOKEN)
  {
    switch (token)
    {
      case COLUMN_TOKEN:
        dtyp = va_arg(args, int);
        name = va_arg(args, const char*);
        size = va_arg(args, int);
        attr = va_arg(args, const char*);
        this->AddColumnToTable(tblHandle, dtyp, name, size, attr);
        break;
      case INDEX_TOKEN:
        dtyp = va_arg(args, int);
        name = va_arg(args, const char*);
        curIndexHandle = this->AddIndexToTable(tblHandle, dtyp, name);
        while ((token = va_arg(args, int)) != END_INDEX_TOKEN)
        {
          name = va_arg(args, const char*);
          dtyp = this->GetColumnHandleFromName(tblName, name);
          this->AddColumnToIndex(tblHandle, curIndexHandle, dtyp);
        }
        break;
      case TRIGGER_TOKEN:
        dtyp = va_arg(args, int);
        name = va_arg(args, const char*);
        attr = va_arg(args, const char*);
        bcke = va_arg(args, const char*);
        this->AddTriggerToTable(tblHandle, dtyp, name, attr, bcke);
        break;
      case OPTION_TOKEN:
        attr = va_arg(args, const char*);
        bcke = va_arg(args, const char*);
        this->AddOptionToTable(tblHandle, attr, bcke);
        break;
      default:
      {
        svtkErrorMacro("Bad token " << token << " passed to AddTable");
        va_end(args);
        return -1;
      }
    }
  }
  va_end(args);
  return tblHandle;
}

// ----------------------------------------------------------------------
void svtkSQLDatabaseSchema::Reset()
{
  this->Internals->Tables.clear();
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetNumberOfPreambles()
{
  return static_cast<int>(this->Internals->Preambles.size());
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetNumberOfTables()
{
  return static_cast<int>(this->Internals->Tables.size());
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetNumberOfColumnsInTable(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get the number of columns of non-existent table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Columns.size());
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetNumberOfIndicesInTable(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get the number of indices of non-existent table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Indices.size());
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetNumberOfColumnNamesInIndex(int tblHandle, int idxHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro(
      "Cannot get the number of column names in index of non-existent table " << tblHandle);
    return -1;
  }

  if (idxHandle < 0 ||
    idxHandle >= static_cast<int>(this->Internals->Tables[tblHandle].Indices.size()))
  {
    svtkErrorMacro("Cannot get the number of column names of non-existent index "
      << idxHandle << " in table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Indices[idxHandle].ColumnNames.size());
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetNumberOfTriggersInTable(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get the number of triggers of non-existent table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Triggers.size());
}

// ----------------------------------------------------------------------
int svtkSQLDatabaseSchema::GetNumberOfOptionsInTable(int tblHandle)
{
  if (tblHandle < 0 || tblHandle >= this->GetNumberOfTables())
  {
    svtkErrorMacro("Cannot get the number of options of non-existent table " << tblHandle);
    return -1;
  }

  return static_cast<int>(this->Internals->Tables[tblHandle].Options.size());
}
