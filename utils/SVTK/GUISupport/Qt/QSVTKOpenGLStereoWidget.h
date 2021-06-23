/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKOpenGLStereoWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef QSVTKOpenGLStereoWidget_h
#define QSVTKOpenGLStereoWidget_h

#include "svtkGUISupportQtModule.h" // For export macro
#include <QWidget>

#include "QSVTKOpenGLWindow.h" // needed for ivar
#include <QPointer>           // needed for ivar

// Forward Qt class declarations
class QSurfaceFormat;
class QOpenGLContext;

// class QSVTKInteractor;
class QSVTKInteractorAdapter;
class QSVTKOpenGLWindow;
class svtkGenericOpenGLRenderWindow;
class svtkRenderWindow;
class svtkRenderWindowInteractor;

/**
 * @class QSVTKOpenGLStereoWidget
 * @brief QWidget for displaying a svtkRenderWindow in a Qt Application.
 *
 * QSVTKOpenGLStereoWidget simplifies using a QSVTKOpenGLWindow as a widget in Qt
 * application so it can be embedded in a layout rather than being a top-level
 * window. QSVTKOpenGLWindow has all the limitations posed by Qt with
 * `QWidget::createWindowContainer` hence developers are advised to refer to Qt
 * docs for more details.
 *
 * In general QSVTKOpenGLNativeWidget may be a better choice, however
 * QSVTKOpenGLWindow-based QSVTKOpenGLStereoWidget may be better choice for applications
 * requiring quad-buffer stereo.
 *
 * Due to Qt limitations, QSVTKOpenGLStereoWidget does not support being a
 * native widget. But native widget are sometimes mandatory, for example within
 * QScrollArea and QMDIArea, so the QSVTKOpenGLNativeWidget should be
 * used when in needs of SVTK rendering in the context of Qt native widget.
 *
 * If a QSVTKOpenGLStereoWidget is used in a QScrollArea or in a QMDIArea, it
 * will force it to be native and this is *NOT* supported.
 *
 * Unlike QSVTKOpenGLNativeWidget, QSVTKOpenGLStereoWidget does not require that the
 * default surface format for the application be changed. One can simply specify
 * the needed QSurfaceFormat for the specific QSVTKOpenGLStereoWidget instance by
 * calling `QSVTKOpenGLStereoWidget::setFormat` before the widget is initialized.
 *
 * @sa QSVTKOpenGLWindow QSVTKOpenGLNativeWidget QSVTKRenderWidget
 */
