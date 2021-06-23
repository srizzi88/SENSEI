/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkChartMatrix
 * @brief   container for a matrix of charts.
 *
 *
 * This class contains a matrix of charts. These charts will be of type
 * svtkChartXY by default, but this can be overridden. The class will manage
 * their layout and object lifetime.
 */

#ifndef svtkChartMatrix_h
#define svtkChartMatrix_h

#include "svtkAbstractContextItem.h"
#include "svtkChartsCoreModule.h" // For export macro
#include "svtkVector.h"           // For ivars

#include <map>     // For specific gutter
#include <utility> // For specific gutter

class svtkChart;

class SVTKCHARTSCORE_EXPORT svtkChartMatrix : public svtkAbstractContextItem
{
public:
  svtkTypeMacro(svtkChartMatrix, svtkAbstractContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a new object.
   */
  static svtkChartMatrix* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   */
  void Update() override;

  /**
   * Paint event for the chart matrix.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Set the width and height of the chart matrix. This will cause an immediate
   * resize of the chart matrix, the default size is 0x0 (no charts). No chart
   * objects are created until Allocate is called.
   */
  virtual void SetSize(const svtkVector2i& size);

  /**
   * Get the width and height of the chart matrix.
   */
  virtual svtkVector2i GetSize() const { return this->Size; }

  //@{
  /**
   * Set/get the borders of the chart matrix (space in pixels around each chart).
   */
  virtual void SetBorders(int left, int bottom, int right, int top);
  void SetBorderLeft(int value);
  void SetBorderBottom(int value);
  void SetBorderRight(int value);
  void SetBorderTop(int value);
  virtual void GetBorders(int borders[4])
  {
    for (int i = 0; i < 4; i++)
    {
      borders[i] = this->Borders[i];
    }
  }
  //@}

  //@{
  /**
   * Set the gutter that should be left between the charts in the matrix.
   */
  virtual void SetGutter(const svtkVector2f& gutter);
  void SetGutterX(float value);
  void SetGutterY(float value);
  //@}

  //@{
  /**
   * Set a specific resize that will move the bottom left point of a chart.
   */
  virtual void SetSpecificResize(const svtkVector2i& index, const svtkVector2f& resize);
  virtual void ClearSpecificResizes();
  //@}

  /**
   * Get the gutter that should be left between the charts in the matrix.
   */
  virtual svtkVector2f GetGutter() const { return this->Gutter; }

  /**
   * Allocate the charts, this will cause any null chart to be allocated.
   */
  virtual void Allocate();

  /**
   * Set the chart element, note that the chart matrix must be large enough to
   * accommodate the element being set. Note that this class will take ownership
   * of the chart object.
   * \return false if the element cannot be set.
   */
  virtual bool SetChart(const svtkVector2i& position, svtkChart* chart);

  /**
   * Get the specified chart element, if the element does not exist nullptr will be
   * returned. If the chart element has not yet been allocated it will be at
   * this point.
   */
  virtual svtkChart* GetChart(const svtkVector2i& position);

  /**
   * Set the span of a chart in the matrix. This defaults to 1x1, and cannot
   * exceed the remaining space in x or y.
   * \return false If the span is not possible.
   */
  virtual bool SetChartSpan(const svtkVector2i& position, const svtkVector2i& span);

  /**
   * Get the span of the specified chart.
   */
  virtual svtkVector2i GetChartSpan(const svtkVector2i& position);

  /**
   * Get the position of the chart in the matrix at the specified location.
   * The position should be specified in scene coordinates.
   */
  virtual svtkVector2i GetChartIndex(const svtkVector2f& position);

protected:
  svtkChartMatrix();
  ~svtkChartMatrix() override;

  // The number of charts in x and y.
  svtkVector2i Size;

  // The gutter between each chart.
  svtkVector2f Gutter;
  std::map<svtkVector2i, svtkVector2f> SpecificResize;
  int Borders[4];
  bool LayoutIsDirty;

private:
  svtkChartMatrix(const svtkChartMatrix&) = delete;
  void operator=(const svtkChartMatrix&) = delete;

  class PIMPL;
  PIMPL* Private;
};

#endif // svtkChartMatrix_h
