/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractPolyDataReader
 * @brief   Superclass for algorithms that read
 * models from a file.
 *
 * This class allows to use a single base class to manage AbstractPolyData
 * reader classes in a uniform manner without needing to know the actual
 * type of the reader.
 * i.e. makes it possible to create maps to associate filename extension
 * and svtkAbstractPolyDataReader object.
 *
 * @sa
 * svtkOBJReader svtkPLYReader svtkSTLReader
 */

#ifndef svtkAbstractPolyDataReader_h
#define svtkAbstractPolyDataReader_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKIOCORE_EXPORT svtkAbstractPolyDataReader : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkAbstractPolyDataReader, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of AbstractPolyData file (obj / ply / stl).
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

protected:
  svtkAbstractPolyDataReader();
  ~svtkAbstractPolyDataReader() override;

  char* FileName;

private:
  svtkAbstractPolyDataReader(const svtkAbstractPolyDataReader&) = delete;
  void operator=(const svtkAbstractPolyDataReader&) = delete;
};

#endif
