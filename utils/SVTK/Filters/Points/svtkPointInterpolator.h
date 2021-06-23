/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointInterpolator
 * @brief   interpolate over point cloud using various kernels
 *
 *
 * svtkPointInterpolator probes a point cloud Pc (the filter Source) with a
 * set of points P (the filter Input), interpolating the data values from Pc
 * onto P. Note however that the descriptive phrase "point cloud" is a
 * misnomer: Pc can be represented by any svtkDataSet type, with the points of
 * the dataset forming Pc. Similarly, the output P can also be represented by
 * any svtkDataSet type; and the topology/geometry structure of P is passed
 * through to the output along with the newly interpolated arrays.
 *
 * A key input to this filter is the specification of the interpolation
 * kernel, and the parameters which control the associated interpolation
 * process. Interpolation kernels include Voronoi, Gaussian, Shepard, and SPH
 * (smoothed particle hydrodynamics), with additional kernels to be added in
 * the future.
 *
 * An overview of the algorithm is as follows. For each p from P, Np "close"
 * points to p are found. (The meaning of what is "close" can be specified as
 * either the N closest points, or all points within a given radius Rp. This
 * depends on how the kernel is defined.) Once the Np close points are found,
 * then the interpolation kernel is applied to compute new data values
 * located on p. Note that for reasonable performance, finding the Np closest
 * points requires a point locator. The locator may be specified as input to
 * the algorithm. (By default, a svtkStaticPointLocator is used because
 * generally it is much faster to build, delete, and search with. However,
 * with highly non-uniform point distributions, octree- or kd-tree based
 * locators may perform better.)
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * For widely spaced points in Pc, or when p is located outside the bounding
 * region of Pc, the interpolation may behave badly and the interpolation
 * process will adapt as necessary to produce output. For example, if the N
 * closest points within R are requested to interpolate p, if N=0 then the
 * interpolation will switch to a different strategy (which can be controlled
 * as in the NullPointsStrategy).
 *
 * @sa
 * svtkPointInterpolator2D svtkProbeFilter svtkGaussianSplatter
 * svtkCheckerboardSplatter svtkShepardMethod svtkVoronoiKernel svtkShepardKernel
 * svtkGaussianKernel svtkSPHKernel
 */

#ifndef svtkPointInterpolator_h
#define svtkPointInterpolator_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkStdString.h"           // For svtkStdString ivars
#include <vector>                   //For STL vector

class svtkAbstractPointLocator;
class svtkIdList;
class svtkDoubleArray;
class svtkInterpolationKernel;
class svtkCharArray;

