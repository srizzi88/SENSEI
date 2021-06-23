/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QSVTKApplication.h"

#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
#include "svtkTDxQtUnixDevices.h"
#include <QWidget>
#include <X11/Xlib.h> // Needed for X types used in the public interface
#endif

// ----------------------------------------------------------------------------
QSVTKApplication::QSVTKApplication(int& Argc, char** Argv)
  : QApplication(Argc, Argv)
{
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  this->Devices = new svtkTDxQtUnixDevices;
  QObject::connect(
    this->Devices, SIGNAL(CreateDevice(svtkTDxDevice*)), this, SLOT(setDevice(svtkTDxDevice*)));
#endif
}

// ----------------------------------------------------------------------------
QSVTKApplication::~QSVTKApplication()
{
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  delete this->Devices;
#endif
}

// ----------------------------------------------------------------------------
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
bool QSVTKApplication::x11EventFilter(XEvent* event)
{
  // the only lines required in this method
  this->Devices->ProcessEvent(static_cast<svtkTDxUnixDeviceXEvent*>(event));
  return false;
}
#endif

// ----------------------------------------------------------------------------
#ifdef SVTK_USE_TDX
void QSVTKApplication::setDevice(svtkTDxDevice* device)
{
#ifdef Q_WS_X11 || Q_OS_LINUX
  emit CreateDevice(device);
#else
  (void)device; // to avoid warnings.
#endif
}
#endif
