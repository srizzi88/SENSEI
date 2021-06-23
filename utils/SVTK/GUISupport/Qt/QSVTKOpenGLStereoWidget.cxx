/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKOpenGLStereoWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QSVTKOpenGLStereoWidget.h"

#include "QSVTKInteractor.h"
#include "QSVTKInteractorAdapter.h"
#include "svtkGenericOpenGLRenderWindow.h"
#include "svtkLogger.h"
#include "svtkRenderWindowInteractor.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QLayout>
#include <QOpenGLContext>
#include <QResizeEvent>
#include <QSurfaceFormat>
#include <QtDebug>

//-----------------------------------------------------------------------------
QSVTKOpenGLStereoWidget::QSVTKOpenGLStereoWidget(QWidget* parent, Qt::WindowFlags f)
  : QSVTKOpenGLStereoWidget(svtkSmartPointer<svtkGenericOpenGLRenderWindow>::New(), nullptr, parent, f)
{
}

//-----------------------------------------------------------------------------
QSVTKOpenGLStereoWidget::QSVTKOpenGLStereoWidget(
  QOpenGLContext* shareContext, QWidget* parent, Qt::WindowFlags f)
  : QSVTKOpenGLStereoWidget(
      svtkSmartPointer<svtkGenericOpenGLRenderWindow>::New(), shareContext, parent, f)
{
}

//-----------------------------------------------------------------------------
QSVTKOpenGLStereoWidget::QSVTKOpenGLStereoWidget(
  svtkGenericOpenGLRenderWindow* w, QWidget* parent, Qt::WindowFlags f)
  : QSVTKOpenGLStereoWidget(w, QOpenGLContext::currentContext(), parent, f)
{
}

//-----------------------------------------------------------------------------
QSVTKOpenGLStereoWidget::QSVTKOpenGLStereoWidget(
  svtkGenericOpenGLRenderWindow* w, QOpenGLContext* shareContext, QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f)
  , SVTKOpenGLWindow(nullptr)
{
  QVBoxLayout* vBoxLayout = new QVBoxLayout(this);
  vBoxLayout->setContentsMargins(0, 0, 0, 0);

  this->SVTKOpenGLWindow = new QSVTKOpenGLWindow(w, shareContext);
  QWidget* container = QWidget::createWindowContainer(this->SVTKOpenGLWindow, this, f);
  container->setAttribute(Qt::WA_TransparentForMouseEvents);
  container->setMouseTracking(true);
  vBoxLayout->addWidget(container);

  // Forward signals triggered by the internal QSVTKOpenGLWindow
  QObject::connect(this->SVTKOpenGLWindow.data(), &QSVTKOpenGLWindow::windowEvent,
    [this](QEvent* evt) { QApplication::sendEvent(this, evt); });

  // enable mouse tracking to process mouse events
  this->setMouseTracking(true);

  // default to strong focus to accept focus by tabbing and clicking
  this->setFocusPolicy(Qt::StrongFocus);

  // Work around for bug paraview/paraview#18285
  // https://gitlab.kitware.com/paraview/paraview/issues/18285
  // This ensure that kde will not grab the window
  this->setProperty("_kde_no_window_grab", true);

  // enable qt gesture events
  grabGesture(Qt::PinchGesture);
  grabGesture(Qt::PanGesture);
  grabGesture(Qt::TapGesture);
  grabGesture(Qt::TapAndHoldGesture);
  grabGesture(Qt::SwipeGesture);
}

//-----------------------------------------------------------------------------
QSVTKOpenGLStereoWidget::~QSVTKOpenGLStereoWidget() {}

//-----------------------------------------------------------------------------
QImage QSVTKOpenGLStereoWidget::grabFramebuffer()
{
  return this->SVTKOpenGLWindow->grabFramebuffer();
}

//-----------------------------------------------------------------------------
void QSVTKOpenGLStereoWidget::resizeEvent(QResizeEvent* evt)
{
  svtkLogScopeF(TRACE, "resizeEvent(%d, %d)", evt->size().width(), evt->size().height());
  this->Superclass::resizeEvent(evt);
}

//-----------------------------------------------------------------------------
void QSVTKOpenGLStereoWidget::paintEvent(QPaintEvent* evt)
{
  svtkLogScopeF(TRACE, "paintEvent");
  this->Superclass::paintEvent(evt);

  // this is generally not needed; however, there are cases when the after a
  // resize, the embedded QSVTKOpenGLWindow doesn't repaint even though it
  // correctly gets the resize event; explicitly triggering update on the
  // internal widget overcomes that issue.
  this->SVTKOpenGLWindow->update();
}

//-----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
void QSVTKOpenGLStereoWidget::SetRenderWindow(svtkRenderWindow* win)
{
  SVTK_LEGACY_REPLACED_BODY(
    QSVTKOpenGLStereoWidget::SetRenderWindow, "SVTK 8.3", QSVTKOpenGLStereoWidget::setRenderWindow);
  svtkGenericOpenGLRenderWindow* gwin = svtkGenericOpenGLRenderWindow::SafeDownCast(win);
  if (gwin == nullptr && win != nullptr)
  {
    qDebug() << "QSVTKOpenGLStereoWidget requires a `svtkGenericOpenGLRenderWindow`. `"
             << win->GetClassName() << "` is not supported.";
  }
  this->setRenderWindow(gwin);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
void QSVTKOpenGLStereoWidget::SetRenderWindow(svtkGenericOpenGLRenderWindow* win)
{
  SVTK_LEGACY_REPLACED_BODY(
    QSVTKOpenGLStereoWidget::SetRenderWindow, "SVTK 8.3", QSVTKOpenGLStereoWidget::setRenderWindow);
  this->setRenderWindow(win);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
svtkRenderWindow* QSVTKOpenGLStereoWidget::GetRenderWindow()
{
  SVTK_LEGACY_REPLACED_BODY(
    QSVTKOpenGLStereoWidget::GetRenderWindow, "SVTK 8.3", QSVTKOpenGLStereoWidget::renderWindow);
  return this->renderWindow();
}
#endif

//-----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
QSVTKInteractorAdapter* QSVTKOpenGLStereoWidget::GetInteractorAdapter()
{
  SVTK_LEGACY_BODY(QSVTKOpenGLStereoWidget::GetInteractorAdapter, "SVTK 8.3");
  return nullptr;
}
#endif

//-----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
QSVTKInteractor* QSVTKOpenGLStereoWidget::GetInteractor()
{
  SVTK_LEGACY_REPLACED_BODY(
    QSVTKOpenGLStereoWidget::GetInteractor, "SVTK 8.3", QSVTKOpenGLStereoWidget::interactor);
  return this->interactor();
}
#endif

//-----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
void QSVTKOpenGLStereoWidget::setQSVTKCursor(const QCursor& cursor)
{
  SVTK_LEGACY_REPLACED_BODY(
    QSVTKOpenGLStereoWidget::setQSVTKCursor, "SVTK 8.3", QSVTKOpenGLStereoWidget::setCursor);
  this->setCursor(cursor);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(SVTK_LEGACY_REMOVE)
void QSVTKOpenGLStereoWidget::setDefaultQSVTKCursor(const QCursor& cursor)
{
  SVTK_LEGACY_REPLACED_BODY(QSVTKOpenGLStereoWidget::setDefaultQSVTKCursor, "SVTK 8.3",
    QSVTKOpenGLStereoWidget::setDefaultCursor);
  this->setDefaultCursor(cursor);
}
#endif
