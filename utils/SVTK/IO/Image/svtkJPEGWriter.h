/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkJPEGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkJPEGWriter
 * @brief   Writes JPEG files.
 *
 * svtkJPEGWriter writes JPEG files. It supports 1 and 3 component data of
 * unsigned char. It relies on the IJG's libjpeg.  Thanks to IJG for
 * supplying a public jpeg IO library.
 *
 * @sa
 * svtkJPEGReader
 */

#ifndef svtkJPEGWriter_h
#define svtkJPEGWriter_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageWriter.h"

class svtkUnsignedCharArray;
class svtkImageData;

class SVTKIOIMAGE_EXPORT svtkJPEGWriter : public svtkImageWriter
{
public:
  static svtkJPEGWriter* New();
  svtkTypeMacro(svtkJPEGWriter, svtkImageWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The main interface which triggers the writer to start.
   */
  void Write() override;

  //@{
  /**
   * Compression quality. 0 = Low quality, 100 = High quality
   */
  svtkSetClampMacro(Quality, int, 0, 100);
  svtkGetMacro(Quality, int);
  //@}

  //@{
  /**
   * Progressive JPEG generation.
   */
  svtkSetMacro(Progressive, svtkTypeUBool);
  svtkGetMacro(Progressive, svtkTypeUBool);
  svtkBooleanMacro(Progressive, svtkTypeUBool);
  //@}

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
   * When writing to memory this is the result, it will be nullptr until the
   * data is written the first time
   */
  virtual void SetResult(svtkUnsignedCharArray*);
  svtkGetObjectMacro(Result, svtkUnsignedCharArray);
  //@}

protected:
  svtkJPEGWriter();
  ~svtkJPEGWriter() override;

  void WriteSlice(svtkImageData* data, int* uExtent);

private:
  int Quality;
  svtkTypeUBool Progressive;
  svtkUnsignedCharArray* Result;
  FILE* TempFP;

private:
  svtkJPEGWriter(const svtkJPEGWriter&) = delete;
  void operator=(const svtkJPEGWriter&) = delete;
};

#endif