class SVTKFILTERSPOINTS_EXPORT svtkPointInterpolator : public svtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing.
   */
  static svtkPointInterpolator* New();
  svtkTypeMacro(svtkPointInterpolator, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the dataset Pc that will be probed by the input points P.  The
   * Input P defines the dataset structure (the points and cells) for the
   * output, while the Source Pc is probed (interpolated) to generate the
   * scalars, vectors, etc. for the output points based on the point
   * locations.
   */
  void SetSourceData(svtkDataObject* source);
  svtkDataObject* GetSource();
  //@}

  /**
   * Specify the dataset Pc that will be probed by the input points P.  The
   * Input P defines the structure (the points and cells) for the output,
   * while the Source Pc is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(svtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Specify a point locator. By default a svtkStaticPointLocator is
   * used. The locator performs efficient searches to locate near a
   * specified interpolation position.
   */
  void SetLocator(svtkAbstractPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkAbstractPointLocator);
  //@}

  //@{
  /**
   * Specify an interpolation kernel. By default a svtkLinearKernel is used
   * (i.e., linear combination of closest points). The interpolation kernel
   * changes the basis of the interpolation.
   */
  void SetKernel(svtkInterpolationKernel* kernel);
  svtkGetObjectMacro(Kernel, svtkInterpolationKernel);
  //@}

  enum Strategy
  {
    MASK_POINTS = 0,
    NULL_VALUE = 1,
    CLOSEST_POINT = 2
  };

  //@{
  /**
   * Specify a strategy to use when encountering a "null" point during the
   * interpolation process. Null points occur when the local neighborhood (of
   * nearby points to interpolate from) is empty. If the strategy is set to
   * MaskPoints, then an output array is created that marks points as being
   * valid (=1) or null (invalid =0) (and the NullValue is set as well). If
   * the strategy is set to NullValue (this is the default), then the output
   * data value(s) are set to the NullPoint value (specified in the output
   * point data). Finally, the strategy ClosestPoint is to simply use the
   * closest point to perform the interpolation.
   */
  svtkSetMacro(NullPointsStrategy, int);
  svtkGetMacro(NullPointsStrategy, int);
  void SetNullPointsStrategyToMaskPoints() { this->SetNullPointsStrategy(MASK_POINTS); }
  void SetNullPointsStrategyToNullValue() { this->SetNullPointsStrategy(NULL_VALUE); }
  void SetNullPointsStrategyToClosestPoint() { this->SetNullPointsStrategy(CLOSEST_POINT); }
  //@}

  //@{
  /**
   * If the NullPointsStrategy == MASK_POINTS, then an array is generated for
   * each input point. This svtkCharArray is placed into the output of the filter,
   * with a non-zero value for a valid point, and zero otherwise. The name of
   * this masking array is specified here.
   */
  svtkSetMacro(ValidPointsMaskArrayName, svtkStdString);
  svtkGetMacro(ValidPointsMaskArrayName, svtkStdString);
  //@}

  //@{
  /**
   * Specify the null point value. When a null point is encountered then all
   * components of each null tuple are set to this value. By default the
   * null value is set to zero.
   */
  svtkSetMacro(NullValue, double);
  svtkGetMacro(NullValue, double);
  //@}

  //@{
  /**
   * Adds an array to the list of arrays which are to be excluded from the
   * interpolation process.
   */
  void AddExcludedArray(const svtkStdString& excludedArray)
  {
    this->ExcludedArrays.push_back(excludedArray);
    this->Modified();
  }
  //@}

  //@{
  /**
   * Clears the contents of excluded array list.
   */
  void ClearExcludedArrays()
  {
    this->ExcludedArrays.clear();
    this->Modified();
  }
  //@}

  /**
   * Return the number of excluded arrays.
   */
  int GetNumberOfExcludedArrays() { return static_cast<int>(this->ExcludedArrays.size()); }

  //@{
  /**
   * Return the name of the ith excluded array.
   */
  const char* GetExcludedArray(int i)
  {
    if (i < 0 || i >= static_cast<int>(this->ExcludedArrays.size()))
    {
      return nullptr;
    }
    return this->ExcludedArrays[i].c_str();
  }
  //@}

  //@{
  /**
   * If enabled, then input arrays that are non-real types (i.e., not float
   * or double) are promoted to float type on output. This is because the
   * interpolation process may not be well behaved when integral types are
   * combined using interpolation weights.
   */
  svtkSetMacro(PromoteOutputArrays, bool);
  svtkBooleanMacro(PromoteOutputArrays, bool);
  svtkGetMacro(PromoteOutputArrays, bool);
  //@}

  //@{
  /**
   * Indicate whether to shallow copy the input point data arrays to the
   * output.  On by default.
   */
  svtkSetMacro(PassPointArrays, bool);
  svtkBooleanMacro(PassPointArrays, bool);
  svtkGetMacro(PassPointArrays, bool);
  //@}

  //@{
  /**
   * Indicate whether to shallow copy the input cell data arrays to the
   * output.  On by default.
   */
  svtkSetMacro(PassCellArrays, bool);
  svtkBooleanMacro(PassCellArrays, bool);
  svtkGetMacro(PassCellArrays, bool);
  //@}

  //@{
  /**
   * Indicate whether to pass the field-data arrays from the input to the
   * output. On by default.
   */
  svtkSetMacro(PassFieldArrays, bool);
  svtkBooleanMacro(PassFieldArrays, bool);
  svtkGetMacro(PassFieldArrays, bool);
  //@}

  /**
   * Get the MTime of this object also considering the locator and kernel.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkPointInterpolator();
  ~svtkPointInterpolator() override;

  svtkAbstractPointLocator* Locator;
  svtkInterpolationKernel* Kernel;

  int NullPointsStrategy;
  double NullValue;
  svtkStdString ValidPointsMaskArrayName;
  svtkCharArray* ValidPointsMask;

  std::vector<svtkStdString> ExcludedArrays;

  bool PromoteOutputArrays;

  bool PassCellArrays;
  bool PassPointArrays;
  bool PassFieldArrays;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Virtual for specialized subclass(es)
   */
  virtual void Probe(svtkDataSet* input, svtkDataSet* source, svtkDataSet* output);

  /**
   * Call at end of RequestData() to pass attribute data respecting the
   * PassCellArrays, PassPointArrays, PassFieldArrays flags.
   */
  virtual void PassAttributeData(svtkDataSet* input, svtkDataObject* source, svtkDataSet* output);

  /**
   * Internal method to extract image metadata
   */
  void ExtractImageDescription(
    svtkImageData* input, int dims[3], double origin[3], double spacing[3]);

private:
  svtkPointInterpolator(const svtkPointInterpolator&) = delete;
  void operator=(const svtkPointInterpolator&) = delete;
};

#endif
