/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCubeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCubeSource
 * @brief   create a polygonal representation of a cube
 *
 * svtkCubeSource creates a cube centered at origin. The cube is represented
 * with four-sided polygons. It is possible to specify the length, width,
 * and height of the cube independently.
 */

#ifndef svtkCubeSource_h
#define svtkCubeSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSSOURCES_EXPORT svtkCubeSource : public svtkPolyDataAlgorithm
{
public:
  static svtkCubeSource* New();
  svtkTypeMacro(svtkCubeSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the length of the cube in the x-direction.
   */
  svtkSetClampMacro(XLength, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(XLength, double);
  //@}

  //@{
  /**
   * Set the length of the cube in the y-direction.
   */
  svtkSetClampMacro(YLength, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(YLength, double);
  //@}

  //@{
  /**
   * Set the length of the cube in the z-direction.
   */
  svtkSetClampMacro(ZLength, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(ZLength, double);
  //@}

  //@{
  /**
   * Set the center of the cube.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Convenience methods allows creation of cube by specifying bounding box.
   */
  void SetBounds(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
  void SetBounds(const double bounds[6]);
  void GetBounds(double bounds[6]);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkCubeSource(double xL = 1.0, double yL = 1.0, double zL = 1.0);
  ~svtkCubeSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double XLength;
  double YLength;
  double ZLength;
  double Center[3];
  int OutputPointsPrecision;

private:
  svtkCubeSource(const svtkCubeSource&) = delete;
  void operator=(const svtkCubeSource&) = delete;
};

#endif
