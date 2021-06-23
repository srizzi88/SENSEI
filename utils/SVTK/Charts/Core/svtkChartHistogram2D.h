/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChart2DHistogram.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkChart2DHistogram
 * @brief   Chart for 2D histograms.
 *
 *
 * This defines the interface for a 2D histogram chart.
 */

#ifndef svtkChartHistogram2D_h
#define svtkChartHistogram2D_h

#include "svtkChartXY.h"
#include "svtkChartsCoreModule.h" // For export macro
#include "svtkSmartPointer.h"     // For SP ivars

class svtkColorLegend;
class svtkPlotHistogram2D;
class svtkImageData;
class svtkScalarsToColors;

class SVTKCHARTSCORE_EXPORT svtkChartHistogram2D : public svtkChartXY
{
public:
  svtkTypeMacro(svtkChartHistogram2D, svtkChartXY);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D histogram chart
   */
  static svtkChartHistogram2D* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  virtual void SetInputData(svtkImageData* data, svtkIdType z = 0);
  virtual void SetTransferFunction(svtkScalarsToColors* function);

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  svtkPlot* GetPlot(svtkIdType index) override;

protected:
  svtkChartHistogram2D();
  ~svtkChartHistogram2D() override;

  svtkSmartPointer<svtkPlotHistogram2D> Histogram;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

  class Private;
  Private* Storage;

  bool UpdateLayout(svtkContext2D* painter) override;

private:
  svtkChartHistogram2D(const svtkChartHistogram2D&) = delete;
  void operator=(const svtkChartHistogram2D&) = delete;
};

#endif // svtkChartHistogram2D_h
