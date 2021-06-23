/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSPHInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSPHInterpolator
 * @brief   interpolate over point cloud using SPH kernels
 *
 *
 * This filter uses SPH (smooth particle hydrodynamics) kernels to
 * interpolate a data source onto an input structure. For example, while the
 * data source is a set of particles, the data from these particles can be
 * interpolated onto an input object such as a line, plane or volume. Then
 * the output (which consists of the input structure plus interpolated data)
 * can then be visualized using classical visualization techniques such as
 * isocontouring, slicing, heat maps and so on.
 *
 * To use this filter, besides setting the input P and source Pc, specify a
 * point locator (which accelerates queries about points and their neighbors)
 * and an interpolation kernel (a subclass of svtkSPHKernel). In addition, the
 * name of the source's density and mass arrays can optionally be provided;
 * however if not provided then the local volume is computed from the
 * kernel's spatial step. Finally, a cutoff distance array can optionally be
 * provided when the local neighborhood around each point varies. The cutoff
 * distance defines a local neighborhood in which the points in that
 * neighborhood are used to interpolate values. If not provided, then the
 * cutoff distance is computed from the spatial step size times the cutoff
 * factor (see svtkSPHKernel).
 *
 * Other options to the filter include specifying which data attributes to
 * interpolate from the source. By default, all data attributes contained in
 * the source are interpolated. However, by adding array names to the
 * exclusion list, these arrays will not be interpolated. Also, it is
 * possible to use a SPH derivative formulation to interpolate from the
 * source data attributes. This requires adding arrays (by name) to the
 * derivative list, in which case the derivative formulation will be applied
 * to create a new output array named "X_deriv" where X is the name of a
 * source point attribute array.
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
 * @warning
 * For more information and technical reference, see D.J. Price, Smoothed
 * particle hydrodynamics and magnetohydrodynamics,
 * J. Comput. Phys. 231:759-794, 2012. Especially equation 49.
 *
 * @par Acknowledgments:
 * The following work has been generously supported by Altair Engineering
 * and FluiDyna GmbH. Please contact Steve Cosgrove or Milos Stanic for
 * more information.
 *
 * @sa
 * svtkPointInterpolator svtkSPHKernel svtkSPHQuinticKernel
 */

#ifndef svtkSPHInterpolator_h
#define svtkSPHInterpolator_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkStdString.h"           // For svtkStdString ivars
#include <vector>                   //For STL vector

class svtkAbstractPointLocator;
class svtkIdList;
class svtkDoubleArray;
class svtkSPHKernel;
class svtkCharArray;
class svtkFloatArray;

