/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGradientFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkGradientFilter
 * @brief   A general filter for gradient estimation.
 *
 *
 * Estimates the gradient of a field in a data set.  The gradient calculation
 * is dependent on the input dataset type.  The created gradient array is
 * of the same type as the array it is calculated from (e.g. point data or cell
 * data) but the data type will be either float or double.  At the boundary
 * the gradient is not central differencing.  The output gradient array has
 * 3*number of components of the input data array.  The ordering for the
 * output gradient tuple will be {du/dx, du/dy, du/dz, dv/dx, dv/dy, dv/dz, dw/dx,
 * dw/dy, dw/dz} for an input array {u, v, w}. There are also the options
 * to additionally compute the vorticity and Q criterion of a vector field.
 * Unstructured grids and polydata can have cells of different dimensions.
 * To handle different use cases in this situation, the user can specify
 * which cells contribute to the computation. The options for this are
 * All (default), Patch and DataSetMax. Patch uses only the highest dimension
 * cells attached to a point. DataSetMax uses the highest cell dimension in
 * the entire data set. For Patch or DataSetMax it is possible that some values
 * will not be computed. The ReplacementValueOption specifies what to use
 * for these values.
 */

#ifndef svtkGradientFilter_h
#define svtkGradientFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkGradientFilter : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkGradientFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /// Options to choose what cells contribute to the gradient calculation
  enum ContributingCellEnum
  {
    All = 0,   //!< All cells
    Patch = 1, //!< Highest dimension cells in the patch of cells contributing to the calculation
    DataSetMax = 2 //!< Highest dimension cells in the data set
  };

  /// The replacement value or entities that don't have any gradient computed over them based on
  /// the ContributingCellOption
  enum ReplacementValueEnum
  {
    Zero = 0,        //!< 0
    NaN = 1,         //!< NaN
    DataTypeMin = 2, //!< The minimum possible value of the input array data type
    DataTypeMax = 3  //!< The maximum possible value of the input array data type
  };

  static svtkGradientFilter* New();

  //@{
  /**
   * These are basically a convenience method that calls SetInputArrayToProcess
   * to set the array used as the input scalars.  The fieldAssociation comes
   * from the svtkDataObject::FieldAssociations enum.  The fieldAttributeType
   * comes from the svtkDataSetAttributes::AttributeTypes enum.
   */
  virtual void SetInputScalars(int fieldAssociation, const char* name);
  virtual void SetInputScalars(int fieldAssociation, int fieldAttributeType);
  //@}

  //@{
  /**
   * Get/Set the name of the gradient array to create.  This is only
   * used if ComputeGradient is non-zero. If nullptr (the
   * default) then the output array will be named "Gradients".
   */
  svtkGetStringMacro(ResultArrayName);
  svtkSetStringMacro(ResultArrayName);
  //@}

  //@{
  /**
   * Get/Set the name of the divergence array to create. This is only
   * used if ComputeDivergence is non-zero. If nullptr (the
   * default) then the output array will be named "Divergence".
   */
  svtkGetStringMacro(DivergenceArrayName);
  svtkSetStringMacro(DivergenceArrayName);
  //@}

  //@{
  /**
   * Get/Set the name of the vorticity array to create. This is only
   * used if ComputeVorticity is non-zero. If nullptr (the
   * default) then the output array will be named "Vorticity".
   */
  svtkGetStringMacro(VorticityArrayName);
  svtkSetStringMacro(VorticityArrayName);
  //@}

  //@{
  /**
   * Get/Set the name of the Q criterion array to create. This is only
   * used if ComputeQCriterion is non-zero. If nullptr (the
   * default) then the output array will be named "Q-criterion".
   */
  svtkGetStringMacro(QCriterionArrayName);
  svtkSetStringMacro(QCriterionArrayName);
  //@}

  //@{
  /**
   * When this flag is on (default is off), the gradient filter will provide a
   * less accurate (but close) algorithm that performs fewer derivative
   * calculations (and is therefore faster).  The error contains some smoothing
   * of the output data and some possible errors on the boundary.  This
   * parameter has no effect when performing the gradient of cell data.
   * This only applies if the input grid is a svtkUnstructuredGrid or a
   * svtkPolyData.
   */
  svtkGetMacro(FasterApproximation, svtkTypeBool);
  svtkSetMacro(FasterApproximation, svtkTypeBool);
  svtkBooleanMacro(FasterApproximation, svtkTypeBool);
  //@}

  //@{
  /**
   * Add the gradient to the output field data.  The name of the array
   * will be ResultArrayName and will be the same type as the input
   * array. The default is on.
   */
  svtkSetMacro(ComputeGradient, svtkTypeBool);
  svtkGetMacro(ComputeGradient, svtkTypeBool);
  svtkBooleanMacro(ComputeGradient, svtkTypeBool);
  //@}

  //@{
  /**
   * Add divergence to the output field data.  The name of the array
   * will be DivergenceArrayName and will be the same type as the input
   * array.  The input array must have 3 components in order to
   * compute this. The default is off.
   */
  svtkSetMacro(ComputeDivergence, svtkTypeBool);
  svtkGetMacro(ComputeDivergence, svtkTypeBool);
  svtkBooleanMacro(ComputeDivergence, svtkTypeBool);
  //@}

  //@{
  /**
   * Add voriticity/curl to the output field data.  The name of the array
   * will be VorticityArrayName and will be the same type as the input
   * array.  The input array must have 3 components in order to
   * compute this. The default is off.
   */
  svtkSetMacro(ComputeVorticity, svtkTypeBool);
  svtkGetMacro(ComputeVorticity, svtkTypeBool);
  svtkBooleanMacro(ComputeVorticity, svtkTypeBool);
  //@}

  //@{
  /**
   * Add Q-criterion to the output field data.  The name of the array
   * will be QCriterionArrayName and will be the same type as the input
   * array.  The input array must have 3 components in order to
   * compute this.  Note that Q-citerion is a balance of the rate
   * of vorticity and the rate of strain. The default is off.
   */
  svtkSetMacro(ComputeQCriterion, svtkTypeBool);
  svtkGetMacro(ComputeQCriterion, svtkTypeBool);
  svtkBooleanMacro(ComputeQCriterion, svtkTypeBool);
  //@}

  //@{
  /**
   * Option to specify what cells to include in the gradient computation.
   * Options are all cells (All, Patch and DataSetMax). The default is All.
   */
  svtkSetClampMacro(ContributingCellOption, int, 0, 2);
  svtkGetMacro(ContributingCellOption, int);
  //@}

  //@{
  /**
   * Option to specify what replacement value or entities that don't have any gradient computed
   * over them based on the ContributingCellOption. Options are (Zero, NaN, DataTypeMin,
   * DataTypeMax). The default is Zero.
   */
  svtkSetClampMacro(ReplacementValueOption, int, 0, 3);
  svtkGetMacro(ReplacementValueOption, int);
  //@}

