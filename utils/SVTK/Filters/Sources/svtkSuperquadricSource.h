/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSuperquadricSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSuperquadricSource
 * @brief   create a polygonal superquadric centered
 * at the origin
 *
 * svtkSuperquadricSource creates a superquadric (represented by polygons) of
 * specified size centered at the origin. The alignment of the axis of the
 * superquadric along one of the global axes can be specified. The resolution
 * (polygonal discretization)
 * in both the latitude (phi) and longitude (theta) directions can be
 * specified. Roundness parameters (PhiRoundness and ThetaRoundness) control
 * the shape of the superquadric.  The Toroidal boolean controls whether
 * a toroidal superquadric is produced.  If so, the Thickness parameter
 * controls the thickness of the toroid:  0 is the thinnest allowable
 * toroid, and 1 has a minimum sized hole.  The Scale parameters allow
 * the superquadric to be scaled in x, y, and z (normal vectors are correctly
 * generated in any case).  The Size parameter controls size of the
 * superquadric.
 *
 * This code is based on "Rigid physically based superquadrics", A. H. Barr,
 * in "Graphics Gems III", David Kirk, ed., Academic Press, 1992.
 *
 * @warning
 * Resolution means the number of latitude or longitude lines for a complete
 * superquadric. The resolution parameters are rounded to the nearest 4
 * in phi and 8 in theta.
 *
 * @warning
 * Texture coordinates are not equally distributed around all superquadrics.
 *
 * @warning
 * The Size and Thickness parameters control coefficients of superquadric
 * generation, and may do not exactly describe the size of the superquadric.
 *
 */

#ifndef svtkSuperquadricSource_h
#define svtkSuperquadricSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_MAX_SUPERQUADRIC_RESOLUTION 1024
#define SVTK_MIN_SUPERQUADRIC_THICKNESS 1e-4
#define SVTK_MIN_SUPERQUADRIC_ROUNDNESS 1e-24

class SVTKFILTERSSOURCES_EXPORT svtkSuperquadricSource : public svtkPolyDataAlgorithm
{
public:
  /**
   * Create a default superquadric with a radius of 0.5, non-toroidal,
   * spherical, and centered at the origin, with a scaling factor of 1 in each
   * direction, a theta resolution and a phi resolutions of 16.
   */
  static svtkSuperquadricSource* New();

  svtkTypeMacro(svtkSuperquadricSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the center of the superquadric. Default is 0,0,0.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set the scale factors of the superquadric. Default is 1,1,1.
   */
  svtkSetVector3Macro(Scale, double);
  svtkGetVectorMacro(Scale, double, 3);
  //@}

  //@{
  /**
   * Set the number of points in the longitude direction. Initial value is 16.
   */
  svtkGetMacro(ThetaResolution, int);
  void SetThetaResolution(int i);
  //@}

  //@{
  /**
   * Set the number of points in the latitude direction. Initial value is 16.
   */
  svtkGetMacro(PhiResolution, int);
  void SetPhiResolution(int i);
  //@}

  //@{
  /**
   * Set/Get Superquadric ring thickness (toroids only).
   * Changing thickness maintains the outside diameter of the toroid.
   * Initial value is 0.3333.
   */
  svtkGetMacro(Thickness, double);
  svtkSetClampMacro(Thickness, double, SVTK_MIN_SUPERQUADRIC_THICKNESS, 1.0);
  //@}

  //@{
  /**
   * Set/Get Superquadric north/south roundness.
   * Values range from 0 (rectangular) to 1 (circular) to higher orders.
   * Initial value is 1.0.
   */
  svtkGetMacro(PhiRoundness, double);
  void SetPhiRoundness(double e);
  //@}

  //@{
  /**
   * Set/Get Superquadric east/west roundness.
   * Values range from 0 (rectangular) to 1 (circular) to higher orders.
   * Initial value is 1.0.
   */
  svtkGetMacro(ThetaRoundness, double);
  void SetThetaRoundness(double e);
  //@}

  //@{
  /**
   * Set/Get Superquadric isotropic size. Initial value is 0.5;
   */
  svtkSetMacro(Size, double);
  svtkGetMacro(Size, double);
  //@}

  //@{
  /**
   * Set/Get axis of symmetry for superquadric (x axis: 0, y axis: 1, z axis: 2). Initial value
   * is 1.
   */
  svtkSetMacro(AxisOfSymmetry, int);
  svtkGetMacro(AxisOfSymmetry, int);
  void SetXAxisOfSymmetry() { this->SetAxisOfSymmetry(0); }
  void SetYAxisOfSymmetry() { this->SetAxisOfSymmetry(1); }
  void SetZAxisOfSymmetry() { this->SetAxisOfSymmetry(2); }
  //@}

  //@{
  /**
   * Set/Get whether or not the superquadric is toroidal (1) or ellipsoidal (0).
   * Initial value is 0.
   */
  svtkBooleanMacro(Toroidal, svtkTypeBool);
  svtkGetMacro(Toroidal, svtkTypeBool);
  svtkSetMacro(Toroidal, svtkTypeBool);
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
  svtkSuperquadricSource(int res = 16);
  ~svtkSuperquadricSource() override {}

  svtkTypeBool Toroidal;
  int AxisOfSymmetry;
  double Thickness;
  double Size;
  double PhiRoundness;
  double ThetaRoundness;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double Center[3];
  double Scale[3];
  int ThetaResolution;
  int PhiResolution;
  int OutputPointsPrecision;

private:
  svtkSuperquadricSource(const svtkSuperquadricSource&) = delete;
  void operator=(const svtkSuperquadricSource&) = delete;
};

#endif
