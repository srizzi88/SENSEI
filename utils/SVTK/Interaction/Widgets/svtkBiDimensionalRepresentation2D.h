/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBiDimensionalRepresentation2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBiDimensionalRepresentation2D
 * @brief   represent the svtkBiDimensionalWidget
 *
 * The svtkBiDimensionalRepresentation2D is used to represent the
 * bi-dimensional measure in a 2D (overlay) context. This representation
 * consists of two perpendicular lines defined by four
 * svtkHandleRepresentations. The four handles can be independently
 * manipulated consistent with the orthogonal constraint on the lines. (Note:
 * the four points are referred to as Point1, Point2, Point3 and
 * Point4. Point1 and Point2 define the first line; and Point3 and Point4
 * define the second orthogonal line.)
 *
 * To create this widget, you click to place the first two points. The third
 * point is mirrored with the fourth point; when you place the third point
 * (which is orthogonal to the lined defined by the first two points), the
 * fourth point is dropped as well. After definition, the four points can be
 * moved (in constrained fashion, preserving orthogonality). Further, the
 * entire widget can be translated by grabbing the center point of the widget;
 * each line can be moved along the other line; and the entire widget can be
 * rotated around its center point.
 *
 * @sa
 * svtkAngleWidget svtkHandleRepresentation svtkBiDimensionalRepresentation
 */

#ifndef svtkBiDimensionalRepresentation2D_h
#define svtkBiDimensionalRepresentation2D_h

#include "svtkBiDimensionalRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkHandleRepresentation;
class svtkCellArray;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkTextMapper;
class svtkActor2D;
class svtkProperty2D;
class svtkTextProperty;

class SVTKINTERACTIONWIDGETS_EXPORT svtkBiDimensionalRepresentation2D
  : public svtkBiDimensionalRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkBiDimensionalRepresentation2D* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkBiDimensionalRepresentation2D, svtkBiDimensionalRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Retrieve the property used to control the appearance of the two
   * orthogonal lines.
   */
  svtkGetObjectMacro(LineProperty, svtkProperty2D);
  svtkGetObjectMacro(SelectedLineProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Retrieve the property used to control the appearance of the text
   * labels.
   */
  svtkGetObjectMacro(TextProperty, svtkTextProperty);
  //@}

  // Used to communicate about the state of the representation
  enum
  {
    Outside = 0,
    NearP1,
    NearP2,
    NearP3,
    NearP4,
    OnL1Inner,
    OnL1Outer,
    OnL2Inner,
    OnL2Outer,
    OnCenter
  };

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetDefinition(double e[2]) override;
  void Point2WidgetInteraction(double e[2]) override;
  void Point3WidgetInteraction(double e[2]) override;
  void StartWidgetManipulation(double e[2]) override;
  void WidgetInteraction(double e[2]) override;
  void Highlight(int highlightOn) override;
  //@}

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOverlay(svtkViewport* viewport) override;
  //@}

  /**
   * Get the text shown in the widget's label.
   */
  char* GetLabelText() override;

  //@{
  /**
   * Get the position of the widget's label in display coordinates.
   */
  double* GetLabelPosition() override;
  void GetLabelPosition(double pos[3]) override;
  void GetWorldLabelPosition(double pos[3]) override;
  //@}

protected:
  svtkBiDimensionalRepresentation2D();
  ~svtkBiDimensionalRepresentation2D() override;

  // Geometry of the lines
  svtkCellArray* LineCells;
  svtkPoints* LinePoints;
  svtkPolyData* LinePolyData;
  svtkPolyDataMapper2D* LineMapper;
  svtkActor2D* LineActor;
  svtkProperty2D* LineProperty;
  svtkProperty2D* SelectedLineProperty;

  // The labels for the line lengths
  svtkTextProperty* TextProperty;
  svtkTextMapper* TextMapper;
  svtkActor2D* TextActor;

  // Helper method
  void ProjectOrthogonalPoint(
    double x[4], double y[3], double x1[3], double x2[3], double x21[3], double dir, double xP[3]);

private:
  svtkBiDimensionalRepresentation2D(const svtkBiDimensionalRepresentation2D&) = delete;
  void operator=(const svtkBiDimensionalRepresentation2D&) = delete;
};

#endif
