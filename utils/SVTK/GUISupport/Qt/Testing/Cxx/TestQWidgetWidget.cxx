/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "TestQtCommon.h"

#include <QApplication>
#include <QPushButton>

#include "svtkNew.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPlaneSource.h"
#include "svtkQWidgetRepresentation.h"
#include "svtkQWidgetWidget.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

int TestQWidgetWidget(int argc, char* argv[])
{
  // disable multisampling.
  svtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);

  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);
  QPushButton hello("Hello world!", 0);

  svtkNew<svtkTesting> svtktesting;
  svtktesting->AddArguments(argc, argv);

  auto widgetOrWindow = detail::create_widget_or_window(type, nullptr);
  svtkNew<svtkGenericOpenGLRenderWindow> window0;
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.2, 0.3, 0.4);
  window0->AddRenderer(renderer);
  detail::set_render_window(widgetOrWindow, window0);
  detail::show(widgetOrWindow, QSize(300, 300));
  detail::process_events_and_wait(100);
  window0->Render();

  svtkNew<svtkQWidgetWidget> widget;
  widget->CreateDefaultRepresentation();
  widget->GetQWidgetRepresentation()->GetPlaneSource()->SetPoint2(-0.5, 0.5, -0.5);
  widget->SetWidget(&hello);
  widget->SetCurrentRenderer(renderer);
  widget->SetInteractor(window0->GetInteractor());

  widget->SetEnabled(1);
  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();
  window0->Render();
  detail::process_events_and_wait(100);
  svtktesting->SetRenderWindow(window0);
  renderer->ResetCamera();
  window0->Render();
  detail::process_events_and_wait(100);
  window0->Render();

  // return app.exec();

  // clear the widget first, to avoid using it
  // after it may have been freed.
  widget->SetWidget(nullptr);

  return EXIT_SUCCESS;
}
