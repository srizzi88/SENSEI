/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataSetWriter
 * @brief   write any type of svtk dataset to file
 *
 * svtkDataSetWriter is an abstract class for mapper objects that write their
 * data to disk (or into a communications port). The input to this object is
 * a dataset of any type.
 */

#ifndef svtkDataSetWriter_h
#define svtkDataSetWriter_h

#include "svtkDataWriter.h"
#include "svtkIOLegacyModule.h" // For export macro

class SVTKIOLEGACY_EXPORT svtkDataSetWriter : public svtkDataWriter
{
public:
  static svtkDataSetWriter* New();
  svtkTypeMacro(svtkDataSetWriter, svtkDataWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkDataSet* GetInput();
  svtkDataSet* GetInput(int port);
  //@}

protected:
  svtkDataSetWriter() {}
  ~svtkDataSetWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkDataSetWriter(const svtkDataSetWriter&) = delete;
  void operator=(const svtkDataSetWriter&) = delete;
};

#endif
