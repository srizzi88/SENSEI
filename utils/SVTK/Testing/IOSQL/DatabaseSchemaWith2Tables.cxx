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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .SECTION Thanks
// Thanks to Philippe Pebay from Sandia National Laboratories for implementing
// this example of a database schema.

#include "DatabaseSchemaWith2Tables.h"

#include <stdexcept>

#include "svtkSQLDatabaseSchema.h"

DatabaseSchemaWith2Tables::DatabaseSchemaWith2Tables()
{
  this->Create();
}

DatabaseSchemaWith2Tables::~DatabaseSchemaWith2Tables()
{

  if (this->Schema)
  {
    this->Schema->Delete();
  }
}

void DatabaseSchemaWith2Tables::Create()
{
  cerr << "@@ Creating a schema...";

  this->Schema = svtkSQLDatabaseSchema::New();
  this->Schema->SetName("TestSchema");

  // Create PostgreSQL-specific preambles to load the PL/PGSQL language and create a function
  // with this language. These will be ignored by other backends.
  this->Schema->AddPreamble("dropplpgsql", "DROP EXTENSION IF EXISTS PLPGSQL", SVTK_SQL_POSTGRESQL);
  this->Schema->AddPreamble("loadplpgsql", "CREATE LANGUAGE PLPGSQL", SVTK_SQL_POSTGRESQL);
  this->Schema->AddPreamble("createsomefunction",
    "CREATE OR REPLACE FUNCTION somefunction() RETURNS TRIGGER AS $btable$ "
    "BEGIN "
    "INSERT INTO btable (somevalue) VALUES (NEW.somenmbr); "
    "RETURN NEW; "
    "END; $btable$ LANGUAGE PLPGSQL",
    SVTK_SQL_POSTGRESQL);

  // Insert in alphabetical order so that SHOW TABLES does not mix handles
  this->TableAHandle = this->Schema->AddTableMultipleArguments("atable",
    svtkSQLDatabaseSchema::COLUMN_TOKEN, svtkSQLDatabaseSchema::SERIAL, "tablekey", 0, "",
    svtkSQLDatabaseSchema::COLUMN_TOKEN, svtkSQLDatabaseSchema::VARCHAR, "somename", 64, "NOT NULL",
    svtkSQLDatabaseSchema::COLUMN_TOKEN, svtkSQLDatabaseSchema::BIGINT, "somenmbr", 17, "DEFAULT 0",
    svtkSQLDatabaseSchema::INDEX_TOKEN, svtkSQLDatabaseSchema::PRIMARY_KEY, "bigkey",
    svtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "tablekey", svtkSQLDatabaseSchema::END_INDEX_TOKEN,
    svtkSQLDatabaseSchema::INDEX_TOKEN, svtkSQLDatabaseSchema::UNIQUE, "reverselookup",
    svtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "somename", svtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN,
    "somenmbr", svtkSQLDatabaseSchema::END_INDEX_TOKEN, svtkSQLDatabaseSchema::TRIGGER_TOKEN,
    svtkSQLDatabaseSchema::AFTER_INSERT, "inserttrigger", "DO NOTHING", SVTK_SQL_SQLITE,
    svtkSQLDatabaseSchema::TRIGGER_TOKEN, svtkSQLDatabaseSchema::AFTER_INSERT, "inserttrigger",
    "FOR EACH ROW EXECUTE PROCEDURE somefunction ()", SVTK_SQL_POSTGRESQL,
    svtkSQLDatabaseSchema::TRIGGER_TOKEN, svtkSQLDatabaseSchema::AFTER_INSERT, "inserttrigger",
    "FOR EACH ROW INSERT INTO btable SET somevalue = NEW.somenmbr", SVTK_SQL_MYSQL,
    svtkSQLDatabaseSchema::END_TABLE_TOKEN);

  if (this->TableAHandle < 0)
  {
    throw std::runtime_error("Could not create test schema: Failed to create atable");
  }

  this->TableBHandle =
    this->Schema->AddTableMultipleArguments("btable", svtkSQLDatabaseSchema::COLUMN_TOKEN,
      svtkSQLDatabaseSchema::SERIAL, "tablekey", 0, "", svtkSQLDatabaseSchema::COLUMN_TOKEN,
      svtkSQLDatabaseSchema::BIGINT, "somevalue", 12, "DEFAULT 0", svtkSQLDatabaseSchema::INDEX_TOKEN,
      svtkSQLDatabaseSchema::PRIMARY_KEY, "", svtkSQLDatabaseSchema::INDEX_COLUMN_TOKEN, "tablekey",
      svtkSQLDatabaseSchema::END_INDEX_TOKEN, svtkSQLDatabaseSchema::END_TABLE_TOKEN);

  if (this->TableBHandle < 0)
  {
    throw std::runtime_error("Could not create test schema: Failed to create btable");
  }
  cerr << " done." << endl;
}
