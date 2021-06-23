/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCapsuleSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.

  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Program: Bender
  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

/**
 * @class   svtkCapsuleSource
 * @brief   Generate a capsule centered at the origin
 *
 * svtkCapsuleSource creates a capsule (represented by polygons) of specified
 * radius centered at the origin. The resolution (polygonal discretization) in
 * both the latitude (phi) and longitude (theta) directions can be specified as
 * well as the length of the capsule cylinder (CylinderLength). By default, the
 * surface tessellation of the sphere uses triangles; however you can set
 * LatLongTessellation to produce a tessellation using quadrilaterals (except
 * at the poles of the capsule).
 */

#ifndef svtkCapsuleSource_h
#define svtkCapsuleSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkSphereSource.h" // For SVTK_MAX_SPHERE_RESOLUTION

class SVTKFILTERSSOURCES_EXPORT svtkCapsuleSource : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkCapsuleSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a capsule with radius 0.5 and resolution 8 in both the Phi and Theta directions and a
   * cylinder of length 1.0.
   */
  static svtkCapsuleSource* New();

  //@{
  /**
   * Set/get the radius of the capsule. The initial value is 0.5.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set/get the center of the capsule. The initial value is (0.0, 0.0, 0.0).
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set/get the length of the cylinder. The initial value is 1.0.
   */
  svtkSetClampMacro(CylinderLength, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(CylinderLength, double);
  //@}

  //@{
  /**
   * Set/get the number of points in the longitude direction for the spheres. The initial value
   * is 8.
   */
  svtkSetClampMacro(ThetaResolution, int, 8, SVTK_MAX_SPHERE_RESOLUTION);
  svtkGetMacro(ThetaResolution, int);
  //@}

  //@{
  /**
   * Set/get the number of points in the latitude direction for the spheres. The initial value is 8.
   */
  svtkSetClampMacro(PhiResolution, int, 8, SVTK_MAX_SPHERE_RESOLUTION);
  svtkGetMacro(PhiResolution, int);
  //@}

  //@{
  /**
   * Cause the spheres to be tessellated with edges along the latitude and longitude lines. If off,
   * triangles are generated at non-polar regions, which results in edges that are not parallel to
   * latitude and longitude lines. If on, quadrilaterals are generated everywhere except at the
   * poles. This can be useful for generating wireframe spheres with natural latitude and longitude
   * lines.
   */
  svtkSetMacro(LatLongTessellation, int);
  svtkGetMacro(LatLongTessellation, int);
  svtkBooleanMacro(LatLongTessellation, int);
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
  svtkCapsuleSource(int res = 8);
  ~svtkCapsuleSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Radius;
  double Center[3];
  int ThetaResolution;
  int PhiResolution;
  int LatLongTessellation;
  int FillPoles;
  double CylinderLength;
  int OutputPointsPrecision;

private:
  svtkCapsuleSource(const svtkCapsuleSource&) = delete;
  void operator=(const svtkCapsuleSource&) = delete;
};

#endif
