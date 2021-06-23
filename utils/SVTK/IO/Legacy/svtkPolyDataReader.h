/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataReader
 * @brief   read svtk polygonal data file
 *
 * svtkPolyDataReader is a source object that reads ASCII or binary
 * polygonal data files in svtk format (see text for format details).
 * The output of this reader is a single svtkPolyData data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkPolyData svtkDataReader
 */

#ifndef svtkPolyDataReader_h
#define svtkPolyDataReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkPolyData;

class SVTKIOLEGACY_EXPORT svtkPolyDataReader : public svtkDataReader
{
public:
  static svtkPolyDataReader* New();
  svtkTypeMacro(svtkPolyDataReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkPolyData* GetOutput();
  svtkPolyData* GetOutput(int idx);
  void SetOutput(svtkPolyData* output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkPolyDataReader();
  ~svtkPolyDataReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkPolyDataReader(const svtkPolyDataReader&) = delete;
  void operator=(const svtkPolyDataReader&) = delete;
};

#endif
