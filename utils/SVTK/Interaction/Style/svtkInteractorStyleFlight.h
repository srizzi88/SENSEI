/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleFlight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkInteractorStyleFlight
 * @brief   provides flight motion routines
 *
 *
 * Left  mouse button press produces forward motion.
 * Right mouse button press produces reverse motion.
 * Moving mouse during motion steers user in desired direction.
 * Keyboard controls are:
 * Left/Right/Up/Down Arrows for steering direction
 * 'A' forward, 'Z' reverse motion
 * Ctrl Key causes sidestep instead of steering in mouse and key modes
 * Shift key is accelerator in mouse and key modes
 * Ctrl and Shift together causes Roll in mouse and key modes
 *
 * By default, one "step" of motion corresponds to 1/250th of the diagonal
 * of bounding box of visible actors, '+' and '-' keys allow user to
 * increase or decrease step size.
 */

#ifndef svtkInteractorStyleFlight_h
#define svtkInteractorStyleFlight_h

#include "svtkInteractionStyleModule.h" // For export macro
#include "svtkInteractorStyle.h"
class svtkCamera;
class svtkPerspectiveTransform;

class CPIDControl;

class SVTKINTERACTIONSTYLE_EXPORT svtkInteractorStyleFlight : public svtkInteractorStyle
{
public:
  static svtkInteractorStyleFlight* New();
  svtkTypeMacro(svtkInteractorStyleFlight, svtkInteractorStyle);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Move the Eye/Camera to a specific location (no intermediate
   * steps are taken
   */
  void JumpTo(double campos[3], double focpos[3]);

  //@{
  /**
   * Set the basic unit step size : by default 1/250 of bounding diagonal
   */
  svtkSetMacro(MotionStepSize, double);
  svtkGetMacro(MotionStepSize, double);
  //@}

  //@{
  /**
   * Set acceleration factor when shift key is applied : default 10
   */
  svtkSetMacro(MotionAccelerationFactor, double);
  svtkGetMacro(MotionAccelerationFactor, double);
  //@}

  //@{
  /**
   * Set the basic angular unit for turning : default 1 degree
   */
  svtkSetMacro(AngleStepSize, double);
  svtkGetMacro(AngleStepSize, double);
  //@}

  //@{
  /**
   * Set angular acceleration when shift key is applied : default 5
   */
  svtkSetMacro(AngleAccelerationFactor, double);
  svtkGetMacro(AngleAccelerationFactor, double);
  //@}

  //@{
  /**
   * Disable motion (temporarily - for viewing etc)
   */
  svtkSetMacro(DisableMotion, svtkTypeBool);
  svtkGetMacro(DisableMotion, svtkTypeBool);
  svtkBooleanMacro(DisableMotion, svtkTypeBool);
  //@}

  //@{
  /**
   * When flying, apply a restorative force to the "Up" vector.
   * This is activated when the current 'up' is close to the actual 'up'
   * (as defined in DefaultUpVector). This prevents excessive twisting forces
   * when viewing from arbitrary angles, but keep the horizon level when
   * the user is flying over terrain.
   */
  svtkSetMacro(RestoreUpVector, svtkTypeBool);
  svtkGetMacro(RestoreUpVector, svtkTypeBool);
  svtkBooleanMacro(RestoreUpVector, svtkTypeBool);
  //@}

  // Specify "up" (by default {0,0,1} but can be changed)
  svtkGetVectorMacro(DefaultUpVector, double, 3);
  svtkSetVectorMacro(DefaultUpVector, double, 3);

  //@{
  /**
   * Concrete implementation of Mouse event bindings for flight
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  //@}

  //@{
  /**
   * Concrete implementation of Keyboard event bindings for flight
   */
  void OnChar() override;
  void OnKeyDown() override;
  void OnKeyUp() override;
  void OnTimer() override;
  //
  virtual void ForwardFly();
  virtual void ReverseFly();
  //
  virtual void StartForwardFly();
  virtual void EndForwardFly();
  virtual void StartReverseFly();
  virtual void EndReverseFly();
  //@}

protected:
  svtkInteractorStyleFlight();
  ~svtkInteractorStyleFlight() override;

  //@{
  /**
   * Routines used internally for computing motion and steering
   */
  void UpdateSteering(svtkCamera* cam);
  void UpdateMouseSteering(svtkCamera* cam);
  void FlyByMouse(svtkCamera* cam);
  void FlyByKey(svtkCamera* cam);
  void GetLRVector(double vector[3], svtkCamera* cam);
  void MotionAlongVector(double vector[3], double amount, svtkCamera* cam);
  void SetupMotionVars(svtkCamera* cam);
  void FinishCamera(svtkCamera* cam);
  //
  //
  unsigned char KeysDown;
  svtkTypeBool DisableMotion;
  svtkTypeBool RestoreUpVector;
  double DiagonalLength;
  double MotionStepSize;
  double MotionUserScale;
  double MotionAccelerationFactor;
  double AngleStepSize;
  double AngleAccelerationFactor;
  double DefaultUpVector[3];
  double AzimuthStepSize;
  double IdealFocalPoint[3];
  svtkPerspectiveTransform* Transform;
  double DeltaYaw;
  double lYaw;
  double DeltaPitch;
  double lPitch;
  //@}

  CPIDControl* PID_Yaw;
  CPIDControl* PID_Pitch;

private:
  svtkInteractorStyleFlight(const svtkInteractorStyleFlight&) = delete;
  void operator=(const svtkInteractorStyleFlight&) = delete;
};

#endif
