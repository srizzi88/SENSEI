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
/**
 * @class   svtkTDxQtUnixDevices
 * @brief   Manage a list svtkTDXUnixDevice(s).
 *
 * This class is essentially a map between a X11 window id and a
 * svtkTDXUnixDevice. It is uses internally by QSVTKApplication.
 *
 * @sa
 * svtkTDxUnixDevice QSVTKOpenGLNativeWidget QSVTKApplication
 */

#ifndef svtkTDxQtUnixDevices_h
#define svtkTDxQtUnixDevices_h

#include "QSVTKWin32Header.h"       // for SVTKGUISUPPORTQT_EXPORT
#include "svtkGUISupportQtModule.h" // For export macro
#include "svtkTDxUnixDevice.h"      // required for svtkTDxUnixDeviceXEvent
#include <QObject>

class svtkTDxQtUnixDevicesPrivate;

class SVTKGUISUPPORTQT_EXPORT svtkTDxQtUnixDevices : public QObject
{
  Q_OBJECT
public:
  svtkTDxQtUnixDevices();
  ~svtkTDxQtUnixDevices();

  /**
   * Process X11 event `e'. Create a device and emit signal CreateDevice if it
   * does not exist yet.
   * \pre e_exists: e!=0
   */
  void ProcessEvent(svtkTDxUnixDeviceXEvent* e);

signals:
  /**
   * This signal should be connected to a slot in the QApplication.
   * The slot in the QApplication is supposed to remit this signal.
   * The QSVTKOpenGLNativeWidget have slot to receive this signal from the QApplication.
   */
  void CreateDevice(svtkTDxDevice* device);

protected:
  svtkTDxQtUnixDevicesPrivate* Private;

private:
  svtkTDxQtUnixDevices(const svtkTDxQtUnixDevices&) = delete;
  void operator=(const svtkTDxQtUnixDevices&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkTDxQtUnixDevices.h
