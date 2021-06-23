/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKOpenGLWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QSVTKOpenGLWindow
 * @brief display a svtkGenericOpenGLRenderWindow in a Qt QOpenGLWindow.
 *
 * QSVTKOpenGLWindow is one of the mechanisms for displaying SVTK rendering
 * results in a Qt application. QSVTKOpenGLWindow extends QOpenGLWindow to
 * display the rendering results of a svtkGenericOpenGLRenderWindow.
 *
 * Since QSVTKOpenGLWindow is based on QOpenGLWindow it is intended for
 * rendering in a top-level window. QSVTKOpenGLWindow can be embedded in a
 * another QWidget using `QWidget::createWindowContainer` or by using
 * QSVTKOpenGLStereoWidget instead. However, developers are encouraged to check
 * Qt documentation for `QWidget::createWindowContainer` idiosyncrasies.
 * Using QSVTKOpenGLNativeWidget instead is generally a better choice for causes
 * where you want to embed SVTK rendering results in a QWidget. QSVTKOpenGLWindow
 * or QSVTKOpenGLStereoWidget is still preferred for applications that want to support
 * quad-buffer based stereo rendering.
 *
 * To request a specific configuration for the context, use
 * `QWindow::setFormat()` like for any other QWindow. This allows, among others,
 * requesting a given OpenGL version and profile. Use
 * `QOpenGLWindow::defaultFormat()` to obtain a QSurfaceFormat with appropriate
 * OpenGL version configuration. To enable quad-buffer stereo, you'll need to
 * call `QSurfaceFormat::setStereo(true)`.
 *
 * SVTK Rendering features like multi-sampling, double buffering etc.
 * are enabled/disabled by directly setting the corresponding attributes on
 * svtkGenericOpenGLRenderWindow and not when specifying the OpenGL context
 * format in `setFormat`. If not specified, then `QSurfaceFormat::defaultFormat`
 * will be used.
 *
 * @note QSVTKOpenGLWindow requires Qt version 5.9 and above.
 * @sa QSVTKOpenGLStereoWidget QSVTKOpenGLNativeWidget
 */
#ifndef QSVTKOpenGLWindow_h
#define QSVTKOpenGLWindow_h

#include <QOpenGLWindow>
#include <QScopedPointer> // for QScopedPointer.

#include "QSVTKInteractor.h"        // needed for QSVTKInteractor
#include "svtkGUISupportQtModule.h" // for export macro
#include "svtkNew.h"                // needed for svtkNew
#include "svtkSmartPointer.h"       // needed for svtkSmartPointer

class QSVTKInteractor;
class QSVTKInteractorAdapter;
class QSVTKRenderWindowAdapter;
class svtkGenericOpenGLRenderWindow;

class SVTKGUISUPPORTQT_EXPORT QSVTKOpenGLWindow : public QOpenGLWindow
{
  Q_OBJECT
  typedef QOpenGLWindow Superclass;

public:
  QSVTKOpenGLWindow(
    QOpenGLWindow::UpdateBehavior updateBehavior = NoPartialUpdate, QWindow* parent = nullptr);
  QSVTKOpenGLWindow(QOpenGLContext* shareContext,
    QOpenGLWindow::UpdateBehavior updateBehavior = NoPartialUpdate, QWindow* parent = nullptr);
  QSVTKOpenGLWindow(svtkGenericOpenGLRenderWindow* renderWindow,
    QOpenGLWindow::UpdateBehavior updateBehavior = NoPartialUpdate, QWindow* parent = nullptr);
  QSVTKOpenGLWindow(svtkGenericOpenGLRenderWindow* renderWindow, QOpenGLContext* shareContext,
    QOpenGLWindow::UpdateBehavior updateBehavior = NoPartialUpdate, QWindow* parent = nullptr);
  ~QSVTKOpenGLWindow() override;

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
   * @deprecated in SVTK 9.0. Use `setRenderWindow` instead.
   */
  SVTK_LEGACY(void SetRenderWindow(svtkGenericOpenGLRenderWindow* win));
  SVTK_LEGACY(void SetRenderWindow(svtkRenderWindow* win));
  //@}

  //@{
  /**
   * These methods have be deprecated to fix naming style. Since
   * QSVTKOpenGLWindow is QObject subclass, we follow Qt naming conventions
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

signals:
  /**
   * Signal emitted when any event has been receive, with the corresponding
   * event as argument.
   */
  void windowEvent(QEvent* e);

protected slots:
  /**
   * Called as a response to `QOpenGLContext::aboutToBeDestroyed`. This may be
   * called anytime during the widget lifecycle. We need to release any OpenGL
   * resources allocated in SVTK work in this method.
   */
  void cleanupContext();

  void updateSize();

  /**
   * QSVTKOpenGLStereoWidget is given friendship so it can call `cleanupContext` in its
   * destructor to ensure that OpenGL state is proporly cleaned up before the
   * widget goes away.
   */
  friend class QSVTKOpenGLStereoWidget;

protected:
  bool event(QEvent* evt) override;
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

protected:
  svtkSmartPointer<svtkGenericOpenGLRenderWindow> RenderWindow;
  QScopedPointer<QSVTKRenderWindowAdapter> RenderWindowAdapter;

private:
  Q_DISABLE_COPY(QSVTKOpenGLWindow);
  bool EnableHiDPI;
  int UnscaledDPI;
  QCursor DefaultCursor;
};

#endif
