/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBMPWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBMPWriter
 * @brief   Writes Windows BMP files.
 *
 * svtkBMPWriter writes BMP files. The data type
 * of the file is unsigned char regardless of the input type.
 *
 * @sa
 * svtkBMPReader
 */

#ifndef svtkBMPWriter_h
#define svtkBMPWriter_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageWriter.h"

class svtkUnsignedCharArray;

class SVTKIOIMAGE_EXPORT svtkBMPWriter : public svtkImageWriter
{
public:
  static svtkBMPWriter* New();
  svtkTypeMacro(svtkBMPWriter, svtkImageWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Write the image to memory (a svtkUnsignedCharArray)
   */
  svtkSetMacro(WriteToMemory, svtkTypeUBool);
  svtkGetMacro(WriteToMemory, svtkTypeUBool);
  svtkBooleanMacro(WriteToMemory, svtkTypeUBool);
  //@}

  //@{
  /**
   * When writing to memory this is the result, it will be NULL until the
   * data is written the first time
   */
  virtual void SetResult(svtkUnsignedCharArray*);
  svtkGetObjectMacro(Result, svtkUnsignedCharArray);
  //@}

protected:
  svtkBMPWriter();
  ~svtkBMPWriter() override;

  void WriteFile(ostream* file, svtkImageData* data, int ext[6], int wExt[6]) override;
  void WriteFileHeader(ostream*, svtkImageData*, int wExt[6]) override;
  void MemoryWrite(int, svtkImageData*, int wExt[6], svtkInformation* inInfo) override;

private:
  svtkBMPWriter(const svtkBMPWriter&) = delete;
  void operator=(const svtkBMPWriter&) = delete;

  svtkUnsignedCharArray* Result;
};

#endif
