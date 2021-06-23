/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOBJReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOBJReader
 * @brief   read Wavefront .obj files
 *
 * svtkOBJReader is a source object that reads Wavefront .obj
 * files. The output of this source object is polygonal data.
 * @sa
 * svtkOBJImporter
 */

#ifndef svtkOBJReader_h
#define svtkOBJReader_h

#include "svtkAbstractPolyDataReader.h"
#include "svtkIOGeometryModule.h" // For export macro

class SVTKIOGEOMETRY_EXPORT svtkOBJReader : public svtkAbstractPolyDataReader
{
public:
  static svtkOBJReader* New();
  svtkTypeMacro(svtkOBJReader, svtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get first comment in the file.
   * Comment may be multiple lines. # and leading spaces are removed.
   */
  svtkGetStringMacro(Comment);
  //@}

protected:
  svtkOBJReader();
  ~svtkOBJReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set comment string. Internal use only.
   */
  svtkSetStringMacro(Comment);

  char* Comment;

private:
  svtkOBJReader(const svtkOBJReader&) = delete;
  void operator=(const svtkOBJReader&) = delete;
};

#endif
