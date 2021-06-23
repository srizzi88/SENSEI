/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelectEnclosedPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSelectEnclosedPoints
 * @brief   mark points as to whether they are inside a closed surface
 *
 * svtkSelectEnclosedPoints is a filter that evaluates all the input points to
 * determine whether they are in an enclosed surface. The filter produces a
 * (0,1) mask (in the form of a svtkDataArray) that indicates whether points
 * are outside (mask value=0) or inside (mask value=1) a provided surface.
 * (The name of the output svtkDataArray is "SelectedPoints".)
 *
 * After running the filter, it is possible to query it as to whether a point
 * is inside/outside by invoking the IsInside(ptId) method.
 *
 * @warning
 * The filter assumes that the surface is closed and manifold. A boolean flag
 * can be set to force the filter to first check whether this is true. If false,
 * all points will be marked outside. Note that if this check is not performed
 * and the surface is not closed, the results are undefined.
 *
 * @warning
 * This filter produces and output data array, but does not modify the input
 * dataset. If you wish to extract cells or points, various threshold filters
 * are available (i.e., threshold the output array). Also, see the filter
 * svtkExtractEnclosedPoints which operates on point clouds.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkMaskPoints svtkExtractEnclosedPoints
 */

#ifndef svtkSelectEnclosedPoints_h
#define svtkSelectEnclosedPoints_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkIntersectionCounter.h"   // to count intersections along ray

class svtkUnsignedCharArray;
class svtkAbstractCellLocator;
class svtkStaticCellLocator;
class svtkIdList;
class svtkGenericCell;
class svtkRandomPool;

class SVTKFILTERSMODELING_EXPORT svtkSelectEnclosedPoints : public svtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  svtkTypeMacro(svtkSelectEnclosedPoints, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Instantiate this class.
   */
  static svtkSelectEnclosedPoints* New();

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
   * By default, points inside the surface are marked inside or sent to
   * the output. If InsideOut is on, then the points outside the surface
   * are marked inside.
   */
  svtkSetMacro(InsideOut, svtkTypeBool);
  svtkBooleanMacro(InsideOut, svtkTypeBool);
  svtkGetMacro(InsideOut, svtkTypeBool);
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

  /**
   * Query an input point id as to whether it is inside or outside. Note that
   * the result requires that the filter execute first.
   */
  int IsInside(svtkIdType inputPtId);

  //@{
  /**
   * Specify the tolerance on the intersection. The tolerance is expressed as
   * a fraction of the diagonal of the bounding box of the enclosing surface.
   */
  svtkSetClampMacro(Tolerance, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * This is a backdoor that can be used to test many points for containment.
   * First initialize the instance, then repeated calls to IsInsideSurface()
   * can be used without rebuilding the search structures. The Complete()
   * method releases memory.
   */
  void Initialize(svtkPolyData* surface);
  int IsInsideSurface(double x[3]);
  int IsInsideSurface(double x, double y, double z);
  void Complete();
  //@}

  /**
   * A static method for determining whether a point is inside a
   * surface. This is the heart of the algorithm and is thread safe. The user
   * must provide an input point x, the enclosing surface, the bounds of the
   * enclosing surface, the diagonal length of the enclosing surface, an
   * intersection tolerance, a cell locator for the surface, and two working
   * objects (cellIds, genCell) to support computation. Finally, in threaded
   * execution, generating random numbers is hard, so a precomputed random
   * sequence can be provided with an index into the sequence.
   */
  static int IsInsideSurface(double x[3], svtkPolyData* surface, double bds[6], double length,
    double tol, svtkAbstractCellLocator* locator, svtkIdList* cellIds, svtkGenericCell* genCell,
    svtkIntersectionCounter& counter, svtkRandomPool* poole = nullptr, svtkIdType seqIdx = 0);

  /**
   * A static method for determining whether a surface is closed. Provide as input
   * a svtkPolyData. The method returns >0 is the surface is closed and manifold.
   */
  static int IsSurfaceClosed(svtkPolyData* surface);

protected:
  svtkSelectEnclosedPoints();
  ~svtkSelectEnclosedPoints() override;

  svtkTypeBool CheckSurface;
  svtkTypeBool InsideOut;
  double Tolerance;

  svtkUnsignedCharArray* InsideOutsideArray;

  // Internal structures for accelerating the intersection test
  svtkStaticCellLocator* CellLocator;
  svtkIdList* CellIds;
  svtkGenericCell* Cell;
  svtkPolyData* Surface;
  double Bounds[6];
  double Length;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  void ReportReferences(svtkGarbageCollector*) override;

private:
  svtkSelectEnclosedPoints(const svtkSelectEnclosedPoints&) = delete;
  void operator=(const svtkSelectEnclosedPoints&) = delete;
};

#endif
