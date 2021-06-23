/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWorldPointPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWorldPointPicker
 * @brief   find world x,y,z corresponding to display x,y,z
 *
 * svtkWorldPointPicker is used to find the x,y,z world coordinate of a
 * screen x,y,z. This picker cannot pick actors and/or mappers, it
 * simply determines an x-y-z coordinate in world space. (It will always
 * return a x-y-z, even if the selection point is not over a prop/actor.)
 *
 * @warning
 * The PickMethod() is not invoked, but StartPickMethod() and EndPickMethod()
 * are.
 *
 * @sa
 * svtkPropPicker svtkPicker svtkCellPicker svtkPointPicker
 */

#ifndef svtkWorldPointPicker_h
#define svtkWorldPointPicker_h

#include "svtkAbstractPicker.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkWorldPointPicker : public svtkAbstractPicker
{
public:
  static svtkWorldPointPicker* New();
  svtkTypeMacro(svtkWorldPointPicker, svtkAbstractPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Perform the pick. (This method overload's the superclass.)
   */
  int Pick(double selectionX, double selectionY, double selectionZ, svtkRenderer* renderer) override;
  int Pick(double selectionPt[3], svtkRenderer* renderer)
  {
    return this->svtkAbstractPicker::Pick(selectionPt, renderer);
  }
  //@}

protected:
  svtkWorldPointPicker();
  ~svtkWorldPointPicker() override {}

private:
  svtkWorldPointPicker(const svtkWorldPointPicker&) = delete;
  void operator=(const svtkWorldPointPicker&) = delete;
};

#endif
