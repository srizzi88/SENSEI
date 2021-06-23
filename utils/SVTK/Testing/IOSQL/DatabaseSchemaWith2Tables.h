/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DatabaseSchemaWith2Tables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTestingIOSQLModule.h"

class svtkSQLDatabaseSchema;

class SVTKTESTINGIOSQL_EXPORT DatabaseSchemaWith2Tables
{
public:
  DatabaseSchemaWith2Tables();
  ~DatabaseSchemaWith2Tables();
  svtkSQLDatabaseSchema* GetSchema() { return Schema; }
  int GetTableAHandle() { return TableAHandle; }
  int GetTableBHandle() { return TableBHandle; }
  svtkSQLDatabaseSchema* operator->() const { return this->Schema; }

private:
  void Create();
  svtkSQLDatabaseSchema* Schema;
  int TableAHandle;
  int TableBHandle;
};

// SVTK-HeaderTest-Exclude: DatabaseSchemaWith2Tables.h
