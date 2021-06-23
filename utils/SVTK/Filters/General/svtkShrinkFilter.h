/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShrinkFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkShrinkFilter
 * @brief   shrink cells composing an arbitrary data set
 *
 * svtkShrinkFilter shrinks cells composing an arbitrary data set
 * towards their centroid. The centroid of a cell is computed as the
 * average position of the cell points. Shrinking results in
 * disconnecting the cells from one another. The output of this filter
 * is of general dataset type svtkUnstructuredGrid.
 *
 * @warning
 * It is possible to turn cells inside out or cause self intersection
 * in special cases.
 *
 * @sa
 * svtkShrinkPolyData
 */

#ifndef svtkShrinkFilter_h
#define svtkShrinkFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkShrinkFilter : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkShrinkFilter* New();
  svtkTypeMacro(svtkShrinkFilter, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the fraction of shrink for each cell. The default is 0.5.
   */
  svtkSetClampMacro(ShrinkFactor, double, 0.0, 1.0);
  svtkGetMacro(ShrinkFactor, double);
  //@}

protected:
  svtkShrinkFilter();
  ~svtkShrinkFilter() override;

  // Override to specify support for any svtkDataSet input type.
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Main implementation.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double ShrinkFactor;

private:
  svtkShrinkFilter(const svtkShrinkFilter&) = delete;
  void operator=(const svtkShrinkFilter&) = delete;
};

#endif
