/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyle3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInteractorStyle3D
 * @brief   extends interaction to support 3D input
 *
 * svtkInteractorStyle3D allows the user to interact with (rotate,
 * pan, etc.) objects in the scene indendent of each other. It is designed
 * to use 3d positions and orientations instead of 2D.
 *
 * The following interactions are specified by default.
 *
 * A click and hold in 3D within the bounding box of a prop
 * will pick up that prop allowing you to translate and
 * orient that prop as desired with the 3D controller.
 *
 * Click/dragging two controllers and pulling them apart or
 * pushing them together will initial a scale gesture
 * that will scale the world larger or smaller.
 *
 * Click/dragging two controllers and translating them in the same
 * direction will translate the camera/world
 * pushing them together will initial a scale gesture
 * that will scale the world larger or smaller.
 *
 * If a controller is right clicked (push touchpad on Vive)
 * then it starts a fly motion where the camer moves in the
 * direction the controller is pointing. It moves at a speed
 * scaled by the position of your thumb on the trackpad.
 * Higher moves faster forward. Lower moves faster backwards.
 *
 * For the Vive left click is mapped to the trigger and right
 * click is mapped to pushing the trackpad down.
 *
 * @sa
 * svtkRenderWindowInteractor3D
 */

#ifndef svtkInteractorStyle3D_h
#define svtkInteractorStyle3D_h

#include "svtkInteractorStyle.h"
#include "svtkNew.h"                 // ivars
#include "svtkRenderingCoreModule.h" // For export macro

class svtkAbstractPropPicker;
class svtkCamera;
class svtkProp3D;
class svtkMatrix3x3;
class svtkMatrix4x4;
class svtkTimerLog;
class svtkTransform;

class SVTKRENDERINGCORE_EXPORT svtkInteractorStyle3D : public svtkInteractorStyle
{
public:
  static svtkInteractorStyle3D* New();
  svtkTypeMacro(svtkInteractorStyle3D, svtkInteractorStyle);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // This method handles updating the prop based on changes in the devices
  // pose. We use rotate as the state to mean adjusting-the-actor-pose
  virtual void PositionProp(svtkEventData*);

  // This method handles updating the camera based on changes in the devices
  // pose. We use Dolly as the state to mean moving the camera forward
  virtual void Dolly3D(svtkEventData*);

  //@{
  /**
   * Set/Get the maximum dolly speed used when flying in 3D, in meters per second.
   * Default is 1.6666, corresponding to walking speed (= 6 km/h).
   * This speed is scaled by the touchpad position as well.
   */
  svtkSetMacro(DollyPhysicalSpeed, double);
  svtkGetMacro(DollyPhysicalSpeed, double);
  //@}

  /**
   * Set the scaling factor from world to physical space.
   * In VR when we set it to a new value we also adjust the
   * HMD position to maintain the same relative position.
   */
  virtual void SetScale(svtkCamera* cam, double newScale);

  //@{
  /**
   * Get/Set the interaction picker.
   * By default, a svtkPropPicker is instancied.
   */
  svtkGetObjectMacro(InteractionPicker, svtkAbstractPropPicker);
  void SetInteractionPicker(svtkAbstractPropPicker* prop);

protected:
  svtkInteractorStyle3D();
  ~svtkInteractorStyle3D() override;

  void FindPickedActor(double pos[3], double orient[4]);

  void Prop3DTransform(
    svtkProp3D* prop3D, double* boxCenter, int NumRotation, double** rotate, double* scale);

  svtkAbstractPropPicker* InteractionPicker;
  svtkProp3D* InteractionProp;
  svtkMatrix3x3* TempMatrix3;
  svtkMatrix4x4* TempMatrix4;

  svtkTransform* TempTransform;
  double AppliedTranslation[3];

  double DollyPhysicalSpeed;
  svtkNew<svtkTimerLog> LastDolly3DEventTime;

private:
  svtkInteractorStyle3D(const svtkInteractorStyle3D&) = delete; // Not implemented.
  void operator=(const svtkInteractorStyle3D&) = delete;       // Not implemented.
};

#endif
