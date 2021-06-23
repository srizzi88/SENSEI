/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointSource
 * @brief   create a random cloud of points
 *
 * svtkPointSource is a source object that creates a user-specified number
 * of points within a specified radius about a specified center point.
 * By default location of the points is random within the sphere. It is
 * also possible to generate random points only on the surface of the
 * sphere. The output PolyData has the specified number of points and
 * 1 cell - a svtkPolyVertex containing all of the points.
 */

#ifndef svtkPointSource_h
#define svtkPointSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_POINT_UNIFORM 1
#define SVTK_POINT_SHELL 0

class svtkRandomSequence;

class SVTKFILTERSSOURCES_EXPORT svtkPointSource : public svtkPolyDataAlgorithm
{
public:
  static svtkPointSource* New();
  svtkTypeMacro(svtkPointSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the number of points to generate.
   */
  svtkSetClampMacro(NumberOfPoints, svtkIdType, 1, SVTK_ID_MAX);
  svtkGetMacro(NumberOfPoints, svtkIdType);
  //@}

  //@{
  /**
   * Set the center of the point cloud.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set the radius of the point cloud.  If you are
   * generating a Gaussian distribution, then this is
   * the standard deviation for each of x, y, and z.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Specify the distribution to use.  The default is a
   * uniform distribution.  The shell distribution produces
   * random points on the surface of the sphere, none in the interior.
   */
  svtkSetMacro(Distribution, int);
  void SetDistributionToUniform() { this->SetDistribution(SVTK_POINT_UNIFORM); }
  void SetDistributionToShell() { this->SetDistribution(SVTK_POINT_SHELL); }
  svtkGetMacro(Distribution, int);
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

  //@{
  /**
   * Set/Get a random sequence generator.
   * By default, the generator in svtkMath is used to maintain backwards
   * compatibility.
   */
  virtual void SetRandomSequence(svtkRandomSequence* randomSequence);
  svtkGetObjectMacro(RandomSequence, svtkRandomSequence);
  //@}

protected:
  svtkPointSource(svtkIdType numPts = 10);
  ~svtkPointSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Random();

  svtkIdType NumberOfPoints;
  double Center[3];
  double Radius;
  int Distribution;
  int OutputPointsPrecision;
  svtkRandomSequence* RandomSequence;

private:
  svtkPointSource(const svtkPointSource&) = delete;
  void operator=(const svtkPointSource&) = delete;
};

#endif
