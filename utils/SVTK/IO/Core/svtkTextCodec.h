/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkTextCodec.h

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
 * @class   svtkTextCodec
 * @brief   Virtual class to act as an interface for all text codecs
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
 * svtkTextCodecFactory
 *
 */

#ifndef svtkTextCodec_h
#define svtkTextCodec_h

#include "svtkIOCoreModule.h" // For export macro
#include "svtkObject.h"
#include "svtkUnicodeString.h" // for the value type and for function return.

class SVTKIOCORE_EXPORT svtkTextCodec : public svtkObject
{
public:
  svtkTypeMacro(svtkTextCodec, svtkObject);

  //@{
  /**
   * The name this codec goes by - should match the string the factory will take
   * to create it
   */
  virtual const char* Name();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  virtual bool CanHandle(const char* NameString);

  /**
   * is the given sample valid for this codec?  The stream will not be advanced.
   */
  virtual bool IsValid(istream& InputStream);

  //@{
  /**
   * a base class that any output iterators need to derive from to use the first
   * signature of to_unicode.  Templates will not allow the vtable to
   * re-reference to the correct class so even though we only need the interface
   * we have to use derivation.
   */
  class OutputIterator
  {
  public:
    virtual OutputIterator& operator++(int) = 0;
    virtual OutputIterator& operator*() = 0;
    virtual OutputIterator& operator=(const svtkUnicodeString::value_type value) = 0;
    //@}

    OutputIterator() {}
    virtual ~OutputIterator() {}

  private:
    OutputIterator(const OutputIterator&) = delete;
    OutputIterator& operator=(const OutputIterator&) = delete;
  };

  /**
   * Iterate through the sequence represented by the stream assigning the result
   * to the output iterator.  The stream will be advanced to its end so
   * subsequent use would need to reset it.
   */
  virtual void ToUnicode(istream& InputStream, svtkTextCodec::OutputIterator& output) = 0;

  /**
   * convenience method to take data from the stream and put it into a
   * svtkUnicodeString.
   */
  svtkUnicodeString ToUnicode(istream& inputStream);

  /**
   * Return the next code point from the sequence represented by the stream
   * advancing the stream through however many places needed to assemble that
   * code point.
   */
  virtual svtkUnicodeString::value_type NextUnicode(istream& inputStream) = 0;

protected:
  svtkTextCodec();
  ~svtkTextCodec() override;

private:
  svtkTextCodec(const svtkTextCodec&) = delete;
  void operator=(const svtkTextCodec&) = delete;
};

#endif
