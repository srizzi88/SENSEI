/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTableReader
 * @brief   read svtkTable data file
 *
 * svtkTableReader is a source object that reads ASCII or binary
 * svtkTable data files in svtk format. (see text for format details).
 * The output of this reader is a single svtkTable data object.
 * The superclass of this class, svtkDataReader, provides many methods for
 * controlling the reading of the data file, see svtkDataReader for more
 * information.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * @sa
 * svtkTable svtkDataReader svtkTableWriter
 */

#ifndef svtkTableReader_h
#define svtkTableReader_h

#include "svtkDataReader.h"
#include "svtkIOLegacyModule.h" // For export macro

class svtkTable;

class SVTKIOLEGACY_EXPORT svtkTableReader : public svtkDataReader
{
public:
  static svtkTableReader* New();
  svtkTypeMacro(svtkTableReader, svtkDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkTable* GetOutput();
  svtkTable* GetOutput(int idx);
  void SetOutput(svtkTable* output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkTableReader();
  ~svtkTableReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;

private:
  svtkTableReader(const svtkTableReader&) = delete;
  void operator=(const svtkTableReader&) = delete;
};

#endif
