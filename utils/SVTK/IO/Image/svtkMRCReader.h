/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMRCReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMRCReader
 * @brief   read MRC image files
 *
 *
 * A reader to load MRC images.  See http://bio3d.colorado.edu/imod/doc/mrc_format.txt
 * for the file format specification.
 */

#ifndef svtkMRCReader_h
#define svtkMRCReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class svtkInformation;
class svtkInformationVector;

class SVTKIOIMAGE_EXPORT svtkMRCReader : public svtkImageAlgorithm
{
public:
  static svtkMRCReader* New();
  svtkTypeMacro(svtkMRCReader, svtkImageAlgorithm);

  void PrintSelf(ostream& stream, svtkIndent indent) override;

  // .Description
  // Get/Set the file to read
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

protected:
  svtkMRCReader();
  ~svtkMRCReader() override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  void ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo) override;

  char* FileName;

private:
  svtkMRCReader(const svtkMRCReader&) = delete;
  void operator=(const svtkMRCReader&) = delete;
  class svtkInternal;
  svtkInternal* Internals;
};

#endif
