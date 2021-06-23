/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkCocoaTkUtilities.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCocoaTkUtilities
 * @brief   Internal Tk Routines for Cocoa
 *
 *
 * svtkCocoaTkUtilities provide access to the Tk internals for Cocoa
 * implementations of Tk.  These internals must be implemented in a .mm
 * file, since Cocoa is Objective-C, but the header file itself is
 * pure C++ so that it can be included by other SVTK classes.
 *
 * @sa
 * svtkCocoaGLView
 *
 * @warning
 * This header must be in C++ only because it is included by .cxx files.
 * That means no Objective-C may be used. That's why some instance variables
 * are void* instead of what they really should be.
 */

#ifndef svtkCocoaTkUtilities_h
#define svtkCocoaTkUtilities_h

#include "svtkObject.h"

struct Tk_Window_;

class svtkCocoaTkUtilities : public svtkObject
{
public:
  static svtkCocoaTkUtilities* New();
  svtkTypeMacro(svtkCocoaTkUtilities, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the NSView for a Tk_Window.  It is returned as a void pointer
   * so that users of this function don't need to compile as Objective C.
   */
  static void* GetDrawableView(Tk_Window_* window);

protected:
  svtkCocoaTkUtilities() {}
  ~svtkCocoaTkUtilities() override {}

private:
  svtkCocoaTkUtilities(const svtkCocoaTkUtilities&) = delete;
  void operator=(const svtkCocoaTkUtilities&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkCocoaTkUtilities.h
