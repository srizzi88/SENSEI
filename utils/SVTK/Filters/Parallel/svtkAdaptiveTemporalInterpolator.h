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
 * @class   svtkAdaptiveTemporalInterpolator
 * @brief   interpolate datasets between time steps to produce a new dataset
 *
 * svtkAdaptiveTemporalInterpolator extends svtkTemporalInterpolator to
 * interpolate between timesteps when mesh topology appears to be different.
 */

#ifndef svtkAdaptiveTemporalInterpolator_h
#define svtkAdaptiveTemporalInterpolator_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkTemporalInterpolator.h"

class svtkDataSet;
class svtkPointSet;

class SVTKFILTERSPARALLEL_EXPORT svtkAdaptiveTemporalInterpolator : public svtkTemporalInterpolator
{
public:
  static svtkAdaptiveTemporalInterpolator* New();
  svtkTypeMacro(svtkAdaptiveTemporalInterpolator, svtkTemporalInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkAdaptiveTemporalInterpolator();
  ~svtkAdaptiveTemporalInterpolator() override;

  /**
   * Root level interpolation for a concrete dataset object.
   * Point/Cell data and points are interpolated.
   * Needs improving if connectivity is to be handled
   */
  svtkDataSet* InterpolateDataSet(svtkDataSet* in1, svtkDataSet* in2, double ratio) override;

  /**
   * When the mesh topology appears to be different between timesteps,
   * this method is invoked to resample point- and cell-data of one
   * dataset onto the points/cells of the other before interpolation.
   *
   * This will overwrite either \a a or \a b with a reference to the
   * resampled point-set (depending on the value of \a source). The
   * resampled point-set will also be the return value.
   * If \a source is 0, then \a b will be overwritten with a mesh
   * whose points are taken from \a a but whose point-data and cell-data
   * values correspond to \a b.
   * If \a source is non-zero, the opposite is done.
   *
   * Returns a non-null pointer to the resampled point-set on success
   * and nullptr on failure. If a valid pointer is returned, you are
   * responsible for calling Delete() on it.
   */
  svtkPointSet* ResampleDataObject(svtkPointSet*& a, svtkPointSet*& b, int source);

private:
  svtkAdaptiveTemporalInterpolator(const svtkAdaptiveTemporalInterpolator&) = delete;
  void operator=(const svtkAdaptiveTemporalInterpolator&) = delete;

  class ResamplingHelperImpl;
  ResamplingHelperImpl* ResampleImpl;
};

#endif
