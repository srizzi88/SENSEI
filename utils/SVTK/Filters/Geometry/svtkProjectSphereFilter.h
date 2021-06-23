/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProjectSphereFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProjectSphereFilter
 * @brief   A filter to 'unroll' a sphere.  The
 * unroll longitude is -180.
 *
 *
 */

#ifndef svtkProjectSphereFilter_h
#define svtkProjectSphereFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class svtkCell;
class svtkCellArray;
class svtkDataSetAttributes;
class svtkIdList;
class svtkIncrementalPointLocator;
class svtkUnstructuredGrid;

class SVTKFILTERSGEOMETRY_EXPORT svtkProjectSphereFilter : public svtkPointSetAlgorithm
{
public:
  svtkTypeMacro(svtkProjectSphereFilter, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkProjectSphereFilter* New();

  //@{
  /**
   * Set the center of the sphere to be split. Default is 0,0,0.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Specify whether or not to keep the cells using a point at
   * a pole. The default is false.
   */
  svtkGetMacro(KeepPolePoints, bool);
  svtkSetMacro(KeepPolePoints, bool);
  svtkBooleanMacro(KeepPolePoints, bool);
  //@}

  //@{
  /**
   * Specify whether (true) or not to translate the points in the projected
   * transformation such that the input point with the smallest
   * radius is at 0. The default is false.
   */
  svtkGetMacro(TranslateZ, bool);
  svtkSetMacro(TranslateZ, bool);
  svtkBooleanMacro(TranslateZ, bool);
  //@}

protected:
  svtkProjectSphereFilter();
  ~svtkProjectSphereFilter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void TransformPointInformation(svtkPointSet* input, svtkPointSet* output, svtkIdList*);
  void TransformCellInformation(svtkPointSet* input, svtkPointSet* output, svtkIdList*);
  void TransformTensors(svtkIdType id, double* coord, svtkDataSetAttributes* arrays);

  /**
   * Parallel part of the algorithm to figure out the closest point
   * to the centerline (i.e. line connecting -90 latitude to 90 latitude)
   * if we don't build cells using points at the poles.
   */
  virtual void ComputePointsClosestToCenterLine(double, svtkIdList*) {}

  /**
   * If TranslateZ is true then this is the method that computes
   * the amount to translate.
   */
  virtual double GetZTranslation(svtkPointSet* input);

  /**
   * Split a cell into multiple cells because it stretches across the
   * SplitLongitude. splitSide is 1 for left side and 0 for sight side.
   */
  void SplitCell(svtkPointSet* input, svtkPointSet* output, svtkIdType inputCellId,
    svtkIncrementalPointLocator* locator, svtkCellArray* connectivity, int splitSide);

  void SetCellInformation(svtkUnstructuredGrid* output, svtkCell* cell, svtkIdType numberOfNewCells);

private:
  svtkProjectSphereFilter(const svtkProjectSphereFilter&) = delete;
  void operator=(const svtkProjectSphereFilter&) = delete;

  double Center[3];
  const double SplitLongitude;
  bool KeepPolePoints;
  bool TranslateZ;
};

#endif // svtkProjectSphereFilter_h
