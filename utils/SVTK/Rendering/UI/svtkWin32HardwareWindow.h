/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHardwareWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWin32HardwareWindow
 * @brief   represents a window in a windows GUI
 */

#ifndef svtkWin32HardwareWindow_h
#define svtkWin32HardwareWindow_h

#include "svtkHardwareWindow.h"
#include "svtkRenderingUIModule.h" // For export macro
#include "svtkWindows.h"           // For windows API

class SVTKRENDERINGUI_EXPORT svtkWin32HardwareWindow : public svtkHardwareWindow
{
public:
  static svtkWin32HardwareWindow* New();
  svtkTypeMacro(svtkWin32HardwareWindow, svtkHardwareWindow);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  HINSTANCE GetApplicationInstance();

  HWND GetWindowId();

  void Create() override;
  void Destroy() override;

  //@{
  /**
   * These are window system independent methods that are used
   * to help interface svtkWindow to native windowing systems.
   */
  void SetDisplayId(void*) override;
  void SetWindowId(void*) override;
  void SetParentId(void*) override;
  void* GetGenericDisplayId() override;
  void* GetGenericWindowId() override;
  void* GetGenericParentId() override;
  //@}

protected:
  svtkWin32HardwareWindow();
  ~svtkWin32HardwareWindow() override;

  HWND ParentId;
  HWND WindowId;
  HINSTANCE ApplicationInstance;

private:
  svtkWin32HardwareWindow(const svtkWin32HardwareWindow&) = delete;
  void operator=(const svtkWin32HardwareWindow&) = delete;
};

#endif
