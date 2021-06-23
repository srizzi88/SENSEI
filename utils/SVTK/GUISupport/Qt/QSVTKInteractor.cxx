/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QSVTKInteractor.cxx

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

#include "QSVTKInteractor.h"
#include "QSVTKInteractorInternal.h"

#if defined(SVTK_USE_TDX) && defined(Q_OS_WIN)
#include "svtkTDxWinDevice.h"
#endif

#if defined(SVTK_USE_TDX) && defined(Q_OS_MAC)
#include "svtkTDxMacDevice.h"
#endif

#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
#include "svtkTDxUnixDevice.h"
#endif

#include <QEvent>
#include <QResizeEvent>
#include <QSignalMapper>
#include <QTimer>

#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"

QSVTKInteractorInternal::QSVTKInteractorInternal(QSVTKInteractor* p)
  : Parent(p)
{
  this->SignalMapper = new QSignalMapper(this);
  QObject::connect(this->SignalMapper, SIGNAL(mapped(int)), this, SLOT(TimerEvent(int)));
}

QSVTKInteractorInternal::~QSVTKInteractorInternal() {}

void QSVTKInteractorInternal::TimerEvent(int id)
{
  Parent->TimerEvent(id);
}

/*! allocation method for Qt/SVTK interactor
 */
svtkStandardNewMacro(QSVTKInteractor);

/*! constructor for Qt/SVTK interactor
 */
QSVTKInteractor::QSVTKInteractor()
{
  this->Internal = new QSVTKInteractorInternal(this);

#if defined(SVTK_USE_TDX) && defined(Q_OS_WIN)
  this->Device = svtkTDxWinDevice::New();
#endif
#if defined(SVTK_USE_TDX) && defined(Q_OS_MAC)
  this->Device = svtkTDxMacDevice::New();
#endif
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  this->Device = 0;
#endif
}

void QSVTKInteractor::Initialize()
{
#if defined(SVTK_USE_TDX) && defined(Q_OS_WIN)
  if (this->UseTDx)
  {
    // this is QWidget::winId();
    HWND hWnd = static_cast<HWND>(this->GetRenderWindow()->GetGenericWindowId());
    if (!this->Device->GetInitialized())
    {
      this->Device->SetInteractor(this);
      this->Device->SetWindowHandle(hWnd);
      this->Device->Initialize();
    }
  }
#endif
#if defined(SVTK_USE_TDX) && defined(Q_OS_MAC)
  if (this->UseTDx)
  {
    if (!this->Device->GetInitialized())
    {
      this->Device->SetInteractor(this);
      // Do not initialize the device here.
    }
  }
#endif
  this->Initialized = 1;
  this->Enable();
}

#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
// ----------------------------------------------------------------------------
svtkTDxUnixDevice* QSVTKInteractor::GetDevice()
{
  return this->Device;
}

// ----------------------------------------------------------------------------
void QSVTKInteractor::SetDevice(svtkTDxDevice* device)
{
  if (this->Device != device)
  {
    this->Device = static_cast<svtkTDxUnixDevice*>(device);
  }
}
#endif

/*! start method for interactor
 */
void QSVTKInteractor::Start()
{
  svtkErrorMacro(<< "QSVTKInteractor cannot control the event loop.");
}

/*! terminate the application
 */
void QSVTKInteractor::TerminateApp()
{
  // we are in a GUI so let's terminate the GUI the normal way
  // qApp->exit();
}

// ----------------------------------------------------------------------------
void QSVTKInteractor::StartListening()
{
#if defined(SVTK_USE_TDX) && defined(Q_OS_WIN)
  if (this->Device->GetInitialized() && !this->Device->GetIsListening())
  {
    this->Device->StartListening();
  }
#endif
#if defined(SVTK_USE_TDX) && defined(Q_OS_MAC)
  if (this->UseTDx && !this->Device->GetInitialized())
  {
    this->Device->Initialize();
  }
#endif
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  if (this->UseTDx && this->Device != 0)
  {
    this->Device->SetInteractor(this);
  }
#endif
}

// ----------------------------------------------------------------------------
void QSVTKInteractor::StopListening()
{
#if defined(SVTK_USE_TDX) && defined(Q_OS_WIN)
  if (this->Device->GetInitialized() && this->Device->GetIsListening())
  {
    this->Device->StopListening();
  }
#endif
#if defined(SVTK_USE_TDX) && defined(Q_OS_MAC)
  if (this->UseTDx && this->Device->GetInitialized())
  {
    this->Device->Close();
  }
#endif
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  if (this->UseTDx && this->Device != 0)
  {
    // this assumes that a outfocus event is emitted prior
    // a infocus event on another widget.
    this->Device->SetInteractor(0);
  }
#endif
}

/*! handle timer event
 */
void QSVTKInteractor::TimerEvent(int timerId)
{
  if (!this->GetEnabled())
  {
    return;
  }
  this->InvokeEvent(svtkCommand::TimerEvent, (void*)&timerId);

  if (this->IsOneShotTimer(timerId))
  {
    this->DestroyTimer(timerId); // 'cause our Qt timers are always repeating
  }
}

/*! constructor
 */
QSVTKInteractor::~QSVTKInteractor()
{
  delete this->Internal;
#if defined(SVTK_USE_TDX) && defined(Q_OS_WIN)
  this->Device->Delete();
#endif
#if defined(SVTK_USE_TDX) && defined(Q_OS_MAC)
  this->Device->Delete();
#endif
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  this->Device = 0;
#endif
}

/*! create Qt timer with an interval of 10 msec.
 */
int QSVTKInteractor::InternalCreateTimer(
  int timerId, int svtkNotUsed(timerType), unsigned long duration)
{
  QTimer* timer = new QTimer(this->Internal);
  timer->start(duration);
  this->Internal->SignalMapper->setMapping(timer, timerId);
  QObject::connect(timer, SIGNAL(timeout()), this->Internal->SignalMapper, SLOT(map()));
  int platformTimerId = timer->timerId();
  this->Internal->Timers.insert(
    QSVTKInteractorInternal::TimerMap::value_type(platformTimerId, timer));
  return platformTimerId;
}

/*! destroy timer
 */
int QSVTKInteractor::InternalDestroyTimer(int platformTimerId)
{
  QSVTKInteractorInternal::TimerMap::iterator iter = this->Internal->Timers.find(platformTimerId);
  if (iter != this->Internal->Timers.end())
  {
    iter->second->stop();
    iter->second->deleteLater();
    this->Internal->Timers.erase(iter);
    return 1;
  }
  return 0;
}
