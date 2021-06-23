/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractEnclosedPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractEnclosedPoints
 * @brief   extract points inside of a closed polygonal surface
 *
 * svtkExtractEnclosedPoints is a filter that evaluates all the input points
 * to determine whether they are contained within an enclosing surface. Those
 * within the surface are sent to the output. The enclosing surface is
 * specified through a second input to the filter.
 *
 * Note: as a derived class of svtkPointCloudFilter, additional methods are
 * available for generating an in/out mask, and also extracting points
 * outside of the enclosing surface.
 *
 * @warning
 * The filter assumes that the surface is closed and manifold. A boolean flag
 * can be set to force the filter to first check whether this is true. If false,
 * all points will be marked outside. Note that if this check is not performed
 * and the surface is not closed, the results are undefined.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * The filter svtkSelectEnclosedPoints marks points as to in/out of the
 * enclosing surface, and operates on any dataset type, producing an output
 * dataset of the same type as the input. Then, thresholding and masking
 * filters can be used to extract parts of the dataset. This filter
 * (svtkExtractEnclosedPoints) is meant to operate on point clouds represented
 * by svtkPolyData, and produces svtkPolyData on output, so it is more
 * efficient for point processing. Note that this filter delegates many of
 * its methods to svtkSelectEnclosedPoints.
 *
 * @sa
 * svtkSelectEnclosedPoints svtkExtractPoints
 */

#ifndef svtkExtractEnclosedPoints_h
#define svtkExtractEnclosedPoints_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPointCloudFilter.h"

class SVTKFILTERSPOINTS_EXPORT svtkExtractEnclosedPoints : public svtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static svtkExtractEnclosedPoints* New();
  svtkTypeMacro(svtkExtractEnclosedPoints, svtkPointCloudFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set the surface to be used to test for containment. Two methods are
   * provided: one directly for svtkPolyData, and one for the output of a
   * filter.
   */
  void SetSurfaceData(svtkPolyData* pd);
  void SetSurfaceConnection(svtkAlgorithmOutput* algOutput);
  //@}

  //@{
  /**
   * Return a pointer to the enclosing surface.
   */
  svtkPolyData* GetSurface();
  svtkPolyData* GetSurface(svtkInformationVector* sourceInfo);
  //@}

  //@{
  /**
   * Specify whether to check the surface for closure. If on, then the
   * algorithm first checks to see if the surface is closed and manifold.
   */
  svtkSetMacro(CheckSurface, svtkTypeBool);
  svtkBooleanMacro(CheckSurface, svtkTypeBool);
  svtkGetMacro(CheckSurface, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the tolerance on the intersection. The tolerance is expressed as
   * a fraction of the diagonal of the bounding box of the enclosing surface.
   */
  svtkSetClampMacro(Tolerance, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Tolerance, double);
  //@}

protected:
  svtkExtractEnclosedPoints();
  ~svtkExtractEnclosedPoints() override;

  svtkTypeBool CheckSurface;
  double Tolerance;

  // Internal structures for managing the intersection testing
  svtkPolyData* Surface;

  // Satisfy svtkPointCloudFilter superclass API
  int FilterPoints(svtkPointSet* input) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkExtractEnclosedPoints(const svtkExtractEnclosedPoints&) = delete;
  void operator=(const svtkExtractEnclosedPoints&) = delete;
};

#endif
