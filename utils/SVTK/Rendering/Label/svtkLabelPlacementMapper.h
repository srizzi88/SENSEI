/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabelPlacementMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkLabelPlacementMapper
 * @brief   Places and renders non-overlapping labels.
 *
 *
 * To use this mapper, first send your data through svtkPointSetToLabelHierarchy,
 * which takes a set of points, associates special arrays to the points (label,
 * priority, etc.), and produces a prioritized spatial tree of labels.
 *
 * This mapper then takes that hierarchy (or hierarchies) as input, and every
 * frame will decide which labels and/or icons to place in order of priority,
 * and will render only those labels/icons. A label render strategy is used to
 * render the labels, and can use e.g. FreeType or Qt for rendering.
 */

#ifndef svtkLabelPlacementMapper_h
#define svtkLabelPlacementMapper_h

#include "svtkMapper2D.h"
#include "svtkRenderingLabelModule.h" // For export macro

class svtkCoordinate;
class svtkLabelRenderStrategy;
class svtkSelectVisiblePoints;

class SVTKRENDERINGLABEL_EXPORT svtkLabelPlacementMapper : public svtkMapper2D
{
public:
  static svtkLabelPlacementMapper* New();
  svtkTypeMacro(svtkLabelPlacementMapper, svtkMapper2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Draw non-overlapping labels to the screen.
   */
  void RenderOverlay(svtkViewport* viewport, svtkActor2D* actor) override;

  //@{
  /**
   * Set the label rendering strategy.
   */
  virtual void SetRenderStrategy(svtkLabelRenderStrategy* s);
  svtkGetObjectMacro(RenderStrategy, svtkLabelRenderStrategy);
  //@}

  //@{
  /**
   * The maximum fraction of the screen that the labels may cover.
   * Label placement stops when this fraction is reached.
   */
  svtkSetClampMacro(MaximumLabelFraction, double, 0., 1.);
  svtkGetMacro(MaximumLabelFraction, double);
  //@}

  //@{
  /**
   * The type of iterator used when traversing the labels.
   * May be svtkLabelHierarchy::FRUSTUM or svtkLabelHierarchy::FULL_SORT
   */
  svtkSetMacro(IteratorType, int);
  svtkGetMacro(IteratorType, int);
  //@}

  //@{
  /**
   * Set whether, or not, to use unicode strings.
   */
  svtkSetMacro(UseUnicodeStrings, bool);
  svtkGetMacro(UseUnicodeStrings, bool);
  svtkBooleanMacro(UseUnicodeStrings, bool);
  //@}

  //@{
  /**
   * Use label anchor point coordinates as normal vectors and eliminate those
   * pointing away from the camera. Valid only when points are on a sphere
   * centered at the origin (such as a 3D geographic view). Off by default.
   */
  svtkGetMacro(PositionsAsNormals, bool);
  svtkSetMacro(PositionsAsNormals, bool);
  svtkBooleanMacro(PositionsAsNormals, bool);
  //@}

  //@{
  /**
   * Enable drawing spokes (lines) to anchor point coordinates that were perturbed
   * for being coincident with other anchor point coordinates.
   */
  svtkGetMacro(GeneratePerturbedLabelSpokes, bool);
  svtkSetMacro(GeneratePerturbedLabelSpokes, bool);
  svtkBooleanMacro(GeneratePerturbedLabelSpokes, bool);
  //@}

  //@{
  /**
   * Use the depth buffer to test each label to see if it should not be displayed if
   * it would be occluded by other objects in the scene. Off by default.
   */
  svtkGetMacro(UseDepthBuffer, bool);
  svtkSetMacro(UseDepthBuffer, bool);
  svtkBooleanMacro(UseDepthBuffer, bool);
  //@}

  //@{
  /**
   * Tells the placer to place every label regardless of overlap.
   * Off by default.
   */
  svtkSetMacro(PlaceAllLabels, bool);
  svtkGetMacro(PlaceAllLabels, bool);
  svtkBooleanMacro(PlaceAllLabels, bool);
  //@}

  //@{
  /**
   * Whether to render traversed bounds. Off by default.
   */
  svtkSetMacro(OutputTraversedBounds, bool);
  svtkGetMacro(OutputTraversedBounds, bool);
  svtkBooleanMacro(OutputTraversedBounds, bool);
  //@}

  enum LabelShape
  {
    NONE,
    RECT,
    ROUNDED_RECT,
    NUMBER_OF_LABEL_SHAPES
  };

  //@{
  /**
   * The shape of the label background, should be one of the
   * values in the LabelShape enumeration.
   */
  svtkSetClampMacro(Shape, int, 0, NUMBER_OF_LABEL_SHAPES - 1);
  svtkGetMacro(Shape, int);
  virtual void SetShapeToNone() { this->SetShape(NONE); }
  virtual void SetShapeToRect() { this->SetShape(RECT); }
  virtual void SetShapeToRoundedRect() { this->SetShape(ROUNDED_RECT); }
  //@}

  enum LabelStyle
  {
    FILLED,
    OUTLINE,
    NUMBER_OF_LABEL_STYLES
  };

  //@{
  /**
   * The style of the label background shape, should be one of the
   * values in the LabelStyle enumeration.
   */
  svtkSetClampMacro(Style, int, 0, NUMBER_OF_LABEL_STYLES - 1);
  svtkGetMacro(Style, int);
  virtual void SetStyleToFilled() { this->SetStyle(FILLED); }
  virtual void SetStyleToOutline() { this->SetStyle(OUTLINE); }
  //@}

  //@{
  /**
   * The size of the margin on the label background shape.
   * Default is 5.
   */
  svtkSetMacro(Margin, double);
  svtkGetMacro(Margin, double);
  //@}

  //@{
  /**
   * The color of the background shape.
   */
  svtkSetVector3Macro(BackgroundColor, double);
  svtkGetVector3Macro(BackgroundColor, double);
  //@}

  //@{
  /**
   * The opacity of the background shape.
   */
  svtkSetClampMacro(BackgroundOpacity, double, 0.0, 1.0);
  svtkGetMacro(BackgroundOpacity, double);
  //@}

  //@{
  /**
   * Get the transform for the anchor points.
   */
  svtkGetObjectMacro(AnchorTransform, svtkCoordinate);
  //@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkLabelPlacementMapper();
  ~svtkLabelPlacementMapper() override;

  virtual void SetAnchorTransform(svtkCoordinate*);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  class Internal;
  Internal* Buckets;

  svtkLabelRenderStrategy* RenderStrategy;
  svtkCoordinate* AnchorTransform;
  svtkSelectVisiblePoints* VisiblePoints;
  double MaximumLabelFraction;
  bool PositionsAsNormals;
  bool GeneratePerturbedLabelSpokes;
  bool UseDepthBuffer;
  bool UseUnicodeStrings;
  bool PlaceAllLabels;
  bool OutputTraversedBounds;

  int LastRendererSize[2];
  double LastCameraPosition[3];
  double LastCameraFocalPoint[3];
  double LastCameraViewUp[3];
  double LastCameraParallelScale;
  int IteratorType;

  int Style;
  int Shape;
  double Margin;
  double BackgroundOpacity;
  double BackgroundColor[3];

private:
  svtkLabelPlacementMapper(const svtkLabelPlacementMapper&) = delete;
  void operator=(const svtkLabelPlacementMapper&) = delete;
};

#endif
