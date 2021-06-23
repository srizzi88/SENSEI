// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNrrdReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkNrrdReader
 * @brief   Read nrrd files file system
 *
 *
 *
 *
 * @bug
 * There are several limitations on what type of nrrd files we can read.  This
 * reader only supports nrrd files in raw or ascii format.  Other encodings
 * like hex will result in errors.  When reading in detached headers, this only
 * supports reading one file that is detached.
 *
 */

#ifndef svtkNrrdReader_h
#define svtkNrrdReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader.h"

class svtkCharArray;

class SVTKIOIMAGE_EXPORT svtkNrrdReader : public svtkImageReader
{
public:
  svtkTypeMacro(svtkNrrdReader, svtkImageReader);
  static svtkNrrdReader* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  int CanReadFile(const char* filename) override;

protected:
  svtkNrrdReader();
  ~svtkNrrdReader() override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int ReadHeaderInternal(svtkCharArray* headerBuffer);
  virtual int ReadHeader();
  virtual int ReadHeader(svtkCharArray* headerBuffer);

  virtual int ReadDataAscii(svtkImageData* output);

  svtkStringArray* DataFiles;

  enum
  {
    ENCODING_RAW,
    ENCODING_ASCII
  };

  int Encoding;

private:
  svtkNrrdReader(const svtkNrrdReader&) = delete;
  void operator=(const svtkNrrdReader&) = delete;
};

#endif // svtkNrrdReader_h
