/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExternalSVTKWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME ExternalSVTKWidget - Use SVTK rendering in an external window/application
// .SECTION Description
// ExternalSVTKWidget provides an easy way to render SVTK objects in an external
// environment using the SVTK rendering framework without drawing a new window.

#ifndef __ExternalSVTKWidget_h
#define __ExternalSVTKWidget_h

#include "svtkExternalOpenGLRenderWindow.h"
#include "svtkExternalOpenGLRenderer.h"
#include "svtkObject.h"
#include "svtkRenderingExternalModule.h" // For export macro

// Class that maintains an external render window.
class SVTKRENDERINGEXTERNAL_EXPORT ExternalSVTKWidget : public svtkObject
{
public:
  static ExternalSVTKWidget* New();
  svtkTypeMacro(ExternalSVTKWidget, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Set/Get an external render window for the ExternalSVTKWidget.
  // Since this is a special environment, the methods are limited to use
  // svtkExternalOpenGLRenderWindow only.
  // \sa svtkExternalOpenGLRenderWindow
  svtkExternalOpenGLRenderWindow* GetRenderWindow(void);
  void SetRenderWindow(svtkExternalOpenGLRenderWindow* renWin);

  // Creates a new renderer and adds it to the render window.
  // Returns a handle to the created renderer.
  // NOTE: To get a list of renderers, one must go through the RenderWindow API
  // i.e. ExternalSVTKWidget->GetRenderWindow()->GetRenderers()
  // \sa svtkRenderWindow::GetRenderers()
  svtkExternalOpenGLRenderer* AddRenderer();

protected:
  ExternalSVTKWidget();
  ~ExternalSVTKWidget() override;

  svtkExternalOpenGLRenderWindow* RenderWindow;

private:
  ExternalSVTKWidget(const ExternalSVTKWidget&) = delete;
  void operator=(const ExternalSVTKWidget&) = delete;
};

#endif //__ExternalSVTKWidget_h
