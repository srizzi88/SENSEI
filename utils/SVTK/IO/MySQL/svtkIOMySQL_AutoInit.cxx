/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIOMySQL_AutoInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMySQLDatabase.h"

#include <svtksys/SystemTools.hxx>

#include <string>

// Registration of MySQL dynamically with the svtkSQLDatabase factory method.
svtkSQLDatabase* MySQLCreateFunction(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol, unused;
  svtkMySQLDatabase* db = 0;

  if (svtksys::SystemTools::ParseURLProtocol(urlstr, protocol, unused) && protocol == "mysql")
  {
    db = svtkMySQLDatabase::New();
    db->ParseURL(URL);
  }

  return db;
}

static unsigned int svtkIOMySQLCount;

SVTKIOMYSQL_EXPORT void svtkIOMySQL_AutoInit_Construct()
{
  if (++svtkIOMySQLCount == 1)
  {
    svtkSQLDatabase::RegisterCreateFromURLCallback(MySQLCreateFunction);
  }
}
