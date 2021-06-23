/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSplineRepresentation
 * @brief   representation for a spline.
 *
 * svtkSplineRepresentation is a svtkWidgetRepresentation for a spline.
 * This 3D widget defines a spline that can be interactively placed in a
 * scene. The spline has handles, the number of which can be changed, plus it
 * can be picked on the spline itself to translate or rotate it in the scene.
 * This is based on svtkSplineWidget.
 * @sa
 * svtkSplineWidget, svtkSplineWidget2
 */

#ifndef svtkSplineRepresentation_h
#define svtkSplineRepresentation_h

#include "svtkCurveRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkActor;
class svtkCellPicker;
class svtkDoubleArray;
class svtkParametricFunctionSource;
class svtkParametricSpline;
class svtkPlaneSource;
class svtkPoints;
class svtkPolyData;
class svtkProp;
class svtkProperty;
class svtkSphereSource;
class svtkTransform;

class SVTKINTERACTIONWIDGETS_EXPORT svtkSplineRepresentation : public svtkCurveRepresentation
{
public:
  static svtkSplineRepresentation* New();
  svtkTypeMacro(svtkSplineRepresentation, svtkCurveRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Grab the polydata (including points) that defines the spline.  The
   * polydata consists of points and line segments numbering Resolution + 1
   * and Resolution, respectively. Points are guaranteed to be up-to-date when
   * either the InteractionEvent or EndInteraction events are invoked. The
   * user provides the svtkPolyData and the points and polyline are added to it.
   */
  void GetPolyData(svtkPolyData* pd) override;

  /**
   * Set the number of handles for this widget.
   */
  void SetNumberOfHandles(int npts) override;

  //@{
  /**
   * Set/Get the number of line segments representing the spline for
   * this widget.
   */
  void SetResolution(int resolution);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Set the parametric spline object. Through svtkParametricSpline's API, the
   * user can supply and configure one of two types of spline:
   * svtkCardinalSpline, svtkKochanekSpline. The widget controls the open
   * or closed configuration of the spline.
   * WARNING: The widget does not enforce internal consistency so that all
   * three are of the same type.
   */
  virtual void SetParametricSpline(svtkParametricSpline*);
  svtkGetObjectMacro(ParametricSpline, svtkParametricSpline);
  //@}

  /**
   * Get the position of the spline handles.
   */
  svtkDoubleArray* GetHandlePositions() override;

  /**
   * Get the approximate vs. the true arc length of the spline. Calculated as
   * the summed lengths of the individual straight line segments. Use
   * SetResolution to control the accuracy.
   */
  double GetSummedLength() override;

  /**
   * Convenience method to allocate and set the handles from a svtkPoints
   * instance.  If the first and last points are the same, the spline sets
   * Closed to the on InteractionState and disregards the last point, otherwise Closed
   * remains unchanged.
   */
  void InitializeHandles(svtkPoints* points) override;

  /**
   * These are methods that satisfy svtkWidgetRepresentation's API. Note that a
   * version of place widget is available where the center and handle position
   * are specified.
   */
  void BuildRepresentation() override;

protected:
  svtkSplineRepresentation();
  ~svtkSplineRepresentation() override;

  // The spline
  svtkParametricSpline* ParametricSpline;
  svtkParametricFunctionSource* ParametricFunctionSource;

  // The number of line segments used to represent the spline.
  int Resolution;

  // Specialized method to insert a handle on the poly line.
  int InsertHandleOnLine(double* pos) override;

private:
  svtkSplineRepresentation(const svtkSplineRepresentation&) = delete;
  void operator=(const svtkSplineRepresentation&) = delete;
};

#endif
