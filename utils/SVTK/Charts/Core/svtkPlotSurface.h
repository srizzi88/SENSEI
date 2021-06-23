/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotSurface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotSurface
 * @brief   3D surface plot.
 *
 *
 * 3D surface plot.
 *
 */

#ifndef svtkPlotSurface_h
#define svtkPlotSurface_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkNew.h"              //  For svtkNew ivar
#include "svtkPlot3D.h"

class svtkContext2D;
class svtkLookupTable;
class svtkTable;

class SVTKCHARTSCORE_EXPORT svtkPlotSurface : public svtkPlot3D
{
public:
  svtkTypeMacro(svtkPlotSurface, svtkPlot3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkPlotSurface* New();

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Set the input to the surface plot.
   */
  void SetInputData(svtkTable* input) override;

  //@{
  /**
   * Set the input to the surface plot.
   * Do not use these versions of SetInputData, as all the parameters
   * beyond the svtkTable are ignored.
   */
  void SetInputData(svtkTable* input, const svtkStdString& xName, const svtkStdString& yName,
    const svtkStdString& zName) override;
  void SetInputData(svtkTable* input, const svtkStdString& xName, const svtkStdString& yName,
    const svtkStdString& zName, const svtkStdString& colorName) override;
  void SetInputData(
    svtkTable* input, svtkIdType xColumn, svtkIdType yColumn, svtkIdType zColumn) override;
  //@}

  /**
   * Set the range of the input data for the X dimension.  By default it is
   * (1, NumberOfColumns).  Calling this method after SetInputData() results
   * in recomputation of the plot's data.  Therefore, it is more efficient
   * to call it before SetInputData() when possible.
   */
  void SetXRange(float min, float max);

  /**
   * Set the range of the input data for the Y dimension.  By default it is
   * (1, NumberOfRows).  Calling this method after SetInputData() results
   * in recomputation of the plot's data.  Therefore, it is more efficient
   * to call it before SetInputData() when possible.
   */
  void SetYRange(float min, float max);

protected:
  svtkPlotSurface();
  ~svtkPlotSurface() override;

  /**
   * Generate a surface (for OpenGL) from our list of points.
   */
  void GenerateSurface();

  /**
   * Helper function used to setup a colored surface.
   */
  void InsertSurfaceVertex(float* data, float value, int i, int j, int& pos);

  /**
   * Change data values if SetXRange() or SetYRange() were called.
   */
  void RescaleData();

  /**
   * Map a column index to the user-specified range for the X-axis.
   */
  float ColumnToX(int columnIndex);

  /**
   * Map a row index to the user-specified range for the Y-axis.
   */
  float RowToY(int rowIndex);

  /**
   * Surface to render.
   */
  std::vector<svtkVector3f> Surface;

  /**
   * The number of rows in the input table.
   */
  svtkIdType NumberOfRows;

  /**
   * The number of columns in the input table.
   */
  svtkIdType NumberOfColumns;

  /**
   * The number of vertices in the surface.
   */
  svtkIdType NumberOfVertices;

  /**
   * The number of components used to color the surface.
   */
  int ColorComponents;

  /**
   * The input table used to generate the surface.
   */
  svtkTable* InputTable;

  /**
   * The lookup table used to color the surface by height (Z dimension).
   */
  svtkNew<svtkLookupTable> LookupTable;

  //@{
  /**
   * user-defined data ranges
   */
  float XMinimum;
  float XMaximum;
  float YMinimum;
  float YMaximum;
  //@}

  /**
   * true if user-defined data scaling has already been applied,
   * false otherwise.
   */
  bool DataHasBeenRescaled;

private:
  svtkPlotSurface(const svtkPlotSurface&) = delete;
  void operator=(const svtkPlotSurface&) = delete;
};

#endif // svtkPlotSurface_h
