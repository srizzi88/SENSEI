/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleSwitchBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyleSwitchBase
 * @brief   dummy interface class.
 *
 * The class svtkInteractorStyleSwitchBase is here to allow the
 * svtkRenderWindowInteractor to instantiate a default interactor style and
 * preserve backward compatible behavior when the object factory is overridden
 * and svtkInteractorStyleSwitch is returned.
 *
 * @sa
 * svtkInteractorStyleSwitchBase svtkRenderWindowInteractor
 */

#ifndef svtkInteractorStyleSwitchBase_h
#define svtkInteractorStyleSwitchBase_h

#include "svtkInteractorStyle.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkInteractorStyleSwitchBase : public svtkInteractorStyle
{
public:
  static svtkInteractorStyleSwitchBase* New();
  svtkTypeMacro(svtkInteractorStyleSwitchBase, svtkInteractorStyle);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkRenderWindowInteractor* GetInteractor() override;

protected:
  svtkInteractorStyleSwitchBase();
  ~svtkInteractorStyleSwitchBase() override;

private:
  svtkInteractorStyleSwitchBase(const svtkInteractorStyleSwitchBase&) = delete;
  void operator=(const svtkInteractorStyleSwitchBase&) = delete;
};

#endif
