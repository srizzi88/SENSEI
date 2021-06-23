/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPropPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPropPicker
 * @brief   pick an actor/prop using graphics hardware
 *
 * svtkPropPicker is used to pick an actor/prop given a selection
 * point (in display coordinates) and a renderer. This class uses
 * graphics hardware/rendering system to pick rapidly (as compared
 * to using ray casting as does svtkCellPicker and svtkPointPicker).
 * This class determines the actor/prop and pick position in world
 * coordinates; point and cell ids are not determined.
 *
 * @sa
 * svtkPicker svtkWorldPointPicker svtkCellPicker svtkPointPicker
 */

#ifndef svtkPropPicker_h
#define svtkPropPicker_h

#include "svtkAbstractPropPicker.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkProp;
class svtkWorldPointPicker;

class SVTKRENDERINGCORE_EXPORT svtkPropPicker : public svtkAbstractPropPicker
{
public:
  static svtkPropPicker* New();

  svtkTypeMacro(svtkPropPicker, svtkAbstractPropPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform the pick and set the PickedProp ivar. If something is picked, a
   * 1 is returned, otherwise 0 is returned.  Use the GetViewProp() method
   * to get the instance of svtkProp that was picked.  Props are picked from
   * the renderers list of pickable Props.
   */
  int PickProp(double selectionX, double selectionY, svtkRenderer* renderer);

  /**
   * Perform a pick from the user-provided list of svtkProps and not from the
   * list of svtkProps that the render maintains.
   */
  int PickProp(
    double selectionX, double selectionY, svtkRenderer* renderer, svtkPropCollection* pickfrom);

  /**
   * override superclasses' Pick() method.
   */
  int Pick(double selectionX, double selectionY, double selectionZ, svtkRenderer* renderer) override;
  int Pick(double selectionPt[3], svtkRenderer* renderer)
  {
    return this->Pick(selectionPt[0], selectionPt[1], selectionPt[2], renderer);
  }

  /**
   * Perform pick operation with selection point provided. The
   * selectionPt is in world coordinates.
   * Return non-zero if something was successfully picked.
   */
  int Pick3DPoint(double selectionPt[3], svtkRenderer* ren) override;

  /**
   * Perform the pick and set the PickedProp ivar. If something is picked, a
   * 1 is returned, otherwise 0 is returned.  Use the GetViewProp() method
   * to get the instance of svtkProp that was picked.  Props are picked from
   * the renderers list of pickable Props.
   */
  int PickProp3DPoint(double pos[3], svtkRenderer* renderer);

  /**
   * Perform a pick from the user-provided list of svtkProps and not from the
   * list of svtkProps that the render maintains.
   */
  int PickProp3DPoint(double pos[3], svtkRenderer* renderer, svtkPropCollection* pickfrom);

  /**
   * Perform a pick from the user-provided list of svtkProps.
   */
  virtual int PickProp3DRay(double selectionPt[3], double eventWorldOrientation[4],
    svtkRenderer* renderer, svtkPropCollection* pickfrom);

  /**
   * Perform pick operation with selection point provided. The
   * selectionPt is in world coordinates.
   * Return non-zero if something was successfully picked.
   */
  int Pick3DRay(double selectionPt[3], double orient[4], svtkRenderer* ren) override;

protected:
  svtkPropPicker();
  ~svtkPropPicker() override;

  void Initialize() override;

  svtkPropCollection* PickFromProps;

  // Used to get x-y-z pick position
  svtkWorldPointPicker* WorldPointPicker;

private:
  svtkPropPicker(const svtkPropPicker&) = delete;
  void operator=(const svtkPropPicker&) = delete;
};

#endif
