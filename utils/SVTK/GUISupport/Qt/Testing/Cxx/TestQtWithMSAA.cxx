/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtWithMSAA.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests QSVTKOpenGLStereoWidget/QSVTKOpenGLNativeWidget/QSVTKOpenGLWindow with MSAA
#include "TestQtCommon.h"
#include "svtkActor.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTesting.h"

#include <QApplication>
#include <QSurfaceFormat>

int TestQtWithMSAA(int argc, char* argv[])
{
  // disable multisampling globally.
  svtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);

  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);

  svtkNew<svtkTesting> svtktesting;
  svtktesting->AddArguments(argc, argv);

  svtkNew<svtkGenericOpenGLRenderWindow> window;
  window->SetMultiSamples(8); // enable multisampling

  auto widgetOrWindow = detail::create_widget_or_window(type, window);

  svtkNew<svtkRenderer> ren;
  ren->SetGradientBackground(1);
  ren->SetBackground2(0.7, 0.7, 0.7);
  window->AddRenderer(ren);

  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  ren->AddActor(actor);

  detail::show(widgetOrWindow, QSize(300, 300));

  svtktesting->SetRenderWindow(window);

  int retVal = svtktesting->RegressionTest(10);
  switch (retVal)
  {
    case svtkTesting::DO_INTERACTOR:
      return app.exec();
    case svtkTesting::FAILED:
    case svtkTesting::NOT_RUN:
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
