/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderWindowInteractor3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderWindowInteractor3D
 * @brief   adds support for 3D events to svtkRenderWindowInteractor.
 *
 *
 * svtkRenderWindowInteractor3D provides a platform-independent interaction
 * support for 3D events including 3D clicks and 3D controller
 * orientations. It follows the same basic model as
 * svtkRenderWindowInteractor but adds methods to set and get 3D event
 * locations and orientations. VR systems will subclass this class to
 * provide the code to set these values based on events from their VR
 * controllers.
 */

#ifndef svtkRenderWindowInteractor3D_h
#define svtkRenderWindowInteractor3D_h

#include "svtkRenderWindowInteractor.h"
#include "svtkRenderingCoreModule.h" // For export macro

#include "svtkNew.h" // ivars

class svtkCamera;
class svtkMatrix4x4;
enum class svtkEventDataDevice;
enum class svtkEventDataDeviceInput;

class SVTKRENDERINGCORE_EXPORT svtkRenderWindowInteractor3D : public svtkRenderWindowInteractor
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static svtkRenderWindowInteractor3D* New();

  svtkTypeMacro(svtkRenderWindowInteractor3D, svtkRenderWindowInteractor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Enable/Disable interactions.  By default interactors are enabled when
   * initialized.  Initialize() must be called prior to enabling/disabling
   * interaction. These methods are used when a window/widget is being
   * shared by multiple renderers and interactors.  This allows a "modal"
   * display where one interactor is active when its data is to be displayed
   * and all other interactors associated with the widget are disabled
   * when their data is not displayed.
   */
  void Enable() override;
  void Disable() override;
  //@}

  //@{
  /**
   * With VR we know the world coordinate positions and orientations of events.
   * These methods support querying them instead of going through a display X,Y
   * coordinate approach as is standard for mouse/touch events
   */
  virtual double* GetWorldEventPosition(int pointerIndex)
  {
    if (pointerIndex >= SVTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->WorldEventPositions[pointerIndex];
  }
  virtual double* GetLastWorldEventPosition(int pointerIndex)
  {
    if (pointerIndex >= SVTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->LastWorldEventPositions[pointerIndex];
  }
  virtual double* GetWorldEventOrientation(int pointerIndex)
  {
    if (pointerIndex >= SVTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->WorldEventOrientations[pointerIndex];
  }
  virtual double* GetLastWorldEventOrientation(int pointerIndex)
  {
    if (pointerIndex >= SVTKI_MAX_POINTERS)
    {
      return nullptr;
    }
    return this->LastWorldEventOrientations[pointerIndex];
  }
  virtual void GetWorldEventPose(svtkMatrix4x4* poseMatrix, int pointerIndex);
  virtual void GetLastWorldEventPose(svtkMatrix4x4* poseMatrix, int pointerIndex);
  //@}

  //@{
  /**
   * With VR we know the physical/room coordinate positions
   * and orientations of events.
   * These methods support setting them.
   */
  virtual void SetPhysicalEventPosition(double x, double y, double z, int pointerIndex)
  {
    if (pointerIndex < 0 || pointerIndex >= SVTKI_MAX_POINTERS)
    {
      return;
    }
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting PhysicalEventPosition to ("
                  << x << "," << y << "," << z << ") for pointerIndex number " << pointerIndex);
    if (this->PhysicalEventPositions[pointerIndex][0] != x ||
      this->PhysicalEventPositions[pointerIndex][1] != y ||
      this->PhysicalEventPositions[pointerIndex][2] != z ||
      this->LastPhysicalEventPositions[pointerIndex][0] != x ||
      this->LastPhysicalEventPositions[pointerIndex][1] != y ||
      this->LastPhysicalEventPositions[pointerIndex][2] != z)
    {
      this->LastPhysicalEventPositions[pointerIndex][0] =
        this->PhysicalEventPositions[pointerIndex][0];
      this->LastPhysicalEventPositions[pointerIndex][1] =
        this->PhysicalEventPositions[pointerIndex][1];
      this->LastPhysicalEventPositions[pointerIndex][2] =
        this->PhysicalEventPositions[pointerIndex][2];
      this->PhysicalEventPositions[pointerIndex][0] = x;
      this->PhysicalEventPositions[pointerIndex][1] = y;
      this->PhysicalEventPositions[pointerIndex][2] = z;
      this->Modified();
    }
  }
  virtual void SetPhysicalEventPose(svtkMatrix4x4* poseMatrix, int pointerIndex);
  //@}

  //@{
  /**
   * With VR we know the physical/room coordinate positions
   * and orientations of events.
   * These methods support getting them.
   */
  virtual void GetPhysicalEventPose(svtkMatrix4x4* poseMatrix, int pointerIndex);
  virtual void GetLastPhysicalEventPose(svtkMatrix4x4* poseMatrix, int pointerIndex);
  virtual void GetStartingPhysicalEventPose(svtkMatrix4x4* poseMatrix, int pointerIndex);
  //@}

  //@{
  /**
   * With VR we know the world coordinate positions
   * and orientations of events. These methods
   * support setting them.
   */
  virtual void SetWorldEventPosition(double x, double y, double z, int pointerIndex)
  {
    if (pointerIndex < 0 || pointerIndex >= SVTKI_MAX_POINTERS)
    {
      return;
    }
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting WorldEventPosition to ("
                  << x << "," << y << "," << z << ") for pointerIndex number " << pointerIndex);
    if (this->WorldEventPositions[pointerIndex][0] != x ||
      this->WorldEventPositions[pointerIndex][1] != y ||
      this->WorldEventPositions[pointerIndex][2] != z ||
      this->LastWorldEventPositions[pointerIndex][0] != x ||
      this->LastWorldEventPositions[pointerIndex][1] != y ||
      this->LastWorldEventPositions[pointerIndex][2] != z)
    {
      this->LastWorldEventPositions[pointerIndex][0] = this->WorldEventPositions[pointerIndex][0];
      this->LastWorldEventPositions[pointerIndex][1] = this->WorldEventPositions[pointerIndex][1];
      this->LastWorldEventPositions[pointerIndex][2] = this->WorldEventPositions[pointerIndex][2];
      this->WorldEventPositions[pointerIndex][0] = x;
      this->WorldEventPositions[pointerIndex][1] = y;
      this->WorldEventPositions[pointerIndex][2] = z;
      this->Modified();
    }
  }
  virtual void SetWorldEventOrientation(double w, double x, double y, double z, int pointerIndex)
  {
    if (pointerIndex < 0 || pointerIndex >= SVTKI_MAX_POINTERS)
    {
      return;
    }
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting WorldEventOrientation to ("
                  << w << "," << x << "," << y << "," << z << ") for pointerIndex number "
                  << pointerIndex);
    if (this->WorldEventOrientations[pointerIndex][0] != w ||
      this->WorldEventOrientations[pointerIndex][1] != x ||
      this->WorldEventOrientations[pointerIndex][2] != y ||
      this->WorldEventOrientations[pointerIndex][3] != z ||
      this->LastWorldEventOrientations[pointerIndex][0] != w ||
      this->LastWorldEventOrientations[pointerIndex][1] != x ||
      this->LastWorldEventOrientations[pointerIndex][2] != y ||
      this->LastWorldEventOrientations[pointerIndex][3] != z)
    {
      this->LastWorldEventOrientations[pointerIndex][0] =
        this->WorldEventOrientations[pointerIndex][0];
      this->LastWorldEventOrientations[pointerIndex][1] =
        this->WorldEventOrientations[pointerIndex][1];
      this->LastWorldEventOrientations[pointerIndex][2] =
        this->WorldEventOrientations[pointerIndex][2];
      this->LastWorldEventOrientations[pointerIndex][3] =
        this->WorldEventOrientations[pointerIndex][3];
      this->WorldEventOrientations[pointerIndex][0] = w;
      this->WorldEventOrientations[pointerIndex][1] = x;
      this->WorldEventOrientations[pointerIndex][2] = y;
      this->WorldEventOrientations[pointerIndex][3] = z;
      this->Modified();
    }
  }
  virtual void SetWorldEventPose(svtkMatrix4x4* poseMatrix, int pointerIndex);
  //@}

  //@{
  /**
   * Override to set pointers down
   */
  void RightButtonPressEvent() override;
  void RightButtonReleaseEvent() override;
  //@}

  //@{
  /**
   * Override to set pointers down
   */
  void MiddleButtonPressEvent() override;
  void MiddleButtonReleaseEvent() override;
  //@}

  //@{
  /**
   * Get the latest touchpad or joystick position for a device
   */
  virtual void GetTouchPadPosition(svtkEventDataDevice, svtkEventDataDeviceInput, float[3]) {}
  //@}

  //@{
  /**
   * Set/Get the optional scale translation to map world coordinates into the
   * 3D physical space (meters, 0,0,0).
   */
  virtual void SetPhysicalTranslation(svtkCamera*, double, double, double) {}
  virtual double* GetPhysicalTranslation(svtkCamera*) { return nullptr; }
  virtual void SetPhysicalScale(double) {}
  virtual double GetPhysicalScale() { return 1.0; }
  //@}

  //@{
  /**
   * Set/get the translation for pan/swipe gestures, update LastTranslation
   */
  void SetTranslation3D(double val[3]);
  svtkGetVector3Macro(Translation3D, double);
  svtkGetVector3Macro(LastTranslation3D, double);
  //@}

protected:
  svtkRenderWindowInteractor3D();
  ~svtkRenderWindowInteractor3D() override;

  int MouseInWindow;
  int StartedMessageLoop;
  double Translation3D[3];
  double LastTranslation3D[3];

  double WorldEventPositions[SVTKI_MAX_POINTERS][3];
  double LastWorldEventPositions[SVTKI_MAX_POINTERS][3];
  double PhysicalEventPositions[SVTKI_MAX_POINTERS][3];
  double LastPhysicalEventPositions[SVTKI_MAX_POINTERS][3];
  double StartingPhysicalEventPositions[SVTKI_MAX_POINTERS][3];
  double WorldEventOrientations[SVTKI_MAX_POINTERS][4];
  double LastWorldEventOrientations[SVTKI_MAX_POINTERS][4];
  svtkNew<svtkMatrix4x4> WorldEventPoses[SVTKI_MAX_POINTERS];
  svtkNew<svtkMatrix4x4> LastWorldEventPoses[SVTKI_MAX_POINTERS];
  svtkNew<svtkMatrix4x4> PhysicalEventPoses[SVTKI_MAX_POINTERS];
  svtkNew<svtkMatrix4x4> LastPhysicalEventPoses[SVTKI_MAX_POINTERS];
  svtkNew<svtkMatrix4x4> StartingPhysicalEventPoses[SVTKI_MAX_POINTERS];
  void RecognizeGesture(svtkCommand::EventIds) override;

private:
  svtkRenderWindowInteractor3D(const svtkRenderWindowInteractor3D&) = delete; // Not implemented.
  void operator=(const svtkRenderWindowInteractor3D&) = delete;              // Not implemented.
};

#endif
