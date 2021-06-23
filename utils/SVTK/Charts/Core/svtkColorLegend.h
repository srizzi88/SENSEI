/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkColorLegend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkColorLegend
 * @brief   Legend item to display svtkScalarsToColors.
 *
 * svtkColorLegend is an item that will display the svtkScalarsToColors
 * using a 1D texture, and a svtkAxis to show both the color and numerical range.
 */

#ifndef svtkColorLegend_h
#define svtkColorLegend_h

#include "svtkChartLegend.h"
#include "svtkChartsCoreModule.h" // For export macro
#include "svtkSmartPointer.h"     // For SP ivars
#include "svtkVector.h"           // For svtkRectf

class svtkAxis;
class svtkContextMouseEvent;
class svtkImageData;
class svtkScalarsToColors;
class svtkCallbackCommand;

class SVTKCHARTSCORE_EXPORT svtkColorLegend : public svtkChartLegend
{
public:
  svtkTypeMacro(svtkColorLegend, svtkChartLegend);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkColorLegend* New();

  /**
   * Enum of legend orientation types
   */
  enum
  {
    VERTICAL = 0,
    HORIZONTAL
  };

  /**
   * Bounds of the item, by default (0, 1, 0, 1) but it mainly depends on the
   * range of the svtkScalarsToColors function.
   */
  virtual void GetBounds(double bounds[4]);

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint the texture into a rectangle defined by the bounds. If
   * MaskAboveCurve is true and a shape has been provided by a subclass, it
   * draws the texture into the shape
   */
  bool Paint(svtkContext2D* painter) override;

  //@{
  /**
   * Set/Get the transfer function that is used to draw the scalar bar
   * within this legend.
   */
  virtual void SetTransferFunction(svtkScalarsToColors* transfer);
  virtual svtkScalarsToColors* GetTransferFunction();
  //@}

  /**
   * Set the point this legend is anchored to.
   */
  void SetPoint(float x, float y) override;

  /**
   * Set the size of the scalar bar drawn by this legend.
   */
  virtual void SetTextureSize(float w, float h);

  /**
   * Set the origin, width, and height of the scalar bar drawn by this legend.
   * This method overrides the anchor point, as well as any horizontal and
   * vertical alignment that has been set for this legend.  If this is a
   * problem for you, use SetPoint() and SetTextureSize() instead.
   */
  virtual void SetPosition(const svtkRectf& pos);

  /**
   * Returns the origin, width, and height of the scalar bar drawn by this
   * legend.
   */
  virtual svtkRectf GetPosition();

  /**
   * Request the space the legend requires to be drawn. This is returned as a
   * svtkRect4f, with the corner being the offset from Point, and the width/
   * height being the total width/height required by the axis. In order to
   * ensure the numbers are correct, Update() should be called first.
   */
  svtkRectf GetBoundingRect(svtkContext2D* painter) override;

  //@{
  /**
   * Set/get the orientation of the legend.
   * Valid orientations are VERTICAL (default) and HORIZONTAL.
   */
  virtual void SetOrientation(int orientation);
  svtkGetMacro(Orientation, int);
  //@}

  //@{
  /**
   * Get/set the title text of the legend.
   */
  virtual void SetTitle(const svtkStdString& title);
  virtual svtkStdString GetTitle();
  //@}

  //@{
  /**
   * Toggle whether or not a border should be drawn around this legend.
   * The default behavior is to not draw a border.
   */
  svtkSetMacro(DrawBorder, bool);
  svtkGetMacro(DrawBorder, bool);
  svtkBooleanMacro(DrawBorder, bool);
  //@}

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;

protected:
  svtkColorLegend();
  ~svtkColorLegend() override;

  /**
   * Need to be reimplemented by subclasses, ComputeTexture() is called at
   * paint time if the texture is not up to date compared to svtkColorLegend
   */
  virtual void ComputeTexture();

  //@{
  /**
   * Called whenever the ScalarsToColors function(s) is modified. It internally
   * calls Modified(). Can be reimplemented by subclasses.
   */
  virtual void ScalarsToColorsModified(svtkObject* caller, unsigned long eid, void* calldata);
  static void OnScalarsToColorsModified(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  //@}

  /**
   * Moves the axis whenever the position of this legend changes.
   */
  void UpdateAxisPosition();

  svtkScalarsToColors* TransferFunction;
  svtkSmartPointer<svtkImageData> ImageData;
  svtkSmartPointer<svtkAxis> Axis;
  svtkSmartPointer<svtkCallbackCommand> Callback;
  bool Interpolate;
  bool CustomPositionSet;
  bool DrawBorder;
  svtkRectf Position;
  int Orientation;

private:
  svtkColorLegend(const svtkColorLegend&) = delete;
  void operator=(const svtkColorLegend&) = delete;
};

#endif
