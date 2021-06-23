/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectilinearGridWriter
 * @brief   write svtk rectilinear grid data file
 *
 * svtkRectilinearGridWriter is a source object that writes ASCII or binary
 * rectilinear grid data files in svtk format. See text for format details.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef svtkRectilinearGridWriter_h
#define svtkRectilinearGridWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkRectilinearGrid;

class SVTKIOLEGACY_EXPORT svtkRectilinearGridWriter : public svtkDataWriter
{
public:
  static svtkRectilinearGridWriter* New();
  svtkTypeMacro(svtkRectilinearGridWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkRectilinearGrid* GetInput();
  svtkRectilinearGrid* GetInput(int port);
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
  svtkRectilinearGridWriter()
    : WriteExtent(false)
  {
  }
  ~svtkRectilinearGridWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  bool WriteExtent;

private:
  svtkRectilinearGridWriter(const svtkRectilinearGridWriter&) = delete;
  void operator=(const svtkRectilinearGridWriter&) = delete;
};

#endif
