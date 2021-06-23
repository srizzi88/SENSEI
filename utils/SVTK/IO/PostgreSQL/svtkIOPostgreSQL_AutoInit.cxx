/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIOPostgreSQL_AutoInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "svtkPostgreSQLDatabase.h"

#include <svtksys/SystemTools.hxx>

#include <string>

// Registration of PostgreSQL dynamically with the svtkSQLDatabase factory method.
svtkSQLDatabase* PostgreSQLCreateFunction(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol, unused;
  svtkPostgreSQLDatabase* db = 0;

  if (svtksys::SystemTools::ParseURLProtocol(urlstr, protocol, unused) && protocol == "psql")
  {
    db = svtkPostgreSQLDatabase::New();
    db->ParseURL(URL);
  }

  return db;
}

static unsigned int svtkIOPostgreSQLCount;

struct SVTKIOPOSTGRESQL_EXPORT svtkIOPostgreSQL_AutoInit
{
  svtkIOPostgreSQL_AutoInit();
  ~svtkIOPostgreSQL_AutoInit();
};

SVTKIOPOSTGRESQL_EXPORT void svtkIOPostgreSQL_AutoInit_Construct()
{
  if (++svtkIOPostgreSQLCount == 1)
  {
    svtkSQLDatabase::RegisterCreateFromURLCallback(PostgreSQLCreateFunction);
  }
}
