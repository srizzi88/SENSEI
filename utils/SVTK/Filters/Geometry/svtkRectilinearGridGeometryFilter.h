/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectilinearGridGeometryFilter
 * @brief   extract geometry for a rectilinear grid
 *
 * svtkRectilinearGridGeometryFilter is a filter that extracts geometry from a
 * rectilinear grid. By specifying appropriate i-j-k indices, it is possible
 * to extract a point, a curve, a surface, or a "volume". The volume
 * is actually a (n x m x o) region of points.
 *
 * The extent specification is zero-offset. That is, the first k-plane in
 * a 50x50x50 rectilinear grid is given by (0,49, 0,49, 0,0).
 *
 * @warning
 * If you don't know the dimensions of the input dataset, you can use a large
 * number to specify extent (the number will be clamped appropriately). For
 * example, if the dataset dimensions are 50x50x50, and you want a the fifth
 * k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will
 * automatically be clamped to 49.
 *
 * @sa
 * svtkGeometryFilter svtkExtractGrid
 */

#ifndef svtkRectilinearGridGeometryFilter_h
#define svtkRectilinearGridGeometryFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGEOMETRY_EXPORT svtkRectilinearGridGeometryFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkRectilinearGridGeometryFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with initial extent (0,100, 0,100, 0,0) (i.e., a k-plane).
   */
  static svtkRectilinearGridGeometryFilter* New();

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
  svtkRectilinearGridGeometryFilter();
  ~svtkRectilinearGridGeometryFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int Extent[6];

private:
  svtkRectilinearGridGeometryFilter(const svtkRectilinearGridGeometryFilter&) = delete;
  void operator=(const svtkRectilinearGridGeometryFilter&) = delete;
};

#endif
