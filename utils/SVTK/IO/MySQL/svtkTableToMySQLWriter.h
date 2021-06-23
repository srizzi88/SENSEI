/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToMySQLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTableToMySQLWriter
 * @brief   store a svtkTable in a MySQL database
 *
 * svtkTableToMySQLWriter reads a svtkTable and inserts it into a MySQL
 * database.
 */

#ifndef svtkTableToMySQLWriter_h
#define svtkTableToMySQLWriter_h

#include "svtkIOMySQLModule.h" // For export macro
#include "svtkTableToDatabaseWriter.h"

class svtkMySQLDatabase;

class SVTKIOMYSQL_EXPORT svtkTableToMySQLWriter : public svtkTableToDatabaseWriter
{
public:
  static svtkTableToMySQLWriter* New();
  svtkTypeMacro(svtkTableToMySQLWriter, svtkTableToDatabaseWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkTable* GetInput();
  svtkTable* GetInput(int port);
  //@}

protected:
  svtkTableToMySQLWriter();
  ~svtkTableToMySQLWriter() override;
  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkTableToMySQLWriter(const svtkTableToMySQLWriter&) = delete;
  void operator=(const svtkTableToMySQLWriter&) = delete;
};

#endif
