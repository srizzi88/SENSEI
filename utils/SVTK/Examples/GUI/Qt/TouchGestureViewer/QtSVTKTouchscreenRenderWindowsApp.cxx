#include <QApplication>
#include <QSurfaceFormat>

#include "QSVTKOpenGLStereoWidget.h"
#include "QtSVTKTouchscreenRenderWindows.h"

int main(int argc, char** argv)
{
  // Needed to ensure appropriate OpenGL context is created for SVTK rendering.
  QSurfaceFormat format = QSVTKOpenGLStereoWidget::defaultFormat();
#if _WINDOWS
  format.setProfile(QSurfaceFormat::CompatibilityProfile);
#endif
  QSurfaceFormat::setDefaultFormat(format);

  // QT Stuff
  QApplication app(argc, argv);

  QtSVTKTouchscreenRenderWindows myQtSVTKTouchscreenRenderWindows(argc, argv);
  myQtSVTKTouchscreenRenderWindows.show();

  return app.exec();
}
