/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtk2DHistogramItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtk2DHistogramItem
 * @brief   2D histogram item.
 *
 *
 *
 */

#ifndef svtkPlotHistogram2D_h
#define svtkPlotHistogram2D_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlot.h"
#include "svtkRect.h"         // Needed for svtkRectf
#include "svtkSmartPointer.h" // Needed for SP ivars

class svtkImageData;
class svtkScalarsToColors;

class SVTKCHARTSCORE_EXPORT svtkPlotHistogram2D : public svtkPlot
{
public:
  svtkTypeMacro(svtkPlotHistogram2D, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a new object.
   */
  static svtkPlotHistogram2D* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the item, called whenever it needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Set the input, we are expecting a svtkImageData with just one component,
   * this would normally be a float or a double. It will be passed to the other
   * functions as a double to generate a color.
   */
  virtual void SetInputData(svtkImageData* data, svtkIdType z = 0);
  void SetInputData(svtkTable*) override {}
  void SetInputData(svtkTable*, const svtkStdString&, const svtkStdString&) override {}

  /**
   * Get the input table used by the plot.
   */
  svtkImageData* GetInputImageData();

  /**
   * Set the color transfer function that will be used to generate the 2D
   * histogram.
   */
  void SetTransferFunction(svtkScalarsToColors* transfer);

  /**
   * Get the color transfer function that is used to generate the histogram.
   */
  svtkScalarsToColors* GetTransferFunction();

  void GetBounds(double bounds[4]) override;

  virtual void SetPosition(const svtkRectf& pos);
  virtual svtkRectf GetPosition();

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex parameter is ignored.
   * The member variable TooltipLabelFormat can be set as a
   * printf-style string to build custom tooltip labels from,
   * and may contain:
   * An empty string generates the default tooltip labels.
   * The following case-sensitive format tags (without quotes) are recognized:
   * '%x' The X position of the histogram cell
   * '%y' The Y position of the histogram cell
   * '%v' The scalar value of the histogram cell
   * Note: the %i and %j tags are valid only if there is a 1:1 correspondence
   * between individual histogram cells and axis tick marks
   * '%i' The X axis tick label for the histogram cell
   * '%j' The Y axis tick label for the histogram cell
   * Any other characters or unrecognized format tags are printed in the
   * tooltip label verbatim.
   */
  svtkStdString GetTooltipLabel(
    const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex) override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns an index between 0 and (number of histogram cells - 1), or -1.
   * The index 0 is at cell x=0, y=0 of the histogram, and the index increases
   * in a minor fashon with x and in a major fashon with y.
   * The referent of "location" is set to the x and y integer indices of the
   * histogram cell.
   */
  svtkIdType GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
    svtkVector2f* location,
#ifndef SVTK_LEGACY_REMOVE
    svtkIdType* segmentId) override;
#else
    svtkIdType* segmentId = nullptr) override;
#endif // SVTK_LEGACY_REMOVE

#ifndef SVTK_LEGACY_REMOVE
  using svtkPlot::GetNearestPoint;
#endif // SVTK_LEGACY_REMOVE

protected:
  svtkPlotHistogram2D();
  ~svtkPlotHistogram2D() override;

  /**
   * Where all the magic happens...
   */
  void GenerateHistogram();

  svtkSmartPointer<svtkImageData> Input;
  svtkSmartPointer<svtkImageData> Output;
  svtkSmartPointer<svtkScalarsToColors> TransferFunction;
  svtkRectf Position;

private:
  svtkPlotHistogram2D(const svtkPlotHistogram2D&) = delete;
  void operator=(const svtkPlotHistogram2D&) = delete;
};

#endif // svtkPlotHistogram2D_h
