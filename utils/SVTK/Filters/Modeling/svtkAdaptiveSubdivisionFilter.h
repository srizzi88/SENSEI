/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAdaptiveSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAdaptiveSubdivisionFilter
 * @brief   subdivide triangles based on edge and/or area metrics
 *
 *
 * svtkAdaptiveSubdivisionFilter is a filter that subdivides triangles based
 * on maximum edge length and/or triangle area. It uses a simple case-based,
 * multi-pass approach to repeatedly subdivide the input triangle mesh to
 * meet the area and/or edge length criteria. New points may be inserted only
 * on edges; depending on the number of edges to be subdivided a different
 * number of triangles are inserted ranging from two (i.e., two triangles
 * replace the original one) to four.
 *
 * Triangle subdivision is controlled by specifying a maximum edge length
 * and/or triangle area that any given triangle may have. Subdivision
 * proceeds until their criteria are satisfied. Note that using excessively
 * small criteria values can produce enormous meshes with the possibility of
 * exhausting system memory. Also, if you want to ignore a particular
 * criterion value (e.g., triangle area) then simply set the criterion value
 * to a very large value (e.g., SVTK_DOUBLE_MAX).
 *
 * An incremental point locator is used because as new points are created, a
 * search is made to ensure that a point has not already been created. This
 * ensures that the mesh remains compatible (watertight) as long as certain
 * criteria are not used (triangle area limit, and number of triangles limit).
 *
 * To prevent overly large triangle meshes from being created, it is possible
 * to set a limit on the number of triangles created. By default this number
 * is a very large number (i.e., no limit). Further, a limit on the number of
 * passes can also be set, this is mostly useful to generated animations of
 * the algorithm.
 *
 * Finally, the attribute data (point and cell data) is treated as follows.
 * The cell data from a parent triangle is assigned to its subdivided
 * children.  Point data is interpolated along edges as the edges are
 * subdivided.
 *
 * @warning
 * The subdivision is linear along edges. Thus do not expect smoothing or
 * blending effects to occur. If you need to smooth the resulting mesh use an
 * algorithm like svtkWindowedSincPolyDataFilter or svtkSmoothPolyDataFilter.
 *
 * The filter retains mesh compatibility (watertightness) if the mesh was
 * originally compatible; and the area, max triangles criteria are not used.
 *
 * @warning
 * The filter requires a triangle mesh. Use svtkTriangleFilter to tessellate
 * the mesh if necessary.
 *
 * @sa
 * svtkInterpolatingSubdivisionFilter svtkLinearSubdivisionFilter
 * svtkButterflySubdivisionFilter svtkTriangleFilter
 */

#ifndef svtkAdaptiveSubdivisionFilter_h
#define svtkAdaptiveSubdivisionFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIncrementalPointLocator;

class SVTKFILTERSMODELING_EXPORT svtkAdaptiveSubdivisionFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, type info, and printing.
   */
  static svtkAdaptiveSubdivisionFilter* New();
  svtkTypeMacro(svtkAdaptiveSubdivisionFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the maximum edge length that a triangle may have. Edges longer
   * than this value are split in half and the associated triangles are
   * modified accordingly.
   */
  svtkSetClampMacro(MaximumEdgeLength, double, 0.000001, SVTK_DOUBLE_MAX);
  svtkGetMacro(MaximumEdgeLength, double);
  //@}

  //@{
  /**
   * Specify the maximum area that a triangle may have. Triangles larger
   * than this value are subdivided to meet this threshold. Note that if
   * this criterion is used it may produce non-watertight meshes as a
   * result.
   */
  svtkSetClampMacro(MaximumTriangleArea, double, 0.000001, SVTK_DOUBLE_MAX);
  svtkGetMacro(MaximumTriangleArea, double);
  //@}

  //@{
  /**
   * Set a limit on the maximum number of triangles that can be created.  If
   * the limit is hit, it may result in premature termination of the
   * algorithm and the results may be less than satisfactory (for example
   * non-watertight meshes may be created). By default, the limit is set to a
   * very large number (i.e., no effective limit).
   */
  svtkSetClampMacro(MaximumNumberOfTriangles, svtkIdType, 1, SVTK_ID_MAX);
  svtkGetMacro(MaximumNumberOfTriangles, svtkIdType);
  //@}

  //@{
  /**
   * Set a limit on the number of passes (i.e., levels of subdivision).  If
   * the limit is hit, then the subdivision process stops and additional
   * passes (needed to meet other criteria) are aborted. The default limit is
   * set to a very large number (i.e., no effective limit).
   */
  svtkSetClampMacro(MaximumNumberOfPasses, svtkIdType, 1, SVTK_ID_MAX);
  svtkGetMacro(MaximumNumberOfPasses, svtkIdType);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of svtkMergePoints is used. This is used to merge
   * coincident points during subdivision.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

  /**
   * Create a default locator. Used to create one when none is
   * specified.
   */
  void CreateDefaultLocator();

  /**
   * Modified GetMTime because of the dependence on the locator.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkAdaptiveSubdivisionFilter();
  ~svtkAdaptiveSubdivisionFilter() override;

  double MaximumEdgeLength;
  double MaximumTriangleArea;
  svtkIdType MaximumNumberOfTriangles;
  svtkIdType MaximumNumberOfPasses;
  svtkIncrementalPointLocator* Locator;
  int OutputPointsPrecision;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkAdaptiveSubdivisionFilter(const svtkAdaptiveSubdivisionFilter&) = delete;
  void operator=(const svtkAdaptiveSubdivisionFilter&) = delete;
};

#endif
