/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStructuredGridWriter
 * @brief   write svtk structured grid data file
 *
 * svtkStructuredGridWriter is a source object that writes ASCII or binary
 * structured grid data files in svtk format. See text for format details.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef svtkStructuredGridWriter_h
#define svtkStructuredGridWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkStructuredGrid;

class SVTKIOLEGACY_EXPORT svtkStructuredGridWriter : public svtkDataWriter
{
public:
  static svtkStructuredGridWriter* New();
  svtkTypeMacro(svtkStructuredGridWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkStructuredGrid* GetInput();
  svtkStructuredGrid* GetInput(int port);
  //@}

  //@{
  /**
   * When WriteExtent is on, svtkStructuredPointsWriter writes
   * data extent in the output file. Otherwise, it writes dimensions.
   * The only time this option is useful is when the extents do
   * not start at (0, 0, 0). This is an options to support writing
   * of older formats while still using a newer SVTK.
   */
  svtkSetMacro(WriteExtent, bool);
  svtkGetMacro(WriteExtent, bool);
  svtkBooleanMacro(WriteExtent, bool);
  //@}

protected:
  svtkStructuredGridWriter()
    : WriteExtent(false)
  {
  }
  ~svtkStructuredGridWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  bool WriteExtent;

private:
  svtkStructuredGridWriter(const svtkStructuredGridWriter&) = delete;
  void operator=(const svtkStructuredGridWriter&) = delete;
};

#endif
