/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGESignaReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGESignaReader
 * @brief   read GE Signa ximg files
 *
 * svtkGESignaReader is a source object that reads some GE Signa ximg files It
 * does support reading in pixel spacing, slice spacing and it computes an
 * origin for the image in millimeters. It always produces greyscale unsigned
 * short data and it supports reading in rectangular, packed, compressed, and
 * packed&compressed. It does not read in slice orientation, or position
 * right now. To use it you just need to specify a filename or a file prefix
 * and pattern.
 *
 *
 * @sa
 * svtkImageReader2
 */

#ifndef svtkGESignaReader_h
#define svtkGESignaReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkMedicalImageReader2.h"

class SVTKIOIMAGE_EXPORT svtkGESignaReader : public svtkMedicalImageReader2
{
public:
  static svtkGESignaReader* New();
  svtkTypeMacro(svtkGESignaReader, svtkMedicalImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Is the given file a GESigna file?
   */
  int CanReadFile(const char* fname) override;

  /**
   * Valid extentsions
   */
  const char* GetFileExtensions() override { return ".MR .CT"; }

  /**
   * A descriptive name for this format
   */
  const char* GetDescriptiveName() override { return "GESigna"; }

protected:
  svtkGESignaReader() {}
  ~svtkGESignaReader() override {}

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(svtkDataObject* out, svtkInformation* outInfo) override;

private:
  svtkGESignaReader(const svtkGESignaReader&) = delete;
  void operator=(const svtkGESignaReader&) = delete;
};
#endif
