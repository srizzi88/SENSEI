/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnstructuredGridWriter
 * @brief   write svtk unstructured grid data file
 *
 * svtkUnstructuredGridWriter is a source object that writes ASCII or binary
 * unstructured grid data files in svtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef svtkUnstructuredGridWriter_h
#define svtkUnstructuredGridWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro
class svtkUnstructuredGrid;

class SVTKIOLEGACY_EXPORT svtkUnstructuredGridWriter : public svtkDataWriter
{
public:
  static svtkUnstructuredGridWriter* New();
  svtkTypeMacro(svtkUnstructuredGridWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkUnstructuredGrid* GetInput();
  svtkUnstructuredGrid* GetInput(int port);
  //@}

protected:
  svtkUnstructuredGridWriter() {}
  ~svtkUnstructuredGridWriter() override {}

  void WriteData() override;

  int WriteCellsAndFaces(ostream* fp, svtkUnstructuredGrid* grid, const char* label);

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkUnstructuredGridWriter(const svtkUnstructuredGridWriter&) = delete;
  void operator=(const svtkUnstructuredGridWriter&) = delete;
};

#endif
