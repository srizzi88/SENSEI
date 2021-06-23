/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkDepthPeelingPass.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTDxQtUnixDevices.h"
#include "svtkSmartPointer.h"
#include <QApplication> // topLevelWidgets()
#include <QWidget>
#include <X11/Xlib.h> // Needed for X types used in the public interface
#include <map>

class svtkLessThanWindowId
{
public:
  bool operator()(const svtkTDxUnixDeviceWindow& w1, const svtkTDxUnixDeviceWindow& w2) const
  {
    return w1 < w2;
  }
};

typedef std::map<svtkTDxUnixDeviceWindow, svtkSmartPointer<svtkTDxUnixDevice>, svtkLessThanWindowId>
  svtkWindowIdToDevice;

class svtkTDxQtUnixDevicesPrivate
{
public:
  svtkWindowIdToDevice Map;
};

// ----------------------------------------------------------------------------
svtkTDxQtUnixDevices::svtkTDxQtUnixDevices()
{
  this->Private = new svtkTDxQtUnixDevicesPrivate;
}

// ----------------------------------------------------------------------------
svtkTDxQtUnixDevices::~svtkTDxQtUnixDevices()
{
  delete this->Private;
}

// ----------------------------------------------------------------------------
void svtkTDxQtUnixDevices::ProcessEvent(svtkTDxUnixDeviceXEvent* e)
{
  const XEvent* event = static_cast<const XEvent*>(e);

  // Find the real X11 window id.
  QWidgetList l = static_cast<QApplication*>(QApplication::instance())->topLevelWidgets();
  int winIdLast = 0;
  foreach (QWidget* w, l)
  {
    if (!w->isHidden())
    {
      winIdLast = w->winId();
    }
  }

  svtkTDxUnixDeviceWindow winId = static_cast<svtkTDxUnixDeviceWindow>(winIdLast);
  // ;event->xany.window;

  if (winId != 0)
  {
    svtkWindowIdToDevice::iterator it = this->Private->Map.find(winId);
    svtkSmartPointer<svtkTDxUnixDevice> device = 0;
    if (it == this->Private->Map.end())
    {
      // not yet created.
      device = svtkSmartPointer<svtkTDxUnixDevice>::New();
      this->Private->Map.insert(
        std::pair<const svtkTDxUnixDeviceWindow, svtkSmartPointer<svtkTDxUnixDevice> >(winId, device));

      device->SetDisplayId(event->xany.display);
      device->SetWindowId(winId);
      device->SetInteractor(0);
      device->Initialize();

      if (!device->GetInitialized())
      {
        svtkGenericWarningMacro("failed to initialize device.");
      }
      else
      {
        cout << "device initialized on window" << winId << hex << winId << dec;
        emit CreateDevice(device);
      }
    }
    else
    {
      device = (*it).second;
    }

    if (event->type == ClientMessage) // 33
    {
      bool caught = false;
      if (device->GetInitialized())
      {
        caught = device->ProcessEvent(e);
      }
    }
  }
}
