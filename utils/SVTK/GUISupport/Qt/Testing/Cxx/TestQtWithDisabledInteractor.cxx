/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQSVTKOpenGLNativeWidgetWithDisabledInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests QSVTKOpenGLNativeWidget with a svtkRenderWindowInteractor that has its
// EnableRender flag disabled.
#include "TestQtCommon.h"
#include "svtkActor.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTesting.h"

#include <QImage>

int TestQtWithDisabledInteractor(int argc, char* argv[])
{
  // Disable multisampling
  svtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);

  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);

  auto svtktesting = svtkSmartPointer<svtkTesting>::New();
  svtktesting->AddArguments(argc, argv);

  auto widgetOrWindow = detail::create_widget_or_window(type, nullptr);

  auto renWin = detail::get_render_window(widgetOrWindow);
  auto ren = svtkSmartPointer<svtkRenderer>::New();
  ren->GradientBackgroundOn();
  ren->SetBackground2(0.7, 0.7, 0.7);
  renWin->AddRenderer(ren);
  renWin->Render();

  detail::show(widgetOrWindow, QSize(100, 100));

  // Set interactor to not call Render() on the svtkRenderWindow. Clients might
  // set this to enforce a specified framerate by rendering only when a timer
  // fires, for example.
  renWin->GetInteractor()->EnableRenderOff();

  auto source = svtkSmartPointer<svtkSphereSource>::New();
  auto mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(source->GetOutputPort());
  auto actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  ren->ResetCamera();
  renWin->Render(); // this will render a sphere at 100x100.

  // Resize widget. this should to retrigger a SVTK render since
  // the interactor is disabled. We should still see the rendering result from
  // earlier.
  detail::show(widgetOrWindow, QSize(300, 300));

  // Get output image filename
  const std::string tempDir(svtktesting->GetTempDirectory());
  std::string fileName(svtktesting->GetValidImageFileName());
  auto slashPos = fileName.rfind('/');
  if (slashPos != std::string::npos)
  {
    fileName = fileName.substr(slashPos + 1);
  }
  fileName = tempDir + '/' + fileName;

  // Capture widget using Qt. Don't use svtkTesting to capture the image, because
  // this should test what the widget displays, not what SVTK renders.
  const QImage image = detail::grab_framebuffer(widgetOrWindow);
  if (!image.save(QString::fromStdString(fileName)))
  {
    std::cout << "ERROR: Saving image failed" << std::endl;
    return EXIT_FAILURE;
  }

  int retVal = svtktesting->RegressionTest(fileName, 0);
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
