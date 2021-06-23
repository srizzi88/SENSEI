/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMySQLToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMySQLToTableReader
 * @brief   Read a MySQL table as a svtkTable
 *
 * svtkMySQLToTableReader reads a table from a MySQL database and
 * outputs it as a svtkTable.
 */

#ifndef svtkMySQLToTableReader_h
#define svtkMySQLToTableReader_h

#include "svtkDatabaseToTableReader.h"
#include "svtkIOMySQLModule.h" // For export macro

class svtkMySQLDatabase;

class SVTKIOMYSQL_EXPORT svtkMySQLToTableReader : public svtkDatabaseToTableReader
{
public:
  static svtkMySQLToTableReader* New();
  svtkTypeMacro(svtkMySQLToTableReader, svtkDatabaseToTableReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkMySQLToTableReader();
  ~svtkMySQLToTableReader() override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkMySQLToTableReader(const svtkMySQLToTableReader&) = delete;
  void operator=(const svtkMySQLToTableReader&) = delete;
};

#endif
