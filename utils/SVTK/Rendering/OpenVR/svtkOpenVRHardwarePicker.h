/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpemVRHardwarePicker.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRHardwarePicker
 * @brief   pick an actor/prop given a controller position and orientation
 *
 * svtkOpenVRHardwarePicker is used to pick an actor/prop along a ray.
 * This version uses a hardware selector to do the picking.
 *
 * @sa
 * svtkProp3DPicker svtkOpenVRInteractorStylePointer
 */

#ifndef svtkOpenVRHardwarePicker_h
#define svtkOpenVRHardwarePicker_h

#include "svtkPropPicker.h"
#include "svtkRenderingOpenVRModule.h" // For export macro

class svtkSelection;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRHardwarePicker : public svtkPropPicker
{
public:
  static svtkOpenVRHardwarePicker* New();

  svtkTypeMacro(svtkOpenVRHardwarePicker, svtkPropPicker);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform a pick from the user-provided list of svtkProps.
   */
  virtual int PickProp(double selectionPt[3], double eventWorldOrientation[4],
    svtkRenderer* renderer, svtkPropCollection* pickfrom, bool actorPassOnly);

  svtkSelection* GetSelection() { return this->Selection; }

protected:
  svtkOpenVRHardwarePicker();
  ~svtkOpenVRHardwarePicker() override;

  void Initialize() override;
  svtkSelection* Selection;

private:
  svtkOpenVRHardwarePicker(const svtkOpenVRHardwarePicker&) = delete; // Not implemented.
  void operator=(const svtkOpenVRHardwarePicker&) = delete;          // Not implemented.
};

#endif
