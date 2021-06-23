/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRecursiveDividingCubes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRecursiveDividingCubes
 * @brief   create points laying on isosurface (using recursive approach)
 *
 * svtkRecursiveDividingCubes is a filter that generates points laying on a
 * surface of constant scalar value (i.e., an isosurface). Dense point
 * clouds (i.e., at screen resolution) will appear as a surface. Less dense
 * clouds can be used as a source to generate streamlines or to generate
 * "transparent" surfaces.
 *
 * This implementation differs from svtkDividingCubes in that it uses a
 * recursive procedure. In many cases this can result in generating
 * more points than the procedural implementation of svtkDividingCubes. This is
 * because the recursive procedure divides voxels by multiples of powers of
 * two. This can over-constrain subdivision. One of the advantages of the
 * recursive technique is that the recursion is terminated earlier, which in
 * some cases can be more efficient.
 *
 * @sa
 * svtkDividingCubes svtkContourFilter svtkMarchingCubes
 */

#ifndef svtkRecursiveDividingCubes_h
#define svtkRecursiveDividingCubes_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkVoxel;

class SVTKFILTERSGENERAL_EXPORT svtkRecursiveDividingCubes : public svtkPolyDataAlgorithm
{
public:
  static svtkRecursiveDividingCubes* New();
  svtkTypeMacro(svtkRecursiveDividingCubes, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set isosurface value.
   */
  svtkSetMacro(Value, double);
  svtkGetMacro(Value, double);
  //@}

  //@{
  /**
   * Specify sub-voxel size at which to generate point.
   */
  svtkSetClampMacro(Distance, double, 1.0e-06, SVTK_DOUBLE_MAX);
  svtkGetMacro(Distance, double);
  //@}

  //@{
  /**
   * Every "Increment" point is added to the list of points. This parameter, if
   * set to a large value, can be used to limit the number of points while
   * retaining good accuracy.
   */
  svtkSetClampMacro(Increment, int, 1, SVTK_INT_MAX);
  svtkGetMacro(Increment, int);
  //@}

protected:
  svtkRecursiveDividingCubes();
  ~svtkRecursiveDividingCubes() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  void SubDivide(double origin[3], double h[3], double values[8]);

  double Value;
  double Distance;
  int Increment;

  // working variable
  int Count;

  // to replace a static
  svtkVoxel* Voxel;

private:
  svtkRecursiveDividingCubes(const svtkRecursiveDividingCubes&) = delete;
  void operator=(const svtkRecursiveDividingCubes&) = delete;
};

#endif
