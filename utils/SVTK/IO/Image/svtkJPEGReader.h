/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkJPEGReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkJPEGReader
 * @brief   read JPEG files
 *
 * svtkJPEGReader is a source object that reads JPEG files.
 * The reader can also read an image from a memory buffer,
 * see svtkImageReader2::MemoryBuffer.
 * It should be able to read most any JPEG file.
 *
 * @sa
 * svtkJPEGWriter
 */

#ifndef svtkJPEGReader_h
#define svtkJPEGReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader2.h"

class SVTKIOIMAGE_EXPORT svtkJPEGReader : public svtkImageReader2
{
public:
  static svtkJPEGReader* New();
  svtkTypeMacro(svtkJPEGReader, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Is the given file a JPEG file?
   */
  int CanReadFile(const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override { return ".jpeg .jpg"; }

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "JPEG"; }

protected:
  svtkJPEGReader() {}
  ~svtkJPEGReader() override {}

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(svtkDataObject* out, svtkInformation* outInfo) override;

private:
  svtkJPEGReader(const svtkJPEGReader&) = delete;
  void operator=(const svtkJPEGReader&) = delete;
};
#endif
