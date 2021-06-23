/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenSlideReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenSlideReader
 * @brief   read digital whole slide images supported by
 * openslide library
 *
 * svtkOpenSlideReader is a source object that uses openslide library to
 * read multiple supported image formats used for whole slide images in
 * microscopy community.
 *
 * @sa
 * svtkPTIFWriter
 */

#ifndef svtkOpenSlideReader_h
#define svtkOpenSlideReader_h

#include "svtkDomainsMicroscopyModule.h" // For export macro
#include "svtkImageReader2.h"

extern "C"
{
#include "openslide/openslide.h" // For openslide support
}

class SVTKDOMAINSMICROSCOPY_EXPORT svtkOpenSlideReader : public svtkImageReader2
{
public:
  static svtkOpenSlideReader* New();
  svtkTypeMacro(svtkOpenSlideReader, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Is the given file supported ?
   */
  int CanReadFile(const char* fname) override;

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions() override
  {
    return ".ndpi .svs"; // TODO: Get exaustive list of formats
  }

  //@{
  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "Openslide::WholeSlideImage"; }

protected:
  svtkOpenSlideReader() {}
  ~svtkOpenSlideReader() override;
  //@}

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(svtkDataObject* out, svtkInformation* outInfo) override;

private:
  openslide_t* openslide_handle;

  svtkOpenSlideReader(const svtkOpenSlideReader&) = delete;
  void operator=(const svtkOpenSlideReader&) = delete;
};
#endif
