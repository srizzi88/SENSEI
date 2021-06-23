/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartXYZ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkChartXYZ
 * @brief   Factory class for drawing 3D XYZ charts.
 *
 *
 */

#ifndef svtkChartXYZ_h
#define svtkChartXYZ_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkColor.h"            // For svtkColor4ub
#include "svtkContextItem.h"
#include "svtkNew.h"          // For ivars
#include "svtkRect.h"         // For svtkRectf ivars
#include "svtkSmartPointer.h" // For ivars
#include <vector>            // For ivars

class svtkAnnotationLink;
class svtkAxis;
class svtkContext3D;
class svtkContextMouseEvent;
class svtkPen;
class svtkPlaneCollection;
class svtkPlot3D;
class svtkTable;
class svtkTransform;
class svtkUnsignedCharArray;

class SVTKCHARTSCORE_EXPORT svtkChartXYZ : public svtkContextItem
{
public:
  svtkTypeMacro(svtkChartXYZ, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkChartXYZ* New();

  /**
   * Set the geometry in pixel coordinates (origin and width/height).
   * This method also sets up the end points of the axes of the chart.
   * For this reason, if you call SetAroundX(), you should call SetGeometry()
   * afterwards.
   */
  void SetGeometry(const svtkRectf& bounds);

  /**
   * Set the rotation angle for the chart (AutoRotate mode only).
   */
  void SetAngle(double angle);

  /**
   * Set whether or not we're rotating about the X axis.
   */
  void SetAroundX(bool isX);

  /**
   * Set the svtkAnnotationLink for the chart.
   */
  virtual void SetAnnotationLink(svtkAnnotationLink* link);

  /**
   * Get the x (0), y (1) or z (2) axis.
   */
  svtkAxis* GetAxis(int axis);

  /**
   * Set the x (0), y (1) or z (2) axis.
   */
  virtual void SetAxis(int axisIndex, svtkAxis* axis);

  //@{
  /**
   * Set the color for the axes.
   */
  void SetAxisColor(const svtkColor4ub& color);
  svtkColor4ub GetAxisColor();
  //@}

  /**
   * Set whether or not we're using this chart to rotate on a timer.
   * Default value is false.
   */
  void SetAutoRotate(bool b);

  /**
   * Set whether or not axes labels & tick marks should be drawn.
   * Default value is true.
   */
  void SetDecorateAxes(bool b);

  /**
   * Set whether or not the chart should automatically resize itself to fill
   * the scene.  Default value is true.
   */
  void SetFitToScene(bool b);

  /**
   * Perform any updates to the item that may be necessary before rendering.
   */
  void Update() override;

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Adds a plot to the chart.
   */
  virtual svtkIdType AddPlot(svtkPlot3D* plot);

  /**
   * Remove all the plots from this chart.
   */
  void ClearPlots();

  /**
   * Determine the XYZ bounds of the plots within this chart.
   * This information is then used to set the range of the axes.
   */
  void RecalculateBounds();

  /**
   * Use this chart's Geometry to set the endpoints of its axes.
   * This method also sets up a transformation that is used to
   * properly render the data within the chart.
   */
  void RecalculateTransform();

  /**
   * Returns true if the transform is interactive, false otherwise.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse press event. Keep track of zoom anchor position.
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event. Perform pan or zoom as specified by the mouse bindings.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse wheel event.  Zooms in or out.
   */
  bool MouseWheelEvent(const svtkContextMouseEvent& mouse, int delta) override;

  /**
   * Key press event.  This allows the user to snap the chart to one of three
   * different 2D views.  "x" changes the view so we're looking down the X axis.
   * Similar behavior occurs for "y" or "z".
   */
  bool KeyPressEvent(const svtkContextKeyEvent& key) override;

protected:
  svtkChartXYZ();
  ~svtkChartXYZ() override;

  /**
   * Calculate the transformation matrices used to draw data points and axes
   * in the scene.  This function also sets up clipping planes that determine
   * whether or not a data point is within range.
   */
  virtual void CalculateTransforms();

  /**
   * Given the x, y and z svtkAxis, and a transform, calculate the transform that
   * the points in a chart would need to be drawn within the axes. This assumes
   * that the axes have the correct start and end positions, and that they are
   * perpendicular.
   */
  bool CalculatePlotTransform(svtkAxis* x, svtkAxis* y, svtkAxis* z, svtkTransform* transform);

  /**
   * Rotate the chart in response to a mouse movement.
   */
  bool Rotate(const svtkContextMouseEvent& mouse);

  /**
   * Pan the data within the chart in response to a mouse movement.
   */
  bool Pan(const svtkContextMouseEvent& mouse);

  /**
   * Zoom in or out on the data in response to a mouse movement.
   */
  bool Zoom(const svtkContextMouseEvent& mouse);

  /**
   * Spin the chart in response to a mouse movement.
   */
  bool Spin(const svtkContextMouseEvent& mouse);

  /**
   * Adjust the rotation of the chart so that we are looking down the X axis.
   */
  void LookDownX();

  /**
   * Adjust the rotation of the chart so that we are looking down the Y axis.
   */
  void LookDownY();

  /**
   * Adjust the rotation of the chart so that we are looking down the Z axis.
   */
  void LookDownZ();

  /**
   * Adjust the rotation of the chart so that we are looking up the X axis.
   */
  void LookUpX();

  /**
   * Adjust the rotation of the chart so that we are looking up the Y axis.
   */
  void LookUpY();

  /**
   * Adjust the rotation of the chart so that we are looking up the Z axis.
   */
  void LookUpZ();

  /**
   * Check to see if the scene changed size since the last render.
   */
  bool CheckForSceneResize();

  /**
   * Scale the axes up or down in response to a scene resize.
   */
  void RescaleAxes();

  /**
   * Scale up the axes when the scene gets larger.
   */
  void ScaleUpAxes();

  /**
   * Scale down the axes when the scene gets smaller.
   */
  void ScaleDownAxes();

  /**
   * Change the scaling of the axes by a specified amount.
   */
  void ZoomAxes(int delta);

  /**
   * Initialize a list of "test points".  These are used to determine whether
   * or not the chart fits completely within the bounds of the current scene.
   */
  void InitializeAxesBoundaryPoints();

  /**
   * Initialize the "future box" transform.  This transform is a duplicate of
   * the Box transform, which dictates how the chart's axes should be drawn.
   * In ScaleUpAxes() and ScaleDownAxes(), we incrementally change the scaling
   * of the FutureBox transform to determine how much we need to zoom in or
   * zoom out to fit the chart within the newly resized scene.  Using a
   * separate transform for this process allows us to resize the Box in a
   * single step.
   */
  void InitializeFutureBox();

  /**
   * Compute a bounding box for the data that is rendered within the axes.
   */
  void ComputeDataBounds();

  /**
   * Draw the cube axes of this chart.
   */
  void DrawAxes(svtkContext3D* context);

  /**
   * For each of the XYZ dimensions, find the axis line that is furthest
   * from the rendered data.
   */
  void DetermineWhichAxesToLabel();

  /**
   * Draw tick marks and tick mark labels along the axes.
   */
  void DrawTickMarks(svtkContext2D* painter);

  /**
   * Label the axes.
   */
  void DrawAxesLabels(svtkContext2D* painter);

  /**
   * Compute how some text should be offset from an axis.  The parameter
   * bounds contains the bounding box of the text to be rendered.  The
   * result is stored in the parameter offset.
   */
  void GetOffsetForAxisLabel(int axis, float* bounds, float* offset);

  /**
   * Calculate the next "nicest" numbers above and below the current minimum.
   * \return the "nice" spacing of the numbers.
   * This function was mostly copied from svtkAxis.
   */
  double CalculateNiceMinMax(double& min, double& max, int axis);

  /**
   * Get the equation for the ith face of our bounding cube.
   */
  void GetClippingPlaneEquation(int i, double* planeEquation);

  /**
   * The size and position of this chart.
   */
  svtkRectf Geometry;

  /**
   * The 3 axes of this chart.
   */
  std::vector<svtkSmartPointer<svtkAxis> > Axes;

  /**
   * This boolean indicates whether or not we're using this chart to rotate
   * on a timer.
   */
  bool AutoRotate;

  /**
   * When we're in AutoRotate mode, this boolean tells us if we should rotate
   * about the X axis or the Y axis.
   */
  bool IsX;

  /**
   * When we're in AutoRotate mode, this value tells the chart how much it
   * should be rotated.
   */
  double Angle;

  /**
   * This boolean indicates whether or not we should draw tick marks
   * and axes labels.
   */
  bool DrawAxesDecoration;

  /**
   * This boolean indicates whether or not we should automatically resize the
   * chart so that it snugly fills up the scene.
   */
  bool FitToScene;

  /**
   * This is the transform that is applied when rendering data from the plots.
   */
  svtkNew<svtkTransform> ContextTransform;

  /**
   * This transform translates and scales the plots' data points so that they
   * appear within the axes of this chart.  It is one of the factors that
   * makes up the ContextTransform.
   */
  svtkNew<svtkTransform> PlotTransform;

  /**
   * This is the transform that is applied when rendering data from the plots.
   */
  svtkNew<svtkTransform> Box;

  /**
   * This transform keeps track of how the chart has been rotated.
   */
  svtkNew<svtkTransform> Rotation;

  /**
   * This transform keeps track of how the data points have been panned within
   * the chart.
   */
  svtkNew<svtkTransform> Translation;

  /**
   * This transform keeps track of how the data points have been scaled
   * (zoomed in or zoomed out) within the chart.
   */
  svtkNew<svtkTransform> Scale;

  /**
   * This transform keeps track of how the axes have been scaled
   * (zoomed in or zoomed out).
   */
  svtkNew<svtkTransform> BoxScale;

  /**
   * This transform is initialized as a copy of Box.  It is used within
   * ScaleUpAxes() and ScaleDownAxes() to figure out how much we need to
   * zoom in or zoom out to fit our chart within the newly resized scene.
   */
  svtkNew<svtkTransform> FutureBox;

  /**
   * This transform keeps track of the Scale of the FutureBox transform.
   */
  svtkNew<svtkTransform> FutureBoxScale;

  /**
   * This is the pen that is used to draw data from the plots.
   */
  svtkNew<svtkPen> Pen;

  /**
   * This is the pen that is used to draw the axes.
   */
  svtkNew<svtkPen> AxisPen;

  /**
   * This link is used to share selected points with other classes.
   */
  svtkSmartPointer<svtkAnnotationLink> Link;

  /**
   * The plots that are drawn within this chart.
   */
  std::vector<svtkPlot3D*> Plots;

  /**
   * The label for the X Axis.
   */
  std::string XAxisLabel;

  /**
   * The label for the Y Axis.
   */
  std::string YAxisLabel;

  /**
   * The label for the Z Axis.
   */
  std::string ZAxisLabel;

  /**
   * The six planes that define the bounding cube of our 3D axes.
   */
  svtkNew<svtkPlaneCollection> BoundingCube;

  /**
   * Points used to determine whether the axes will fit within the scene as
   * currently sized, regardless of rotation.
   */
  float AxesBoundaryPoints[14][3];

  /**
   * This member variable stores the size of the tick labels for each axis.
   * It is used to determine the position of the axis labels.
   */
  float TickLabelOffset[3][2];

  /**
   * The height of the scene, as of the most recent call to Paint().
   */
  int SceneHeight;

  /**
   * The weight of the scene, as of the most recent call to Paint().
   */
  int SceneWidth;

  //@{
  /**
   * Which line to label.
   */
  int XAxisToLabel[3];
  int YAxisToLabel[3];
  int ZAxisToLabel[3];
  //@}

  /**
   * What direction the data is from each labeled axis line.
   */
  int DirectionToData[3];

  /**
   * A bounding box surrounding the currently rendered data points.
   */
  double DataBounds[4];

private:
  svtkChartXYZ(const svtkChartXYZ&) = delete;
  void operator=(const svtkChartXYZ&) = delete;
};

#endif