class SVTKGUISUPPORTQT_EXPORT QSVTKOpenGLStereoWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  QSVTKOpenGLStereoWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  QSVTKOpenGLStereoWidget(
    QOpenGLContext* shareContext, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  QSVTKOpenGLStereoWidget(svtkGenericOpenGLRenderWindow* w, QWidget* parent = nullptr,
    Qt::WindowFlags f = Qt::WindowFlags());
  QSVTKOpenGLStereoWidget(svtkGenericOpenGLRenderWindow* w, QOpenGLContext* shareContext,
    QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~QSVTKOpenGLStereoWidget() override;

  //@{
  /**
   * @copydoc QSVTKOpenGLWindow::setRenderWindow()
   */
  void setRenderWindow(svtkGenericOpenGLRenderWindow* win)
  {
    this->SVTKOpenGLWindow->setRenderWindow(win);
  }
  void setRenderWindow(svtkRenderWindow* win) { this->SVTKOpenGLWindow->setRenderWindow(win); }
  //@}

  /**
   * @copydoc QSVTKOpenGLWindow::renderWindow()
   */
  svtkRenderWindow* renderWindow() const { return this->SVTKOpenGLWindow->renderWindow(); }

  /**
   * @copydoc QSVTKOpenGLWindow::interactor()
   */
  QSVTKInteractor* interactor() const { return this->SVTKOpenGLWindow->interactor(); }

  /**
   * @copydoc QSVTKRenderWindowAdapter::defaultFormat(bool)
   */
  static QSurfaceFormat defaultFormat(bool stereo_capable = false)
  {
    return QSVTKOpenGLWindow::defaultFormat(stereo_capable);
  }

  /**
   * @copydoc QSVTKOpenGLWindow::setEnableHiDPI()
   */
  void setEnableHiDPI(bool enable) { this->SVTKOpenGLWindow->setEnableHiDPI(enable); }
  bool enableHiDPI() const { return this->SVTKOpenGLWindow->enableHiDPI(); }

  //@{
  /**
   * Set/Get unscaled DPI value. Defaults to 72, which is also the default value
   * in svtkWindow.
   */
  void setUnscaledDPI(int dpi) { this->SVTKOpenGLWindow->setUnscaledDPI(dpi); }
  int unscaledDPI() const { return this->SVTKOpenGLWindow->unscaledDPI(); }
  //@}

  //@{
  /**
   * @copydoc QSVTKOpenGLWindow::setDefaultCursor()
   */
  void setDefaultCursor(const QCursor& cursor) { this->SVTKOpenGLWindow->setDefaultCursor(cursor); }
  const QCursor& defaultCursor() const { return this->SVTKOpenGLWindow->defaultCursor(); }
  //@}

  /**
   * Returns true if the internal QOpenGLWindow's is valid, i.e. if OpenGL
   * resources, like the context, have been successfully initialized.
   */
  bool isValid() { return this->SVTKOpenGLWindow->isValid(); }

  /**
   * Expose internal QSVTKOpenGLWindow::grabFramebuffer(). Renders and returns
   * a 32-bit RGB image of the framebuffer.
   */
  QImage grabFramebuffer();

  /**
   * Returns the embedded QSVTKOpenGLWindow.
   */
  QSVTKOpenGLWindow* embeddedOpenGLWindow() const { return this->SVTKOpenGLWindow; }

  /**
   * Sets the requested surface format.
   *
   * When the format is not explicitly set via this function, the format
   * returned by QSurfaceFormat::defaultFormat() will be used. This means that
   * when having multiple OpenGL widgets, individual calls to this function can
   * be replaced by one single call to QSurfaceFormat::setDefaultFormat() before
   * creating the first widget.
   */
  void setFormat(const QSurfaceFormat& fmt) { this->SVTKOpenGLWindow->setFormat(fmt); }

  /**
   * Returns the context and surface format used by this widget and its toplevel window.
   */
  QSurfaceFormat format() const { return this->SVTKOpenGLWindow->format(); }

  //@{
  /**
   * @deprecated in SVTK 9.0
   */
  SVTK_LEGACY(void SetRenderWindow(svtkGenericOpenGLRenderWindow* win));
  SVTK_LEGACY(void SetRenderWindow(svtkRenderWindow* win));
  //@}

  //@{
  /**
   * These methods have be deprecated to fix naming style. Since
   * QSVTKOpenGLNativeWidget is QObject subclass, we follow Qt naming conventions
   * rather than SVTK's.
   */
  SVTK_LEGACY(svtkRenderWindow* GetRenderWindow());
  SVTK_LEGACY(QSVTKInteractor* GetInteractor());
  //@}

  /**
   * @deprecated in SVTK 9.0
   * QSVTKInteractorAdapter is an internal helper. Hence the API was removed.
   */
  SVTK_LEGACY(QSVTKInteractorAdapter* GetInteractorAdapter());

  /**
   * @deprecated in SVTK 9.0. Simply use `QWidget::setCursor` API to change
   * cursor.
   */
  SVTK_LEGACY(void setQSVTKCursor(const QCursor& cursor));

  /**
   * @deprecated in SVTK 9.0. Use `setDefaultCursor` instead.
   */
  SVTK_LEGACY(void setDefaultQSVTKCursor(const QCursor& cursor));

protected:
  void resizeEvent(QResizeEvent* evt) override;
  void paintEvent(QPaintEvent* evt) override;

private:
  QPointer<QSVTKOpenGLWindow> SVTKOpenGLWindow;
};

#endif
