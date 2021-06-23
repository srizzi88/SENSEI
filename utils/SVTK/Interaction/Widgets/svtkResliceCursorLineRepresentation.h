/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceCursorLineRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkResliceCursorLineRepresentation
 * @brief   represent the svtkResliceCursorWidget
 *
 * This class provides a representation for the reslice cursor widget. It
 * consists of two cross sectional hairs, with an optional thickness. The
 * hairs may have a hole in the center. These may be translated or rotated
 * independent of each other in the view. The result is used to reslice
 * the data along these cross sections. This allows the user to perform
 * multi-planar thin or thick reformat of the data on an image view, rather
 * than a 3D view.
 * @sa
 * svtkResliceCursorWidget svtkResliceCursor svtkResliceCursorPolyDataAlgorithm
 * svtkResliceCursorRepresentation svtkResliceCursorThickLineRepresentation
 * svtkResliceCursorActor svtkImagePlaneWidget
 */

#ifndef svtkResliceCursorLineRepresentation_h
#define svtkResliceCursorLineRepresentation_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkResliceCursorRepresentation.h"

class svtkPolyData;
class svtkResliceCursorActor;
class svtkResliceCursorPolyDataAlgorithm;
class svtkResliceCursorPicker;
class svtkResliceCursor;

class SVTKINTERACTIONWIDGETS_EXPORT svtkResliceCursorLineRepresentation
  : public svtkResliceCursorRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static svtkResliceCursorLineRepresentation* New();

  //@{
  /**
   * Standard SVTK methods.
   */
  svtkTypeMacro(svtkResliceCursorLineRepresentation, svtkResliceCursorRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * These are methods that satisfy svtkWidgetRepresentation's API.
   */
  void BuildRepresentation() override;
  int ComputeInteractionState(int X, int Y, int modify = 0) override;
  void StartWidgetInteraction(double startEventPos[2]) override;
  void WidgetInteraction(double e[2]) override;
  void Highlight(int highlightOn) override;
  //@}

  //@{
  /**
   * Methods required by svtkProp superclass.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /**
   * Get the bounds of this prop. This simply returns the bounds of the
   * reslice cursor object.
   */
  double* GetBounds() override;

  //@{
  /**
   * Get the reslice cursor actor. You must set the reslice cursor on this
   * class
   */
  svtkGetObjectMacro(ResliceCursorActor, svtkResliceCursorActor);
  //@}

  /**
   * Get the reslice cursor.
   */
  svtkResliceCursor* GetResliceCursor() override;

  /**
   * Set the user matrix on all the internal actors.
   */
  virtual void SetUserMatrix(svtkMatrix4x4* matrix);

protected:
  svtkResliceCursorLineRepresentation();
  ~svtkResliceCursorLineRepresentation() override;

  svtkResliceCursorPolyDataAlgorithm* GetCursorAlgorithm() override;

  double RotateAxis(double evenPos[2], int axis);

  void RotateAxis(int axis, double angle);

  void RotateVectorAboutVector(double vectorToBeRotated[3],
    double axis[3], // vector about which we rotate
    double angle,   // angle in radians
    double o[3]);
  int DisplayToReslicePlaneIntersection(double displayPos[2], double intersectionPos[3]);

  svtkResliceCursorActor* ResliceCursorActor;
  svtkResliceCursorPicker* Picker;

  double StartPickPosition[3];
  double StartCenterPosition[3];

  // Transformation matrices. These have no offset. Offset is recomputed
  // based on the cursor, so that the center of the cursor has the same
  // location in transformed space as it does in physical space.
  svtkMatrix4x4* MatrixReslice;
  svtkMatrix4x4* MatrixView;
  svtkMatrix4x4* MatrixReslicedView;

private:
  svtkResliceCursorLineRepresentation(const svtkResliceCursorLineRepresentation&) = delete;
  void operator=(const svtkResliceCursorLineRepresentation&) = delete;
};

#endif
