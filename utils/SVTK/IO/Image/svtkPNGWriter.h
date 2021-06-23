/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPNGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPNGWriter
 * @brief   Writes PNG files.
 *
 * svtkPNGWriter writes PNG files. It supports 1 to 4 component data of
 * unsigned char or unsigned short
 *
 * @sa
 * svtkPNGReader
 */

#ifndef svtkPNGWriter_h
#define svtkPNGWriter_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageWriter.h"

class svtkImageData;
class svtkUnsignedCharArray;

class SVTKIOIMAGE_EXPORT svtkPNGWriter : public svtkImageWriter
{
public:
  static svtkPNGWriter* New();
  svtkTypeMacro(svtkPNGWriter, svtkImageWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The main interface which triggers the writer to start.
   */
  void Write() override;

  //@{
  /**
   * Set/Get the zlib compression level.
   * The range is 0-9, with 0 meaning no compression
   * corresponding to the largest file size, and 9 meaning
   * best compression, corresponding to the smallest file size.
   * The default is 5.
   */
  svtkSetClampMacro(CompressionLevel, int, 0, 9);
  svtkGetMacro(CompressionLevel, int);
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

  /**
   * Adds a text chunk to the PNG. More than one text chunk with the same key is permissible.
   * There are a number of predefined keywords that should be used
   * when appropriate. See
   * http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
   * for more information.
   */
  void AddText(const char* key, const char* value);
  //@{
  /**
   * Standard keys
   */
  static const char* TITLE;
  static const char* AUTHOR;
  static const char* DESCRIPTION;
  static const char* COPYRIGHT;
  static const char* CREATION_TIME;
  static const char* SOFTWARE;
  static const char* DISCLAIMER;
  static const char* WARNING;
  static const char* SOURCE;
  static const char* COMMENT;
  //@}

protected:
  svtkPNGWriter();
  ~svtkPNGWriter() override;

  void WriteSlice(svtkImageData* data, int* uExtent);
  int CompressionLevel;
  svtkUnsignedCharArray* Result;
  FILE* TempFP;
  class svtkInternals;
  svtkInternals* Internals;

private:
  svtkPNGWriter(const svtkPNGWriter&) = delete;
  void operator=(const svtkPNGWriter&) = delete;
};

#endif
