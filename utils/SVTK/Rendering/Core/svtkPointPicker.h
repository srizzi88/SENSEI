/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointPicker
 * @brief   select a point by shooting a ray into a graphics window
 *
 *
 * svtkPointPicker is used to select a point by shooting a ray into a graphics
 * window and intersecting with actor's defining geometry - specifically its
 * points. Beside returning coordinates, actor, and mapper, svtkPointPicker
 * returns the id of the point projecting closest onto the ray (within the
 * specified tolerance).  Ties are broken (i.e., multiple points all
 * projecting within the tolerance along the pick ray) by choosing the point
 * closest to the ray.
 *
 *
 * @sa
 * svtkPicker svtkCellPicker.
 */

#ifndef svtkPointPicker_h
#define svtkPointPicker_h

#include "svtkPicker.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkDataSet;

class SVTKRENDERINGCORE_EXPORT svtkPointPicker : public svtkPicker
{
public:
  static svtkPointPicker* New();
  svtkTypeMacro(svtkPointPicker, svtkPicker);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the id of the picked point. If PointId = -1, nothing was picked.
   */
  svtkGetMacro(PointId, svtkIdType);
  //@}

  //@{
  /**
   * Specify whether the point search should be based on cell points or
   * directly on the point list.
   */
  svtkSetMacro(UseCells, svtkTypeBool);
  svtkGetMacro(UseCells, svtkTypeBool);
  svtkBooleanMacro(UseCells, svtkTypeBool);
  //@}

protected:
  svtkPointPicker();
  ~svtkPointPicker() override {}

  svtkIdType PointId;    // picked point
  svtkTypeBool UseCells; // Use cell points vs. points directly

  double IntersectWithLine(const double p1[3], const double p2[3], double tol,
    svtkAssemblyPath* path, svtkProp3D* p, svtkAbstractMapper3D* m) override;
  void Initialize() override;

  svtkIdType IntersectDataSetWithLine(const double p1[3], double ray[3], double rayFactor,
    double tol, svtkDataSet* dataSet, double& tMin, double minXYZ[3]);
  bool UpdateClosestPoint(double x[3], const double p1[3], double ray[3], double rayFactor,
    double tol, double& tMin, double& distMin);

private:
  svtkPointPicker(const svtkPointPicker&) = delete;
  void operator=(const svtkPointPicker&) = delete;
};

#endif
