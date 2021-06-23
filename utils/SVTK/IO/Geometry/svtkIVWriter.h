/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIVWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkIVWriter
 * @brief   export polydata into OpenInventor 2.0 format.
 *
 * svtkIVWriter is a concrete subclass of svtkWriter that writes OpenInventor 2.0
 * files.
 *
 * @sa
 * svtkPolyDataWriter
 */

#ifndef svtkIVWriter_h
#define svtkIVWriter_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkWriter.h"

class svtkPolyData;

class SVTKIOGEOMETRY_EXPORT svtkIVWriter : public svtkWriter
{
public:
  static svtkIVWriter* New();
  svtkTypeMacro(svtkIVWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  svtkPolyData* GetInput();
  svtkPolyData* GetInput(int port);
  //@}

  //@{
  /**
   * Specify file name of svtk polygon data file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkIVWriter() { this->FileName = nullptr; }

  ~svtkIVWriter() override { delete[] this->FileName; }

  void WriteData() override;
  void WritePolyData(svtkPolyData* polyData, FILE* fp);

  char* FileName;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkIVWriter(const svtkIVWriter&) = delete;
  void operator=(const svtkIVWriter&) = delete;
};

#endif
