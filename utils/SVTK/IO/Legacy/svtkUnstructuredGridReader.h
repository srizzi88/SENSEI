/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnstructuredGridReader
 * @brief   read svtk unstructured grid data file
 *
 * svtkUnstructuredGridReader is a source object that reads ASCII or binary
 * unstructured grid data files in svtk format. (see text for format details).
 * The output of this reader is a single svtkUnstructuredGrid data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkUnstructuredGrid svtkDataReader
 */

#ifndef svtkUnstructuredGridReader_h
#define svtkUnstructuredGridReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkUnstructuredGrid;

class SVTKIOLEGACY_EXPORT svtkUnstructuredGridReader : public svtkDataReader
{
public:
  static svtkUnstructuredGridReader* New();
  svtkTypeMacro(svtkUnstructuredGridReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkUnstructuredGrid* GetOutput();
  svtkUnstructuredGrid* GetOutput(int idx);
  void SetOutput(svtkUnstructuredGrid* output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkUnstructuredGridReader();
  ~svtkUnstructuredGridReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkUnstructuredGridReader(const svtkUnstructuredGridReader&) = delete;
  void operator=(const svtkUnstructuredGridReader&) = delete;
};

#endif
