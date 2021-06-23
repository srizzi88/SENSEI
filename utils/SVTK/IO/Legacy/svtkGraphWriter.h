/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGraphWriter
 * @brief   write svtkGraph data to a file
 *
 * svtkGraphWriter is a sink object that writes ASCII or binary
 * svtkGraph data files in svtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef svtkGraphWriter_h
#define svtkGraphWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkGraph;
class svtkMolecule;

class SVTKIOLEGACY_EXPORT svtkGraphWriter : public svtkDataWriter
{
public:
  static svtkGraphWriter* New();
  svtkTypeMacro(svtkGraphWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkGraph* GetInput();
  svtkGraph* GetInput(int port);
  //@}

protected:
  svtkGraphWriter() {}
  ~svtkGraphWriter() override {}

  void WriteData() override;

  void WriteMoleculeData(ostream* fp, svtkMolecule* m);

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkGraphWriter(const svtkGraphWriter&) = delete;
  void operator=(const svtkGraphWriter&) = delete;
};

#endif
