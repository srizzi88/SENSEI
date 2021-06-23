/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkUTF16TextCodec.h

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
 * @class   svtkUTF16TextCodec
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
 * svtkUTF16TextCodecFactory
 *
 */

#ifndef svtkUTF16TextCodec_h
#define svtkUTF16TextCodec_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkTextCodec.h"

class SVTKIOCORE_EXPORT svtkUTF16TextCodec : public svtkTextCodec
{
public:
  svtkTypeMacro(svtkUTF16TextCodec, svtkTextCodec);
  static svtkUTF16TextCodec* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name this codec goes by - should match the string the factory will take to create it
   */
  const char* Name() override;
  bool CanHandle(const char* NameString) override;
  //@}

  /**
   * Set the endianness - true if Big false is little
   */
  void SetBigEndian(bool);

  /**
   * Set the endianness - true if Big false is little
   */
  void FindEndianness(istream& InputStream);

  /**
   * is the given sample valid for this codec? - will take endianness into account
   */
  bool IsValid(istream& InputStream) override;

  /**
   * Iterate through the sequence represented by the begin and end iterators assigning the result
   * to the output iterator.  This is the current pattern in svtkDelimitedTextReader
   */
  void ToUnicode(istream& InputStream, svtkTextCodec::OutputIterator& output) override;

  /**
   * Return the next code point from the sequence represented by the begin, end iterators
   * advancing begin through however many places needed to assemble that code point
   */
  svtkUnicodeString::value_type NextUnicode(istream& inputStream) override;

protected:
  svtkUTF16TextCodec();
  ~svtkUTF16TextCodec() override;

  bool _endianExplicitlySet;
  bool _bigEndian;

private:
  svtkUTF16TextCodec(const svtkUTF16TextCodec&) = delete;
  void operator=(const svtkUTF16TextCodec&) = delete;
};

#endif
