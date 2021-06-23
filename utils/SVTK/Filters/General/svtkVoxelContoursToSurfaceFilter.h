/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVoxelContoursToSurfaceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVoxelContoursToSurfaceFilter
 * @brief   create surface from contours
 *
 * svtkVoxelContoursToSurfaceFilter is a filter that takes contours and
 * produces surfaces. There are some restrictions for the contours:
 *
 *   - The contours are input as svtkPolyData, with the contours being
 *     polys in the svtkPolyData.
 *   - The contours lie on XY planes - each contour has a constant Z
 *   - The contours are ordered in the polys of the svtkPolyData such
 *     that all contours on the first (lowest) XY plane are first, then
 *     continuing in order of increasing Z value.
 *   - The X, Y and Z coordinates are all integer values.
 *   - The desired sampling of the contour data is 1x1x1 - Aspect can
 *     be used to control the aspect ratio in the output polygonal
 *     dataset.
 *
 * This filter takes the contours and produces a structured points
 * dataset of signed floating point number indicating distance from
 * a contour. A contouring filter is then applied to generate 3D
 * surfaces from a stack of 2D contour distance slices. This is
 * done in a streaming fashion so as not to use to much memory.
 *
 * @sa
 * svtkPolyDataAlgorithm
 */

#ifndef svtkVoxelContoursToSurfaceFilter_h
#define svtkVoxelContoursToSurfaceFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkVoxelContoursToSurfaceFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkVoxelContoursToSurfaceFilter* New();
  svtkTypeMacro(svtkVoxelContoursToSurfaceFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set / Get the memory limit in bytes for this filter. This is the limit
   * of the size of the structured points data set that is created for
   * intermediate processing. The data will be streamed through this volume
   * in as many pieces as necessary.
   */
  svtkSetMacro(MemoryLimitInBytes, int);
  svtkGetMacro(MemoryLimitInBytes, int);
  //@}

  svtkSetVector3Macro(Spacing, double);
  svtkGetVectorMacro(Spacing, double, 3);

protected:
  svtkVoxelContoursToSurfaceFilter();
  ~svtkVoxelContoursToSurfaceFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int MemoryLimitInBytes;

  double Spacing[3];

  double* LineList;
  int LineListLength;
  int LineListSize;

  double* SortedXList;
  double* SortedYList;
  int SortedListSize;

  int* WorkingList;
  int WorkingListLength;

  double* IntersectionList;
  int IntersectionListLength;

  void AddLineToLineList(double x1, double y1, double x2, double y2);
  void SortLineList();

  void CastLines(float* slice, double gridOrigin[3], int gridSize[3], int type);

  void PushDistances(float* ptr, int gridSize[3], int chunkSize);

private:
  svtkVoxelContoursToSurfaceFilter(const svtkVoxelContoursToSurfaceFilter&) = delete;
  void operator=(const svtkVoxelContoursToSurfaceFilter&) = delete;
};

#endif
