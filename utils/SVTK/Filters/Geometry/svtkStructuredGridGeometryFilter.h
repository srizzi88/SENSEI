/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStructuredGridGeometryFilter
 * @brief   extract geometry for structured grid
 *
 * svtkStructuredGridGeometryFilter is a filter that extracts geometry from a
 * structured grid. By specifying appropriate i-j-k indices, it is possible
 * to extract a point, a curve, a surface, or a "volume". Depending upon the
 * type of data, the curve and surface may be curved or planar. (The volume
 * is actually a (n x m x o) region of points.)
 *
 * The extent specification is zero-offset. That is, the first k-plane in
 * a 50x50x50 structured grid is given by (0,49, 0,49, 0,0).
 *
 * The output of this filter is affected by the structured grid blanking.
 * If blanking is on, and a blanking array defined, then those cells
 * attached to blanked points are not output. (Blanking is a property of
 * the input svtkStructuredGrid.)
 *
 * @warning
 * If you don't know the dimensions of the input dataset, you can use a large
 * number to specify extent (the number will be clamped appropriately). For
 * example, if the dataset dimensions are 50x50x50, and you want a the fifth
 * k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will
 * automatically be clamped to 49.
 *
 * @sa
 * svtkGeometryFilter svtkExtractGrid svtkStructuredGrid
 */

#ifndef svtkStructuredGridGeometryFilter_h
#define svtkStructuredGridGeometryFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGEOMETRY_EXPORT svtkStructuredGridGeometryFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkStructuredGridGeometryFilter* New();
  svtkTypeMacro(svtkStructuredGridGeometryFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the extent in topological coordinate range (imin,imax, jmin,jmax,
   * kmin,kmax).
   */
  svtkGetVectorMacro(Extent, int, 6);
  //@}

  /**
   * Specify (imin,imax, jmin,jmax, kmin,kmax) indices.
   */
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);

  /**
   * Specify (imin,imax, jmin,jmax, kmin,kmax) indices in array form.
   */
  void SetExtent(int extent[6]);

protected:
  svtkStructuredGridGeometryFilter();
  ~svtkStructuredGridGeometryFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int Extent[6];

private:
  svtkStructuredGridGeometryFilter(const svtkStructuredGridGeometryFilter&) = delete;
  void operator=(const svtkStructuredGridGeometryFilter&) = delete;
};

#endif
