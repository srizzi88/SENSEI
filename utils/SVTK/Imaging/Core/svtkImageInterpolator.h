/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageInterpolator
 * @brief   interpolate data values from images
 *
 * svtkImageInterpolator provides a simple interface for interpolating image
 * data.  It provides linear, cubic, and nearest-neighbor interpolation.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * svtkImageReslice
 */

#ifndef svtkImageInterpolator_h
#define svtkImageInterpolator_h

#include "svtkAbstractImageInterpolator.h"
#include "svtkImagingCoreModule.h" // For export macro

class SVTKIMAGINGCORE_EXPORT svtkImageInterpolator : public svtkAbstractImageInterpolator
{
public:
  static svtkImageInterpolator* New();
  svtkTypeMacro(svtkImageInterpolator, svtkAbstractImageInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The interpolation mode for point scalars (default: linear).  Subclasses
   * will provide additional interpolation modes, so this is a virtual method.
   */
  virtual void SetInterpolationMode(int mode);
  void SetInterpolationModeToNearest() { this->SetInterpolationMode(SVTK_NEAREST_INTERPOLATION); }
  void SetInterpolationModeToLinear() { this->SetInterpolationMode(SVTK_LINEAR_INTERPOLATION); }
  void SetInterpolationModeToCubic() { this->SetInterpolationMode(SVTK_CUBIC_INTERPOLATION); }
  int GetInterpolationMode() { return this->InterpolationMode; }
  virtual const char* GetInterpolationModeAsString();
  //@}

  /**
   * Get the support size for use in computing update extents.  If the data
   * will be sampled on a regular grid, then pass a matrix describing the
   * structured coordinate transformation between the output and the input.
   * Otherwise, pass nullptr as the matrix to retrieve the full kernel size.
   */
  void ComputeSupportSize(const double matrix[16], int support[3]) override;

  /**
   * Returns true if the interpolator supports weight precomputation.
   * This will always return true for this interpolator.
   */
  bool IsSeparable() override;

  //@{
  /**
   * If the data is going to be sampled on a regular grid, then the
   * interpolation weights can be precomputed.  A matrix must be supplied
   * that provides a transformation between the provided extent and the
   * structured coordinates of the input.  This matrix must perform only
   * permutations, scales, and translation, i.e. each of the three columns
   * must have only one non-zero value.  A new extent is provided that can
   * be used for out-of-bounds checks. THIS METHOD IS THREAD SAFE.
   */
  void PrecomputeWeightsForExtent(const double matrix[16], const int extent[6], int newExtent[6],
    svtkInterpolationWeights*& weights) override;
  void PrecomputeWeightsForExtent(const float matrix[16], const int extent[6], int newExtent[6],
    svtkInterpolationWeights*& weights) override;
  //@}

  /**
   * Free the precomputed weights.  THIS METHOD IS THREAD SAFE.
   */
  void FreePrecomputedWeights(svtkInterpolationWeights*& weights) override;

protected:
  svtkImageInterpolator();
  ~svtkImageInterpolator() override;

  /**
   * Update the interpolator.
   */
  void InternalUpdate() override;

  /**
   * Copy all members.
   */
  void InternalDeepCopy(svtkAbstractImageInterpolator* obj) override;

  //@{
  /**
   * Get the interpolation functions.
   */
  void GetInterpolationFunc(
    void (**doublefunc)(svtkInterpolationInfo*, const double[3], double*)) override;
  void GetInterpolationFunc(
    void (**floatfunc)(svtkInterpolationInfo*, const float[3], float*)) override;
  //@}

  //@{
  /**
   * Get the row interpolation functions.
   */
  void GetRowInterpolationFunc(
    void (**doublefunc)(svtkInterpolationWeights*, int, int, int, double*, int)) override;
  void GetRowInterpolationFunc(
    void (**floatfunc)(svtkInterpolationWeights*, int, int, int, float*, int)) override;
  //@}

  int InterpolationMode;

private:
  svtkImageInterpolator(const svtkImageInterpolator&) = delete;
  void operator=(const svtkImageInterpolator&) = delete;
};

#endif
