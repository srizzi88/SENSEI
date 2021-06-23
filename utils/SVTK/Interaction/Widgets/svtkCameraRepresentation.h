/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCameraRepresentation
 * @brief   represent the svtkCameraWidget
 *
 * This class provides support for interactively saving a series of camera
 * views into an interpolated path (using svtkCameraInterpolator). The class
 * typically works in conjunction with svtkCameraWidget. To use this class
 * simply specify the camera to interpolate and use the methods
 * AddCameraToPath(), AnimatePath(), and InitializePath() to add a new camera
 * view, animate the current views, and initialize the interpolation.
 *
 * @sa
 * svtkCameraWidget svtkCameraInterpolator
 */

#ifndef svtkCameraRepresentation_h
#define svtkCameraRepresentation_h

#include "svtkBorderRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkRenderer;
class svtkRenderWindowInteractor;
class svtkCamera;
class svtkCameraInterpolator;
class svtkPoints;
class svtkPolyData;
class svtkTransformPolyDataFilter;
class svtkPolyDataMapper2D;
class svtkProperty2D;
class svtkActor2D;

class SVTKINTERACTIONWIDGETS_EXPORT svtkCameraRepresentation : public svtkBorderRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkCameraRepresentation* New();

  //@{
  /**
   * Standard SVTK class methods.
   */
  svtkTypeMacro(svtkCameraRepresentation, svtkBorderRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the camera to interpolate. This must be specified by
   * the user.
   */
  void SetCamera(svtkCamera* camera);
  svtkGetObjectMacro(Camera, svtkCamera);
  //@}

  //@{
  /**
   * Get the svtkCameraInterpolator used to interpolate and save the
   * sequence of camera views. If not defined, one is created
   * automatically. Note that you can access this object to set
   * the interpolation type (linear, spline) and other instance
   * variables.
   */
  void SetInterpolator(svtkCameraInterpolator* camInt);
  svtkGetObjectMacro(Interpolator, svtkCameraInterpolator);
  //@}

  //@{
  /**
   * Set the number of frames to generate when playback is initiated.
   */
  svtkSetClampMacro(NumberOfFrames, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfFrames, int);
  //@}

  //@{
  /**
   * By obtaining this property you can specify the properties of the
   * representation.
   */
  svtkGetObjectMacro(Property, svtkProperty2D);
  //@}

  //@{
  /**
   * These methods are used to create interpolated camera paths.  The
   * AddCameraToPath() method adds the view defined by the current camera
   * (via SetCamera()) to the interpolated camera path. AnimatePath()
   * interpolates NumberOfFrames along the current path. InitializePath()
   * resets the interpolated path to its initial, empty configuration.
   */
  void AddCameraToPath();
  void AnimatePath(svtkRenderWindowInteractor* rwi);
  void InitializePath();
  //@}

  /**
   * Satisfy the superclasses' API.
   */
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
  {
    size[0] = 6.0;
    size[1] = 2.0;
  }

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a svtkProp.
   */
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport*) override;
  int RenderOpaqueGeometry(svtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport*) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

protected:
  svtkCameraRepresentation();
  ~svtkCameraRepresentation() override;

  // the camera and the interpolator
  svtkCamera* Camera;
  svtkCameraInterpolator* Interpolator;
  int NumberOfFrames;
  double CurrentTime;

  // representation of the camera
  svtkPoints* Points;
  svtkPolyData* PolyData;
  svtkTransformPolyDataFilter* TransformFilter;
  svtkPolyDataMapper2D* Mapper;
  svtkProperty2D* Property;
  svtkActor2D* Actor;

private:
  svtkCameraRepresentation(const svtkCameraRepresentation&) = delete;
  void operator=(const svtkCameraRepresentation&) = delete;
};

#endif
