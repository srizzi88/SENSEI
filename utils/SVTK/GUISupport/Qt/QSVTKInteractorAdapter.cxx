/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKInteractorAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/*========================================================================
 For general information about using SVTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/svtksupport.html
=========================================================================*/

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable : 4127)
#pragma warning(disable : 4512)
#endif

#include "QSVTKInteractorAdapter.h"
#include "QSVTKInteractor.h"

#include <QEvent>
#include <QGestureEvent>
#include <QSignalMapper>
#include <QTimer>
#include <QWidget>

#include "svtkCommand.h"

// function to get SVTK keysyms from ascii characters
static const char* ascii_to_key_sym(int);
// function to get SVTK keysyms from Qt keys
static const char* qt_key_to_key_sym(Qt::Key, Qt::KeyboardModifiers modifiers);

// Tolerance used when truncating the device pixel ratio scaled
// window size in calls to SetSize.
const double QSVTKInteractorAdapter::DevicePixelRatioTolerance = 1e-5;

QSVTKInteractorAdapter::QSVTKInteractorAdapter(QObject* parentObject)
  : QObject(parentObject)
  , AccumulatedDelta(0)
  , DevicePixelRatio(1.0)
{
}

QSVTKInteractorAdapter::~QSVTKInteractorAdapter() {}

void QSVTKInteractorAdapter::SetDevicePixelRatio(float ratio, svtkRenderWindowInteractor* iren)
{
  if (ratio != DevicePixelRatio)
  {
    if (iren)
    {
      int tmp[2];
      iren->GetSize(tmp);
      if (ratio == 1.0)
      {
        iren->SetSize(tmp[0] / 2, tmp[1] / 2);
      }
      else
      {
        iren->SetSize(static_cast<int>(tmp[0] * ratio + DevicePixelRatioTolerance),
          static_cast<int>(tmp[1] * ratio + DevicePixelRatioTolerance));
      }
    }
    this->DevicePixelRatio = ratio;
  }
}

