/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTreeWriter
 * @brief   write svtkTree data to a file
 *
 * svtkTreeWriter is a sink object that writes ASCII or binary
 * svtkTree data files in svtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef svtkTreeWriter_h
#define svtkTreeWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkTree;

class SVTKIOLEGACY_EXPORT svtkTreeWriter : public svtkDataWriter
{
public:
  static svtkTreeWriter* New();
  svtkTypeMacro(svtkTreeWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkTree* GetInput();
  svtkTree* GetInput(int port);
  //@}

protected:
  svtkTreeWriter() {}
  ~svtkTreeWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkTreeWriter(const svtkTreeWriter&) = delete;
  void operator=(const svtkTreeWriter&) = delete;

  void WriteEdges(ostream& Stream, svtkTree* Tree);
};

#endif
