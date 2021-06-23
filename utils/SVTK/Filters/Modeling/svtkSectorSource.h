/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkSectorSource.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSectorSource
 * @brief   create a sector of a disk
 *
 * svtkSectorSource creates a sector of a polygonal disk. The
 * disk has zero height. The user can specify the inner and outer radius
 * of the disk, the z-coordinate, and the radial and
 * circumferential resolution of the polygonal representation.
 * @sa
 * svtkLinearExtrusionFilter
 */

#ifndef svtkSectorSource_h
#define svtkSectorSource_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSMODELING_EXPORT svtkSectorSource : public svtkPolyDataAlgorithm
{
public:
  static svtkSectorSource* New();
  svtkTypeMacro(svtkSectorSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify inner radius of the sector.
   */
  svtkSetClampMacro(InnerRadius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(InnerRadius, double);
  //@}

  //@{
  /**
   * Specify outer radius of the sector.
   */
  svtkSetClampMacro(OuterRadius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(OuterRadius, double);
  //@}

  //@{
  /**
   * Specify the z coordinate of the sector.
   */
  svtkSetClampMacro(ZCoord, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(ZCoord, double);
  //@}

  //@{
  /**
   * Set the number of points in radius direction.
   */
  svtkSetClampMacro(RadialResolution, int, 1, SVTK_INT_MAX);
  svtkGetMacro(RadialResolution, int);
  //@}

  //@{
  /**
   * Set the number of points in circumferential direction.
   */
  svtkSetClampMacro(CircumferentialResolution, int, 3, SVTK_INT_MAX);
  svtkGetMacro(CircumferentialResolution, int);
  //@}

  //@{
  /**
   * Set the start angle of the sector.
   */
  svtkSetClampMacro(StartAngle, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(StartAngle, double);
  //@}

  //@{
  /**
   * Set the end angle of the sector.
   */
  svtkSetClampMacro(EndAngle, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(EndAngle, double);
  //@}

protected:
  svtkSectorSource();
  ~svtkSectorSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double InnerRadius;
  double OuterRadius;
  double ZCoord;
  int RadialResolution;
  int CircumferentialResolution;
  double StartAngle;
  double EndAngle;

private:
  svtkSectorSource(const svtkSectorSource&) = delete;
  void operator=(const svtkSectorSource&) = delete;
};

#endif
