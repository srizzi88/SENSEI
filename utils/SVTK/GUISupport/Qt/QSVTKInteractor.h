/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

#ifndef Q_SVTK_INTERACTOR_H
#define Q_SVTK_INTERACTOR_H

#include "QSVTKWin32Header.h"
#include "svtkGUISupportQtModule.h" // For export macro
#include <QtCore/QObject>
#include <svtkCommand.h>
#include <svtkRenderWindowInteractor.h>

#include "svtkTDxConfigure.h" // defines SVTK_USE_TDX
#if defined(SVTK_USE_TDX) && defined(Q_OS_WIN)
class svtkTDxWinDevice;
#endif
#if defined(SVTK_USE_TDX) && defined(Q_OS_MAC)
class svtkTDxMacDevice;
#endif
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
class svtkTDxDevice;
class svtkTDxUnixDevice;
#endif

class QSVTKInteractorInternal;

/**
 * @class QSVTKInteractor
 * @brief - an interactor for QSVTKOpenGLNativeWidget (and QSVTKWiget).
 *
 * QSVTKInteractor handles relaying Qt events to SVTK.
 * @sa QSVTKOpenGLNativeWidget
 */

class SVTKGUISUPPORTQT_EXPORT QSVTKInteractor : public svtkRenderWindowInteractor
{
public:
  static QSVTKInteractor* New();
  svtkTypeMacro(QSVTKInteractor, svtkRenderWindowInteractor);

  /**
   * Enum for additional event types supported.
   * These events can be picked up by command observers on the interactor.
   */
  enum svtkCustomEvents
  {
    ContextMenuEvent = svtkCommand::UserEvent + 100,
    DragEnterEvent,
    DragMoveEvent,
    DragLeaveEvent,
    DropEvent
  };

  /**
   * Overloaded terminate app, which does nothing in Qt.
   * Use qApp->exit() instead.
   */
  void TerminateApp() override;

  /**
   * Overloaded start method does nothing.
   * Use qApp->exec() instead.
   */
  void Start() override;
  void Initialize() override;

  /**
   * Start listening events on 3DConnexion device.
   */
  virtual void StartListening();

  /**
   * Stop listening events on 3DConnexion device.
   */
  virtual void StopListening();

  /**
   * timer event slot
   */
  virtual void TimerEvent(int timerId);

#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  virtual svtkTDxUnixDevice* GetDevice();
  virtual void SetDevice(svtkTDxDevice* device);
#endif

protected:
  // constructor
  QSVTKInteractor();
  // destructor
  ~QSVTKInteractor() override;

  // create a Qt Timer
  int InternalCreateTimer(int timerId, int timerType, unsigned long duration) override;
  // destroy a Qt Timer
  int InternalDestroyTimer(int platformTimerId) override;
#if defined(SVTK_USE_TDX) && defined(Q_OS_WIN)
  svtkTDxWinDevice* Device;
#endif
#if defined(SVTK_USE_TDX) && defined(Q_OS_MAC)
  svtkTDxMacDevice* Device;
#endif
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  svtkTDxUnixDevice* Device;
#endif

private:
  QSVTKInteractorInternal* Internal;

  QSVTKInteractor(const QSVTKInteractor&) = delete;
  void operator=(const QSVTKInteractor&) = delete;
};

#endif
