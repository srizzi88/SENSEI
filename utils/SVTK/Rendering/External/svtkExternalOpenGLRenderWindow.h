/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExternalOpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExternalOpenGLRenderWindow
 * @brief   OpenGL render window that allows using
 * an external window to render svtk objects
 *
 * svtkExternalOpenGLRenderWindow is a concrete implementation of the abstract
 * class svtkRenderWindow. svtkExternalOpenGLRenderer interfaces to the OpenGL
 * graphics library.
 *
 * This class extends svtkGenericOpenGLRenderWindow to allow sharing the
 * same OpenGL context by various visualization applications. Basically, this
 * class prevents SVTK from creating a new OpenGL context. Thus, it requires that
 * an OpenGL context be initialized before Render is called.
 * \sa Render()
 *
 * It is a generic implementation; this window is platform agnostic. However,
 * the application user must explicitly make sure the window size is
 * synchronized when the external application window/viewport resizes.
 * \sa SetSize()
 *
 * It has the same requirements as the svtkGenericOpenGLRenderWindow, whereby,
 * one must register an observer for WindowMakeCurrentEvent,
 * WindowIsCurrentEvent and WindowFrameEvent.
 * \sa svtkGenericOpenGLRenderWindow
 */

#ifndef svtkExternalOpenGLRenderWindow_h
#define svtkExternalOpenGLRenderWindow_h

#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkRenderingExternalModule.h" // For export macro

class SVTKRENDERINGEXTERNAL_EXPORT svtkExternalOpenGLRenderWindow
  : public svtkGenericOpenGLRenderWindow
{
public:
  static svtkExternalOpenGLRenderWindow* New();
  svtkTypeMacro(svtkExternalOpenGLRenderWindow, svtkGenericOpenGLRenderWindow);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Begin the rendering process using the existing context.
   */
  void Start(void) override;

  /**
   * Tells if this window is the current graphics context for the calling
   * thread.
   */
  bool IsCurrent() override;

  //@{
  /**
   * Turn on/off a flag which enables/disables automatic positioning and
   * resizing of the render window. By default, svtkExternalOpenGLRenderWindow
   * queries the viewport position and size (glViewport) from the OpenGL state
   * and uses it to resize itself. However, in special circumstances this
   * feature is undesirable. One such circumstance may be to avoid performance
   * penalty of querying OpenGL state variables. So the following boolean is
   * provided to disable automatic window resize.
   * (Turn AutomaticWindowPositionAndResize off if you do not want the viewport
   * to be queried from the OpenGL state.)
   */
  svtkGetMacro(AutomaticWindowPositionAndResize, int);
  svtkSetMacro(AutomaticWindowPositionAndResize, int);
  svtkBooleanMacro(AutomaticWindowPositionAndResize, int);
  //@}

  //@{
  /**
   * Turn on/off a flag which enables/disables using the content from an
   * outside applicaiton.  When on the active read buffer is first blitted
   * into SVTK and becomes the starting poiint for SVTK's rendering.
   */
  svtkGetMacro(UseExternalContent, bool);
  svtkSetMacro(UseExternalContent, bool);
  svtkBooleanMacro(UseExternalContent, bool);
  //@}

protected:
  svtkExternalOpenGLRenderWindow();
  ~svtkExternalOpenGLRenderWindow() override;

  int AutomaticWindowPositionAndResize;
  bool UseExternalContent;

private:
  svtkExternalOpenGLRenderWindow(const svtkExternalOpenGLRenderWindow&) = delete;
  void operator=(const svtkExternalOpenGLRenderWindow&) = delete;
};
#endif // svtkExternalOpenGLRenderWindow_h
