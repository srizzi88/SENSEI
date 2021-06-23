/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPostgreSQLDatabasePrivate.h

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

/**
 * @class   svtkPostgreSQLDatabasePrivate
 * @brief   internal details of a connection to a PostgreSQL database
 *
 *
 *
 * This class does two things.  First, it holds the (pointer to the)
 * PGconn struct that represents an actual database connection.  Second,
 * it holds a map from Postgres data types as they exist in the database
 * to SVTK data types.
 *
 * You should never have to deal with this class outside of
 * svtkPostgreSQLDatabase and svtkPostgreSQLQuery.
 */

#ifndef svtkPostgreSQLDatabasePrivate_h
#define svtkPostgreSQLDatabasePrivate_h

#include "svtkStdString.h"
#include "svtkTimeStamp.h"
#include "svtkType.h"

#include <libpq-fe.h>
#include <map>

class svtkPostgreSQLDatabasePrivate
{
public:
  svtkPostgreSQLDatabasePrivate() { this->Connection = nullptr; }

  /**
   * Destroy the database connection. Any uncommitted transaction will be aborted.
   */
  virtual ~svtkPostgreSQLDatabasePrivate()
  {
    if (this->Connection)
    {
      PQfinish(this->Connection);
    }
  }

  // Given a Postgres column type OID, return a SVTK array type (see svtkType.h).
  int GetSVTKTypeFromOID(Oid pgtype)
  {
    std::map<Oid, int>::const_iterator it = this->DataTypeMap.find(pgtype);
    if (it == this->DataTypeMap.end())
    {
      return SVTK_STRING;
    }
    return it->second;
  }

  // This is the actual database connection.  It will be nullptr if no
  // connection is open.
  PGconn* Connection;

  // Map Postgres column types to SVTK types.
  std::map<Oid, int> DataTypeMap;
};

#endif // svtkPostgreSQLDatabasePrivate_h
// SVTK-HeaderTest-Exclude: svtkPostgreSQLDatabasePrivate.h
