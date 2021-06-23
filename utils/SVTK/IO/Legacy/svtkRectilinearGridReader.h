/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectilinearGridReader
 * @brief   read svtk rectilinear grid data file
 *
 * svtkRectilinearGridReader is a source object that reads ASCII or binary
 * rectilinear grid data files in svtk format (see text for format details).
 * The output of this reader is a single svtkRectilinearGrid data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkRectilinearGrid svtkDataReader
 */

#ifndef svtkRectilinearGridReader_h
#define svtkRectilinearGridReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkRectilinearGrid;

class SVTKIOLEGACY_EXPORT svtkRectilinearGridReader : public svtkDataReader
{
public:
  static svtkRectilinearGridReader* New();
  svtkTypeMacro(svtkRectilinearGridReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get and set the output of this reader.
   */
  svtkRectilinearGrid* GetOutput();
  svtkRectilinearGrid* GetOutput(int idx);
  void SetOutput(svtkRectilinearGrid* output);
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
  svtkRectilinearGridReader();
  ~svtkRectilinearGridReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkRectilinearGridReader(const svtkRectilinearGridReader&) = delete;
  void operator=(const svtkRectilinearGridReader&) = delete;
};

#endif
