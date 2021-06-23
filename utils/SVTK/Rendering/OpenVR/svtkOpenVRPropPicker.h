/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRPropPicker.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRPropPicker
 * @brief   Deprecated. Use svtkPropPicker directly
 */

#ifndef svtkOpenVRPropPicker_h
#define svtkOpenVRPropPicker_h

#include "svtkPropPicker.h"
#include "svtkRenderingOpenVRModule.h" // For export macro

class svtkProp;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRPropPicker : public svtkPropPicker
{
public:
  static svtkOpenVRPropPicker* New();

  svtkTypeMacro(svtkOpenVRPropPicker, svtkPropPicker);

  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkOpenVRPropPicker();
  ~svtkOpenVRPropPicker() override;

  void Initialize() override;

private:
  svtkOpenVRPropPicker(const svtkOpenVRPropPicker&) = delete; // Not implemented.
  void operator=(const svtkOpenVRPropPicker&) = delete;      // Not implemented.
};

#endif
