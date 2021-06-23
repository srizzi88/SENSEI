/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPolyDataGeometry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractPolyDataGeometry
 * @brief   extract svtkPolyData cells that lies either entirely inside or outside of a specified
 * implicit function
 *
 *
 * svtkExtractPolyDataGeometry extracts from its input svtkPolyData all cells
 * that are either completely inside or outside of a specified implicit
 * function. This filter is specialized to svtkPolyData. On output the
 * filter generates svtkPolyData.
 *
 * To use this filter you must specify an implicit function. You must also
 * specify whether to extract cells laying inside or outside of the implicit
 * function. (The inside of an implicit function is the negative values
 * region.) An option exists to extract cells that are neither inside nor
 * outside (i.e., boundary).
 *
 * Note that this filter also has the option to directly pass all points or cull
 * the points that do not satisfy the implicit function test. Passing all points
 * is a tad faster, but then points remain that do not pass the test and may mess
 * up subsequent glyphing operations and so on. By default points are culled.
 *
 * A more general version of this filter is available for arbitrary
 * svtkDataSet input (see svtkExtractGeometry).
 *
 * @sa
 * svtkExtractGeometry svtkClipPolyData
 */

#ifndef svtkExtractPolyDataGeometry_h
#define svtkExtractPolyDataGeometry_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkImplicitFunction;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractPolyDataGeometry : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkExtractPolyDataGeometry, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with ExtractInside turned on.
   */
  static svtkExtractPolyDataGeometry* New();

  /**
   * Return the MTime taking into account changes to the implicit function
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the implicit function for inside/outside checks.
   */
  virtual void SetImplicitFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(ImplicitFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Boolean controls whether to extract cells that are inside of implicit
   * function (ExtractInside == 1) or outside of implicit function
   * (ExtractInside == 0).
   */
  svtkSetMacro(ExtractInside, svtkTypeBool);
  svtkGetMacro(ExtractInside, svtkTypeBool);
  svtkBooleanMacro(ExtractInside, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean controls whether to extract cells that are partially inside.
   * By default, ExtractBoundaryCells is off.
   */
  svtkSetMacro(ExtractBoundaryCells, svtkTypeBool);
  svtkGetMacro(ExtractBoundaryCells, svtkTypeBool);
  svtkBooleanMacro(ExtractBoundaryCells, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean controls whether points are culled or simply passed through
   * to the output.
   */
  svtkSetMacro(PassPoints, svtkTypeBool);
  svtkGetMacro(PassPoints, svtkTypeBool);
  svtkBooleanMacro(PassPoints, svtkTypeBool);
  //@}

protected:
  svtkExtractPolyDataGeometry(svtkImplicitFunction* f = nullptr);
  ~svtkExtractPolyDataGeometry() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkImplicitFunction* ImplicitFunction;
  svtkTypeBool ExtractInside;
  svtkTypeBool ExtractBoundaryCells;
  svtkTypeBool PassPoints;

  svtkIdType InsertPointInMap(svtkIdType i, svtkPoints* inPts, svtkPoints* newPts, svtkIdType* pointMap);

private:
  svtkExtractPolyDataGeometry(const svtkExtractPolyDataGeometry&) = delete;
  void operator=(const svtkExtractPolyDataGeometry&) = delete;
};

//@{
/**
 * When not passing points, have to use a point map to keep track of things.
 */
inline svtkIdType svtkExtractPolyDataGeometry::InsertPointInMap(
  svtkIdType i, svtkPoints* inPts, svtkPoints* newPts, svtkIdType* pointMap)
{
  double x[3];
  inPts->GetPoint(i, x);
  pointMap[i] = newPts->InsertNextPoint(x);
  return pointMap[i];
}
//@}

#endif
