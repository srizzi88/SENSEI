/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTreeReader
 * @brief   read svtkTree data file
 *
 * svtkTreeReader is a source object that reads ASCII or binary
 * svtkTree data files in svtk format. (see text for format details).
 * The output of this reader is a single svtkTree data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkTree svtkDataReader svtkTreeWriter
 */

#ifndef svtkTreeReader_h
#define svtkTreeReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkTree;

class SVTKIOLEGACY_EXPORT svtkTreeReader : public svtkDataReader
{
public:
  static svtkTreeReader* New();
  svtkTypeMacro(svtkTreeReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkTree* GetOutput();
  svtkTree* GetOutput(int idx);
  void SetOutput(svtkTree* output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkTreeReader();
  ~svtkTreeReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkTreeReader(const svtkTreeReader&) = delete;
  void operator=(const svtkTreeReader&) = delete;
};

#endif
