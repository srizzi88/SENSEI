/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCameraInterpolator
 * @brief   interpolate a series of cameras to update a new camera
 *
 * This class is used to interpolate a series of cameras to update a
 * specified camera. Either linear interpolation or spline interpolation may
 * be used. The instance variables currently interpolated include position,
 * focal point, view up, view angle, parallel scale, and clipping range.
 *
 * To use this class, specify the type of interpolation to use, and add a
 * series of cameras at various times "t" to the list of cameras from which to
 * interpolate. Then to interpolate in between cameras, simply invoke the
 * function InterpolateCamera(t,camera) where "camera" is the camera to be
 * updated with interpolated values. Note that "t" should be in the range
 * (min,max) times specified with the AddCamera() method. If outside this
 * range, the interpolation is clamped. This class copies the camera information
 * (as compared to referencing the cameras) so you do not need to keep separate
 * instances of the camera around for each camera added to the list of cameras
 * to interpolate.
 *
 * @warning
 * The interpolator classes are initialized the first time InterpolateCamera()
 * is called. Any later changes to the interpolators, or additions to the list of
 * cameras to be interpolated, causes a reinitialization of the
 * interpolators the next time InterpolateCamera() is invoked. Thus the
 * best performance is obtained by 1) configuring the interpolators, 2) adding
 * all the cameras, and 3) finally performing interpolation.
 *
 * @warning
 * Currently position, focal point and view up are interpolated to define
 * the orientation of the camera. Quaternion interpolation may be added in the
 * future as an alternative interpolation method for camera orientation.
 */

#ifndef svtkCameraInterpolator_h
#define svtkCameraInterpolator_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkCamera;
class svtkCameraList;
class svtkTupleInterpolator;
class svtkCameraList;

