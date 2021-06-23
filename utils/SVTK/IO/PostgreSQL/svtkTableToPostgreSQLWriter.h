/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToPostgreSQLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTableToPostgreSQLWriter
 * @brief   store a svtkTable in a PostgreSQL database
 *
 * svtkTableToPostgreSQLWriter reads a svtkTable and inserts it into a PostgreSQL
 * database.
 */

#ifndef svtkTableToPostgreSQLWriter_h
#define svtkTableToPostgreSQLWriter_h

#include "svtkIOPostgreSQLModule.h" // For export macro
#include "svtkTableToDatabaseWriter.h"

class svtkPostgreSQLDatabase;

class SVTKIOPOSTGRESQL_EXPORT svtkTableToPostgreSQLWriter : public svtkTableToDatabaseWriter
{
public:
  static svtkTableToPostgreSQLWriter* New();
  svtkTypeMacro(svtkTableToPostgreSQLWriter, svtkTableToDatabaseWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkTable* GetInput();
  svtkTable* GetInput(int port);
  //@}

protected:
  svtkTableToPostgreSQLWriter();
  ~svtkTableToPostgreSQLWriter() override;
  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkTableToPostgreSQLWriter(const svtkTableToPostgreSQLWriter&) = delete;
  void operator=(const svtkTableToPostgreSQLWriter&) = delete;
};

#endif
