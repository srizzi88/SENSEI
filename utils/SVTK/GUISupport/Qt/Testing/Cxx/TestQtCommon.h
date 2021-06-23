#include "QSVTKOpenGLNativeWidget.h"
#include "QSVTKOpenGLStereoWidget.h"
#include "QSVTKOpenGLWindow.h"
#include "QSVTKRenderWidget.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkLogger.h"

#ifndef SVTK_LEGACY_REMOVE
#include "QSVTKOpenGLWidget.h"
#endif

#include <QApplication>
#include <QEventLoop>
#include <QScopedPointer>
#include <QSurfaceFormat>
#include <QTimer>
#include <memory>

namespace detail
{
enum class Type
{
  USE_QSVTKRENDERWIDGET = 0,
  USE_QSVTKOPENGLNATIVEWIDGET = 1,
  USE_QSVTKOPENGLWINDOW = 2,
  USE_QSVTKOPENGLSTEREOWIDGET = 3,
  USE_QSVTKOPENGLWIDGET = 4
};

Type select_widget(int argc, char* argv[]);
void set_default_format(Type type);
std::shared_ptr<QObject> create_widget_or_window(Type type, svtkGenericOpenGLRenderWindow* renWin);
std::shared_ptr<QWidget> create_widget(
  Type type, svtkGenericOpenGLRenderWindow* renWin, QWidget* parent);
svtkRenderWindow* get_render_window(std::shared_ptr<QObject> widgetOrWindow);
void set_render_window(std::shared_ptr<QObject> widgetOrWindow, svtkRenderWindow* renWin);
void process_events_and_wait(int msec);
void show(std::shared_ptr<QObject> widgetOrWindow, const QSize& size);
QImage grab_framebuffer(std::shared_ptr<QObject> widgetOrWindow);

}