class SVTKRENDERINGCORE_EXPORT svtkCameraInterpolator : public svtkObject
{
public:
  svtkTypeMacro(svtkCameraInterpolator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate the class.
   */
  static svtkCameraInterpolator* New();

  /**
   * Return the number of cameras in the list of cameras.
   */
  int GetNumberOfCameras();

  //@{
  /**
   * Obtain some information about the interpolation range. The numbers
   * returned are undefined if the list of cameras is empty.
   */
  double GetMinimumT();
  double GetMaximumT();
  //@}

  /**
   * Clear the list of cameras.
   */
  void Initialize();

  /**
   * Add another camera to the list of cameras defining
   * the camera function. Note that using the same time t value
   * more than once replaces the previous camera value at t.
   * At least one camera must be added to define a function.
   */
  void AddCamera(double t, svtkCamera* camera);

  /**
   * Delete the camera at a particular parameter t. If there is no
   * camera defined at location t, then the method does nothing.
   */
  void RemoveCamera(double t);

  /**
   * Interpolate the list of cameras and determine a new camera (i.e.,
   * fill in the camera provided). If t is outside the range of
   * (min,max) values, then t is clamped to lie within this range.
   */
  void InterpolateCamera(double t, svtkCamera* camera);

  /**
   * Enums to control the type of interpolation to use.
   */
  enum
  {
    INTERPOLATION_TYPE_LINEAR = 0,
    INTERPOLATION_TYPE_SPLINE,
    INTERPOLATION_TYPE_MANUAL
  };

  //@{
  /**
   * These are convenience methods to switch between linear and spline
   * interpolation. The methods simply forward the request for linear or
   * spline interpolation to the instance variable interpolators (i.e.,
   * position, focal point, clipping range, orientation, etc.)
   * interpolators. Note that if the InterpolationType is set to "Manual",
   * then the interpolators are expected to be directly manipulated and this
   * class does not forward the request for interpolation type to its
   * interpolators.
   */
  svtkSetClampMacro(InterpolationType, int, INTERPOLATION_TYPE_LINEAR, INTERPOLATION_TYPE_MANUAL);
  svtkGetMacro(InterpolationType, int);
  void SetInterpolationTypeToLinear() { this->SetInterpolationType(INTERPOLATION_TYPE_LINEAR); }
  void SetInterpolationTypeToSpline() { this->SetInterpolationType(INTERPOLATION_TYPE_SPLINE); }
  void SetInterpolationTypeToManual() { this->SetInterpolationType(INTERPOLATION_TYPE_MANUAL); }
  //@}

  //@{
  /**
   * Set/Get the tuple interpolator used to interpolate the position portion
   * of the camera. Note that you can modify the behavior of the interpolator
   * (linear vs spline interpolation; change spline basis) by manipulating
   * the interpolator instances directly.
   */
  virtual void SetPositionInterpolator(svtkTupleInterpolator*);
  svtkGetObjectMacro(PositionInterpolator, svtkTupleInterpolator);
  //@}

  //@{
  /**
   * Set/Get the tuple interpolator used to interpolate the focal point portion
   * of the camera. Note that you can modify the behavior of the interpolator
   * (linear vs spline interpolation; change spline basis) by manipulating
   * the interpolator instances directly.
   */
  virtual void SetFocalPointInterpolator(svtkTupleInterpolator*);
  svtkGetObjectMacro(FocalPointInterpolator, svtkTupleInterpolator);
  //@}

  //@{
  /**
   * Set/Get the tuple interpolator used to interpolate the view up portion
   * of the camera. Note that you can modify the behavior of the interpolator
   * (linear vs spline interpolation; change spline basis) by manipulating
   * the interpolator instances directly.
   */
  virtual void SetViewUpInterpolator(svtkTupleInterpolator*);
  svtkGetObjectMacro(ViewUpInterpolator, svtkTupleInterpolator);
  //@}

  //@{
  /**
   * Set/Get the tuple interpolator used to interpolate the view angle portion
   * of the camera. Note that you can modify the behavior of the interpolator
   * (linear vs spline interpolation; change spline basis) by manipulating
   * the interpolator instances directly.
   */
  virtual void SetViewAngleInterpolator(svtkTupleInterpolator*);
  svtkGetObjectMacro(ViewAngleInterpolator, svtkTupleInterpolator);
  //@}

  //@{
  /**
   * Set/Get the tuple interpolator used to interpolate the parallel scale portion
   * of the camera. Note that you can modify the behavior of the interpolator
   * (linear vs spline interpolation; change spline basis) by manipulating
   * the interpolator instances directly.
   */
  virtual void SetParallelScaleInterpolator(svtkTupleInterpolator*);
  svtkGetObjectMacro(ParallelScaleInterpolator, svtkTupleInterpolator);
  //@}

  //@{
  /**
   * Set/Get the tuple interpolator used to interpolate the clipping range portion
   * of the camera. Note that you can modify the behavior of the interpolator
   * (linear vs spline interpolation; change spline basis) by manipulating
   * the interpolator instances directly.
   */
  virtual void SetClippingRangeInterpolator(svtkTupleInterpolator*);
  svtkGetObjectMacro(ClippingRangeInterpolator, svtkTupleInterpolator);
  //@}

  /**
   * Override GetMTime() because we depend on the interpolators which may be
   * modified outside of this class.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkCameraInterpolator();
  ~svtkCameraInterpolator() override;

  // Control the interpolation type
  int InterpolationType;

  // These perform the interpolation
  svtkTupleInterpolator* PositionInterpolator;
  svtkTupleInterpolator* FocalPointInterpolator;
  svtkTupleInterpolator* ViewUpInterpolator;
  svtkTupleInterpolator* ViewAngleInterpolator;
  svtkTupleInterpolator* ParallelScaleInterpolator;
  svtkTupleInterpolator* ClippingRangeInterpolator;

  // Initialize the interpolating splines
  int Initialized;
  svtkTimeStamp InitializeTime;
  void InitializeInterpolation();

  // Hold the list of cameras. PIMPL'd STL list.
  svtkCameraList* CameraList;

private:
  svtkCameraInterpolator(const svtkCameraInterpolator&) = delete;
  void operator=(const svtkCameraInterpolator&) = delete;
};

#endif
