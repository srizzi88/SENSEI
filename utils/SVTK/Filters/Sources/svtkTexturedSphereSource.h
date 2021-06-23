/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexturedSphereSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTexturedSphereSource
 * @brief   create a sphere centered at the origin
 *
 * svtkTexturedSphereSource creates a polygonal sphere of specified radius
 * centered at the origin. The resolution (polygonal discretization) in both
 * the latitude (phi) and longitude (theta) directions can be specified.
 * It also is possible to create partial sphere by specifying maximum phi and
 * theta angles.
 */

#ifndef svtkTexturedSphereSource_h
#define svtkTexturedSphereSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_MAX_SPHERE_RESOLUTION 1024

class SVTKFILTERSSOURCES_EXPORT svtkTexturedSphereSource : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkTexturedSphereSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct sphere with radius=0.5 and default resolution 8 in both Phi
   * and Theta directions.
   */
  static svtkTexturedSphereSource* New();

  //@{
  /**
   * Set radius of sphere.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set the number of points in the longitude direction.
   */
  svtkSetClampMacro(ThetaResolution, int, 4, SVTK_MAX_SPHERE_RESOLUTION);
  svtkGetMacro(ThetaResolution, int);
  //@}

  //@{
  /**
   * Set the number of points in the latitude direction.
   */
  svtkSetClampMacro(PhiResolution, int, 4, SVTK_MAX_SPHERE_RESOLUTION);
  svtkGetMacro(PhiResolution, int);
  //@}

  //@{
  /**
   * Set the maximum longitude angle.
   */
  svtkSetClampMacro(Theta, double, 0.0, 360.0);
  svtkGetMacro(Theta, double);
  //@}

  //@{
  /**
   * Set the maximum latitude angle (0 is at north pole).
   */
  svtkSetClampMacro(Phi, double, 0.0, 180.0);
  svtkGetMacro(Phi, double);
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
  svtkTexturedSphereSource(int res = 8);
  ~svtkTexturedSphereSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double Radius;
  double Theta;
  double Phi;
  int ThetaResolution;
  int PhiResolution;
  int OutputPointsPrecision;

private:
  svtkTexturedSphereSource(const svtkTexturedSphereSource&) = delete;
  void operator=(const svtkTexturedSphereSource&) = delete;
};

#endif
