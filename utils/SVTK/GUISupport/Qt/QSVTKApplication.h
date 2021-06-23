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
// .NAME QSVTKApplication - A superclass for QApplication using SVTK.
// .SECTION Description
// This is a superclass for QApplication using SVTK. It essentially redefined
// x11EventFilter() in order to catch X11 ClientMessage coming from the
// 3DConnexion driver.
//
// You don't have to inherit from QSVTKApplication to be able to use SVTK:
// you can reimplement QSVTKApplication(), ~QSVTKApplication(), x11EventFilter(),
// setDevice(), CreateDevice() in your own subclass of QApplication.
// It you don't, SVTK will work but without the 3Dconnexion device under X11.
// In this case, QSVTKApplication provides a model of implementation.

// .SECTION See Also
// svtkTDxQtUnixDevices

#ifndef __QSVTKApplication_h
#define __QSVTKApplication_h

#include "QSVTKWin32Header.h"       // for SVTKGUISUPPORTQT_EXPORT
#include "svtkGUISupportQtModule.h" // For export macro
#include "svtkTDxConfigure.h"       // defines SVTK_USE_TDX

#include <QApplication>

#ifdef SVTK_USE_TDX
class svtkTDxDevice;
#if defined(Q_WS_X11) || defined(Q_OS_LINUX)
class svtkTDxQtUnixDevices;
#endif
#endif

class SVTKGUISUPPORTQT_EXPORT QSVTKApplication : public QApplication
{
  Q_OBJECT
public:
  // Description:
  // Constructor.
  QSVTKApplication(int& Argc, char** Argv);

  // Description:
  // Destructor.
  ~QSVTKApplication() override;

#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  // Description:
  // Intercept X11 events.
  // Redefined from QApplication.
  virtual bool x11EventFilter(XEvent* event);
#endif

#ifdef SVTK_USE_TDX
public Q_SLOTS:
  // Description:
  // Slot to receive signal CreateDevice coming from svtkTDxQtUnixDevices.
  // It re-emit signal CreateDevice
  // No-op if not X11 (ie Q_OS_LINUX and Q_WS_X11 is not defined).
  void setDevice(svtkTDxDevice* device);

Q_SIGNALS:
  // Description:
  // Signal for SVTKWidget slots.
  void CreateDevice(svtkTDxDevice* device);
#endif

protected:
#if defined(SVTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
  svtkTDxQtUnixDevices* Devices;
#endif
};

#endif
