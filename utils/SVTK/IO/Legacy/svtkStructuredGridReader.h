/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStructuredGridReader
 * @brief   read svtk structured grid data file
 *
 * svtkStructuredGridReader is a source object that reads ASCII or binary
 * structured grid data files in svtk format. (see text for format details).
 * The output of this reader is a single svtkStructuredGrid data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkStructuredGrid svtkDataReader
 */

#ifndef svtkStructuredGridReader_h
#define svtkStructuredGridReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkStructuredGrid;

class SVTKIOLEGACY_EXPORT svtkStructuredGridReader : public svtkDataReader
{
public:
  static svtkStructuredGridReader* New();
  svtkTypeMacro(svtkStructuredGridReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkStructuredGrid* GetOutput();
  svtkStructuredGrid* GetOutput(int idx);
  void SetOutput(svtkStructuredGrid* output);
  //@}

  /**
   * Read the meta information from the file (WHOLE_EXTENT).
   */
  int ReadMetaDataSimple(const std::string& fname, svtkInformation* metadata) override;

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkStructuredGridReader();
  ~svtkStructuredGridReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkStructuredGridReader(const svtkStructuredGridReader&) = delete;
  void operator=(const svtkStructuredGridReader&) = delete;
};

#endif
