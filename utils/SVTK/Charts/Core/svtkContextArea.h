/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextArea.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextArea
 * @brief   Clipped, transformed area with axes for context items.
 *
 *
 * svtkContextArea provides an clipped drawing area surrounded by four axes.
 * The drawing area is transformed to map the 2D area described by
 * DrawAreaBounds into pixel coordinates. DrawAreaBounds is also used to
 * configure the axes. Item to be rendered in the draw area should be added
 * to the context item returned by GetDrawAreaItem().
 *
 * The size and shape of the draw area is configured by the following member
 * variables:
 * - Geometry: The rect (pixel coordinates) defining the location of the context
 *   area in the scene. This includes the draw area and axis ticks/labels.
 * - FillViewport: If true (default), Geometry is set to span the size returned
 *   by svtkContextDevice2D::GetViewportSize().
 * - DrawAreaResizeBehavior: Controls how the draw area should be shaped.
 *   Available options: Expand (default), FixedAspect, FixedRect, FixedMargins.
 * - FixedAspect: Aspect ratio to enforce for FixedAspect resize behavior.
 * - FixedRect: Rect used to enforce for FixedRect resize behavior.
 * - FixedMargins: Margins to enforce for FixedMargins resize behavior.
 */

#ifndef svtkContextArea_h
#define svtkContextArea_h

#include "svtkAbstractContextItem.h"

#include "svtkAxis.h"             // For enums
#include "svtkChartsCoreModule.h" // For export macro
#include "svtkNew.h"              // For svtkNew
#include "svtkRect.h"             // For svtkRect/svtkVector/svtkTuple

class svtkContextClip;
class svtkContextTransform;
class svtkPlotGrid;

