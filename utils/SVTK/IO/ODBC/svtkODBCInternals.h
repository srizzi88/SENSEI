/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkODBCInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkODBCInternals
 * @brief   Simple class to hide ODBC structures
 *
 *
 *
 * There is no .cxx file to go along with this header.  Its sole
 * purpose is to let svtkODBCQuery and svtkODBCDatabase share the
 * variables that describe a single connection.
 *
 * @sa
 * svtkODBCDatabase svtkODBCQuery
 *
 */

#ifndef svtkODBCInternals_h
#define svtkODBCInternals_h

#include <sql.h>

class svtkODBCInternals
{
  friend class svtkODBCDatabase;
  friend class svtkODBCQuery;

public:
  svtkODBCInternals()
    : Environment(0)
    , Connection(0)
  {
  }

private:
  SQLHANDLE Environment;
  SQLHANDLE Connection;
};

#endif
// SVTK-HeaderTest-Exclude: svtkODBCInternals.h
