/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOutputStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOutputStream
 * @brief   Wraps a binary output stream with a SVTK interface.
 *
 * svtkOutputStream provides a SVTK-style interface wrapping around a
 * standard output stream.  The access methods are virtual so that
 * subclasses can transparently provide encoding of the output.  Data
 * lengths for Write calls refer to the length of the data in memory.
 * The actual length in the stream may differ for subclasses that
 * implement an encoding scheme.
 */

#ifndef svtkOutputStream_h
#define svtkOutputStream_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkObject.h"

class SVTKIOCORE_EXPORT svtkOutputStream : public svtkObject
{
public:
  svtkTypeMacro(svtkOutputStream, svtkObject);
  static svtkOutputStream* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the real output stream.
   */
  svtkSetMacro(Stream, ostream*);
  svtkGetMacro(Stream, ostream*);
  //@}

  /**
   * Called after the stream position has been set by the caller, but
   * before any Write calls.  The stream position should not be
   * adjusted by the caller until after an EndWriting call.
   */
  virtual int StartWriting();

  /**
   * Write output data of the given length.
   */
  virtual int Write(void const* data, size_t length);

  /**
   * Called after all desired calls to Write have been made.  After
   * this call, the caller is free to change the position of the
   * stream.  Additional writes should not be done until after another
   * call to StartWriting.
   */
  virtual int EndWriting();

protected:
  svtkOutputStream();
  ~svtkOutputStream() override;

  // The real output stream.
  ostream* Stream;
  int WriteStream(const char* data, size_t length);

private:
  svtkOutputStream(const svtkOutputStream&) = delete;
  void operator=(const svtkOutputStream&) = delete;
};

#endif
