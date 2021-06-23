#include "TestQtCommon.h"

namespace detail
{

Type select_widget(int argc, char* argv[])
{
  for (int cc = 0; cc < argc; ++cc)
  {
    if (argv[cc] && strcmp(argv[cc], "-w") == 0 && (cc + 1) < argc)
    {
      auto typestr = argv[cc + 1];
      if (strcmp(typestr, "QSVTKRenderWidget") == 0)
      {
        return Type::USE_QSVTKRENDERWIDGET;
      }
      if (strcmp(typestr, "QSVTKOpenGLNativeWidget") == 0)
      {
        return Type::USE_QSVTKOPENGLNATIVEWIDGET;
      }
      else if (strcmp(typestr, "QSVTKOpenGLWindow") == 0)
      {
        return Type::USE_QSVTKOPENGLWINDOW;
      }
      else if (strcmp(typestr, "QSVTKOpenGLStereoWidget") == 0)
      {
        return Type::USE_QSVTKOPENGLSTEREOWIDGET;
      }
#ifndef SVTK_LEGACY_REMOVE
      else if (strcmp(typestr, "QSVTKOpenGLWidget") == 0)
      {
        return Type::USE_QSVTKOPENGLWIDGET;
      }
#endif
    }
  }
  // default.
  return Type::USE_QSVTKOPENGLNATIVEWIDGET;
}

void set_default_format(Type type)
{
  switch (type)
  {
    case Type::USE_QSVTKOPENGLNATIVEWIDGET:
    case Type::USE_QSVTKRENDERWIDGET: // TODO this may be a problem in the future in order to have a
                                     // generic widget
      svtkLogF(INFO, "setting default QSurfaceFormat.");
      QSurfaceFormat::setDefaultFormat(QSVTKOpenGLNativeWidget::defaultFormat());
      break;

    default:
      svtkLogF(INFO, "no need to set default format, skipping.");
      break;
  }
}

std::shared_ptr<QObject> create_widget_or_window(Type type, svtkGenericOpenGLRenderWindow* renWin)
{
  switch (type)
  {
    case Type::USE_QSVTKRENDERWIDGET:
    {
      svtkLogF(INFO, "creating QSVTKRenderWidget.");
      auto widget = std::make_shared<QSVTKRenderWidget>();
      if (renWin)
      {
        widget->setRenderWindow(renWin);
      }
      return std::static_pointer_cast<QObject>(widget);
    }
    case Type::USE_QSVTKOPENGLNATIVEWIDGET:
    {
      svtkLogF(INFO, "creating QSVTKOpenGLNativeWidget.");
      auto widget = std::make_shared<QSVTKOpenGLNativeWidget>();
      if (renWin)
      {
        widget->setRenderWindow(renWin);
      }
      return std::static_pointer_cast<QObject>(widget);
    }
    case Type::USE_QSVTKOPENGLWINDOW:
    {
      svtkLogF(INFO, "creating QSVTKOpenGLWindow.");
      auto widget = std::make_shared<QSVTKOpenGLWindow>();
      svtkLogF(INFO, "set format on Qt window explicitly");
      widget->setFormat(QSVTKOpenGLWindow::defaultFormat());
      if (renWin)
      {
        widget->setRenderWindow(renWin);
      }
      return std::static_pointer_cast<QObject>(widget);
    }
    case Type::USE_QSVTKOPENGLSTEREOWIDGET:
    case Type::USE_QSVTKOPENGLWIDGET:
    {
      svtkLogF(INFO, "creating QSVTKOpenGLStereoWidget.");
      auto widget = std::make_shared<QSVTKOpenGLStereoWidget>();
      svtkLogF(INFO, "set format on Qt widget explicitly");
      widget->setFormat(QSVTKOpenGLWindow::defaultFormat());
      if (renWin)
      {
        widget->setRenderWindow(renWin);
      }
      return std::static_pointer_cast<QObject>(widget);
    }
  }
  return nullptr;
}

std::shared_ptr<QWidget> create_widget(
  Type type, svtkGenericOpenGLRenderWindow* renWin, QWidget* parent)
{
  auto widget = std::dynamic_pointer_cast<QWidget>(create_widget_or_window(type, renWin));
  if (widget && parent)
  {
    widget->setParent(parent);
  }
  return widget;
}

svtkRenderWindow* get_render_window(std::shared_ptr<QObject> widgetOrWindow)
{
  if (auto w1 = qobject_cast<QSVTKRenderWidget*>(widgetOrWindow.get()))
  {
    return w1->renderWindow();
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLStereoWidget*>(widgetOrWindow.get()))
  {
    return w1->renderWindow();
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLNativeWidget*>(widgetOrWindow.get()))
  {
    return w1->renderWindow();
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLWindow*>(widgetOrWindow.get()))
  {
    return w1->renderWindow();
  }

#ifndef SVTK_LEGACY_REMOVE
  if (auto w1 = qobject_cast<QSVTKOpenGLWidget*>(widgetOrWindow.get()))
  {
    return w1->renderWindow();
  }
#endif

  return nullptr;
}

void set_render_window(std::shared_ptr<QObject> widgetOrWindow, svtkRenderWindow* renWin)
{
  if (auto w1 = qobject_cast<QSVTKRenderWidget*>(widgetOrWindow.get()))
  {
    w1->setRenderWindow(renWin);
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLStereoWidget*>(widgetOrWindow.get()))
  {
    w1->setRenderWindow(renWin);
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLNativeWidget*>(widgetOrWindow.get()))
  {
    w1->setRenderWindow(renWin);
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLWindow*>(widgetOrWindow.get()))
  {
    w1->setRenderWindow(renWin);
  }

#ifndef SVTK_LEGACY_REMOVE
  if (auto w1 = qobject_cast<QSVTKOpenGLWidget*>(widgetOrWindow.get()))
  {
    w1->setRenderWindow(renWin);
  }
#endif
}

void process_events_and_wait(int msec)
{
  QApplication::sendPostedEvents();
  QApplication::processEvents();

  if (msec > 0)
  {
    QEventLoop loop;
    QTimer::singleShot(msec, &loop, SLOT(quit()));
    loop.exec();
  }

  QApplication::sendPostedEvents();
  QApplication::processEvents();
  QApplication::sendPostedEvents();
  QApplication::processEvents();
}

void show(std::shared_ptr<QObject> widgetOrWindow, const QSize& size)
{
  if (widgetOrWindow->isWidgetType())
  {
    auto widget = static_cast<QWidget*>(widgetOrWindow.get());
    widget->resize(size);
    widget->show();
  }
  else if (widgetOrWindow->isWindowType())
  {
    auto window = static_cast<QWindow*>(widgetOrWindow.get());
    window->resize(size);
    window->show();
  }

  auto renWindow = svtkGenericOpenGLRenderWindow::SafeDownCast(get_render_window(widgetOrWindow));
  while (renWindow != nullptr && !renWindow->GetReadyForRendering())
  {
    QApplication::sendPostedEvents();
    QApplication::processEvents();
  }
  process_events_and_wait(500);
}

QImage grab_framebuffer(std::shared_ptr<QObject> widgetOrWindow)
{
  if (auto w1 = qobject_cast<QSVTKRenderWidget*>(widgetOrWindow.get()))
  {
    return w1->grabFramebuffer();
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLStereoWidget*>(widgetOrWindow.get()))
  {
    return w1->grabFramebuffer();
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLNativeWidget*>(widgetOrWindow.get()))
  {
    return w1->grabFramebuffer();
  }

  if (auto w1 = qobject_cast<QSVTKOpenGLWindow*>(widgetOrWindow.get()))
  {
    return w1->grabFramebuffer();
  }

#ifndef SVTK_LEGACY_REMOVE
  if (auto w1 = qobject_cast<QSVTKOpenGLWidget*>(widgetOrWindow.get()))
  {
    return w1->grabFramebuffer();
  }
#endif
  return QImage();
}
}
