#include <QApplication>
#include <QSurfaceFormat>

#include "QSVTKOpenGLStereoWidget.h"
#include "QtSVTKRenderWindows.h"

int main(int argc, char** argv)
{
  // needed to ensure appropriate OpenGL context is created for SVTK rendering.
  QSurfaceFormat::setDefaultFormat(QSVTKOpenGLStereoWidget::defaultFormat());

  // QT Stuff
  QApplication app(argc, argv);

  QtSVTKRenderWindows myQtSVTKRenderWindows(argc, argv);
  myQtSVTKRenderWindows.show();

  return app.exec();
}
