/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDecomposeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDecomposeFilter
 * @brief   Filters that execute axes in series.
 *
 * This superclass molds the svtkImageIterateFilter superclass so
 * it iterates over the axes.  The filter uses dimensionality to
 * determine how many axes to execute (starting from x).
 * The filter also provides convenience methods for permuting information
 * retrieved from input, output and svtkImageData.
 */

#ifndef svtkImageDecomposeFilter_h
#define svtkImageDecomposeFilter_h

#include "svtkImageIterateFilter.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImageDecomposeFilter : public svtkImageIterateFilter
{
public:
  //@{
  /**
   * Construct an instance of svtkImageDecomposeFilter filter with default
   * dimensionality 3.
   */
  svtkTypeMacro(svtkImageDecomposeFilter, svtkImageIterateFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Dimensionality is the number of axes which are considered during
   * execution. To process images dimensionality would be set to 2.
   */
  void SetDimensionality(int dim);
  svtkGetMacro(Dimensionality, int);
  //@}

  //@{
  /**
   * Private methods kept public for template execute functions.
   */
  void PermuteIncrements(svtkIdType* increments, svtkIdType& inc0, svtkIdType& inc1, svtkIdType& inc2);
  void PermuteExtent(int* extent, int& min0, int& max0, int& min1, int& max1, int& min2, int& max2);
  //@}

protected:
  svtkImageDecomposeFilter();
  ~svtkImageDecomposeFilter() override {}

  int Dimensionality;

private:
  svtkImageDecomposeFilter(const svtkImageDecomposeFilter&) = delete;
  void operator=(const svtkImageDecomposeFilter&) = delete;
};

#endif
