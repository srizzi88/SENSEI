/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStaticCleanPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStaticCleanPolyData
 * @brief   merge duplicate points, and/or remove unused points and/or remove degenerate cells
 *
 * svtkStaticCleanPolyData is a filter that takes polygonal data as input and
 * generates polygonal data as output. svtkStaticCleanPolyData will merge
 * duplicate points (within specified tolerance), and if enabled, transform
 * degenerate cells into appropriate forms (for example, a triangle is
 * converted into a line if two points of triangle are merged).
 *
 * Conversion of degenerate cells is controlled by the flags
 * ConvertLinesToPoints, ConvertPolysToLines, ConvertStripsToPolys which act
 * cumulatively such that a degenerate strip may become a poly.
 * The full set is
 * Line with 1 points -> Vert (if ConvertLinesToPoints)
 * Poly with 2 points -> Line (if ConvertPolysToLines)
 * Poly with 1 points -> Vert (if ConvertPolysToLines && ConvertLinesToPoints)
 * Strp with 3 points -> Poly (if ConvertStripsToPolys)
 * Strp with 2 points -> Line (if ConvertStripsToPolys && ConvertPolysToLines)
 * Strp with 1 points -> Vert (if ConvertStripsToPolys && ConvertPolysToLines
 *   && ConvertLinesToPoints)
 *
 * Internally this class uses svtkStaticPointLocator, which is a threaded, and
 * much faster locator than the incremental locators that svtkCleanPolyData
 * uses. Note because of these and other differences, the output of this
 * filter may be different than svtkCleanPolyData.
 *
 * Note that if you want to remove points that aren't used by any cells
 * (i.e., disable point merging), then use svtkCleanPolyData.
 *
 * @warning
 * Merging points can alter topology, including introducing non-manifold
 * forms. The tolerance should be chosen carefully to avoid these problems.
 * Large tolerances (of size > locator bin width) may generate poor results.
 *
 * @warning
 * Merging close points with tolerance >0.0 is inherently an unstable problem
 * because the results are order dependent (e.g., the order in which points
 * are processed). When parallel computing, the order of processing points is
 * unpredictable, hence the results may vary between runs.
 *
 * @warning
 * If you wish to operate on a set of coordinates that has no cells, you must
 * add a svtkPolyVertex cell with all of the points to the PolyData (or use a
 * svtkVertexGlyphFilter) before using the svtkStaticCleanPolyData filter.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkCleanPolyData
 */

#ifndef svtkStaticCleanPolyData_h
#define svtkStaticCleanPolyData_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkStaticPointLocator;

class SVTKFILTERSCORE_EXPORT svtkStaticCleanPolyData : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods to instantiate, print, and provide type information.
   */
  static svtkStaticCleanPolyData* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkStaticCleanPolyData, svtkPolyDataAlgorithm);
  //@}

  //@{
  /**
   * By default ToleranceIsAbsolute is false and Tolerance is
   * a fraction of Bounding box diagonal, if true, AbsoluteTolerance is
   * used when adding points to locator (merging)
   */
  svtkSetMacro(ToleranceIsAbsolute, svtkTypeBool);
  svtkBooleanMacro(ToleranceIsAbsolute, svtkTypeBool);
  svtkGetMacro(ToleranceIsAbsolute, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify tolerance in terms of fraction of bounding box length.  Default
   * is 0.0. This takes effect only if ToleranceIsAbsolute is false.
   */
  svtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Specify tolerance in absolute terms. Default is 1.0.
   */
  svtkSetClampMacro(AbsoluteTolerance, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(AbsoluteTolerance, double);
  //@}

  //@{
  /**
   * Turn on/off conversion of degenerate lines to points. Default is On.
   */
  svtkSetMacro(ConvertLinesToPoints, svtkTypeBool);
  svtkBooleanMacro(ConvertLinesToPoints, svtkTypeBool);
  svtkGetMacro(ConvertLinesToPoints, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off conversion of degenerate polys to lines. Default is On.
   */
  svtkSetMacro(ConvertPolysToLines, svtkTypeBool);
  svtkBooleanMacro(ConvertPolysToLines, svtkTypeBool);
  svtkGetMacro(ConvertPolysToLines, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off conversion of degenerate strips to polys. Default is On.
   */
  svtkSetMacro(ConvertStripsToPolys, svtkTypeBool);
  svtkBooleanMacro(ConvertStripsToPolys, svtkTypeBool);
  svtkGetMacro(ConvertStripsToPolys, svtkTypeBool);
  //@}

  // This filter is difficult to stream.
  // To get invariant results, the whole input must be processed at once.
  // This flag allows the user to select whether strict piece invariance
  // is required.  By default it is on.  When off, the filter can stream,
  // but results may change.
  svtkSetMacro(PieceInvariant, svtkTypeBool);
  svtkGetMacro(PieceInvariant, svtkTypeBool);
  svtkBooleanMacro(PieceInvariant, svtkTypeBool);

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

  //@{
  /**
   * Retrieve the internal locator to manually configure it, for example
   * specifying the number of points per bucket. This method is generally
   * used for debugging or testing purposes.
   */
  svtkStaticPointLocator* GetLocator() { return this->Locator; }
  //@}

  /**
   * Get the MTime of this object also considering the locator.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkStaticCleanPolyData();
  ~svtkStaticCleanPolyData() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Tolerance;
  double AbsoluteTolerance;
  svtkTypeBool ConvertLinesToPoints;
  svtkTypeBool ConvertPolysToLines;
  svtkTypeBool ConvertStripsToPolys;
  svtkTypeBool ToleranceIsAbsolute;
  svtkStaticPointLocator* Locator;

  svtkTypeBool PieceInvariant;
  int OutputPointsPrecision;

private:
  svtkStaticCleanPolyData(const svtkStaticCleanPolyData&) = delete;
  void operator=(const svtkStaticCleanPolyData&) = delete;
};

#endif
