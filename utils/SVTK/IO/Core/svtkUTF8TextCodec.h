/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkUTF8TextCodec.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkUTF8TextCodec
 * @brief   Class to read/write UTF-8 text
 *
 *
 * A virtual class interface for codecs that readers/writers can rely on
 *
 * @par Thanks:
 * Thanks to Tim Shed from Sandia National Laboratories for his work
 * on the concepts and to Marcus Hanwell and Jeff Baumes of Kitware for
 * keeping me out of the weeds
 *
 * @sa
 * svtkUTF8TextCodecFactory
 *
 */

#ifndef svtkUTF8TextCodec_h
#define svtkUTF8TextCodec_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkTextCodec.h"

class SVTKIOCORE_EXPORT svtkUTF8TextCodec : public svtkTextCodec
{
public:
  svtkTypeMacro(svtkUTF8TextCodec, svtkTextCodec);
  static svtkUTF8TextCodec* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * The name this codec goes by - should match the string the factory will take to create it
   */
  const char* Name() override { return "UTF-8"; }
  bool CanHandle(const char* testStr) override;

  /**
   * is the given sample valid for this codec?
   */
  bool IsValid(istream& InputStream) override;

  /**
   * Iterate through the sequence represented by the stream assigning the result
   * to the output iterator.  The stream will be advanced to its end so subsequent use
   * would need to reset it.
   */
  void ToUnicode(istream& InputStream, svtkTextCodec::OutputIterator& output) override;

  /**
   * Return the next code point from the sequence represented by the stream
   * advancing the stream through however many places needed to assemble that code point
   */
  svtkUnicodeString::value_type NextUnicode(istream& inputStream) override;

protected:
  svtkUTF8TextCodec();
  ~svtkUTF8TextCodec() override;

private:
  svtkUTF8TextCodec(const svtkUTF8TextCodec&) = delete;
  void operator=(const svtkUTF8TextCodec&) = delete;
};

#endif
