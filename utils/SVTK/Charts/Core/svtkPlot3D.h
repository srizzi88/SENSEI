/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlot3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlot3D
 * @brief   Abstract class for 3D plots.
 *
 *
 * The base class for all plot types used in svtkChart derived charts.
 *
 * @sa
 * svtkPlot3DPoints svtkPlot3DLine svtkPlot3DBar svtkChart svtkChartXY
 */

#ifndef svtkPlot3D_h
#define svtkPlot3D_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkContextItem.h"
#include "svtkNew.h"          // Needed to hold svtkNew ivars
#include "svtkSmartPointer.h" // Needed to hold SP ivars
#include "svtkVector.h"       // For Points ivar
#include <vector>            // For ivars

class svtkChartXYZ;
class svtkDataArray;
class svtkIdTypeArray;
class svtkTable;
class svtkUnsignedCharArray;
class svtkPen;

class SVTKCHARTSCORE_EXPORT svtkPlot3D : public svtkContextItem
{
public:
  svtkTypeMacro(svtkPlot3D, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the svtkPen object that controls how this plot draws (out)lines.
   */
  void SetPen(svtkPen* pen);
  svtkPen* GetPen();
  //@}

  //@{
  /**
   * Set/get the svtkPen object that controls how this plot draws (out)lines.
   */
  void SetSelectionPen(svtkPen* pen);
  svtkPen* GetSelectionPen();
  //@}

  //@{
  /**
   * Set the input to the plot.
   */
  virtual void SetInputData(svtkTable* input);
  virtual void SetInputData(svtkTable* input, const svtkStdString& xName, const svtkStdString& yName,
    const svtkStdString& zName);
  virtual void SetInputData(svtkTable* input, const svtkStdString& xName, const svtkStdString& yName,
    const svtkStdString& zName, const svtkStdString& colorName);
  virtual void SetInputData(
    svtkTable* input, svtkIdType xColumn, svtkIdType yColumn, svtkIdType zColumn);
  //@}

  /**
   * Set the color of each point in the plot.  The input is a single component
   * scalar array.  The values of this array will be passed through a lookup
   * table to generate the color for each data point in the plot.
   */
  virtual void SetColors(svtkDataArray* colorArr);

  /**
   * Get all the data points within this plot.
   */
  std::vector<svtkVector3f> GetPoints();

  //@{
  /**
   * Get/set the chart for this plot.
   */
  svtkGetObjectMacro(Chart, svtkChartXYZ);
  virtual void SetChart(svtkChartXYZ* chart);
  //@}

  /**
   * Get the label for the X axis.
   */
  std::string GetXAxisLabel();

  /**
   * Get the label for the Y axis.
   */
  std::string GetYAxisLabel();

  /**
   * Get the label for the Z axis.
   */
  std::string GetZAxisLabel();

  /**
   * Get the bounding cube surrounding the currently rendered data points.
   */
  std::vector<svtkVector3f> GetDataBounds() { return this->DataBounds; }

  //@{
  /**
   * Set/get the selection array for the plot.
   */
  virtual void SetSelection(svtkIdTypeArray* id);
  virtual svtkIdTypeArray* GetSelection();
  //@}

protected:
  svtkPlot3D();
  ~svtkPlot3D() override;

  /**
   * Generate a bounding cube for our data.
   */
  virtual void ComputeDataBounds();

  /**
   * This object stores the svtkPen that controls how the plot is drawn.
   */
  svtkSmartPointer<svtkPen> Pen;

  /**
   * This object stores the svtkPen that controls how the plot is drawn.
   */
  svtkSmartPointer<svtkPen> SelectionPen;

  /**
   * This array assigns a color to each datum in the plot.
   */
  svtkNew<svtkUnsignedCharArray> Colors;

  /**
   * Number of components in our color vectors.  This value is initialized
   * to zero.  It's typically set to 3 or 4 if the points are to be colored.
   */
  int NumberOfComponents;

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
   * The data points read in during SetInputData().
   */
  std::vector<svtkVector3f> Points;

  /**
   * When the points were last built.
   */
  svtkTimeStamp PointsBuildTime;

  /**
   * The chart containing this plot.
   */
  svtkChartXYZ* Chart;

  /**
   * A bounding cube surrounding the currently rendered data points.
   */
  std::vector<svtkVector3f> DataBounds;

  /**
   * Selected indices for the table the plot is rendering
   */
  svtkSmartPointer<svtkIdTypeArray> Selection;

private:
  svtkPlot3D(const svtkPlot3D&) = delete;
  void operator=(const svtkPlot3D&) = delete;
};

#endif // svtkPlot3D_h
