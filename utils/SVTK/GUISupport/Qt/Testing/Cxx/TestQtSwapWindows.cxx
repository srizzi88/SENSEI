#include "TestQtCommon.h"
#include <svtkNew.h>
#include <svtkRenderWindow.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>

#include <QApplication>
#include <QBoxLayout>
#include <QSurfaceFormat>
#include <QWidget>

int TestQtSwapWindows(int argc, char* argv[])
{
  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);

  // Set up frame with two horizontally stacked panels,
  // Each containing a QSVTKOpenGLStereoWidget
  QWidget frame;
  QHBoxLayout* layout = new QHBoxLayout(&frame);

  QWidget* leftPanel = new QWidget(&frame);
  QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
  auto leftSVTKWidget = detail::create_widget(type, nullptr, leftPanel);
  svtkSmartPointer<svtkRenderer> leftRenderer = svtkSmartPointer<svtkRenderer>::New();
  leftRenderer->SetBackground(1, 0, 0);
  detail::get_render_window(leftSVTKWidget)->AddRenderer(leftRenderer);
  leftLayout->addWidget(leftSVTKWidget.get());

  QWidget* rightPanel = new QWidget(&frame);
  QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
  auto rightSVTKWidget = detail::create_widget(type, nullptr, rightPanel);
  svtkSmartPointer<svtkRenderer> rightRenderer = svtkSmartPointer<svtkRenderer>::New();
  rightRenderer->SetBackground(0, 1, 0);
  detail::get_render_window(rightSVTKWidget)->AddRenderer(rightRenderer);
  rightLayout->addWidget(rightSVTKWidget.get());

  layout->addWidget(leftPanel);
  layout->addWidget(rightPanel);

  // Show stuff and process events
  frame.show();
  detail::get_render_window(leftSVTKWidget)->Render();
  detail::get_render_window(rightSVTKWidget)->Render();
  app.processEvents();

  // Swap QSVTKOpenGLStereoWidget
  rightLayout->removeWidget(rightSVTKWidget.get());
  leftLayout->removeWidget(leftSVTKWidget.get());
  rightSVTKWidget->setParent(leftPanel);
  leftSVTKWidget->setParent(rightPanel);
  rightLayout->addWidget(leftSVTKWidget.get());
  leftLayout->addWidget(rightSVTKWidget.get());

  // Process events again
  detail::get_render_window(leftSVTKWidget)->Render();
  detail::get_render_window(rightSVTKWidget)->Render();
  app.processEvents();
  return EXIT_SUCCESS;
}
