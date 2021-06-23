/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBase64OutputStream.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBase64OutputStream
 * @brief   Writes base64-encoded output to a stream.
 *
 * svtkBase64OutputStream implements base64 encoding with the
 * svtkOutputStream interface.
 */

#ifndef svtkBase64OutputStream_h
#define svtkBase64OutputStream_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkOutputStream.h"

class SVTKIOCORE_EXPORT svtkBase64OutputStream : public svtkOutputStream
{
public:
  svtkTypeMacro(svtkBase64OutputStream, svtkOutputStream);
  static svtkBase64OutputStream* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Called after the stream position has been set by the caller, but
   * before any Write calls.  The stream position should not be
   * adjusted by the caller until after an EndWriting call.
   */
  int StartWriting() override;

  /**
   * Write output data of the given length.
   */
  int Write(void const* data, size_t length) override;

  /**
   * Called after all desired calls to Write have been made.  After
   * this call, the caller is free to change the position of the
   * stream.  Additional writes should not be done until after another
   * call to StartWriting.
   */
  int EndWriting() override;

protected:
  svtkBase64OutputStream();
  ~svtkBase64OutputStream() override;

  // Number of un-encoded bytes left in Buffer from last call to Write.
  unsigned int BufferLength;
  unsigned char Buffer[2];

  // Methods to encode and write data.
  int EncodeTriplet(unsigned char c0, unsigned char c1, unsigned char c2);
  int EncodeEnding(unsigned char c0, unsigned char c1);
  int EncodeEnding(unsigned char c0);

private:
  svtkBase64OutputStream(const svtkBase64OutputStream&) = delete;
  void operator=(const svtkBase64OutputStream&) = delete;
};

#endif
