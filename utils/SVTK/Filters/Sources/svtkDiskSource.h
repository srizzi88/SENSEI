/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDiskSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDiskSource
 * @brief   create a disk with hole in center
 *
 * svtkDiskSource creates a polygonal disk with a hole in the center. The
 * disk has zero height. The user can specify the inner and outer radius
 * of the disk, and the radial and circumferential resolution of the
 * polygonal representation.
 * @sa
 * svtkLinearExtrusionFilter
 */

#ifndef svtkDiskSource_h
#define svtkDiskSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSSOURCES_EXPORT svtkDiskSource : public svtkPolyDataAlgorithm
{
public:
  static svtkDiskSource* New();
  svtkTypeMacro(svtkDiskSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify inner radius of hole in disc.
   */
  svtkSetClampMacro(InnerRadius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(InnerRadius, double);
  //@}

  //@{
  /**
   * Specify outer radius of disc.
   */
  svtkSetClampMacro(OuterRadius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(OuterRadius, double);
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
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkDiskSource();
  ~svtkDiskSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double InnerRadius;
  double OuterRadius;
  int RadialResolution;
  int CircumferentialResolution;
  int OutputPointsPrecision;

private:
  svtkDiskSource(const svtkDiskSource&) = delete;
  void operator=(const svtkDiskSource&) = delete;
};

#endif
