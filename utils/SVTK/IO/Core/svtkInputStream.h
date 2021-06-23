/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInputStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInputStream
 * @brief   Wraps a binary input stream with a SVTK interface.
 *
 * svtkInputStream provides a SVTK-style interface wrapping around a
 * standard input stream.  The access methods are virtual so that
 * subclasses can transparently provide decoding of an encoded stream.
 * Data lengths for Seek and Read calls refer to the length of the
 * input data.  The actual length in the stream may differ for
 * subclasses that implement an encoding scheme.
 */

#ifndef svtkInputStream_h
#define svtkInputStream_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkObject.h"

class SVTKIOCORE_EXPORT svtkInputStream : public svtkObject
{
public:
  svtkTypeMacro(svtkInputStream, svtkObject);
  static svtkInputStream* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the real input stream.
   */
  svtkSetMacro(Stream, istream*);
  svtkGetMacro(Stream, istream*);
  //@}

  /**
   * Called after the stream position has been set by the caller, but
   * before any Seek or Read calls.  The stream position should not be
   * adjusted by the caller until after an EndReading call.
   */
  virtual void StartReading();

  /**
   * Seek to the given offset in the input data.  Returns 1 for
   * success, 0 for failure.
   */
  virtual int Seek(svtkTypeInt64 offset);

  /**
   * Read input data of the given length.  Returns amount actually
   * read.
   */
  virtual size_t Read(void* data, size_t length);

  /**
   * Called after all desired calls to Seek and Read have been made.
   * After this call, the caller is free to change the position of the
   * stream.  Additional reads should not be done until after another
   * call to StartReading.
   */
  virtual void EndReading();

protected:
  svtkInputStream();
  ~svtkInputStream() override;

  // The real input stream.
  istream* Stream;
  size_t ReadStream(char* data, size_t length);

  // The input stream's position when StartReading was called.
  svtkTypeInt64 StreamStartPosition;

private:
  svtkInputStream(const svtkInputStream&) = delete;
  void operator=(const svtkInputStream&) = delete;
};

#endif
