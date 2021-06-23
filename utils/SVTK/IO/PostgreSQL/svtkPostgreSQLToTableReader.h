/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPostgreSQLToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPostgreSQLToTableReader
 * @brief   Read a PostgreSQL table as a svtkTable
 *
 * svtkPostgreSQLToTableReader reads a table from a PostgreSQL database and
 * outputs it as a svtkTable.
 */

#ifndef svtkPostgreSQLToTableReader_h
#define svtkPostgreSQLToTableReader_h

#include "svtkDatabaseToTableReader.h"
#include "svtkIOPostgreSQLModule.h" // For export macro

class svtkPostgreSQLDatabase;

class SVTKIOPOSTGRESQL_EXPORT svtkPostgreSQLToTableReader : public svtkDatabaseToTableReader
{
public:
  static svtkPostgreSQLToTableReader* New();
  svtkTypeMacro(svtkPostgreSQLToTableReader, svtkDatabaseToTableReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPostgreSQLToTableReader();
  ~svtkPostgreSQLToTableReader() override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPostgreSQLToTableReader(const svtkPostgreSQLToTableReader&) = delete;
  void operator=(const svtkPostgreSQLToTableReader&) = delete;
};

#endif