class SVTKCHARTSCORE_EXPORT svtkContextArea : public svtkAbstractContextItem
{
public:
  typedef svtkTuple<int, 4> Margins;
  svtkTypeMacro(svtkContextArea, svtkAbstractContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkContextArea* New();

  /**
   * Get the svtkAxis associated with the specified location.
   */
  svtkAxis* GetAxis(svtkAxis::Location location);

  /**
   * Returns the svtkAbstractContextItem that will draw in the clipped,
   * transformed space. This is the item to add children for.
   */
  svtkAbstractContextItem* GetDrawAreaItem();

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

  //@{
  /**
   * The rect defining the pixel location and size of the entire svtkContextArea,
   * including axis labels, title, etc. Note that this will be updated to the
   * window geometry if FillWindow is true.
   */
  svtkGetMacro(Geometry, svtkRecti);
  svtkSetMacro(Geometry, svtkRecti);
  //@}

  //@{
  /**
   * The data bounds of the clipped and transformed area inside of the axes.
   * This is used to configure the axes labels and setup the transform.
   */
  svtkGetMacro(DrawAreaBounds, svtkRectd);
  svtkSetMacro(DrawAreaBounds, svtkRectd);
  //@}

  enum DrawAreaResizeBehaviorType
  {
    DARB_Expand,
    DARB_FixedAspect,
    DARB_FixedRect,
    DARB_FixedMargins
  };

  //@{
  /**
   * Set the resize behavior for the draw area:
   * - @a Expand: The default behavior. The draw area will automatically resize
   * to take up as much of @a Geometry as possible. Margin sizes are
   * minimized based on the space required for axis labels/tick marks.
   * - FixedAspect: Same as Expand, but a fixed aspect ratio is enforced.
   * See SetFixedAspect.
   * - FixedRect: Draw area is always constrained to a fixed rectangle.
   * See SetFixedRect.
   * - FixMargins: The draw area expands to fill @a Geometry, but margins
   * (axis labels, etc) are fixed, rather than dynamically sized.
   * See SetFixedMargins.
   */
  svtkGetMacro(DrawAreaResizeBehavior, DrawAreaResizeBehaviorType);
  svtkSetMacro(DrawAreaResizeBehavior, DrawAreaResizeBehaviorType);
  //@}

  //@{
  /**
   * The fixed aspect ratio, if DrawAreaResizeBehavior is FixedAspect.
   * Defined as width/height. Default is 1.
   * Setting the aspect ratio will also set DrawAreaResizeBehavior to
   * FixedAspect.
   */
  svtkGetMacro(FixedAspect, float) virtual void SetFixedAspect(float aspect);
  //@}

  //@{
  /**
   * The fixed rect to use for the draw area, if DrawAreaResizeBehavior is
   * FixedRect. Units are in pixels, default is 300x300+0+0.
   * Setting the fixed rect will also set DrawAreaResizeBehavior to
   * FixedRect.
   */
  svtkGetMacro(FixedRect, svtkRecti);
  virtual void SetFixedRect(svtkRecti rect);
  virtual void SetFixedRect(int x, int y, int width, int height);
  //@}

  //@{
  /**
   * The left, right, bottom, and top margins for the draw area, if
   * DrawAreaResizeBehavior is FixedMargins. Units are in pixels, default is
   * { 0, 0, 0, 0 }.
   * Setting the fixed margins will also set DrawAreaResizeBehavior to
   * FixedMargins.
   */
  virtual const Margins& GetFixedMargins() { return this->FixedMargins; }
  virtual void GetFixedMarginsArray(int margins[4]);
  virtual const int* GetFixedMarginsArray();
  virtual void SetFixedMargins(Margins margins);
  virtual void SetFixedMargins(int margins[4]);
  virtual void SetFixedMargins(int left, int right, int bottom, int top);
  //@}

  //@{
  /**
   * If true, Geometry is set to (0, 0, vpSize[0], vpSize[1]) at the start
   * of each Paint call. vpSize is svtkContextDevice2D::GetViewportSize. Default
   * is true.
   */
  svtkGetMacro(FillViewport, bool);
  svtkSetMacro(FillViewport, bool);
  svtkBooleanMacro(FillViewport, bool);
  //@}

  //@{
  /**
   * Turn on/off grid visibility.
   */
  virtual void SetShowGrid(bool show);
  virtual bool GetShowGrid();
  virtual void ShowGridOn() { this->SetShowGrid(true); }
  virtual void ShowGridOff() { this->SetShowGrid(false); }
  //@}

protected:
  svtkContextArea();
  ~svtkContextArea() override;

  /**
   * Sync the Axes locations with Geometry, and update the DrawAreaGeometry
   * to account for Axes size (margins). Must be called while the painter
   * is active.
   */
  void LayoutAxes(svtkContext2D* painter);
  virtual void SetAxisRange(svtkRectd const& data);
  virtual void ComputeViewTransform();

  /**
   * Return the draw area's geometry.
   */
  svtkRecti ComputeDrawAreaGeometry(svtkContext2D* painter);

  //@{
  /**
   * Working implementations for ComputeDrawAreaGeometry.
   */
  svtkRecti ComputeExpandedDrawAreaGeometry(svtkContext2D* painter);
  svtkRecti ComputeFixedAspectDrawAreaGeometry(svtkContext2D* painter);
  svtkRecti ComputeFixedRectDrawAreaGeometry(svtkContext2D* painter);
  svtkRecti ComputeFixedMarginsDrawAreaGeometry(svtkContext2D* painter);
  //@}

  /**
   * Set the transform to map DrawAreaBounds to DrawAreaGeometry. Should be
   * called after LayoutAxes to ensure that DrawAreaGeometry is up to date.
   */
  void UpdateDrawArea();

  /**
   * svtkAxis objects that surround the draw area, indexed by svtkAxis::Location.
   */
  svtkTuple<svtkAxis*, 4> Axes;

  /**
   * The svtkPlotGrid that renders a grid atop the data in the draw area.
   */
  svtkNew<svtkPlotGrid> Grid;

  /**
   * The context item that clips rendered data.
   */
  svtkNew<svtkContextClip> Clip;

  /**
   * The context item that clips rendered data.
   */
  svtkNew<svtkContextTransform> Transform;

  /**
   * The rect defining the pixel location and size of the entire svtkContextArea,
   * including axis label, title, etc.
   */
  svtkRecti Geometry;

  /**
   * The data bounds of the clipped and transformed area inside of the axes.
   * This is used to configure the axes labels and setup the transform.
   */
  svtkRectd DrawAreaBounds;

  /**
   * The rect defining the pixel location and size of the clipped and
   * transformed area inside the axes. Relative to Geometry.
   */
  svtkRecti DrawAreaGeometry;

  /**
   * Controls how the draw area size is determined.
   */
  DrawAreaResizeBehaviorType DrawAreaResizeBehavior;

  /**
   * The fixed aspect ratio, if DrawAreaResizeBehavior is FixedAspect.
   * Defined as width/height. Default is 1.
   */
  float FixedAspect;

  /**
   * The fixed rect to use for the draw area, if DrawAreaResizeBehavior is
   * FixedRect. Units are in pixels, default is 300x300+0+0.
   */
  svtkRecti FixedRect;

  /**
   * The left, right, bottom, and top margins for the draw area, if
   * DrawAreaResizeBehavior is FixedMargins. Units are in pixels, default is
   * { 0, 0, 0, 0 }
   */
  Margins FixedMargins;

  /**
   * If true, Geometry is set to (0, 0, vpSize[0], vpSize[1]) at the start
   * of each Paint call. vpSize is svtkContextDevice2D::GetViewportSize. Default
   * is true.
   */
  bool FillViewport;

  /**
   * Initialize the drawing area's item hierarchy
   */
  virtual void InitializeDrawArea();

  // Smart pointers for axis lifetime management. See this->Axes.
  svtkNew<svtkAxis> TopAxis;
  svtkNew<svtkAxis> BottomAxis;
  svtkNew<svtkAxis> LeftAxis;
  svtkNew<svtkAxis> RightAxis;

private:
  svtkContextArea(const svtkContextArea&) = delete;
  void operator=(const svtkContextArea&) = delete;
};

#endif // svtkContextArea_h
