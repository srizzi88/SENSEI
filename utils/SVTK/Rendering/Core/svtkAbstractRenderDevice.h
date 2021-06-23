/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkAbstractRenderDevice_h
#define svtkAbstractRenderDevice_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include <string>                   // For std::string

class svtkRecti;

class SVTKRENDERINGCORE_EXPORT svtkAbstractRenderDevice : public svtkObject
{
public:
  svtkTypeMacro(svtkAbstractRenderDevice, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * @brief Make a new device, this class is abstract and one of its derived
   * forms will be returned, or NULL if no override has been provided.
   * @return A derived render device, or NULL if no suitable override is set.
   */
  static svtkAbstractRenderDevice* New();

  /**
   * @brief Set the context that should be requested (must be set before the
   * widget is rendered for the first time.
   * @param major Major GL version, default is 2.
   * @param minor Minor GL version, default is 1.
   */
  void SetRequestedGLVersion(int major, int minor);

  /**
   * @brief Create a window with the desired geometry.
   * @param geometry The geometry in screen coordinates for the window.
   * @return True on success, false on failure.
   */
  virtual bool CreateNewWindow(const svtkRecti& geometry, const std::string& name) = 0;

  /**
   * @brief Make the context current so that it can be used by OpenGL. This is
   * an expensive call, and so its use should be minimized to once per render
   * ideally.
   */
  virtual void MakeCurrent() = 0;

protected:
  svtkAbstractRenderDevice();
  ~svtkAbstractRenderDevice() override;

  int GLMajor;
  int GLMinor;

private:
  svtkAbstractRenderDevice(const svtkAbstractRenderDevice&) = delete;
  void operator=(const svtkAbstractRenderDevice&) = delete;
};

#endif
