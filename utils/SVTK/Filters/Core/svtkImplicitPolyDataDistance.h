/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitPolyDataDistance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImplicitPolyDataDistance
 *
 *
 * Implicit function that computes the distance from a point x to the
 * nearest point p on an input svtkPolyData. The sign of the function
 * is set to the sign of the dot product between the angle-weighted
 * pseudonormal at the nearest surface point and the vector x - p.
 * Points interior to the geometry have a negative distance, points on
 * the exterior have a positive distance, and points on the input
 * svtkPolyData have a distance of zero. The gradient of the function
 * is the angle-weighted pseudonormal at the nearest point.
 *
 * Baerentzen, J. A. and Aanaes, H. (2005). Signed distance
 * computation using the angle weighted pseudonormal. IEEE
 * Transactions on Visualization and Computer Graphics, 11:243-253.
 *
 * This code was contributed in the SVTK Journal paper:
 * "Boolean Operations on Surfaces in SVTK Without External Libraries"
 * by Cory Quammen, Chris Weigle C., Russ Taylor
 * http://hdl.handle.net/10380/3262
 * http://www.midasjournal.org/browse/publication/797
 */

#ifndef svtkImplicitPolyDataDistance_h
#define svtkImplicitPolyDataDistance_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkImplicitFunction.h"

class svtkCellLocator;
class svtkPolyData;

class SVTKFILTERSCORE_EXPORT svtkImplicitPolyDataDistance : public svtkImplicitFunction
{
public:
  static svtkImplicitPolyDataDistance* New();
  svtkTypeMacro(svtkImplicitPolyDataDistance, svtkImplicitFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the MTime also considering the Input dependency.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Evaluate plane equation of nearest triangle to point x[3].
   */
  using svtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;

  /**
   * Evaluate function gradient of nearest triangle to point x[3].
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  /**
   * Evaluate plane equation of nearest triangle to point x[3] and provides closest point on an
   * input svtkPolyData.
   */
  double EvaluateFunctionAndGetClosestPoint(double x[3], double closestPoint[3]);

  /**
   * Set the input svtkPolyData used for the implicit function
   * evaluation.  Passes input through an internal instance of
   * svtkTriangleFilter to remove vertices and lines, leaving only
   * triangular polygons for evaluation as implicit planes.
   */
  void SetInput(svtkPolyData* input);

  //@{
  /**
   * Set/get the function value to use if no input svtkPolyData
   * specified.
   */
  svtkSetMacro(NoValue, double);
  svtkGetMacro(NoValue, double);
  //@}

  //@{
  /**
   * Set/get the function gradient to use if no input svtkPolyData
   * specified.
   */
  svtkSetVector3Macro(NoGradient, double);
  svtkGetVector3Macro(NoGradient, double);
  //@}

  //@{
  /**
   * Set/get the closest point to use if no input svtkPolyData
   * specified.
   */
  svtkSetVector3Macro(NoClosestPoint, double);
  svtkGetVector3Macro(NoClosestPoint, double);
  //@}

  //@{
  /**
   * Set/get the tolerance usued for the locator.
   */
  svtkGetMacro(Tolerance, double);
  svtkSetMacro(Tolerance, double);
  //@}

protected:
  svtkImplicitPolyDataDistance();
  ~svtkImplicitPolyDataDistance() override;

  /**
   * Create default locator. Used to create one when none is specified.
   */
  void CreateDefaultLocator(void);

  double SharedEvaluate(double x[3], double g[3], double p[3]);

  double NoGradient[3];
  double NoClosestPoint[3];
  double NoValue;
  double Tolerance;

  svtkPolyData* Input;
  svtkCellLocator* Locator;

private:
  svtkImplicitPolyDataDistance(const svtkImplicitPolyDataDistance&) = delete;
  void operator=(const svtkImplicitPolyDataDistance&) = delete;
};

#endif
