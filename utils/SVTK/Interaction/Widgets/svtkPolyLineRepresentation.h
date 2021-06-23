/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyLineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyLineRepresentation
 * @brief   svtkWidgetRepresentation for a poly line.
 *
 * svtkPolyLineRepresentation is a svtkCurveRepresentation for a poly
 * line. This 3D widget defines a poly line that can be interactively
 * placed in a scene. The poly line has handles, the number of which
 * can be changed, plus the widget can be picked on the poly line
 * itself to translate or rotate it in the scene.
 * Based on svtkCurveRepresentation
 * @sa
 * svtkSplineRepresentation
 */

#ifndef svtkPolyLineRepresentation_h
#define svtkPolyLineRepresentation_h

#include "svtkCurveRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkPolyLineSource;
class svtkPoints;
class svtkPolyData;

class SVTKINTERACTIONWIDGETS_EXPORT svtkPolyLineRepresentation : public svtkCurveRepresentation
{
public:
  static svtkPolyLineRepresentation* New();
  svtkTypeMacro(svtkPolyLineRepresentation, svtkCurveRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Grab the polydata (including points) that defines the poly line.
   * polydata consists of points and line segments between consecutive
   * points. Points are guaranteed to be up-to-date when either the
   * InteractionEvent or EndInteraction events are invoked. The user
   * provides the svtkPolyData and the points and polyline are added to
   * it.
   */
  void GetPolyData(svtkPolyData* pd) override;

  /**
   * Set the number of handles for this widget.
   */
  void SetNumberOfHandles(int npts) override;

  /**
   * Get the positions of the handles.
   */
  svtkDoubleArray* GetHandlePositions() override;

  /**
   * Get the true length of the poly line. Calculated as the summed
   * lengths of the individual straight line segments.
   */
  double GetSummedLength() override;

  /**
   * Convenience method to allocate and set the handles from a
   * svtkPoints instance.  If the first and last points are the same,
   * the poly line sets Closed to on and disregards the last point,
   * otherwise Closed remains unchanged.
   */
  void InitializeHandles(svtkPoints* points) override;

  /**
   * Build the representation for the poly line.
   */
  void BuildRepresentation() override;

protected:
  svtkPolyLineRepresentation();
  ~svtkPolyLineRepresentation() override;

  // The poly line source
  svtkPolyLineSource* PolyLineSource;

  // Specialized method to insert a handle on the poly line.
  int InsertHandleOnLine(double* pos) override;

private:
  svtkPolyLineRepresentation(const svtkPolyLineRepresentation&) = delete;
  void operator=(const svtkPolyLineRepresentation&) = delete;
};

#endif
