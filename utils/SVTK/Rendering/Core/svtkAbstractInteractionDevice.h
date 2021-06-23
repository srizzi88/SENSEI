/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkAbstractInteractionDevice_h
#define svtkAbstractInteractionDevice_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderWidget;
class svtkAbstractRenderDevice;

class SVTKRENDERINGCORE_EXPORT svtkAbstractInteractionDevice : public svtkObject
{
public:
  svtkTypeMacro(svtkAbstractInteractionDevice, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * @brief Make a new device, this class is abstract and one of its derived
   * forms will be returned, or NULL if no override has been provided.
   * @return A derived interaction device, or NULL if no suitable override is set.
   */
  static svtkAbstractInteractionDevice* New();

  /**
   * @brief Initialize the interaction device.
   */
  virtual void Initialize() = 0;

  /**
   * @brief Start the event loop.
   */
  virtual void Start() = 0;

  /**
   * @brief Process any pending events, this can be used to process OS level
   * events without running a full event loop.
   */
  virtual void ProcessEvents() = 0;

  void SetRenderWidget(svtkRenderWidget* widget);
  svtkRenderWidget* GetRenderWidget() { return this->RenderWidget; }
  void SetRenderDevice(svtkAbstractRenderDevice* device);
  svtkAbstractRenderDevice* GetRenderDevice() { return this->RenderDevice; }

protected:
  svtkAbstractInteractionDevice();
  ~svtkAbstractInteractionDevice() override;

  bool Initialized;
  svtkRenderWidget* RenderWidget;
  svtkAbstractRenderDevice* RenderDevice;

private:
  svtkAbstractInteractionDevice(const svtkAbstractInteractionDevice&) = delete;
  void operator=(const svtkAbstractInteractionDevice&) = delete;
};

#endif
