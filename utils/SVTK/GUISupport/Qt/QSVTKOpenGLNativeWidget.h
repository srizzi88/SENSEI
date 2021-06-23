/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKOpenGLNativeWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QSVTKOpenGLNativeWidget
 * @brief QOpenGLWidget subclass to house a svtkGenericOpenGLRenderWindow in a Qt
 * application.
 *
 * QSVTKOpenGLNativeWidget extends QOpenGLWidget to make it work with a
 * svtkGenericOpenGLRenderWindow.
 *
 * Please note that QSVTKOpenGLNativeWidget only works with svtkGenericOpenGLRenderWindow.
 * This is necessary since QOpenGLWidget wants to take over the window management as
 * well as the OpenGL context creation. Getting that to work reliably with
 * svtkXRenderWindow or svtkWin32RenderWindow (and other platform specific
 * svtkRenderWindow subclasses) was tricky and fraught with issues.
 *
 * Since QSVTKOpenGLNativeWidget uses QOpenGLWidget to create the OpenGL context,
 * it uses QSurfaceFormat (set using `QOpenGLWidget::setFormat` or
 * `QSurfaceFormat::setDefaultFormat`) to create appropriate window and context.
 * You can use `QSVTKOpenGLNativeWidget::copyToFormat` to obtain a QSurfaceFormat
 * appropriate for a svtkRenderWindow.
 *
 * A typical usage for QSVTKOpenGLNativeWidget is as follows:
 * @code{.cpp}
 *
 *  // before initializing QApplication, set the default surface format.
 *  QSurfaceFormat::setDefaultFormat(QSVTKOpenGLNativeWidget::defaultFormat());
 *
 *  svtkNew<svtkGenericOpenGLRenderWindow> window;
 *  QPointer<QSVTKOpenGLNativeWidget> widget = new QSVTKOpenGLNativeWidget(...);
 *  widget->SetRenderWindow(window.Get());
 *
 *  // If using any of the standard view e.g. svtkContextView, then
 *  // you can do the following.
 *  svtkNew<svtkContextView> view;
 *  view->SetRenderWindow(window.Get());
 *
 *  // You can continue to use `window` as a regular svtkRenderWindow
 *  // including adding renderers, actors etc.
 *
 * @endcode
 *
 * @section OpenGLContext OpenGL Context
 *
 * In QOpenGLWidget (superclass for QSVTKOpenGLNativeWidget), all rendering happens in a
 * framebuffer object. Thus, care must be taken in the rendering code to never
 * directly re-bind the default framebuffer i.e. ID 0.
 *
 * QSVTKOpenGLNativeWidget creates an internal QOpenGLFramebufferObject, independent of the
 * one created by superclass, for svtkRenderWindow to do the rendering in. This
 * explicit double-buffering is useful in avoiding temporary back-buffer only
 * renders done in SVTK (e.g. when making selections) from destroying the results
 * composed on screen.
 *
 * @section RenderAndPaint Handling Render and Paint.
 *
 * QWidget subclasses (including `QOpenGLWidget` and `QSVTKOpenGLNativeWidget`) display
 * their contents on the screen in `QWidget::paint` in response to a paint event.
 * `QOpenGLWidget` subclasses are expected to do OpenGL rendering in
 * `QOpenGLWidget::paintGL`. QWidget can receive paint events for various
 * reasons including widget getting focus/losing focus, some other widget on
 * the UI e.g. QProgressBar in status bar updating, etc.
 *
 * In SVTK applications, any time the svtkRenderWindow needs to be updated to
 * render a new result, one call `svtkRenderWindow::Render` on it.
 * svtkRenderWindowInteractor set on the render window ensures that as
 * interactions happen that affect the rendered result, it calls `Render` on the
 * render window.
 *
 * Since paint in Qt can be called more often then needed, we avoid potentially
 * expensive `svtkRenderWindow::Render` calls each time that happens. Instead,
 * QSVTKOpenGLNativeWidget relies on the SVTK application calling
 * `svtkRenderWindow::Render` on the render window when it needs to update the
 * rendering. `paintGL` simply passes on the result rendered by the most render
 * svtkRenderWindow::Render to Qt windowing system for composing on-screen.
 *
 * There may still be occasions when we may have to render in `paint` for
 * example if the window was resized or Qt had to recreate the OpenGL context.
 * In those cases, `QSVTKOpenGLNativeWidget::paintGL` can request a render by calling
 * `QSVTKOpenGLNativeWidget::renderSVTK`.
 *
 * @section Caveats
 * QSVTKOpenGLNativeWidget does not support stereo,
 * please use QSVTKOpenGLStereoWidget if you need support for stereo rendering
 *
 * QSVTKOpenGLNativeWidget is targeted for Qt version 5.5 and above.
 *
 * @sa QSVTKOpenGLStereoWidget QSVTKRenderWidget
 *
 */