class SVTKFILTERSPOINTS_EXPORT svtkSPHInterpolator : public svtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing.
   */
  static svtkSPHInterpolator* New();
  svtkTypeMacro(svtkSPHInterpolator, svtkDataSetAlgorithm);
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
   * Specify an interpolation kernel. By default a svtkSPHQuinticKernel is used
   * (i.e., closest point). The interpolation kernel changes the basis of the
   * interpolation.
   */
  void SetKernel(svtkSPHKernel* kernel);
  svtkGetObjectMacro(Kernel, svtkSPHKernel);
  //@}

  //@{
  /**
   * Specify an (optional) cutoff distance for each point in the input P. If
   * not specified, then the kernel cutoff is used.
   */
  svtkSetMacro(CutoffArrayName, svtkStdString);
  svtkGetMacro(CutoffArrayName, svtkStdString);
  //@}

  //@{
  /**
   * Specify the density array name. This is optional. Typically both the density
   * and mass arrays are specified together (in order to compute the local volume).
   * Both the mass and density arrays must consist of tuples of 1-component. (Note that
   * the density array name specifies a point array found in the Pc source.)
   */
  svtkSetMacro(DensityArrayName, svtkStdString);
  svtkGetMacro(DensityArrayName, svtkStdString);
  //@}

  //@{
  /**
   * Specify the mass array name. This is optional. Typically both the
   * density and mass arrays are specified together (in order to compute the
   * local volume).  Both the mass and density arrays must consist of tuples
   * of 1-component. (Note that the mass array name specifies a point
   * array found in the Pc source.)
   */
  svtkSetMacro(MassArrayName, svtkStdString);
  svtkGetMacro(MassArrayName, svtkStdString);
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
   * Adds an array to the list of arrays whose derivative is to be taken. If
   * the name of the array is "derivArray" this will produce an output array
   * with the name "derivArray_deriv" (after filter execution).
   */
  void AddDerivativeArray(const svtkStdString& derivArray)
  {
    this->DerivArrays.push_back(derivArray);
    this->Modified();
  }
  //@}

  //@{
  /**
   * Clears the contents of derivative array list.
   */
  void ClearDerivativeArrays()
  {
    this->DerivArrays.clear();
    this->Modified();
  }
  //@}

  /**
   * Return the number of derivative arrays.
   */
  int GetNumberOfDerivativeArrays() { return static_cast<int>(this->DerivArrays.size()); }

  //@{
  /**
   * Return the name of the ith derivative array.
   */
  const char* GetDerivativeArray(int i)
  {
    if (i < 0 || i >= static_cast<int>(this->DerivArrays.size()))
    {
      return nullptr;
    }
    return this->DerivArrays[i].c_str();
  }
  //@}

  // How to handle NULL/empty points
  enum NullStrategy
  {
    MASK_POINTS = 0,
    NULL_VALUE = 1
  };

  //@{
  /**
   * Specify a strategy to use when encountering a "null" point during the
   * interpolation process. Null points occur when the local neighborhood (of
   * nearby points to interpolate from) is empty. If the strategy is set to
   * MaskPoints, then an output array is created that marks points as being
   * valid (=1) or null (invalid =0) (and the NullValue is set as well). If
   * the strategy is set to NullValue, then the output data value(s) are set
   * to the NullPoint value.
   */
  svtkSetMacro(NullPointsStrategy, int);
  svtkGetMacro(NullPointsStrategy, int);
  void SetNullPointsStrategyToMaskPoints() { this->SetNullPointsStrategy(MASK_POINTS); }
  void SetNullPointsStrategyToNullValue() { this->SetNullPointsStrategy(NULL_VALUE); }
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
   * Indicate whether to compute the summation of weighting coefficients (the
   * so-called Shepard sum). In the interior of a SPH point cloud, the
   * Shepard summation value should be ~1.0.  Towards the boundary, the
   * Shepard summation generally falls off <1.0. If ComputeShepardSum is specified, then the
   * output will contain an array of summed Shepard weights for each output
   * point. On by default.
   */
  svtkSetMacro(ComputeShepardSum, svtkTypeBool);
  svtkBooleanMacro(ComputeShepardSum, svtkTypeBool);
  svtkGetMacro(ComputeShepardSum, svtkTypeBool);
  //@}

  //@{
  /**
   * If ComputeShepardSum is on, then an array is generated with name
   * ShepardSumArrayName for each input point. This svtkFloatArray is placed
   * into the output of the filter, and NullPoints have value =0.0. The
   * default name is "Shepard Summation".
   */
  svtkSetMacro(ShepardSumArrayName, svtkStdString);
  svtkGetMacro(ShepardSumArrayName, svtkStdString);
  //@}

  //@{
  /**
   * If enabled, then input arrays that are non-real types (i.e., not float
   * or double) are promoted to float type on output. This is because the
   * interpolation process may not be well behaved when integral types are
   * combined using interpolation weights.
   */
  svtkSetMacro(PromoteOutputArrays, svtkTypeBool);
  svtkBooleanMacro(PromoteOutputArrays, svtkTypeBool);
  svtkGetMacro(PromoteOutputArrays, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether to shallow copy the input point data arrays to the
   * output. On by default.
   */
  svtkSetMacro(PassPointArrays, svtkTypeBool);
  svtkBooleanMacro(PassPointArrays, svtkTypeBool);
  svtkGetMacro(PassPointArrays, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether to shallow copy the input cell data arrays to the
   * output. On by default.
   */
  svtkSetMacro(PassCellArrays, svtkTypeBool);
  svtkBooleanMacro(PassCellArrays, svtkTypeBool);
  svtkGetMacro(PassCellArrays, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether to pass the field-data arrays from the input to the
   * output. On by default.
   */
  svtkSetMacro(PassFieldArrays, svtkTypeBool);
  svtkBooleanMacro(PassFieldArrays, svtkTypeBool);
  svtkGetMacro(PassFieldArrays, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate whether to normalize all arrays with the Shepard coefficients
   * (except the density array and the Shepard sum array). If the Shepard
   * coefficient is 0, then the data value is set to zero. Note that enabling
   * ShepardNormalization forces the computation of the ShepardSum array.
   */
  svtkSetMacro(ShepardNormalization, svtkTypeBool);
  svtkBooleanMacro(ShepardNormalization, svtkTypeBool);
  svtkGetMacro(ShepardNormalization, svtkTypeBool);
  //@}

  /**
   * Get the MTime of this object also considering the locator and kernel.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkSPHInterpolator();
  ~svtkSPHInterpolator() override;

  svtkAbstractPointLocator* Locator;
  svtkSPHKernel* Kernel;

  svtkStdString CutoffArrayName;

  svtkStdString DensityArrayName;
  svtkStdString MassArrayName;

  std::vector<svtkStdString> ExcludedArrays;
  std::vector<svtkStdString> DerivArrays;

  svtkTypeBool ShepardNormalization;

  int NullPointsStrategy;
  double NullValue;
  svtkStdString ValidPointsMaskArrayName;
  svtkCharArray* ValidPointsMask;

  svtkTypeBool ComputeShepardSum;
  svtkStdString ShepardSumArrayName;
  svtkFloatArray* ShepardSumArray;

  svtkTypeBool PromoteOutputArrays;

  svtkTypeBool PassCellArrays;
  svtkTypeBool PassPointArrays;
  svtkTypeBool PassFieldArrays;

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

private:
  svtkSPHInterpolator(const svtkSPHInterpolator&) = delete;
  void operator=(const svtkSPHInterpolator&) = delete;
};

#endif
