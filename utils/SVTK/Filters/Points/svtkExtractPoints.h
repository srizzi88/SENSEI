/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPoints.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractPoints
 * @brief   extract points within an implicit function
 *
 *
 * svtkExtractPoints removes points that are either inside or outside of a
 * svtkImplicitFunction. Implicit functions in SVTK defined as function of the
 * form f(x,y,z)=c, where values c<=0 are interior values of the implicit
 * function. Typical examples include planes, spheres, cylinders, cones,
 * etc. plus boolean combinations of these functions. (This operation
 * presumes closure on the set, so points on the boundary are also considered
 * to be inside.)
 *
 * Note that while any svtkPointSet type can be provided as input, the output is
 * represented by an explicit representation of points via a
 * svtkPolyData. This output polydata will populate its instance of svtkPoints,
 * but no cells will be defined (i.e., no svtkVertex or svtkPolyVertex are
 * contained in the output). Also, after filter execution, the user can
 * request a svtkIdType* map which indicates how the input points were mapped
 * to the output. A value of map[i] (where i is the ith input point) less
 * than 0 means that the ith input point was removed. (See also the
 * superclass documentation for accessing the removed points through the
 * filter's second output.)
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * The svtkExtractEnclosedPoints filter can be used to extract points inside of
 * a volume defined by a manifold, closed polygonal surface. This filter
 * however is much slower than methods based on implicit functions (like this
 * filter).
 *
 * @sa
 * svtkExtractEnclosedPoints svtkSelectEnclosedPoints svtkPointCloudFilter
 * svtkRadiusOutlierRemoval svtkStatisticalOutlierRemoval svtkThresholdPoints
 * svtkImplicitFunction svtkExtractGeometry svtkFitImplicitFunction
 */

#ifndef svtkExtractPoints_h
#define svtkExtractPoints_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPointCloudFilter.h"

class svtkImplicitFunction;
class svtkPointSet;

class SVTKFILTERSPOINTS_EXPORT svtkExtractPoints : public svtkPointCloudFilter
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static svtkExtractPoints* New();
  svtkTypeMacro(svtkExtractPoints, svtkPointCloudFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the implicit function for inside/outside checks.
   */
  virtual void SetImplicitFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(ImplicitFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Boolean controls whether to extract points that are inside of implicit
   * function (ExtractInside == true) or outside of implicit function
   * (ExtractInside == false). By default, ExtractInside is true.
   */
  svtkSetMacro(ExtractInside, bool);
  svtkGetMacro(ExtractInside, bool);
  svtkBooleanMacro(ExtractInside, bool);
  //@}

  /**
   * Return the MTime taking into account changes to the implicit function
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkExtractPoints();
  ~svtkExtractPoints() override;

  svtkImplicitFunction* ImplicitFunction;
  bool ExtractInside;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(svtkPointSet* input) override;

private:
  svtkExtractPoints(const svtkExtractPoints&) = delete;
  void operator=(const svtkExtractPoints&) = delete;
};

#endif
