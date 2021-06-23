/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTkRenderWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTkRenderWidget
 * @brief   a Tk Widget for svtk rendering
 *
 *
 * svtkTkRenderWidget is a Tk widget that you can render into. It has a
 * GetRenderWindow method that returns a svtkRenderWindow. This can then
 * be used to create a svtkRenderer and etc. You can also specify a
 * svtkRenderWindow to be used when creating the widget by using
 * the -rw option. It also takes -width and -height options.
 * Events can be bound on this widget just like any other Tk widget.
 *
 * @sa
 * svtkRenderWindow svtkRenderer
 */

#ifndef svtkTkRenderWidget_h
#define svtkTkRenderWidget_h

#include "svtkRenderWindow.h"
#include "svtkTcl.h"
#include "svtkWindows.h"

// For the moment, we are not compatible w/Photo compositing
// By defining USE_COMPOSITELESS_PHOTO_PUT_BLOCK, we use the compatible
// call.
#define USE_COMPOSITELESS_PHOTO_PUT_BLOCK
#include "svtkTk.h"

#ifndef SVTK_PYTHON_BUILD
#include "svtkTclUtil.h"
#endif

struct svtkTkRenderWidget
{
  Tk_Window TkWin;    /* Tk window structure */
  Tcl_Interp* Interp; /* Tcl interpreter */
  int Width;
  int Height;
  svtkRenderWindow* RenderWindow;
  char* RW;
#ifdef _WIN32
  WNDPROC OldProc;
#endif
};

#endif
// SVTK-HeaderTest-Exclude: svtkTkRenderWidget.h
