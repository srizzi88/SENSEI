/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp3DAxisFollower.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProp3DAxisFollower
 * @brief   a subclass of svtkProp3DFollower that ensures
 * that data is always parallel to the axis defined by a svtkAxisActor.
 *
 * svtkProp3DAxisFollower is a subclass of svtkProp3DFollower that always follows
 * its specified axis. More specifically it will not change its position or
 * scale, but it will continually update its orientation so that it is aligned
 * with the axis and facing at angle to the camera to provide maximum visibilty.
 * This is typically used for text labels for 3d plots.
 * @sa
 * svtkFollower svtkAxisFollower svtkProp3DFollower
 */

#ifndef svtkProp3DAxisFollower_h
#define svtkProp3DAxisFollower_h

#include "svtkProp3DFollower.h"
#include "svtkRenderingAnnotationModule.h" // For export macro
#include "svtkWeakPointer.h"               // For svtkWeakPointer

class svtkAxisActor;
class svtkViewport;

class SVTKRENDERINGANNOTATION_EXPORT svtkProp3DAxisFollower : public svtkProp3DFollower
{
public:
  /**
   * Creates a follower with no camera set.
   */
  static svtkProp3DAxisFollower* New();

  //@{
  /**
   * Standard SVTK methods for type and printing.
   */
  svtkTypeMacro(svtkProp3DAxisFollower, svtkProp3DFollower);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

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
   * Set/Get the desired screen vertical offset from the axis.
   * Convenience method, using a zero horizontal offset
   */
  double GetScreenOffset();
  void SetScreenOffset(double offset);
  //@}

  //@{
  /**
   * Set/Get the desired screen offset from the axis.
   */
  svtkSetVector2Macro(ScreenOffsetVector, double);
  svtkGetVector2Macro(ScreenOffsetVector, double);
  //@}

  /**
   * Generate the matrix based on ivars. This method overloads its superclasses
   * ComputeMatrix() method due to the special svtkProp3DAxisFollower matrix operations.
   */
  void ComputeMatrix() override;

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

  //@{
  /**
   * This causes the actor to be rendered. It in turn will render the actor's
   * property, texture map and then mapper. If a property hasn't been
   * assigned, then the actor will create one automatically.
   */
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  int RenderVolumetricGeometry(svtkViewport* viewport) override;
  //@}

  virtual void SetViewport(svtkViewport* viewport);
  virtual svtkViewport* GetViewport();

protected:
  svtkProp3DAxisFollower();
  ~svtkProp3DAxisFollower() override;

  void CalculateOrthogonalVectors(
    double Rx[3], double Ry[3], double Rz[3], svtkAxisActor* axis1, double* dop, svtkViewport* ren);

  void ComputeRotationAndTranlation(svtkViewport* ren, double translation[3], double Rx[3],
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
  svtkWeakPointer<svtkViewport> Viewport;

private:
  svtkProp3DAxisFollower(const svtkProp3DAxisFollower&) = delete;
  void operator=(const svtkProp3DAxisFollower&) = delete;

  int TextUpsideDown;
  int VisibleAtCurrentViewAngle;
};

#endif
