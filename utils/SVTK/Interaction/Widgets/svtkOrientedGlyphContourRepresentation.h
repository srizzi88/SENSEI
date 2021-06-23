/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOrientedGlyphContourRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOrientedGlyphContourRepresentation
 * @brief   Default representation for the contour widget
 *
 * This class provides the default concrete representation for the
 * svtkContourWidget. It works in conjunction with the
 * svtkContourLineInterpolator and svtkPointPlacer. See svtkContourWidget
 * for details.
 * @sa
 * svtkContourRepresentation svtkContourWidget svtkPointPlacer
 * svtkContourLineInterpolator
 */

#ifndef svtkOrientedGlyphContourRepresentation_h
#define svtkOrientedGlyphContourRepresentation_h

#include "svtkContourRepresentation.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkProperty;
class svtkActor;
class svtkPolyDataMapper;
class svtkPolyData;
class svtkGlyph3D;
class svtkPoints;

class SVTKINTERACTIONWIDGETS_EXPORT svtkOrientedGlyphContourRepresentation
  : public svtkContourRepresentation
{
public:
  /**
   * Instantiate this class.
   */
  static svtkOrientedGlyphContourRepresentation* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkOrientedGlyphContourRepresentation, svtkContourRepresentation);
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
  svtkGetObjectMacro(Property, svtkProperty);
  //@}

  //@{
  /**
   * This is the property used when the user is interacting
   * with the handle.
   */
  svtkGetObjectMacro(ActiveProperty, svtkProperty);
  //@}

  //@{
  /**
   * This is the property used by the lines.
   */
  svtkGetObjectMacro(LinesProperty, svtkProperty);
  //@}

  //@{
  /**
   * Subclasses of svtkOrientedGlyphContourRepresentation must implement these methods. These
   * are the methods that the widget and its representation use to
   * communicate with each other.
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
  void GetActors(svtkPropCollection*) override;
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

  //@{
  /**
   * Controls whether the contour widget should always appear on top
   * of other actors in the scene. (In effect, this will disable OpenGL
   * Depth buffer tests while rendering the contour).
   * Default is to set it to false.
   */
  svtkSetMacro(AlwaysOnTop, svtkTypeBool);
  svtkGetMacro(AlwaysOnTop, svtkTypeBool);
  svtkBooleanMacro(AlwaysOnTop, svtkTypeBool);
  //@}

  /**
   * Convenience method to set the line color.
   * Ideally one should use GetLinesProperty()->SetColor().
   */
  void SetLineColor(double r, double g, double b);

  /**
   * A flag to indicate whether to show the Selected nodes
   * Default is to set it to false.
   */
  void SetShowSelectedNodes(svtkTypeBool) override;

  /**
   * Return the bounds of the representation
   */
  double* GetBounds() override;

protected:
  svtkOrientedGlyphContourRepresentation();
  ~svtkOrientedGlyphContourRepresentation() override;

  // Render the cursor
  svtkActor* Actor;
  svtkPolyDataMapper* Mapper;
  svtkGlyph3D* Glypher;
  svtkActor* ActiveActor;
  svtkPolyDataMapper* ActiveMapper;
  svtkGlyph3D* ActiveGlypher;
  svtkPolyData* CursorShape;
  svtkPolyData* ActiveCursorShape;
  svtkPolyData* FocalData;
  svtkPoints* FocalPoint;
  svtkPolyData* ActiveFocalData;
  svtkPoints* ActiveFocalPoint;

  svtkPolyData* SelectedNodesData;
  svtkPoints* SelectedNodesPoints;
  svtkActor* SelectedNodesActor;
  svtkPolyDataMapper* SelectedNodesMapper;
  svtkGlyph3D* SelectedNodesGlypher;
  svtkPolyData* SelectedNodesCursorShape;
  void CreateSelectedNodesRepresentation();

  svtkPolyData* Lines;
  svtkPolyDataMapper* LinesMapper;
  svtkActor* LinesActor;

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
  svtkProperty* Property;
  svtkProperty* ActiveProperty;
  svtkProperty* LinesProperty;
  void CreateDefaultProperties();

  // Distance between where the mouse event happens and where the
  // widget is focused - maintain this distance during interaction.
  double InteractionOffset[2];

  svtkTypeBool AlwaysOnTop;

  void BuildLines() override;

private:
  svtkOrientedGlyphContourRepresentation(const svtkOrientedGlyphContourRepresentation&) = delete;
  void operator=(const svtkOrientedGlyphContourRepresentation&) = delete;
};

#endif
