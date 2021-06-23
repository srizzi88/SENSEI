/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHardwareWindow
 * @brief   create a window for renderers to draw into
 *
 * svtkHardwareWindow is an abstract object representing a UI based
 * window that can be drawn to. This class is defines an interface that
 * GUI specific subclasses (Win32, X, Cocoa) should support.
 *
 * This class is meant to be Graphics library agnostic. In that it should
 * contain as little graphics library specific code as possible, ideally none.
 * In contrast to classes such as svtkWinOpenGLRenderWindow which contain
 * significant ties to OpenGL.
 *
 */

#ifndef svtkHardwareWindow_h
#define svtkHardwareWindow_h

#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkWindow.h"

class SVTKRENDERINGCORE_EXPORT svtkHardwareWindow : public svtkWindow
{
public:
  static svtkHardwareWindow* New();
  svtkTypeMacro(svtkHardwareWindow, svtkWindow);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // create the window (not the instance)
  virtual void Create(){};

  // destroy the window (not the instance)
  virtual void Destroy(){};

protected:
  svtkHardwareWindow();
  ~svtkHardwareWindow() override;

  bool Borders;

private:
  svtkHardwareWindow(const svtkHardwareWindow&) = delete;
  void operator=(const svtkHardwareWindow&) = delete;
};

#endif
