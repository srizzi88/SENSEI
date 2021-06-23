/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalInterpolator
 * @brief   interpolate datasets between time steps to produce a new dataset
 *
 * svtkTemporalInterpolator interpolates between two time steps to
 * produce new data for an arbitrary T.
 * svtkTemporalInterpolator has three modes of operation.
 * The default mode is to produce a continuous range of time
 * values as output, which enables a filter downstream to request
 * any value of T within the range.
 * The second mode of operation is enabled by setting
 * DiscreteTimeStepInterval to a non zero value. When this mode is
 * activated, the filter will report a finite number of Time steps
 * separated by deltaT between the original range of values.
 * This mode is useful when a dataset of N time steps has one (or more)
 * missing datasets for certain T values and you simply wish to smooth
 * over the missing steps but otherwise use the original data.
 * The third mode of operation is enabled by setting
 * ResampleFactor to a non zero positive integer value.
 * When this mode is activated, the filter will report a finite number
 * of Time steps which contain the original steps, plus N new values between
 * each original step 1/ResampleFactor time units apart.
 * Note that if the input time steps are irregular, then using ResampleFactor
 * will produce an irregular sequence of regular steps between
 * each of the original irregular steps (clear enough, yes?).
 *
 * @TODO
 * Higher order interpolation schemes will require changes to the API
 * as most calls assume only two timesteps are used.
 *
 * @par Thanks:
 * Ken Martin (Kitware) and John Bidiscombe of
 * CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 * For related material, please refer to :
 * John Biddiscombe, Berk Geveci, Ken Martin, Kenneth Moreland, David Thompson,
 * "Time Dependent Processing in a Parallel Pipeline Architecture",
 * IEEE Visualization 2007.
 */

#ifndef svtkTemporalInterpolator_h
#define svtkTemporalInterpolator_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkMultiTimeStepAlgorithm.h"

class svtkDataSet;
class SVTKFILTERSHYBRID_EXPORT svtkTemporalInterpolator : public svtkMultiTimeStepAlgorithm
{
public:
  static svtkTemporalInterpolator* New();
  svtkTypeMacro(svtkTemporalInterpolator, svtkMultiTimeStepAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If you require a discrete number of outputs steps, to be
   * generated from an input source - for example, you required
   * N steps separated by T, then set DiscreteTimeStepInterval to T
   * and you will get TIME_RANGE/DiscreteTimeStepInterval steps
   * This is a useful option to use if you have a dataset with one
   * missing time step and wish to 'fill-in' the missing data
   * with an interpolated value from the steps either side
   */
  svtkSetMacro(DiscreteTimeStepInterval, double);
  svtkGetMacro(DiscreteTimeStepInterval, double);
  //@}

  //@{
  /**
   * When ResampleFactor is a non zero positive integer, each pair
   * of input time steps will be interpolated between with the number
   * of steps specified. For example an input of 1,2,3,4,5 and a resample factor
   * of 10, will produce steps 0f 1.0, 1.1, 1.2.....1.9, 2.0 etc
   * NB. Irregular input steps will produce irregular output steps.
   * Resample factor wuill only be used if DiscreteTimeStepInterval is zero
   * otherwise the DiscreteTimeStepInterval takes precedence
   */
  svtkSetMacro(ResampleFactor, int);
  svtkGetMacro(ResampleFactor, int);
  //@}

  //@{
  /**
   * Controls whether input data is cached to avoid updating input
   * when multiple interpolations are asked between 2 time steps.
   */
  svtkSetMacro(CacheData, bool);
  svtkGetMacro(CacheData, bool);
  //@}

protected:
  svtkTemporalInterpolator();
  ~svtkTemporalInterpolator() override;

  double DiscreteTimeStepInterval;
  int ResampleFactor;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info) override;

  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * General interpolation routine for any type on input data. This is
   * called recursively when hierarchical/multiblock data is encountered
   */
  svtkDataObject* InterpolateDataObject(svtkDataObject* in1, svtkDataObject* in2, double ratio);

  /**
   * Root level interpolation for a concrete dataset object.
   * Point/Cell data and points are interpolated.
   * Needs improving if connectivity is to be handled
   */
  virtual svtkDataSet* InterpolateDataSet(svtkDataSet* in1, svtkDataSet* in2, double ratio);

  /**
   * Interpolate a single svtkDataArray. Called from the Interpolation routine
   * on the points and pointdata/celldata
   */
  virtual svtkDataArray* InterpolateDataArray(double ratio, svtkDataArray** arrays, svtkIdType N);

  /// Return values for VerifyArrays().
  enum ArrayMatch
  {
    MATCHED = 0,           //!< Arrays match in number of components and tuples.
    MISMATCHED_TUPLES = 1, //!< Arrays match number of components but not tuples.
    MISMATCHED_COMPS = 2   //!< Arrays do not have the same number of components.
  };

  /**
   * Called just before interpolation of each dataset to ensure
   * each data array has the same number of tuples/components etc
   */
  virtual ArrayMatch VerifyArrays(svtkDataArray** arrays, int N);

  // internally used : Ratio is {0,1} between two time steps
  // DeltaT is time between current 2 steps.
  // These are only valid when 2 time steps are interpolated
  // Higher order schemes will require changes to the API
  double Ratio;
  double DeltaT;
  double Tfrac;

private:
  svtkTemporalInterpolator(const svtkTemporalInterpolator&) = delete;
  void operator=(const svtkTemporalInterpolator&) = delete;
};

#endif
