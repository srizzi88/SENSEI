/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataWriter
 * @brief   write svtk polygonal data
 *
 * svtkPolyDataWriter is a source object that writes ASCII or binary
 * polygonal data files in svtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef svtkPolyDataWriter_h
#define svtkPolyDataWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkPolyData;

class SVTKIOLEGACY_EXPORT svtkPolyDataWriter : public svtkDataWriter
{
public:
  static svtkPolyDataWriter* New();
  svtkTypeMacro(svtkPolyDataWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkPolyData* GetInput();
  svtkPolyData* GetInput(int port);
  //@}

protected:
  svtkPolyDataWriter() {}
  ~svtkPolyDataWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkPolyDataWriter(const svtkPolyDataWriter&) = delete;
  void operator=(const svtkPolyDataWriter&) = delete;
};

#endif
