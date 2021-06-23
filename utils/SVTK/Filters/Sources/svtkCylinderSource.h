/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCylinderSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCylinderSource
 * @brief   generate a cylinder centered at origin
 *
 * svtkCylinderSource creates a polygonal cylinder centered at Center;
 * The axis of the cylinder is aligned along the global y-axis.
 * The height and radius of the cylinder can be specified, as well as the
 * number of sides. It is also possible to control whether the cylinder is
 * open-ended or capped. If you have the end points of the cylinder, you
 * should use a svtkLineSource followed by a svtkTubeFilter instead of the
 * svtkCylinderSource.
 */

#ifndef svtkCylinderSource_h
#define svtkCylinderSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkCell.h" // Needed for SVTK_CELL_SIZE

class SVTKFILTERSSOURCES_EXPORT svtkCylinderSource : public svtkPolyDataAlgorithm
{
public:
  static svtkCylinderSource* New();
  svtkTypeMacro(svtkCylinderSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the height of the cylinder. Initial value is 1.
   */
  svtkSetClampMacro(Height, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Height, double);
  //@}

  //@{
  /**
   * Set the radius of the cylinder. Initial value is 0.5
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set/Get cylinder center. Initial value is (0.0,0.0,0.0)
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set the number of facets used to define cylinder. Initial value is 6.
   */
  svtkSetClampMacro(Resolution, int, 2, SVTK_CELL_SIZE);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Turn on/off whether to cap cylinder with polygons. Initial value is true.
   */
  svtkSetMacro(Capping, svtkTypeBool);
  svtkGetMacro(Capping, svtkTypeBool);
  svtkBooleanMacro(Capping, svtkTypeBool);
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
  svtkCylinderSource(int res = 6);
  ~svtkCylinderSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double Height;
  double Radius;
  double Center[3];
  int Resolution;
  svtkTypeBool Capping;
  int OutputPointsPrecision;

private:
  svtkCylinderSource(const svtkCylinderSource&) = delete;
  void operator=(const svtkCylinderSource&) = delete;
};

#endif
