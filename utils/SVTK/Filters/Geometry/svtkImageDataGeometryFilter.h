/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDataGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDataGeometryFilter
 * @brief   extract geometry for structured points
 *
 * svtkImageDataGeometryFilter is a filter that extracts geometry from a
 * structured points dataset. By specifying appropriate i-j-k indices (via the
 * "Extent" instance variable), it is possible to extract a point, a line, a
 * plane (i.e., image), or a "volume" from dataset. (Since the output is
 * of type polydata, the volume is actually a (n x m x o) region of points.)
 *
 * The extent specification is zero-offset. That is, the first k-plane in
 * a 50x50x50 volume is given by (0,49, 0,49, 0,0).
 * @warning
 * If you don't know the dimensions of the input dataset, you can use a large
 * number to specify extent (the number will be clamped appropriately). For
 * example, if the dataset dimensions are 50x50x50, and you want a the fifth
 * k-plane, you can use the extents (0,100, 0,100, 4,4). The 100 will
 * automatically be clamped to 49.
 *
 * @sa
 * svtkGeometryFilter svtkStructuredGridSource
 */

#ifndef svtkImageDataGeometryFilter_h
#define svtkImageDataGeometryFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGEOMETRY_EXPORT svtkImageDataGeometryFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkImageDataGeometryFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with initial extent of all the data
   */
  static svtkImageDataGeometryFilter* New();

  //@{
  /**
   * Set / get the extent (imin,imax, jmin,jmax, kmin,kmax) indices.
   */
  void SetExtent(int extent[6]);
  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  int* GetExtent() SVTK_SIZEHINT(6) { return this->Extent; }
  //@}

  //@{
  /**
   * Set ThresholdCells to true if you wish to skip any voxel/pixels which have scalar
   * values less than the specified threshold.
   * Currently this functionality is only implemented for 2D imagedata
   */
  svtkSetMacro(ThresholdCells, svtkTypeBool);
  svtkGetMacro(ThresholdCells, svtkTypeBool);
  svtkBooleanMacro(ThresholdCells, svtkTypeBool);
  //@}

  //@{
  /**
   * Set ThresholdValue to the scalar value by which to threshold cells when extracting geometry
   * when ThresholdCells is true. Cells with scalar values greater than the threshold will be
   * output.
   */
  svtkSetMacro(ThresholdValue, double);
  svtkGetMacro(ThresholdValue, double);
  svtkBooleanMacro(ThresholdValue, double);
  //@}

  //@{
  /**
   * Set OutputTriangles to true if you wish to generate triangles instead of quads
   * when extracting cells from 2D imagedata
   * Currently this functionality is only implemented for 2D imagedata
   */
  svtkSetMacro(OutputTriangles, svtkTypeBool);
  svtkGetMacro(OutputTriangles, svtkTypeBool);
  svtkBooleanMacro(OutputTriangles, svtkTypeBool);
  //@}

protected:
  svtkImageDataGeometryFilter();
  ~svtkImageDataGeometryFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int Extent[6];
  svtkTypeBool ThresholdCells;
  double ThresholdValue;
  svtkTypeBool OutputTriangles;

private:
  svtkImageDataGeometryFilter(const svtkImageDataGeometryFilter&) = delete;
  void operator=(const svtkImageDataGeometryFilter&) = delete;
};

#endif
