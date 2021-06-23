/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTransformInterpolator
 * @brief   interpolate a series of transformation matrices
 *
 * This class is used to interpolate a series of 4x4 transformation
 * matrices. Position, scale and orientation (i.e., rotations) are
 * interpolated separately, and can be interpolated linearly or with a spline
 * function. Note that orientation is interpolated using quaternions via
 * SLERP (spherical linear interpolation) or the special svtkQuaternionSpline
 * class.
 *
 * To use this class, specify at least two pairs of (t,transformation matrix)
 * with the AddTransform() method.  Then interpolated the transforms with the
 * InterpolateTransform(t,transform) method, where "t" must be in the range
 * of (min,max) times specified by the AddTransform() method.
 *
 * By default, spline interpolation is used for the interpolation of the
 * transformation matrices. The position, scale and orientation of the
 * matrices are interpolated with instances of the classes
 * svtkTupleInterpolator (position,scale) and svtkQuaternionInterpolator
 * (rotation). The user can override the interpolation behavior by gaining
 * access to these separate interpolation classes.  These interpolator
 * classes (svtkTupleInterpolator and svtkQuaternionInterpolator) can be
 * modified to perform linear versus spline interpolation, and/or different
 * spline basis functions can be specified.
 *
 * @warning
 * The interpolator classes are initialized when the InterpolateTransform()
 * is called. Any changes to the interpolators, or additions to the list of
 * transforms to be interpolated, causes a reinitialization of the
 * interpolators the next time InterpolateTransform() is invoked. Thus the
 * best performance is obtained by 1) configuring the interpolators, 2) adding
 * all the transforms, and 3) finally performing interpolation.
 */

#ifndef svtkTransformInterpolator_h
#define svtkTransformInterpolator_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkTransform;
class svtkMatrix4x4;
class svtkProp3D;
class svtkTupleInterpolator;
class svtkQuaternionInterpolator;
class svtkTransformList;

class SVTKRENDERINGCORE_EXPORT svtkTransformInterpolator : public svtkObject
{
public:
  svtkTypeMacro(svtkTransformInterpolator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate the class.
   */
  static svtkTransformInterpolator* New();

  /**
   * Return the number of transforms in the list of transforms.
   */
  int GetNumberOfTransforms();

  //@{
  /**
   * Obtain some information about the interpolation range. The numbers
   * returned (corresponding to parameter t, usually thought of as time)
   * are undefined if the list of transforms is empty.
   */
  double GetMinimumT();
  double GetMaximumT();
  //@}

  /**
   * Clear the list of transforms.
   */
  void Initialize();

  //@{
  /**
   * Add another transform to the list of transformations defining
   * the transform function. Note that using the same time t value
   * more than once replaces the previous transform value at t.
   * At least two transforms must be added to define a function.
   * There are variants to this method depending on whether you are
   * adding a svtkTransform, svtkMaxtirx4x4, and/or svtkProp3D.
   */
  void AddTransform(double t, svtkTransform* xform);
  void AddTransform(double t, svtkMatrix4x4* matrix);
  void AddTransform(double t, svtkProp3D* prop3D);
  //@}

  /**
   * Delete the transform at a particular parameter t. If there is no
   * transform defined at location t, then the method does nothing.
   */
  void RemoveTransform(double t);

  /**
   * Interpolate the list of transforms and determine a new transform (i.e.,
   * fill in the transformation provided). If t is outside the range of
   * (min,max) values, then t is clamped.
   */
  void InterpolateTransform(double t, svtkTransform* xform);

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
   * spline interpolation to the position, scale and orientation
   * interpolators. Note that if the InterpolationType is set to "Manual",
   * then the interpolators are expected to be directly manipulated and
   * this class does not forward the request for interpolation type to its
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
   * of the transformation matrix. Note that you can modify the behavior of
   * the interpolator (linear vs spline interpolation; change spline basis)
   * by manipulating the interpolator instances.
   */
  virtual void SetPositionInterpolator(svtkTupleInterpolator*);
  svtkGetObjectMacro(PositionInterpolator, svtkTupleInterpolator);
  //@}

  //@{
  /**
   * Set/Get the tuple interpolator used to interpolate the scale portion
   * of the transformation matrix. Note that you can modify the behavior of
   * the interpolator (linear vs spline interpolation; change spline basis)
   * by manipulating the interpolator instances.
   */
  virtual void SetScaleInterpolator(svtkTupleInterpolator*);
  svtkGetObjectMacro(ScaleInterpolator, svtkTupleInterpolator);
  //@}

  //@{
  /**
   * Set/Get the tuple interpolator used to interpolate the orientation portion
   * of the transformation matrix. Note that you can modify the behavior of
   * the interpolator (linear vs spline interpolation; change spline basis)
   * by manipulating the interpolator instances.
   */
  virtual void SetRotationInterpolator(svtkQuaternionInterpolator*);
  svtkGetObjectMacro(RotationInterpolator, svtkQuaternionInterpolator);
  //@}

  /**
   * Override GetMTime() because we depend on the interpolators which may be
   * modified outside of this class.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkTransformInterpolator();
  ~svtkTransformInterpolator() override;

  // Control the interpolation type
  int InterpolationType;

  // Interpolators
  svtkTupleInterpolator* PositionInterpolator;
  svtkTupleInterpolator* ScaleInterpolator;
  svtkQuaternionInterpolator* RotationInterpolator;

  // Initialize the interpolating splines
  int Initialized;
  svtkTimeStamp InitializeTime;
  void InitializeInterpolation();

  // Keep track of inserted data
  svtkTransformList* TransformList;

private:
  svtkTransformInterpolator(const svtkTransformInterpolator&) = delete;
  void operator=(const svtkTransformInterpolator&) = delete;
};

#endif
