/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWarpScalar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWarpScalar
 * @brief   deform geometry with scalar data
 *
 * svtkWarpScalar is a filter that modifies point coordinates by moving
 * points along point normals by the scalar amount times the scale factor.
 * Useful for creating carpet or x-y-z plots.
 *
 * If normals are not present in data, the Normal instance variable will
 * be used as the direction along which to warp the geometry. If normals are
 * present but you would like to use the Normal instance variable, set the
 * UseNormal boolean to true.
 *
 * If XYPlane boolean is set true, then the z-value is considered to be
 * a scalar value (still scaled by scale factor), and the displacement is
 * along the z-axis. If scalars are also present, these are copied through
 * and can be used to color the surface.
 *
 * Note that the filter passes both its point data and cell data to
 * its output, except for normals, since these are distorted by the
 * warping.
 */

#ifndef svtkWarpScalar_h
#define svtkWarpScalar_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class svtkDataArray;

class SVTKFILTERSGENERAL_EXPORT svtkWarpScalar : public svtkPointSetAlgorithm
{
public:
  static svtkWarpScalar* New();
  svtkTypeMacro(svtkWarpScalar, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify value to scale displacement.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Turn on/off use of user specified normal. If on, data normals
   * will be ignored and instance variable Normal will be used instead.
   */
  svtkSetMacro(UseNormal, svtkTypeBool);
  svtkGetMacro(UseNormal, svtkTypeBool);
  svtkBooleanMacro(UseNormal, svtkTypeBool);
  //@}

  //@{
  /**
   * Normal (i.e., direction) along which to warp geometry. Only used
   * if UseNormal boolean set to true or no normals available in data.
   */
  svtkSetVector3Macro(Normal, double);
  svtkGetVectorMacro(Normal, double, 3);
  //@}

  //@{
  /**
   * Turn on/off flag specifying that input data is x-y plane. If x-y plane,
   * then the z value is used to warp the surface in the z-axis direction
   * (times the scale factor) and scalars are used to color the surface.
   */
  svtkSetMacro(XYPlane, svtkTypeBool);
  svtkGetMacro(XYPlane, svtkTypeBool);
  svtkBooleanMacro(XYPlane, svtkTypeBool);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;

protected:
  svtkWarpScalar();
  ~svtkWarpScalar() override;

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double ScaleFactor;
  svtkTypeBool UseNormal;
  double Normal[3];
  svtkTypeBool XYPlane;

  double* (svtkWarpScalar::*PointNormal)(svtkIdType id, svtkDataArray* normals);
  double* DataNormal(svtkIdType id, svtkDataArray* normals = nullptr);
  double* InstanceNormal(svtkIdType id, svtkDataArray* normals = nullptr);
  double* ZNormal(svtkIdType id, svtkDataArray* normals = nullptr);

private:
  svtkWarpScalar(const svtkWarpScalar&) = delete;
  void operator=(const svtkWarpScalar&) = delete;
};

#endif
