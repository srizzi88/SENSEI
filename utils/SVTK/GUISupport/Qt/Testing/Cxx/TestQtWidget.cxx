/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests QSVTKOpenGLStereoWidget/QSVTKOpenGLWindow/QSVTKOpenGLNativeWidget

#include "TestQtCommon.h"

#include "svtkActor.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTesting.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <qmainwindow.h>

int TestQtWidget(int argc, char* argv[])
{
  // disable multisampling.
  svtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);

  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);

  svtkNew<svtkTesting> svtktesting;
  svtktesting->AddArguments(argc, argv);

  auto widgetOrWindow = detail::create_widget_or_window(type, nullptr);
  {
    svtkNew<svtkGenericOpenGLRenderWindow> window0;
    detail::set_render_window(widgetOrWindow, window0);
    detail::show(widgetOrWindow, QSize(200, 200));
  }

  // make sure rendering works correctly after switching to a new render window
  svtkNew<svtkGenericOpenGLRenderWindow> window;
  detail::set_render_window(widgetOrWindow, window);

  svtkNew<svtkRenderer> ren;
  ren->SetGradientBackground(1);
  ren->SetBackground2(0.7, 0.7, 0.7);
  window->AddRenderer(ren);

  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  ren->AddActor(actor);

  detail::show(widgetOrWindow, QSize(300, 300));
  detail::process_events_and_wait(1000); // let's wait a little longer for the resize

  const int* windowSize = window->GetSize();
  const int* screenSize = window->GetScreenSize();
  if (screenSize[0] < windowSize[0] || screenSize[1] < windowSize[1])
  {
    std::cout << "Expected svtkGenericOpenGLRenderWindow::GetScreenSize() "
                 "dimensions to be larger than the render window size"
              << std::endl;
    return EXIT_FAILURE;
  }

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
