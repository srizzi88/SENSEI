/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphereSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSphereSource
 * @brief   create a polygonal sphere centered at the origin
 *
 * svtkSphereSource creates a sphere (represented by polygons) of specified
 * radius centered at the origin. The resolution (polygonal discretization)
 * in both the latitude (phi) and longitude (theta) directions can be
 * specified. It also is possible to create partial spheres by specifying
 * maximum phi and theta angles. By default, the surface tessellation of
 * the sphere uses triangles; however you can set LatLongTessellation to
 * produce a tessellation using quadrilaterals.
 * @warning
 * Resolution means the number of latitude or longitude lines for a complete
 * sphere. If you create partial spheres the number of latitude/longitude
 * lines may be off by one.
 */

#ifndef svtkSphereSource_h
#define svtkSphereSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_MAX_SPHERE_RESOLUTION 1024

class SVTKFILTERSSOURCES_EXPORT svtkSphereSource : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkSphereSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct sphere with radius=0.5 and default resolution 8 in both Phi
   * and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
   */
  static svtkSphereSource* New();

  //@{
  /**
   * Set radius of sphere. Default is .5.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set the center of the sphere. Default is 0,0,0.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set the number of points in the longitude direction (ranging from
   * StartTheta to EndTheta).
   */
  svtkSetClampMacro(ThetaResolution, int, 3, SVTK_MAX_SPHERE_RESOLUTION);
  svtkGetMacro(ThetaResolution, int);
  //@}

  //@{
  /**
   * Set the number of points in the latitude direction (ranging
   * from StartPhi to EndPhi).
   */
  svtkSetClampMacro(PhiResolution, int, 3, SVTK_MAX_SPHERE_RESOLUTION);
  svtkGetMacro(PhiResolution, int);
  //@}

  //@{
  /**
   * Set the starting longitude angle. By default StartTheta=0 degrees.
   */
  svtkSetClampMacro(StartTheta, double, 0.0, 360.0);
  svtkGetMacro(StartTheta, double);
  //@}

  //@{
  /**
   * Set the ending longitude angle. By default EndTheta=360 degrees.
   */
  svtkSetClampMacro(EndTheta, double, 0.0, 360.0);
  svtkGetMacro(EndTheta, double);
  //@}

  //@{
  /**
   * Set the starting latitude angle (0 is at north pole). By default
   * StartPhi=0 degrees.
   */
  svtkSetClampMacro(StartPhi, double, 0.0, 360.0);
  svtkGetMacro(StartPhi, double);
  //@}

  //@{
  /**
   * Set the ending latitude angle. By default EndPhi=180 degrees.
   */
  svtkSetClampMacro(EndPhi, double, 0.0, 360.0);
  svtkGetMacro(EndPhi, double);
  //@}

  //@{
  /**
   * Cause the sphere to be tessellated with edges along the latitude
   * and longitude lines. If off, triangles are generated at non-polar
   * regions, which results in edges that are not parallel to latitude and
   * longitude lines. If on, quadrilaterals are generated everywhere
   * except at the poles. This can be useful for generating a wireframe
   * sphere with natural latitude and longitude lines.
   */
  svtkSetMacro(LatLongTessellation, svtkTypeBool);
  svtkGetMacro(LatLongTessellation, svtkTypeBool);
  svtkBooleanMacro(LatLongTessellation, svtkTypeBool);
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
  svtkSphereSource(int res = 8);
  ~svtkSphereSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Radius;
  double Center[3];
  int ThetaResolution;
  int PhiResolution;
  double StartTheta;
  double EndTheta;
  double StartPhi;
  double EndPhi;
  svtkTypeBool LatLongTessellation;
  int OutputPointsPrecision;

private:
  svtkSphereSource(const svtkSphereSource&) = delete;
  void operator=(const svtkSphereSource&) = delete;
};

#endif
