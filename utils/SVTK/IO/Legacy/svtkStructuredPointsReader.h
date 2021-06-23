/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredPointsReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStructuredPointsReader
 * @brief   read svtk structured points data file
 *
 * svtkStructuredPointsReader is a source object that reads ASCII or binary
 * structured points data files in svtk format (see text for format details).
 * The output of this reader is a single svtkStructuredPoints data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkStructuredPoints svtkDataReader
 */

#ifndef svtkStructuredPointsReader_h
#define svtkStructuredPointsReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkStructuredPoints;

class SVTKIOLEGACY_EXPORT svtkStructuredPointsReader : public svtkDataReader
{
public:
  static svtkStructuredPointsReader* New();
  svtkTypeMacro(svtkStructuredPointsReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the output of this reader.
   */
  void SetOutput(svtkStructuredPoints* output);
  svtkStructuredPoints* GetOutput(int idx);
  svtkStructuredPoints* GetOutput();
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
  svtkStructuredPointsReader();
  ~svtkStructuredPointsReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkStructuredPointsReader(const svtkStructuredPointsReader&) = delete;
  void operator=(const svtkStructuredPointsReader&) = delete;
};

#endif
