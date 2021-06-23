/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTableWriter
 * @brief   write svtkTable to a file
 *
 * svtkTableWriter is a sink object that writes ASCII or binary
 * svtkTable data files in svtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef svtkTableWriter_h
#define svtkTableWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro
class svtkTable;

class SVTKIOLEGACY_EXPORT svtkTableWriter : public svtkDataWriter
{
public:
  static svtkTableWriter* New();
  svtkTypeMacro(svtkTableWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkTable* GetInput();
  svtkTable* GetInput(int port);
  //@}

protected:
  svtkTableWriter() {}
  ~svtkTableWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkTableWriter(const svtkTableWriter&) = delete;
  void operator=(const svtkTableWriter&) = delete;
};

#endif
