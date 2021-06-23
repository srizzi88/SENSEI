/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTkWidgetsInit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTcl.h"
#include "svtkTk.h"

#include "svtkImageData.h"
#include "svtkTkImageViewerWidget.h"
#include "svtkTkRenderWidget.h"
#include "svtkVersionMacros.h"

//----------------------------------------------------------------------------
// Vtkrenderingtk_Init
// Called upon system startup to create the widget commands.
extern "C"
{
  SVTK_EXPORT int Vtkrenderingtk_Init(Tcl_Interp* interp);
}

extern "C"
{
  SVTK_EXPORT int Vtktkrenderwidget_Init(Tcl_Interp* interp);
}
extern "C"
{
  SVTK_EXPORT int Vtktkimageviewerwidget_Init(Tcl_Interp* interp);
}

#define SVTKTK_TO_STRING(x) SVTKTK_TO_STRING0(x)
#define SVTKTK_TO_STRING0(x) SVTKTK_TO_STRING1(x)
#define SVTKTK_TO_STRING1(x) #x
#define SVTKTK_VERSION SVTKTK_TO_STRING(SVTK_MAJOR_VERSION) "." SVTKTK_TO_STRING(SVTK_MINOR_VERSION)

int Vtkrenderingtk_Init(Tcl_Interp* interp)
{
  // Forward the call to the real init functions.
  if (Vtktkrenderwidget_Init(interp) == TCL_OK && Vtktkimageviewerwidget_Init(interp) == TCL_OK)
  {
    // Report that the package is provided.
    return Tcl_PkgProvide(interp, (char*)"Vtkrenderingtk", (char*)SVTKTK_VERSION);
  }
  else
  {
    // One of the widgets is not provided.
    return TCL_ERROR;
  }
}
