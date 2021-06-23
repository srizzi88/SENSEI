/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOrientedGlyphFocalPlaneContourRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOrientedGlyphFocalPlaneContourRepresentation
 * @brief   Contours constrained
 * to a focal plane.
 *
 *
 * This class is used to represent a contour drawn on the focal plane (usually
 * overlaid on top of an image or volume widget).
 * The class was written in order to be able to draw contours on a volume widget
 * and have the contours overlaid on the focal plane in order to do contour
 * segmentation.
 *
 * @sa
 * svtkOrientedGlyphContourRepresentation
 */

#ifndef svtkOrientedGlyphFocalPlaneContourRepresentation_h
#define svtkOrientedGlyphFocalPlaneContourRepresentation_h

#include "svtkFocalPlaneContourRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkProperty2D;
class svtkActor2D;
class svtkPolyDataMapper2D;
class svtkPolyData;
class svtkGlyph2D;
class svtkPoints;
class svtkPolyData;

class SVTKINTERACTIONWIDGETS_EXPORT svtkOrientedGlyphFocalPlaneContourRepresentation
  : public svtkFocalPlaneContourRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkOrientedGlyphFocalPlaneContourRepresentation* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkOrientedGlyphFocalPlaneContourRepresentation, svtkFocalPlaneContourRepresentation);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the cursor shape. Keep in mind that the shape will be
   * aligned with the constraining plane by orienting it such that
   * the x axis of the geometry lies along the normal of the plane.
   */
  void SetCursorShape(svtkPolyData* cursorShape);
  svtkPolyData* GetCursorShape();
  //@}

  //@{
  /**
   * Specify the shape of the cursor (handle) when it is active.
   * This is the geometry that will be used when the mouse is
   * close to the handle or if the user is manipulating the handle.
   */
  void SetActiveCursorShape(svtkPolyData* activeShape);
  svtkPolyData* GetActiveCursorShape();
  //@}

  //@{
  /**
   * This is the property used when the handle is not active
   * (the mouse is not near the handle)
   */
  svtkGetObjectMacro(Property, svtkProperty2D);
  //@}

  //@{
  /**
   * This is the property used when the user is interacting
   * with the handle.
   */
  svtkGetObjectMacro(ActiveProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * This is the property used by the lines.
   */
  svtkGetObjectMacro(LinesProperty, svtkProperty2D);
  //@}

  //@{
  /**
   * Subclasses of svtkOrientedGlyphFocalPlaneContourRepresentation must implement these methods.
   * These are the methods that the widget and its representation use to communicate with each
   * other.
   */
  void SetRenderer(svtkRenderer* ren) override;
  void BuildRepresentation() override;
  void StartWidgetInteraction(double eventPos[2]) override;
  void WidgetInteraction(double eventPos[2]) override;
  int ComputeInteractionState(int X, int Y, int modified = 0) override;
  //@}

  //@{
  /**
   * Methods to make this class behave as a svtkProp.
   */
  void GetActors2D(svtkPropCollection*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  int RenderOverlay(svtkViewport* viewport) override;
  int RenderOpaqueGeometry(svtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(svtkViewport* viewport) override;
  svtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  /**
   * Get the points in this contour as a svtkPolyData.
   */
  svtkPolyData* GetContourRepresentationAsPolyData() override;

  /**
   * Direction cosines of the plane on which the contour lies
   * on in world co-ordinates. This would be the same matrix that would be
   * set in svtkImageReslice or svtkImagePlaneWidget if there were a plane
   * passing through the contour points. The origin must be the origin of the
   * data under the contour.
   */
  svtkMatrix4x4* GetContourPlaneDirectionCosines(const double origin[3]);

protected:
  svtkOrientedGlyphFocalPlaneContourRepresentation();
  ~svtkOrientedGlyphFocalPlaneContourRepresentation() override;

  // Render the cursor
  svtkActor2D* Actor;
  svtkPolyDataMapper2D* Mapper;
  svtkGlyph2D* Glypher;
  svtkActor2D* ActiveActor;
  svtkPolyDataMapper2D* ActiveMapper;
  svtkGlyph2D* ActiveGlypher;
  svtkPolyData* CursorShape;
  svtkPolyData* ActiveCursorShape;
  svtkPolyData* FocalData;
  svtkPoints* FocalPoint;
  svtkPolyData* ActiveFocalData;
  svtkPoints* ActiveFocalPoint;

  // The polydata represents the contour in display co-ordinates.
  svtkPolyData* Lines;
  svtkPolyDataMapper2D* LinesMapper;
  svtkActor2D* LinesActor;

  // The polydata represents the contour in world coordinates. It is updated
  // (kept in sync with Lines) every time the GetContourRepresentationAsPolyData()
  // method is called.
  svtkPolyData* LinesWorldCoordinates;

  // Support picking
  double LastPickPosition[3];
  double LastEventPosition[2];

  // Methods to manipulate the cursor
  void Translate(double eventPos[2]);
  void Scale(double eventPos[2]);
  void ShiftContour(double eventPos[2]);
  void ScaleContour(double eventPos[2]);

  void ComputeCentroid(double* ioCentroid);

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  svtkProperty2D* Property;
  svtkProperty2D* ActiveProperty;
  svtkProperty2D* LinesProperty;

  svtkMatrix4x4* ContourPlaneDirectionCosines;

  void CreateDefaultProperties();

  // Distance between where the mouse event happens and where the
  // widget is focused - maintain this distance during interaction.
  double InteractionOffset[2];

  void BuildLines() override;

private:
  svtkOrientedGlyphFocalPlaneContourRepresentation(
    const svtkOrientedGlyphFocalPlaneContourRepresentation&) = delete;
  void operator=(const svtkOrientedGlyphFocalPlaneContourRepresentation&) = delete;
};

#endif
