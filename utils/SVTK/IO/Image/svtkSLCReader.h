/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSLCReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkSLCReader
 * @brief   read an SLC volume file.
 *
 * svtkSLCReader reads an SLC file and creates a structured point dataset.
 * The size of the volume and the data spacing is set from the SLC file
 * header.
 */

#ifndef svtkSLCReader_h
#define svtkSLCReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader2.h"

class SVTKIOIMAGE_EXPORT svtkSLCReader : public svtkImageReader2
{
public:
  static svtkSLCReader* New();
  svtkTypeMacro(svtkSLCReader, svtkImageReader2);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Was there an error on the last read performed?
   */
  svtkGetMacro(Error, int);
  //@}

  /**
   * Is the given file an SLC file?
   */
  int CanReadFile(const char* fname) override;
  /**
   * .slc
   */
  const char* GetFileExtensions() override { return ".slc"; }

  /**
   * SLC
   */
  const char* GetDescriptiveName() override { return "SLC"; }

protected:
  svtkSLCReader();
  ~svtkSLCReader() override;

  // Reads the file name and builds a svtkStructuredPoints dataset.
  void ExecuteDataWithInformation(svtkDataObject*, svtkInformation*) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // Decodes an array of eight bit run-length encoded data.
  unsigned char* Decode8BitData(unsigned char* in_ptr, int size);
  int Error;

private:
  svtkSLCReader(const svtkSLCReader&) = delete;
  void operator=(const svtkSLCReader&) = delete;
};

#endif
