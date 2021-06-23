/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCleanPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCleanPolyData
 * @brief   merge duplicate points, and/or remove unused points and/or remove degenerate cells
 *
 * svtkCleanPolyData is a filter that takes polygonal data as input and
 * generates polygonal data as output. svtkCleanPolyData will merge duplicate
 * points (within specified tolerance and if enabled), eliminate points
 * that are not used in any cell, and if enabled, transform degenerate cells into
 * appropriate forms (for example, a triangle is converted into a line
 * if two points of triangle are merged).
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
 * Cells of type SVTK_POLY_LINE will be converted to a vertex only if
 * ConvertLinesToPoints is on and all points are merged into one. Degenerate line
 * segments (with two identical end points) will be removed.
 *
 * If tolerance is specified precisely=0.0, then svtkCleanPolyData will use
 * the svtkMergePoints object to merge points (which is faster). Otherwise the
 * slower svtkIncrementalPointLocator is used.  Before inserting points into the point
 * locator, this class calls a function OperateOnPoint which can be used (in
 * subclasses) to further refine the cleaning process. See
 * svtkQuantizePolyDataPoints.
 *
 * Note that merging of points can be disabled. In this case, a point locator
 * will not be used, and points that are not used by any cells will be
 * eliminated, but never merged.
 *
 * @warning
 * Merging points can alter topology, including introducing non-manifold
 * forms. The tolerance should be chosen carefully to avoid these problems.
 * Subclasses should handle OperateOnBounds as well as OperateOnPoint
 * to ensure that the locator is correctly initialized (i.e. all modified
 * points must lie inside modified bounds).
 *
 * @warning
 * If you wish to operate on a set of coordinates
 * that has no cells, you must add a svtkPolyVertex cell with all of the points to the PolyData
 * (or use a svtkVertexGlyphFilter) before using the svtkCleanPolyData filter.
 *
 * @sa
 * svtkQuantizePolyDataPoints
 */

#ifndef svtkCleanPolyData_h
#define svtkCleanPolyData_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIncrementalPointLocator;

class SVTKFILTERSCORE_EXPORT svtkCleanPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkCleanPolyData* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkCleanPolyData, svtkPolyDataAlgorithm);

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
   * Specify tolerance in terms of fraction of bounding box length.
   * Default is 0.0.
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

  //@{
  /**
   * Set/Get a boolean value that controls whether point merging is
   * performed. If on, a locator will be used, and points laying within
   * the appropriate tolerance may be merged. If off, points are never
   * merged. By default, merging is on.
   */
  svtkSetMacro(PointMerging, svtkTypeBool);
  svtkGetMacro(PointMerging, svtkTypeBool);
  svtkBooleanMacro(PointMerging, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get a spatial locator for speeding the search process. By
   * default an instance of svtkMergePoints is used.
   */
  virtual void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator(svtkPolyData* input = nullptr);

  /**
   * Release locator
   */
  void ReleaseLocator() { this->SetLocator(nullptr); }

  /**
   * Get the MTime of this object also considering the locator.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Perform operation on a point
   */
  virtual void OperateOnPoint(double in[3], double out[3]);

  /**
   * Perform operation on bounds
   */
  virtual void OperateOnBounds(double in[6], double out[6]);

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

protected:
  svtkCleanPolyData();
  ~svtkCleanPolyData() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool PointMerging;
  double Tolerance;
  double AbsoluteTolerance;
  svtkTypeBool ConvertLinesToPoints;
  svtkTypeBool ConvertPolysToLines;
  svtkTypeBool ConvertStripsToPolys;
  svtkTypeBool ToleranceIsAbsolute;
  svtkIncrementalPointLocator* Locator;

  svtkTypeBool PieceInvariant;
  int OutputPointsPrecision;

private:
  svtkCleanPolyData(const svtkCleanPolyData&) = delete;
  void operator=(const svtkCleanPolyData&) = delete;
};

#endif
