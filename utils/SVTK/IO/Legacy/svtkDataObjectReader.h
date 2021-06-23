/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataObjectReader
 * @brief   read svtk field data file
 *
 * svtkDataObjectReader is a source object that reads ASCII or binary field
 * data files in svtk format. Fields are general matrix structures used
 * represent complex data. (See text for format details).  The output of this
 * reader is a single svtkDataObject.  The superclass of this class,
 * svtkDataReader, provides many methods for controlling the reading of the
 * data file, see svtkDataReader for more information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkFieldData svtkDataObjectWriter
 */

#ifndef svtkDataObjectReader_h
#define svtkDataObjectReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkDataObject;

class SVTKIOLEGACY_EXPORT svtkDataObjectReader : public svtkDataReader
{
public:
  static svtkDataObjectReader* New();
  svtkTypeMacro(svtkDataObjectReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output field of this reader.
   */
  svtkDataObject* GetOutput();
  svtkDataObject* GetOutput(int idx);
  void SetOutput(svtkDataObject*);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkDataObjectReader();
  ~svtkDataObjectReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkDataObjectReader(const svtkDataObjectReader&) = delete;
  void operator=(const svtkDataObjectReader&) = delete;
};

#endif