bool QSVTKInteractorAdapter::ProcessEvent(QEvent* e, svtkRenderWindowInteractor* iren)
{
  if (iren == nullptr || e == nullptr)
    return false;

  const QEvent::Type t = e->type();

  if (t == QEvent::FocusIn)
  {
    // For 3DConnexion devices:
    QSVTKInteractor* qiren = QSVTKInteractor::SafeDownCast(iren);
    if (qiren)
    {
      qiren->StartListening();
    }
    return true;
  }

  if (t == QEvent::FocusOut)
  {
    // For 3DConnexion devices:
    QSVTKInteractor* qiren = QSVTKInteractor::SafeDownCast(iren);
    if (qiren)
    {
      qiren->StopListening();
    }
    return true;
  }

  // the following events only happen if the interactor is enabled
  if (!iren->GetEnabled())
    return false;

  if (t == QEvent::MouseButtonPress || t == QEvent::MouseButtonRelease ||
    t == QEvent::MouseButtonDblClick || t == QEvent::MouseMove)
  {
    QMouseEvent* e2 = static_cast<QMouseEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(
      static_cast<int>(e2->x() * this->DevicePixelRatio + DevicePixelRatioTolerance),
      static_cast<int>(e2->y() * this->DevicePixelRatio + DevicePixelRatioTolerance),
      (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
      (e2->modifiers() & Qt::ShiftModifier) > 0 ? 1 : 0, 0,
      e2->type() == QEvent::MouseButtonDblClick ? 1 : 0);
    iren->SetAltKey((e2->modifiers() & Qt::AltModifier) > 0 ? 1 : 0);

    if (t == QEvent::MouseMove)
    {
      iren->InvokeEvent(svtkCommand::MouseMoveEvent, e2);
    }
    else if (t == QEvent::MouseButtonPress || t == QEvent::MouseButtonDblClick)
    {
      switch (e2->button())
      {
        case Qt::LeftButton:
          iren->InvokeEvent(svtkCommand::LeftButtonPressEvent, e2);
          break;

        case Qt::MidButton:
          iren->InvokeEvent(svtkCommand::MiddleButtonPressEvent, e2);
          break;

        case Qt::RightButton:
          iren->InvokeEvent(svtkCommand::RightButtonPressEvent, e2);
          break;

        default:
          break;
      }
    }
    else if (t == QEvent::MouseButtonRelease)
    {
      switch (e2->button())
      {
        case Qt::LeftButton:
          iren->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, e2);
          break;

        case Qt::MidButton:
          iren->InvokeEvent(svtkCommand::MiddleButtonReleaseEvent, e2);
          break;

        case Qt::RightButton:
          iren->InvokeEvent(svtkCommand::RightButtonReleaseEvent, e2);
          break;

        default:
          break;
      }
    }
    return true;
  }
  if (t == QEvent::TouchBegin || t == QEvent::TouchUpdate || t == QEvent::TouchEnd)
  {
    QTouchEvent* e2 = dynamic_cast<QTouchEvent*>(e);
    foreach (const QTouchEvent::TouchPoint& point, e2->touchPoints())
    {
      if (point.id() >= SVTKI_MAX_POINTERS)
      {
        break;
      }
      // give interactor the event information
      iren->SetEventInformationFlipY(
        static_cast<int>(point.pos().x() * this->DevicePixelRatio + DevicePixelRatioTolerance),
        static_cast<int>(point.pos().y() * this->DevicePixelRatio + DevicePixelRatioTolerance),
        (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
        (e2->modifiers() & Qt::ShiftModifier) > 0 ? 1 : 0, 0, 0, nullptr, point.id());
    }
    foreach (const QTouchEvent::TouchPoint& point, e2->touchPoints())
    {
      if (point.id() >= SVTKI_MAX_POINTERS)
      {
        break;
      }
      iren->SetPointerIndex(point.id());
      if (point.state() & Qt::TouchPointReleased)
      {
        iren->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, nullptr);
      }
      if (point.state() & Qt::TouchPointPressed)
      {
        iren->InvokeEvent(svtkCommand::LeftButtonPressEvent, nullptr);
      }
      if (point.state() & Qt::TouchPointMoved)
      {
        iren->InvokeEvent(svtkCommand::MouseMoveEvent, nullptr);
      }
    }
    e2->accept();
    return true;
  }

  if (t == QEvent::Enter)
  {
    iren->InvokeEvent(svtkCommand::EnterEvent, e);
    return true;
  }

  if (t == QEvent::Leave)
  {
    iren->InvokeEvent(svtkCommand::LeaveEvent, e);
    return true;
  }

  if (t == QEvent::KeyPress || t == QEvent::KeyRelease)
  {
    QKeyEvent* e2 = static_cast<QKeyEvent*>(e);

    // get key and keysym information
    int ascii_key = e2->text().length() ? e2->text().unicode()->toLatin1() : 0;
    const char* keysym = ascii_to_key_sym(ascii_key);
    if (!keysym || e2->modifiers() == Qt::KeypadModifier)
    {
      // get virtual keys
      keysym = qt_key_to_key_sym(static_cast<Qt::Key>(e2->key()), e2->modifiers());
    }

    if (!keysym)
    {
      keysym = "None";
    }

    // give interactor event information
    iren->SetKeyEventInformation((e2->modifiers() & Qt::ControlModifier),
      (e2->modifiers() & Qt::ShiftModifier), ascii_key, e2->count(), keysym);
    iren->SetAltKey((e2->modifiers() & Qt::AltModifier) > 0 ? 1 : 0);

    if (t == QEvent::KeyPress)
    {
      // invoke svtk event
      iren->InvokeEvent(svtkCommand::KeyPressEvent, e2);

      // invoke char event only for ascii characters
      if (ascii_key)
      {
        iren->InvokeEvent(svtkCommand::CharEvent, e2);
      }
    }
    else
    {
      iren->InvokeEvent(svtkCommand::KeyReleaseEvent, e2);
    }
    return true;
  }

  if (t == QEvent::Wheel)
  {
    QWheelEvent* e2 = static_cast<QWheelEvent*>(e);

    iren->SetEventInformationFlipY(
      static_cast<int>(e2->x() * this->DevicePixelRatio + DevicePixelRatioTolerance),
      static_cast<int>(e2->y() * this->DevicePixelRatio + DevicePixelRatioTolerance),
      (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
      (e2->modifiers() & Qt::ShiftModifier) > 0 ? 1 : 0);
    iren->SetAltKey((e2->modifiers() & Qt::AltModifier) > 0 ? 1 : 0);

    this->AccumulatedDelta += e2->angleDelta().y();
    const int threshold = 120;

    // invoke svtk event when accumulated delta passes the threshold
    if (this->AccumulatedDelta >= threshold)
    {
      iren->InvokeEvent(svtkCommand::MouseWheelForwardEvent, e2);
      this->AccumulatedDelta = 0;
    }
    else if (this->AccumulatedDelta <= -threshold)
    {
      iren->InvokeEvent(svtkCommand::MouseWheelBackwardEvent, e2);
      this->AccumulatedDelta = 0;
    }
    return true;
  }

  if (t == QEvent::ContextMenu)
  {
    QContextMenuEvent* e2 = static_cast<QContextMenuEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(
      static_cast<int>(e2->x() * this->DevicePixelRatio + DevicePixelRatioTolerance),
      static_cast<int>(e2->y() * this->DevicePixelRatio + DevicePixelRatioTolerance),
      (e2->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0,
      (e2->modifiers() & Qt::ShiftModifier) > 0 ? 1 : 0);
    iren->SetAltKey((e2->modifiers() & Qt::AltModifier) > 0 ? 1 : 0);

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QSVTKInteractor::ContextMenuEvent, e2);

    return true;
  }

  if (t == QEvent::DragEnter)
  {
    QDragEnterEvent* e2 = static_cast<QDragEnterEvent*>(e);

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QSVTKInteractor::DragEnterEvent, e2);

    return true;
  }

  if (t == QEvent::DragLeave)
  {
    QDragLeaveEvent* e2 = static_cast<QDragLeaveEvent*>(e);

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QSVTKInteractor::DragLeaveEvent, e2);

    return true;
  }

  if (t == QEvent::DragMove)
  {
    QDragMoveEvent* e2 = static_cast<QDragMoveEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(
      static_cast<int>(e2->pos().x() * this->DevicePixelRatio + DevicePixelRatioTolerance),
      static_cast<int>(e2->pos().y() * this->DevicePixelRatio + DevicePixelRatioTolerance));

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QSVTKInteractor::DragMoveEvent, e2);
    return true;
  }

  if (t == QEvent::Drop)
  {
    QDropEvent* e2 = static_cast<QDropEvent*>(e);

    // give interactor the event information
    iren->SetEventInformationFlipY(
      static_cast<int>(e2->pos().x() * this->DevicePixelRatio + DevicePixelRatioTolerance),
      static_cast<int>(e2->pos().y() * this->DevicePixelRatio + DevicePixelRatioTolerance));

    // invoke event and pass qt event for additional data as well
    iren->InvokeEvent(QSVTKInteractor::DropEvent, e2);
    return true;
  }

  if (e->type() == QEvent::Gesture)
  {
    // Store event information to restore after gesture is completed
    int eventPosition[2];
    iren->GetEventPosition(eventPosition);
    int lastEventPosition[2];
    iren->GetLastEventPosition(lastEventPosition);

    QGestureEvent* e2 = static_cast<QGestureEvent*>(e);
    if (QSwipeGesture* swipe = static_cast<QSwipeGesture*>(e2->gesture(Qt::SwipeGesture)))
    {
      e2->accept(Qt::SwipeGesture);
      double angle = swipe->swipeAngle();
      iren->SetRotation(angle);
      switch (swipe->state())
      {
        case Qt::GestureCanceled:
        case Qt::GestureFinished:
          iren->InvokeEvent(svtkCommand::EndSwipeEvent, e2);
          break;
        case Qt::GestureStarted:
          iren->InvokeEvent(svtkCommand::StartSwipeEvent, e2);
          iren->InvokeEvent(svtkCommand::SwipeEvent, e2);
          break;
        default:
          iren->InvokeEvent(svtkCommand::SwipeEvent, e2);
      }
    }

    if (QPinchGesture* pinch = static_cast<QPinchGesture*>(e2->gesture(Qt::PinchGesture)))
    {
      e2->accept(Qt::PinchGesture);

      QPointF position = pinch->centerPoint().toPoint();

      // When using MacOS trackpad, the center of the pinch event is already reported in widget
      // coordinates. For other platforms, the coordinates need to be converted from global to
      // local.
#ifndef Q_OS_OSX
      QWidget* widget = qobject_cast<QWidget*>(this->parent());
      if (widget)
      {
        position = widget->mapFromGlobal(pinch->centerPoint().toPoint());
      }
      else
      {
        // Pinch gesture position is in global coordinates, but could not find a widget to convert
        // to local coordinates QSVTKInteractorAdapter parent is not set in QSVTKRenderWindowAdapter.
        // Gesture coordinate mapping may be incorrect.
        qWarning("Could not find parent widget. Gesture coordinate mapping may be incorrect");
      }
#endif
      iren->SetEventInformationFlipY(
        static_cast<int>(position.x() * this->DevicePixelRatio + DevicePixelRatioTolerance),
        static_cast<int>(position.y() * this->DevicePixelRatio + DevicePixelRatioTolerance));
      iren->SetScale(1.0);
      iren->SetScale(pinch->scaleFactor());
      switch (pinch->state())
      {
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
          iren->InvokeEvent(svtkCommand::EndPinchEvent, e2);
          break;
        case Qt::GestureStarted:
          iren->InvokeEvent(svtkCommand::StartPinchEvent, e2);
          iren->InvokeEvent(svtkCommand::PinchEvent, e2);
          break;
        default:
          iren->InvokeEvent(svtkCommand::PinchEvent, e2);
      }
      iren->SetRotation(-1.0 * pinch->lastRotationAngle());
      iren->SetRotation(-1.0 * pinch->rotationAngle());
      switch (pinch->state())
      {
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
          iren->InvokeEvent(svtkCommand::EndRotateEvent, e2);
          break;
        case Qt::GestureStarted:
          iren->InvokeEvent(svtkCommand::StartRotateEvent, e2);
          iren->InvokeEvent(svtkCommand::RotateEvent, e2);
          break;
        default:
          iren->InvokeEvent(svtkCommand::RotateEvent, e2);
      }
    }

    if (QPanGesture* pan = static_cast<QPanGesture*>(e2->gesture(Qt::PanGesture)))
    {
      e2->accept(Qt::PanGesture);

      QPointF delta = pan->delta();
      double translation[2] = { (delta.x() * this->DevicePixelRatio +
                                  this->DevicePixelRatioTolerance),
        -(delta.y() * this->DevicePixelRatio + this->DevicePixelRatioTolerance) };
      iren->SetTranslation(translation);
      switch (pan->state())
      {
        case Qt::GestureFinished:
        case Qt::GestureCanceled:
          iren->InvokeEvent(svtkCommand::EndPanEvent, e2);
          break;
        case Qt::GestureStarted:
          iren->InvokeEvent(svtkCommand::StartPanEvent, e2);
          iren->InvokeEvent(svtkCommand::PanEvent, e2);
          break;
        default:
          iren->InvokeEvent(svtkCommand::PanEvent, e2);
      }
    }

    if (QTapGesture* tap = static_cast<QTapGesture*>(e2->gesture(Qt::TapGesture)))
    {
      e2->accept(Qt::TapGesture);

      QPointF position = tap->position().toPoint();
      iren->SetEventInformationFlipY(
        static_cast<int>(position.x() * this->DevicePixelRatio + this->DevicePixelRatioTolerance),
        static_cast<int>(position.y() * this->DevicePixelRatio + this->DevicePixelRatioTolerance));
      if (tap->state() == Qt::GestureStarted)
      {
        iren->InvokeEvent(svtkCommand::TapEvent, e2);
      }
    }

    if (QTapAndHoldGesture* tapAndHold =
          static_cast<QTapAndHoldGesture*>(e2->gesture(Qt::TapAndHoldGesture)))
    {
      e2->accept(Qt::TapAndHoldGesture);

      QPointF position = tapAndHold->position().toPoint();
      QWidget* widget = qobject_cast<QWidget*>(this->parent());
      if (widget)
      {
        position = widget->mapFromGlobal(tapAndHold->position().toPoint());
      }
      else
      {
        // TapAndHold gesture position is in global coordinates, but could not find a widget to
        // convert to local coordinates QSVTKInteractorAdapter parent is not set in
        // QSVTKRenderWindowAdapter. Gesture coordinate mapping may be incorrect.
        qWarning("Could not find parent widget. Gesture coordinate mapping may be incorrect");
      }
      iren->SetEventInformationFlipY(
        static_cast<int>(position.x() * this->DevicePixelRatio + this->DevicePixelRatioTolerance),
        static_cast<int>(position.y() * this->DevicePixelRatio + this->DevicePixelRatioTolerance));
      if (tapAndHold->state() == Qt::GestureStarted)
      {
        iren->InvokeEvent(svtkCommand::LongTapEvent, e2);
      }
    }

    iren->SetEventPosition(eventPosition);
    iren->SetLastEventPosition(lastEventPosition);

    return true;
  }
  return false;
}

