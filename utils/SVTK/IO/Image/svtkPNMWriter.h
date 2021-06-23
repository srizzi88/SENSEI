/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPNMWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPNMWriter
 * @brief   Writes PNM (portable any map)  files.
 *
 * svtkPNMWriter writes PNM file. The data type
 * of the file is unsigned char regardless of the input type.
 */

#ifndef svtkPNMWriter_h
#define svtkPNMWriter_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageWriter.h"

class SVTKIOIMAGE_EXPORT svtkPNMWriter : public svtkImageWriter
{
public:
  static svtkPNMWriter* New();
  svtkTypeMacro(svtkPNMWriter, svtkImageWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkPNMWriter() {}
  ~svtkPNMWriter() override {}

  void WriteFile(ostream* file, svtkImageData* data, int extent[6], int wExt[6]) override;
  void WriteFileHeader(ostream*, svtkImageData*, int wExt[6]) override;

private:
  svtkPNMWriter(const svtkPNMWriter&) = delete;
  void operator=(const svtkPNMWriter&) = delete;
};

#endif