#ifndef QSVTKOpenGLNativeWidget_h
#define QSVTKOpenGLNativeWidget_h

#include <QOpenGLWidget>
#include <QScopedPointer> // for QScopedPointer.

#include "QSVTKInteractor.h"        // needed for QSVTKInteractor
#include "svtkGUISupportQtModule.h" // for export macro
#include "svtkNew.h"                // needed for svtkNew
#include "svtkSmartPointer.h"       // needed for svtkSmartPointer

class QSVTKInteractor;
class QSVTKInteractorAdapter;
class QSVTKRenderWindowAdapter;
class svtkGenericOpenGLRenderWindow;

class SVTKGUISUPPORTQT_EXPORT QSVTKOpenGLNativeWidget : public QOpenGLWidget
{
  Q_OBJECT
  typedef QOpenGLWidget Superclass;

public:
  QSVTKOpenGLNativeWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  QSVTKOpenGLNativeWidget(svtkGenericOpenGLRenderWindow* window, QWidget* parent = nullptr,
    Qt::WindowFlags f = Qt::WindowFlags());
  ~QSVTKOpenGLNativeWidget() override;

  //@{
  /**
   * Set a render window to use. It a render window was already set, it will be
   * finalized and all of its OpenGL resource released. If the \c win is
   * non-null and it has no interactor set, then a QSVTKInteractor instance will
   * be created as set on the render window as the interactor.
   */
  void setRenderWindow(svtkGenericOpenGLRenderWindow* win);
  void setRenderWindow(svtkRenderWindow* win);
  //@}

  /**
   * Returns the render window that is being shown in this widget.
   */
  svtkRenderWindow* renderWindow() const;

  /**
   * Get the QSVTKInteractor that was either created by default or set by the user.
   */
  QSVTKInteractor* interactor() const;

  /**
   * @copydoc QSVTKRenderWindowAdapter::defaultFormat(bool)
   */
  static QSurfaceFormat defaultFormat(bool stereo_capable = false);

  //@{
  /**
   * Enable or disable support for HiDPI displays. When enabled, this enabled
   * DPI scaling i.e. `svtkWindow::SetDPI` will be called with a DPI value scaled
   * by the device pixel ratio every time the widget is resized. The unscaled
   * DPI value can be specified by using `setUnscaledDPI`.
   */
  void setEnableHiDPI(bool enable);
  bool enableHiDPI() const { return this->EnableHiDPI; }
  //@}

  //@{
  /**
   * Set/Get unscaled DPI value. Defaults to 72, which is also the default value
   * in svtkWindow.
   */
  void setUnscaledDPI(int);
  int unscaledDPI() const { return this->UnscaledDPI; }
  //@}

  //@{
  /**
   * Set/get the default cursor to use for this widget.
   */
  void setDefaultCursor(const QCursor& cursor);
  const QCursor& defaultCursor() const { return this->DefaultCursor; }
  //@}

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

protected slots:
  /**
   * Called as a response to `QOpenGLContext::aboutToBeDestroyed`. This may be
   * called anytime during the widget lifecycle. We need to release any OpenGL
   * resources allocated in SVTK work in this method.
   */
  virtual void cleanupContext();

  void updateSize();

protected:
  bool event(QEvent* evt) override;
  void initializeGL() override;
  void paintGL() override;

protected:
  svtkSmartPointer<svtkGenericOpenGLRenderWindow> RenderWindow;
  QScopedPointer<QSVTKRenderWindowAdapter> RenderWindowAdapter;

private:
  Q_DISABLE_COPY(QSVTKOpenGLNativeWidget);

  bool EnableHiDPI;
  int UnscaledDPI;
  QCursor DefaultCursor;
};

#endif