// ***** keysym stuff below  *****

static const char* AsciiToKeySymTable[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, "Tab", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, "space", "exclam", "quotedbl", "numbersign", "dollar",
  "percent", "ampersand", "quoteright", "parenleft", "parenright", "asterisk", "plus", "comma",
  "minus", "period", "slash", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "colon",
  "semicolon", "less", "equal", "greater", "question", "at", "A", "B", "C", "D", "E", "F", "G", "H",
  "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
  "bracketleft", "backslash", "bracketright", "asciicircum", "underscore", "quoteleft", "a", "b",
  "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u",
  "v", "w", "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Delete", nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

const char* ascii_to_key_sym(int i)
{
  if (i >= 0)
  {
    return AsciiToKeySymTable[i];
  }
  return nullptr;
}

#define QSVTK_HANDLE(x, y)                                                                          \
  case x:                                                                                          \
    ret = y;                                                                                       \
    break;

#define QSVTK_HANDLE_KEYPAD(x, y, z)                                                                \
  case x:                                                                                          \
    ret = (modifiers & Qt::KeypadModifier) ? (y) : (z);                                            \
    break;

const char* qt_key_to_key_sym(Qt::Key i, Qt::KeyboardModifiers modifiers)
{
  const char* ret = nullptr;
  switch (i)
  {
    // Cancel
    QSVTK_HANDLE(Qt::Key_Backspace, "BackSpace")
    QSVTK_HANDLE(Qt::Key_Tab, "Tab")
    QSVTK_HANDLE(Qt::Key_Backtab, "Tab")
    QSVTK_HANDLE(Qt::Key_Clear, "Clear")
    QSVTK_HANDLE(Qt::Key_Return, "Return")
    QSVTK_HANDLE(Qt::Key_Enter, "Return")
    QSVTK_HANDLE(Qt::Key_Shift, "Shift_L")
    QSVTK_HANDLE(Qt::Key_Control, "Control_L")
    QSVTK_HANDLE(Qt::Key_Alt, "Alt_L")
    QSVTK_HANDLE(Qt::Key_Pause, "Pause")
    QSVTK_HANDLE(Qt::Key_CapsLock, "Caps_Lock")
    QSVTK_HANDLE(Qt::Key_Escape, "Escape")
    QSVTK_HANDLE(Qt::Key_Space, "space")
    QSVTK_HANDLE(Qt::Key_PageUp, "Prior")
    QSVTK_HANDLE(Qt::Key_PageDown, "Next")
    QSVTK_HANDLE(Qt::Key_End, "End")
    QSVTK_HANDLE(Qt::Key_Home, "Home")
    QSVTK_HANDLE(Qt::Key_Left, "Left")
    QSVTK_HANDLE(Qt::Key_Up, "Up")
    QSVTK_HANDLE(Qt::Key_Right, "Right")
    QSVTK_HANDLE(Qt::Key_Down, "Down")
    QSVTK_HANDLE(Qt::Key_Select, "Select")
    QSVTK_HANDLE(Qt::Key_Execute, "Execute")
    QSVTK_HANDLE(Qt::Key_SysReq, "Snapshot")
    QSVTK_HANDLE(Qt::Key_Insert, "Insert")
    QSVTK_HANDLE(Qt::Key_Delete, "Delete")
    QSVTK_HANDLE(Qt::Key_Help, "Help")
    QSVTK_HANDLE_KEYPAD(Qt::Key_0, "KP_0", "0")
    QSVTK_HANDLE_KEYPAD(Qt::Key_1, "KP_1", "1")
    QSVTK_HANDLE_KEYPAD(Qt::Key_2, "KP_2", "2")
    QSVTK_HANDLE_KEYPAD(Qt::Key_3, "KP_3", "3")
    QSVTK_HANDLE_KEYPAD(Qt::Key_4, "KP_4", "4")
    QSVTK_HANDLE_KEYPAD(Qt::Key_5, "KP_5", "5")
    QSVTK_HANDLE_KEYPAD(Qt::Key_6, "KP_6", "6")
    QSVTK_HANDLE_KEYPAD(Qt::Key_7, "KP_7", "7")
    QSVTK_HANDLE_KEYPAD(Qt::Key_8, "KP_8", "8")
    QSVTK_HANDLE_KEYPAD(Qt::Key_9, "KP_9", "9")
    QSVTK_HANDLE(Qt::Key_A, "a")
    QSVTK_HANDLE(Qt::Key_B, "b")
    QSVTK_HANDLE(Qt::Key_C, "c")
    QSVTK_HANDLE(Qt::Key_D, "d")
    QSVTK_HANDLE(Qt::Key_E, "e")
    QSVTK_HANDLE(Qt::Key_F, "f")
    QSVTK_HANDLE(Qt::Key_G, "g")
    QSVTK_HANDLE(Qt::Key_H, "h")
    QSVTK_HANDLE(Qt::Key_I, "i")
    QSVTK_HANDLE(Qt::Key_J, "h")
    QSVTK_HANDLE(Qt::Key_K, "k")
    QSVTK_HANDLE(Qt::Key_L, "l")
    QSVTK_HANDLE(Qt::Key_M, "m")
    QSVTK_HANDLE(Qt::Key_N, "n")
    QSVTK_HANDLE(Qt::Key_O, "o")
    QSVTK_HANDLE(Qt::Key_P, "p")
    QSVTK_HANDLE(Qt::Key_Q, "q")
    QSVTK_HANDLE(Qt::Key_R, "r")
    QSVTK_HANDLE(Qt::Key_S, "s")
    QSVTK_HANDLE(Qt::Key_T, "t")
    QSVTK_HANDLE(Qt::Key_U, "u")
    QSVTK_HANDLE(Qt::Key_V, "v")
    QSVTK_HANDLE(Qt::Key_W, "w")
    QSVTK_HANDLE(Qt::Key_X, "x")
    QSVTK_HANDLE(Qt::Key_Y, "y")
    QSVTK_HANDLE(Qt::Key_Z, "z")
    QSVTK_HANDLE(Qt::Key_Asterisk, "asterisk")
    QSVTK_HANDLE(Qt::Key_Plus, "plus")
    QSVTK_HANDLE(Qt::Key_Bar, "bar")
    QSVTK_HANDLE(Qt::Key_Minus, "minus")
    QSVTK_HANDLE(Qt::Key_Period, "period")
    QSVTK_HANDLE(Qt::Key_Slash, "slash")
    QSVTK_HANDLE(Qt::Key_F1, "F1")
    QSVTK_HANDLE(Qt::Key_F2, "F2")
    QSVTK_HANDLE(Qt::Key_F3, "F3")
    QSVTK_HANDLE(Qt::Key_F4, "F4")
    QSVTK_HANDLE(Qt::Key_F5, "F5")
    QSVTK_HANDLE(Qt::Key_F6, "F6")
    QSVTK_HANDLE(Qt::Key_F7, "F7")
    QSVTK_HANDLE(Qt::Key_F8, "F8")
    QSVTK_HANDLE(Qt::Key_F9, "F9")
    QSVTK_HANDLE(Qt::Key_F10, "F10")
    QSVTK_HANDLE(Qt::Key_F11, "F11")
    QSVTK_HANDLE(Qt::Key_F12, "F12")
    QSVTK_HANDLE(Qt::Key_F13, "F13")
    QSVTK_HANDLE(Qt::Key_F14, "F14")
    QSVTK_HANDLE(Qt::Key_F15, "F15")
    QSVTK_HANDLE(Qt::Key_F16, "F16")
    QSVTK_HANDLE(Qt::Key_F17, "F17")
    QSVTK_HANDLE(Qt::Key_F18, "F18")
    QSVTK_HANDLE(Qt::Key_F19, "F19")
    QSVTK_HANDLE(Qt::Key_F20, "F20")
    QSVTK_HANDLE(Qt::Key_F21, "F21")
    QSVTK_HANDLE(Qt::Key_F22, "F22")
    QSVTK_HANDLE(Qt::Key_F23, "F23")
    QSVTK_HANDLE(Qt::Key_F24, "F24")
    QSVTK_HANDLE(Qt::Key_NumLock, "Num_Lock")
    QSVTK_HANDLE(Qt::Key_ScrollLock, "Scroll_Lock")

    default:
      break;
  }
  return ret;
}
