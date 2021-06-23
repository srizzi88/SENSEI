/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIOODBC_AutoInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkODBCDatabase.h"

#include <svtksys/SystemTools.hxx>

#include <string>

// Registration of ODBC dynamically with the svtkSQLDatabase factory method.
svtkSQLDatabase* ODBCCreateFunction(const char* URL)
{
  std::string urlstr(URL ? URL : "");
  std::string protocol, unused;
  svtkODBCDatabase* db = 0;

  if (svtksys::SystemTools::ParseURLProtocol(urlstr, protocol, unused) && protocol == "odbc")
  {
    db = svtkODBCDatabase::New();
    db->ParseURL(URL);
  }

  return db;
}

static unsigned int svtkIOODBCCount;

SVTKIOODBC_EXPORT void svtkIOODBC_AutoInit_Construct()
{
  if (++svtkIOODBCCount == 1)
  {
    svtkSQLDatabase::RegisterCreateFromURLCallback(ODBCCreateFunction);
  }
}
