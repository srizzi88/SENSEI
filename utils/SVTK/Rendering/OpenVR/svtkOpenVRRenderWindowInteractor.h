/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenVRRenderWindowInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenVRRenderWindowInteractor
 * @brief   implements OpenVR specific functions
 * required by svtkRenderWindowInteractor.
 *
 *
 */

#ifndef svtkOpenVRRenderWindowInteractor_h
#define svtkOpenVRRenderWindowInteractor_h

#include "svtkRenderWindowInteractor3D.h"
#include "svtkRenderingOpenVRModule.h" // For export macro

#include "svtkNew.h"                // ivars
#include "svtkOpenVRRenderWindow.h" // ivars

class svtkTransform;
class svtkMatrix4x4;

class SVTKRENDERINGOPENVR_EXPORT svtkOpenVRRenderWindowInteractor : public svtkRenderWindowInteractor3D
{
public:
  /**
   * Construct object so that light follows camera motion.
   */
  static svtkOpenVRRenderWindowInteractor* New();

  svtkTypeMacro(svtkOpenVRRenderWindowInteractor, svtkRenderWindowInteractor3D);
  void PrintSelf(ostream& os, svtkIndent indent);

  /**
   * Initialize the event handler
   */
  virtual void Initialize();

  //@{
  /**
   * Methods to set the default exit method for the class. This method is
   * only used if no instance level ExitMethod has been defined.  It is
   * provided as a means to control how an interactor is exited given
   * the various language bindings (Win32, etc.).
   */
  static void SetClassExitMethod(void (*f)(void*), void* arg);
  static void SetClassExitMethodArgDelete(void (*f)(void*));
  //@}

  /**
   * These methods correspond to the Exit, User and Pick
   * callbacks. They allow for the Style to invoke them.
   */
  virtual void ExitCallback();

  //@{
  /**
   * Set/Get the optional translation to map world coordinates into the
   * 3D physical space (meters, 0,0,0).
   */
  virtual void SetPhysicalTranslation(svtkCamera*, double, double, double);
  virtual double* GetPhysicalTranslation(svtkCamera*);
  virtual void SetPhysicalScale(double);
  virtual double GetPhysicalScale();
  //@}

  /**
   * Run the event loop and return. This is provided so that you can
   * implement your own event loop but yet use the svtk event handling as
   * well.
   */
  void ProcessEvents() override;

  virtual void DoOneEvent(svtkOpenVRRenderWindow* renWin, svtkRenderer* ren);

  /*
   * Return the pointer index as a device
   */
  svtkEventDataDevice GetPointerDevice();

  /*
   * Convert a device pose to pose matrices
   * \param poseMatrixPhysical Optional output pose matrix in physical frame
   * \param poseMatrixWorld    Optional output pose matrix in world frame
   */
  void ConvertOpenVRPoseToMatrices(const vr::TrackedDevicePose_t& tdPose,
    svtkMatrix4x4* poseMatrixWorld, svtkMatrix4x4* poseMatrixPhysical = nullptr);

  /*
   * Convert a device pose to a world coordinate position and orientation
   * \param pos  Output world position
   * \param wxyz Output world orientation quaternion
   * \param ppos Output physical position
   * \param wdir Output world view direction (-Z)
   */
  void ConvertPoseToWorldCoordinates(const vr::TrackedDevicePose_t& tdPose, double pos[3],
    double wxyz[4], double ppos[3], double wdir[3]);

  //@{
  /**
   * Get the latest touchpad or joystick position for a device
   */
  void GetTouchPadPosition(svtkEventDataDevice, svtkEventDataDeviceInput, float[3]) override;
  //@}

  /*
   * Return starting physical to world matrix
   */
  void GetStartingPhysicalToWorldMatrix(svtkMatrix4x4* startingPhysicalToWorldMatrix);

protected:
  svtkOpenVRRenderWindowInteractor();
  ~svtkOpenVRRenderWindowInteractor() override;

  //@{
  /**
   * Class variables so an exit method can be defined for this class
   * (used to set different exit methods for various language bindings,
   * i.e. java, Win32)
   */
  static void (*ClassExitMethod)(void*);
  static void (*ClassExitMethodArgDelete)(void*);
  static void* ClassExitMethodArg;
  //@}

  //@{
  /**
   * Win32-specific internal timer methods. See the superclass for detailed
   * documentation.
   */
  virtual int InternalCreateTimer(int timerId, int timerType, unsigned long duration);
  virtual int InternalDestroyTimer(int platformTimerId);
  //@}

  /**
   * This will start up the event loop and never return. If you
   * call this method it will loop processing events until the
   * application is exited.
   */
  virtual void StartEventLoop();

  /**
   * Handle multitouch events. Multitouch events recognition starts when
   * both controllers the trigger pressed.
   */
  int DeviceInputDown[SVTKI_MAX_POINTERS][2];
  int DeviceInputDownCount[2];
  virtual void RecognizeComplexGesture(svtkEventDataDevice3D* edata);

  /**
   * Store physical to world matrix at the start of a multi-touch gesture
   */
  svtkNew<svtkMatrix4x4> StartingPhysicalToWorldMatrix;

private:
  svtkOpenVRRenderWindowInteractor(const svtkOpenVRRenderWindowInteractor&) = delete;
  void operator=(const svtkOpenVRRenderWindowInteractor&) = delete;
};

#endif
