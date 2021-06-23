/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkASCIITextCodec.h

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
 * @class   svtkASCIITextCodec
 * @brief   Class to read/write ascii text.
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
 * svtkASCIITextCodecFactory
 *
 */

#ifndef svtkASCIITextCodec_h
#define svtkASCIITextCodec_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkTextCodec.h"

class SVTKIOCORE_EXPORT svtkASCIITextCodec : public svtkTextCodec
{
public:
  svtkTypeMacro(svtkASCIITextCodec, svtkTextCodec);
  static svtkASCIITextCodec* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name this codec goes by - should match the string the factory will take to create it
   */
  const char* Name() override;
  bool CanHandle(const char* NameString) override;
  //@}

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
  svtkASCIITextCodec();
  ~svtkASCIITextCodec() override;

private:
  svtkASCIITextCodec(const svtkASCIITextCodec&) = delete;
  void operator=(const svtkASCIITextCodec&) = delete;
};

#endif
