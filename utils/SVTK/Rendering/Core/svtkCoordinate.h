/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCoordinate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCoordinate
 * @brief   perform coordinate transformation, and represent position, in a variety of svtk
 * coordinate systems
 *
 * svtkCoordinate represents position in a variety of coordinate systems, and
 * converts position to other coordinate systems. It also supports relative
 * positioning, so you can create a cascade of svtkCoordinate objects (no loops
 * please!) that refer to each other. The typical usage of this object is
 * to set the coordinate system in which to represent a position (e.g.,
 * SetCoordinateSystemToNormalizedDisplay()), set the value of the coordinate
 * (e.g., SetValue()), and then invoke the appropriate method to convert to
 * another coordinate system (e.g., GetComputedWorldValue()).
 *
 * The coordinate systems in svtk are as follows:
 * <PRE>
 *   DISPLAY -             x-y pixel values in window
 *      0, 0 is the lower left of the first pixel,
 *      size, size is the upper right of the last pixel
 *   NORMALIZED DISPLAY -  x-y (0,1) normalized values
 *      0, 0 is the lower left of the first pixel,
 *      1, 1 is the upper right of the last pixel
 *   VIEWPORT -            x-y pixel values in viewport
 *      0, 0 is the lower left of the first pixel,
 *      size, size is the upper right of the last pixel
 *   NORMALIZED VIEWPORT - x-y (0,1) normalized value in viewport
 *      0, 0 is the lower left of the first pixel,
 *      1, 1 is the upper right of the last pixel
 *   VIEW -                x-y-z (-1,1) values in pose coordinates. (z is depth)
 *   POSE -                world coords translated and rotated to the camera
 *                         position and view direction
 *   WORLD -               x-y-z global coordinate values
 *   USERDEFINED -         x-y-z in User defined space
 * </PRE>
 *
 * If you cascade svtkCoordinate objects, you refer to another svtkCoordinate
 * object which in turn can refer to others, and so on. This allows you to
 * create composite groups of things like svtkActor2D that are positioned
 * relative to one another. Note that in cascaded sequences, each
 * svtkCoordinate object may be specified in different coordinate systems!
 *
 * @sa
 * svtkActor2D svtkScalarBarActor
 */

#ifndef svtkCoordinate_h
#define svtkCoordinate_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro
class svtkViewport;

#define SVTK_DISPLAY 0
#define SVTK_NORMALIZED_DISPLAY 1
#define SVTK_VIEWPORT 2
#define SVTK_NORMALIZED_VIEWPORT 3
#define SVTK_VIEW 4
#define SVTK_POSE 5
#define SVTK_WORLD 6
#define SVTK_USERDEFINED 7

class SVTKRENDERINGCORE_EXPORT svtkCoordinate : public svtkObject
{
public:
  svtkTypeMacro(svtkCoordinate, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates an instance of this class with the following defaults:
   * value of  (0,0,0) in world coordinates.
   */
  static svtkCoordinate* New();

  //@{
  /**
   * Set/get the coordinate system which this coordinate
   * is defined in. The options are Display, Normalized Display,
   * Viewport, Normalized Viewport, View, and World.
   */
  svtkSetMacro(CoordinateSystem, int);
  svtkGetMacro(CoordinateSystem, int);
  void SetCoordinateSystemToDisplay() { this->SetCoordinateSystem(SVTK_DISPLAY); }
  void SetCoordinateSystemToNormalizedDisplay()
  {
    this->SetCoordinateSystem(SVTK_NORMALIZED_DISPLAY);
  }
  void SetCoordinateSystemToViewport() { this->SetCoordinateSystem(SVTK_VIEWPORT); }
  void SetCoordinateSystemToNormalizedViewport()
  {
    this->SetCoordinateSystem(SVTK_NORMALIZED_VIEWPORT);
  }
  void SetCoordinateSystemToView() { this->SetCoordinateSystem(SVTK_VIEW); }
  void SetCoordinateSystemToPose() { this->SetCoordinateSystem(SVTK_POSE); }
  void SetCoordinateSystemToWorld() { this->SetCoordinateSystem(SVTK_WORLD); }
  //@}

  const char* GetCoordinateSystemAsString();

  //@{
  /**
   * Set/get the value of this coordinate. This can be thought of as
   * the position of this coordinate in its coordinate system.
   */
  svtkSetVector3Macro(Value, double);
  svtkGetVector3Macro(Value, double);
  void SetValue(double a, double b) { this->SetValue(a, b, 0.0); }
  //@}

  //@{
  /**
   * If this coordinate is relative to another coordinate,
   * then specify that coordinate as the ReferenceCoordinate.
   * If this is NULL the coordinate is assumed to be absolute.
   */
  virtual void SetReferenceCoordinate(svtkCoordinate*);
  svtkGetObjectMacro(ReferenceCoordinate, svtkCoordinate);
  //@}

  //@{
  /**
   * If you want this coordinate to be relative to a specific
   * svtkViewport (svtkRenderer) then you can specify that here.
   * NOTE: this is a raw pointer, not a weak pointer nor a reference counted
   * object, to avoid reference cycle loop between rendering classes and filter
   * classes.
   */
  void SetViewport(svtkViewport* viewport);
  svtkGetObjectMacro(Viewport, svtkViewport);
  //@}

  //@{
  /**
   * Return the computed value in a specified coordinate system.
   */
  double* GetComputedWorldValue(svtkViewport*) SVTK_SIZEHINT(3);
  int* GetComputedViewportValue(svtkViewport*) SVTK_SIZEHINT(2);
  int* GetComputedDisplayValue(svtkViewport*) SVTK_SIZEHINT(2);
  int* GetComputedLocalDisplayValue(svtkViewport*) SVTK_SIZEHINT(2);
  //@}

  double* GetComputedDoubleViewportValue(svtkViewport*) SVTK_SIZEHINT(2);
  double* GetComputedDoubleDisplayValue(svtkViewport*) SVTK_SIZEHINT(2);

  /**
   * GetComputedValue() will return either World, Viewport or
   * Display based on what has been set as the coordinate system.
   * This is good for objects like svtkLineSource, where the
   * user might want to use them as World or Viewport coordinates.
   */
  double* GetComputedValue(svtkViewport*) SVTK_SIZEHINT(3);

  /**
   * GetComputedUserDefinedValue() is to be used only when
   * the coordinate system is SVTK_USERDEFINED. The user
   * must subclass svtkCoordinate and override this function,
   * when set as the TransformCoordinate in 2D-Mappers, the user
   * can customize display of 2D polygons
   */
  virtual double* GetComputedUserDefinedValue(svtkViewport*) SVTK_SIZEHINT(3) { return this->Value; }

protected:
  svtkCoordinate();
  ~svtkCoordinate() override;

  double Value[3];
  int CoordinateSystem;
  svtkCoordinate* ReferenceCoordinate;
  svtkViewport* Viewport;
  double ComputedWorldValue[3];
  int ComputedDisplayValue[2];
  int ComputedViewportValue[2];
  int Computing;

  double ComputedDoubleDisplayValue[2];
  double ComputedDoubleViewportValue[2];
  double ComputedUserDefinedValue[3];

private:
  svtkCoordinate(const svtkCoordinate&) = delete;
  void operator=(const svtkCoordinate&) = delete;
};

#endif
