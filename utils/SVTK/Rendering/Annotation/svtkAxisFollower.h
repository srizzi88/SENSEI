/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAxisFollower.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAxisFollower
 * @brief   a subclass of svtkFollower that ensures that
 * data is always parallel to the axis defined by a svtkAxisActor.
 *
 * svtkAxisFollower is a subclass of svtkFollower that always follows its
 * specified axis. More specifically it will not change its position or scale,
 * but it will continually update its orientation so that it is aliged with the
 * axis and facing at angle to the camera to provide maximum visibilty.
 * This is typically used for text labels for 3d plots.
 * @sa
 * svtkActor svtkFollower svtkCamera svtkAxisActor svtkCubeAxesActor
 */

#ifndef svtkAxisFollower_h
#define svtkAxisFollower_h

#include "svtkFollower.h"
#include "svtkRenderingAnnotationModule.h" // For export macro

#include "svtkWeakPointer.h" // For svtkWeakPointer

// Forward declarations.
class svtkAxisActor;
class svtkRenderer;

class SVTKRENDERINGANNOTATION_EXPORT svtkAxisFollower : public svtkFollower
{
public:
  svtkTypeMacro(svtkAxisFollower, svtkFollower);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a follower with no camera set
   */
  static svtkAxisFollower* New();

  //@{
  /**
   * Set axis that needs to be followed.
   */
  virtual void SetAxis(svtkAxisActor*);
  virtual svtkAxisActor* GetAxis();
  //@}

  //@{
  /**
   * Set/Get state of auto center mode where additional
   * translation will be added to make sure the underlying
   * geometry has its pivot point at the center of its bounds.
   */
  svtkSetMacro(AutoCenter, svtkTypeBool);
  svtkGetMacro(AutoCenter, svtkTypeBool);
  svtkBooleanMacro(AutoCenter, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable / disable use of distance based LOD. If enabled the actor
   * will not be visible at a certain distance from the camera.
   * Default is false.
   */
  svtkSetMacro(EnableDistanceLOD, int);
  svtkGetMacro(EnableDistanceLOD, int);
  //@}

  //@{
  /**
   * Set distance LOD threshold (0.0 - 1.0).This determines at what fraction
   * of camera far clip range, actor is not visible.
   * Default is 0.80.
   */
  svtkSetClampMacro(DistanceLODThreshold, double, 0.0, 1.0);
  svtkGetMacro(DistanceLODThreshold, double);
  //@}

  //@{
  /**
   * Enable / disable use of view angle based LOD. If enabled the actor
   * will not be visible at a certain view angle.
   * Default is true.
   */
  svtkSetMacro(EnableViewAngleLOD, int);
  svtkGetMacro(EnableViewAngleLOD, int);
  //@}

  //@{
  /**
   * Set view angle LOD threshold (0.0 - 1.0).This determines at what view
   * angle to geometry will make the geometry not visible.
   * Default is 0.34.
   */
  svtkSetClampMacro(ViewAngleLODThreshold, double, 0.0, 1.0);
  svtkGetMacro(ViewAngleLODThreshold, double);
  //@}

  //@{
  /**
   * Set/Get the desired screen offset from the axis.
   * Convenience method, using a zero horizontal offset
   */
  double GetScreenOffset();
  void SetScreenOffset(double offset);
  //@}

  //@{
  /**
   * Set/Get the desired screen offset from the axis.
   * first component is horizontal, second is vertical.
   */
  svtkSetVector2Macro(ScreenOffsetVector, double);
  svtkGetVector2Macro(ScreenOffsetVector, double);
  //@}

  //@{
  /**
   * This causes the actor to be rendered. It in turn will render the actor's
   * property, texture map and then mapper. If a property hasn't been
   * assigned, then the actor will create one automatically.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  void Render(svtkRenderer* ren) override;
  //@}

  /**
   * Generate the matrix based on ivars. This method overloads its superclasses
   * ComputeMatrix() method due to the special svtkFollower matrix operations.
   */
  virtual void ComputeTransformMatrix(svtkRenderer* ren);

  /**
   * Shallow copy of a follower. Overloads the virtual svtkProp method.
   */
  void ShallowCopy(svtkProp* prop) override;

  /**
   * Calculate scale factor to maintain same size of a object
   * on the screen.
   */
  static double AutoScale(
    svtkViewport* viewport, svtkCamera* camera, double screenSize, double position[3]);

protected:
  svtkAxisFollower();
  ~svtkAxisFollower() override;

  void CalculateOrthogonalVectors(
    double Rx[3], double Ry[3], double Rz[3], svtkAxisActor* axis1, double* dop, svtkRenderer* ren);

  void ComputeRotationAndTranlation(svtkRenderer* ren, double translation[3], double Rx[3],
    double Ry[3], double Rz[3], svtkAxisActor* axis);

  // \NOTE: Not used as of now.
  void ComputerAutoCenterTranslation(const double& autoScaleFactor, double translation[3]);

  int TestDistanceVisibility();
  void ExecuteViewAngleVisibility(double normal[3]);

  bool IsTextUpsideDown(double* a, double* b);

  svtkTypeBool AutoCenter;

  int EnableDistanceLOD;
  double DistanceLODThreshold;

  int EnableViewAngleLOD;
  double ViewAngleLODThreshold;

  double ScreenOffsetVector[2];

  svtkWeakPointer<svtkAxisActor> Axis;

private:
  int TextUpsideDown;
  int VisibleAtCurrentViewAngle;

  svtkAxisFollower(const svtkAxisFollower&) = delete;
  void operator=(const svtkAxisFollower&) = delete;

  // hide the two parameter Render() method from the user and the compiler.
  void Render(svtkRenderer*, svtkMapper*) override {}
};

#endif // svtkAxisFollower_h
