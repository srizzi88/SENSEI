/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOTScatterPlotMatrix.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOTScatterPlotMatrix
 * @brief   container for a matrix of charts.
 *
 *
 * This class specialize svtkScatterPlotMatrix by adding a density map
 * on the chart, computed with OpenTURNS
 *
 * @sa
 * svtkScatterPlotMatrix svtkOTDensityMap
 */

#ifndef svtkOTScatterPlotMatrix_h
#define svtkOTScatterPlotMatrix_h

#include "svtkFiltersOpenTURNSModule.h" // For export macro
#include "svtkScatterPlotMatrix.h"
#include "svtkSmartPointer.h" // For SmartPointer

class svtkOTDensityMap;
class svtkScalarsToColors;

class SVTKFILTERSOPENTURNS_EXPORT svtkOTScatterPlotMatrix : public svtkScatterPlotMatrix
{
public:
  svtkTypeMacro(svtkOTScatterPlotMatrix, svtkScatterPlotMatrix);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a new object.
   */
  static svtkOTScatterPlotMatrix* New();

  /**
   * Set the visibility of density map for the specific plotType, false by default
   */
  void SetDensityMapVisibility(int plotType, bool visible);

  /**
   * Set the density line size for the specified plotType, 2 by default
   */
  void SetDensityLineSize(int plotType, float size);

  /**
   * Set the color for the specified plotType, automatically distributed on HSV by default
   */
  void SetDensityMapColor(int plotType, unsigned int densityLineIndex, const svtkColor4ub& color);

  //@{
  /**
   * Get/Set a custom color transfer function.
   * If none is provided, a default one will be applied based on the range of the density.
   */
  void SetTransferFunction(svtkScalarsToColors* stc);
  svtkScalarsToColors* GetTransferFunction();
  //@}

protected:
  svtkOTScatterPlotMatrix();
  ~svtkOTScatterPlotMatrix() override;

  /**
   * Add a density map as a supplementary plot,
   * with provided row and column, computed with OpenTURNS
   * if DensityMapVisibility is true and we are not animating
   */
  virtual void AddSupplementaryPlot(svtkChart* chart, int plotType, svtkStdString row,
    svtkStdString column, int plotCorner = 0) override;

private:
  svtkOTScatterPlotMatrix(const svtkOTScatterPlotMatrix&) = delete;
  void operator=(const svtkOTScatterPlotMatrix&) = delete;

  class DensityMapSettings;
  std::map<int, DensityMapSettings*> DensityMapsSettings;
  typedef std::map<std::pair<svtkStdString, svtkStdString>, svtkSmartPointer<svtkOTDensityMap> >
    DensityMapCacheMap;
  DensityMapCacheMap DensityMapCache;

  svtkSmartPointer<svtkScalarsToColors> TransferFunction;
};

#endif // svtkOTScatterPlotMatrix_h
