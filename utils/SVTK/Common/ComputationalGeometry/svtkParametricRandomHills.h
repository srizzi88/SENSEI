/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricRandomHills.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricRandomHills
 * @brief   Generate a surface covered with randomly placed hills.
 *
 * svtkParametricRandomHills generates a surface covered with randomly placed
 * hills. Hills will vary in shape and height since the presence
 * of nearby hills will contribute to the shape and height of a given hill.
 * An option is provided for placing hills on a regular grid on the surface.
 * In this case the hills will all have the same shape and height.
 *
 * For further information about this surface, please consult the
 * technical description "Parametric surfaces" in http://www.svtk.org/publications
 * in the "SVTK Technical Documents" section in the VTk.org web pages.
 *
 * @par Thanks:
 * Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
 * class.
 *
 */

#ifndef svtkParametricRandomHills_h
#define svtkParametricRandomHills_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class svtkDoubleArray;
class svtkMinimalStandardRandomSequence;

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricRandomHills : public svtkParametricFunction
{

public:
  svtkTypeMacro(svtkParametricRandomHills, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  /**
   * Construct a surface of random hills with the following parameters:
   * MinimumU = -10, MaximumU = 10,
   * MinimumV = -10, MaximumV = 10,
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0;
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 0,
   * Number of hills = 30,
   * Variance of the hills 2.5 in both x- and y- directions,
   * Scaling factor for the variances 1/3 in both x- and y- directions,
   * Amplitude of each hill = 2,
   * Scaling factor for the amplitude = 1/3,
   * RandomSeed = 1,
   * AllowRandomGeneration = 1.
   */
  static svtkParametricRandomHills* New();

  //@{
  /**
   * Set/Get the number of hills.
   * Default is 30.
   */
  svtkSetMacro(NumberOfHills, int);
  svtkGetMacro(NumberOfHills, int);
  //@}

  //@{
  /**
   * Set/Get the hill variance in the x-direction.
   * Default is 2.5.
   */
  svtkSetMacro(HillXVariance, double);
  svtkGetMacro(HillXVariance, double);
  //@}

  //@{
  /**
   * Set/Get the hill variance in the y-direction.
   * Default is 2.5.
   */
  svtkSetMacro(HillYVariance, double);
  svtkGetMacro(HillYVariance, double);
  //@}

  //@{
  /**
   * Set/Get the hill amplitude (height).
   * Default is 2.
   */
  svtkSetMacro(HillAmplitude, double);
  svtkGetMacro(HillAmplitude, double);
  //@}

  //@{
  /**
   * Set/Get the Seed for the random number generator,
   * a value of 1 will initialize the random number generator,
   * a negative value will initialize it with the system time.
   * Default is 1.
   */
  svtkSetMacro(RandomSeed, int);
  svtkGetMacro(RandomSeed, int);
  //@}

  //@{
  /**
   * Set/Get the random generation flag.
   * A value of 0 will disable the generation of random hills on the surface
   * allowing a reproducible number of identically shaped hills to be
   * generated. If zero, then the number of hills used will be the nearest
   * perfect square less than or equal to the number of hills.
   * For example, selecting 30 hills will result in a 5 X 5 array of
   * hills being generated. Thus a square array of hills will be generated.

   * Any other value means that the hills will be placed randomly on the
   * surface.
   * Default is 1.
   */
  svtkSetClampMacro(AllowRandomGeneration, svtkTypeBool, 0, 1);
  svtkGetMacro(AllowRandomGeneration, svtkTypeBool);
  svtkBooleanMacro(AllowRandomGeneration, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the scaling factor for the variance in the x-direction.
   * Default is 1/3.
   */
  svtkSetMacro(XVarianceScaleFactor, double);
  svtkGetMacro(XVarianceScaleFactor, double);
  //@}

  //@{
  /**
   * Set/Get the scaling factor for the variance in the y-direction.
   * Default is 1/3.
   */
  svtkSetMacro(YVarianceScaleFactor, double);
  svtkGetMacro(YVarianceScaleFactor, double);
  //@}

  //@{
  /**
   * Set/Get the scaling factor for the amplitude.
   * Default is 1/3.
   */
  svtkSetMacro(AmplitudeScaleFactor, double);
  svtkGetMacro(AmplitudeScaleFactor, double);
  //@}

  /**
   * Construct a terrain consisting of hills on a surface.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = Du X Dv\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uvw are the parameters with Pt being the Cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * svtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  svtkParametricRandomHills();
  ~svtkParametricRandomHills() override;

  // Variables
  int NumberOfHills;
  double HillXVariance;
  double HillYVariance;
  double HillAmplitude;
  int RandomSeed;
  double XVarianceScaleFactor;
  double YVarianceScaleFactor;
  double AmplitudeScaleFactor;
  svtkTypeBool AllowRandomGeneration;

  // These variables store the previous values of the above ones.
  int previousNumberOfHills;
  double previousHillXVariance;
  double previousHillYVariance;
  double previousHillAmplitude;
  int previousRandomSeed;
  double previousXVarianceScaleFactor;
  double previousYVarianceScaleFactor;
  double previousAmplitudeScaleFactor;
  int previousAllowRandomGeneration;

private:
  svtkParametricRandomHills(const svtkParametricRandomHills&) = delete;
  void operator=(const svtkParametricRandomHills&) = delete;

  /**
   * Initialise the random number generator.
   */
  void InitRNG(int RandomSeed);

  /**
   * Return a random number between 0 and 1.
   */
  double Rand(void);

  /**
   * A random sequence generator.
   */
  svtkMinimalStandardRandomSequence* randomSequenceGenerator;

  /**
   * Generate the centers of the hills, their standard deviations and
   * their amplitudes. This function creates a series of vectors representing
   * the u, v coordinates of each hill, their variances in the u, v directions
   * and their amplitudes.
   */
  void MakeTheHillData(void);

  /**
   * True if any parameters have changed.
   */
  bool ParametersChanged();

  /**
   * Set the previous values of the parameters with the current values.
   */
  void CopyParameters();

  //@{
  /**
   * Centers (x,y), variances (x,y) and amplitudes of the hills.
   */
  svtkDoubleArray* hillData;
};
//@}

#endif
