/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTkImageViewerWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTkImageViewerWidget
 * @brief   a Tk Widget for viewing svtk images
 *
 *
 * svtkTkImageViewerWidget is a Tk widget that you can render into. It has a
 * GetImageViewer method that returns a svtkImageViewer. You can also
 * specify a svtkImageViewer to be used when creating the widget by using
 * the -iv option. It also takes -width and -height options.
 * Events can be bound on this widget just like any other Tk widget.
 *
 * @sa
 * svtkImageViewer
 */

#ifndef svtkTkImageViewerWidget_h
#define svtkTkImageViewerWidget_h

#include "svtkImageViewer.h"
#include "svtkTcl.h"
#include "svtkTk.h"
#ifndef SVTK_PYTHON_BUILD
#include "svtkTclUtil.h"
#endif
#include "svtkWindows.h"

struct svtkTkImageViewerWidget
{
  Tk_Window TkWin;    /* Tk window structure */
  Tcl_Interp* Interp; /* Tcl interpreter */
  int Width;
  int Height;
  svtkImageViewer* ImageViewer;
  char* IV;
#ifdef _WIN32
  WNDPROC OldProc;
#endif
};

#endif
// SVTK-HeaderTest-Exclude: svtkTkImageViewerWidget.h