protected:
  svtkGradientFilter();
  ~svtkGradientFilter() override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Compute the gradients for grids that are not a svtkImageData,
   * svtkRectilinearGrid, or svtkStructuredGrid.
   * Returns non-zero if the operation was successful.
   */
  virtual int ComputeUnstructuredGridGradient(svtkDataArray* Array, int fieldAssociation,
    svtkDataSet* input, bool computeVorticity, bool computeQCriterion, bool computeDivergence,
    svtkDataSet* output);

  /**
   * Compute the gradients for either a svtkImageData, svtkRectilinearGrid or
   * a svtkStructuredGrid.  Computes the gradient using finite differences.
   * Returns non-zero if the operation was successful.
   */
  virtual int ComputeRegularGridGradient(svtkDataArray* Array, int fieldAssociation,
    bool computeVorticity, bool computeQCriterion, bool computeDivergence, svtkDataSet* output);

  /**
   * Get the proper array type to compute requested derivative quantities for.
   * If the input array is an unsigned type then we switch to a float array
   * for the output but otherwise the output array type is the same as the
   * input array type.
   **/
  int GetOutputArrayType(svtkDataArray* inputArray);

  /**
   * If non-null then it contains the name of the outputted gradient array.
   * By derault it is "Gradients".
   */
  char* ResultArrayName;

  /**
   * If non-null then it contains the name of the outputted divergence array.
   * By derault it is "Divergence".
   */
  char* DivergenceArrayName;

  /**
   * If non-null then it contains the name of the outputted vorticity array.
   * By derault it is "Vorticity".
   */
  char* VorticityArrayName;

  /**
   * If non-null then it contains the name of the outputted Q criterion array.
   * By derault it is "Q-criterion".
   */
  char* QCriterionArrayName;

  /**
   * When this flag is on (default is off), the gradient filter will provide a
   * less accurate (but close) algorithm that performs fewer derivative
   * calculations (and is therefore faster).  The error contains some smoothing
   * of the output data and some possible errors on the boundary.  This
   * parameter has no effect when performing the gradient of cell data.
   * This only applies if the input grid is a svtkUnstructuredGrid or a
   * svtkPolyData.
   */
  svtkTypeBool FasterApproximation;

  /**
   * Flag to indicate that the gradient of the input vector is to
   * be computed. By default ComputeDivergence is on.
   */
  svtkTypeBool ComputeGradient;

  /**
   * Flag to indicate that the divergence of the input vector is to
   * be computed.  The input array to be processed must have
   * 3 components.  By default ComputeDivergence is off.
   */
  svtkTypeBool ComputeDivergence;

  /**
   * Flag to indicate that the Q-criterion of the input vector is to
   * be computed.  The input array to be processed must have
   * 3 components.  By default ComputeQCriterion is off.
   */
  svtkTypeBool ComputeQCriterion;

  /**
   * Flag to indicate that vorticity/curl of the input vector is to
   * be computed.  The input array to be processed must have
   * 3 components.  By default ComputeVorticity is off.
   */
  svtkTypeBool ComputeVorticity;

  /**
   * Option to specify what cells to include in the gradient computation.
   * Options are all cells (All, Patch and DataSet). The default is all.
   */
  int ContributingCellOption;

  /**
   * Option to specify what replacement value or entities that don't have any gradient computed
   * over them based on the ContributingCellOption. Options are (Zero, NaN, DataTypeMin,
   * DataTypeMax). The default is Zero.
   */
  int ReplacementValueOption;

private:
  svtkGradientFilter(const svtkGradientFilter&) = delete;
  void operator=(const svtkGradientFilter&) = delete;
};

#endif //_svtkGradientFilter_h
