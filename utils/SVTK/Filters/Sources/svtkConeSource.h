/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkConeSource
 * @brief   generate polygonal cone
 *
 * svtkConeSource creates a cone centered at a specified point and pointing in
 * a specified direction. (By default, the center is the origin and the
 * direction is the x-axis.) Depending upon the resolution of this object,
 * different representations are created. If resolution=0 a line is created;
 * if resolution=1, a single triangle is created; if resolution=2, two
 * crossed triangles are created. For resolution > 2, a 3D cone (with
 * resolution number of sides) is created. It also is possible to control
 * whether the bottom of the cone is capped with a (resolution-sided)
 * polygon, and to specify the height and radius of the cone.
 */

#ifndef svtkConeSource_h
#define svtkConeSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkCell.h" // Needed for SVTK_CELL_SIZE

class SVTKFILTERSSOURCES_EXPORT svtkConeSource : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkConeSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with default resolution 6, height 1.0, radius 0.5, and
   * capping on. The cone is centered at the origin and points down
   * the x-axis.
   */
  static svtkConeSource* New();

  //@{
  /**
   * Set the height of the cone. This is the height along the cone in
   * its specified direction.
   */
  svtkSetClampMacro(Height, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Height, double);
  //@}

  //@{
  /**
   * Set the base radius of the cone.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set the number of facets used to represent the cone.
   */
  svtkSetClampMacro(Resolution, int, 0, SVTK_CELL_SIZE);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Set the center of the cone. It is located at the middle of the axis of
   * the cone. Warning: this is not the center of the base of the cone!
   * The default is 0,0,0.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set the orientation vector of the cone. The vector does not have
   * to be normalized. The direction goes from the center of the base toward
   * the apex. The default is (1,0,0).
   */
  svtkSetVector3Macro(Direction, double);
  svtkGetVectorMacro(Direction, double, 3);
  //@}

  //@{
  /**
   * Set the angle of the cone. This is the angle between the axis of the cone
   * and a generatrix. Warning: this is not the aperture! The aperture is
   * twice this angle.
   * As a side effect, the angle plus height sets the base radius of the cone.
   * Angle is expressed in degrees.
   */
  void SetAngle(double angle);
  double GetAngle();
  //@}

  //@{
  /**
   * Turn on/off whether to cap the base of the cone with a polygon.
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
  svtkConeSource(int res = 6);
  ~svtkConeSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Height;
  double Radius;
  int Resolution;
  svtkTypeBool Capping;
  double Center[3];
  double Direction[3];
  int OutputPointsPrecision;

private:
  svtkConeSource(const svtkConeSource&) = delete;
  void operator=(const svtkConeSource&) = delete;
};

#endif
