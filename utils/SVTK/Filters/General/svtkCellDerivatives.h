/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellDerivatives.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellDerivatives
 * @brief   compute derivatives of scalars and vectors
 *
 * svtkCellDerivatives is a filter that computes derivatives of scalars
 * and vectors at the center of cells. You can choose to generate
 * different output including the scalar gradient (a vector), computed
 * tensor vorticity (a vector), gradient of input vectors (a tensor),
 * and strain matrix (linearized or Green-Lagrange) of the input vectors
 * (a tensor); or you may choose to pass data through to the output.
 *
 * Note that it is assumed that on input scalars and vector point data
 * is available, which are then used to generate cell vectors and tensors.
 * (The interpolation functions of the cells are used to compute the
 * derivatives which is why point data is required.)
 *
 * Note that the tensor components used to be sent out in column, but they
 * are now sent out not in row.
 *
 * @warning
 * The computed derivatives are cell attribute data; you can convert them to
 * point attribute data by using the svtkCellDataToPointData filter.
 * Note that, due to the interpolation function used (obtained using
 * 1/r**2 normalized sum), the derivatives calculated for polygons
 * with more than 4 vertices are inaccurate in most cases.
 *
 * @warning
 * The point data is passed through the filter to the output.
 *
 * @sa
 * svtkVectorNorm
 */

#ifndef svtkCellDerivatives_h
#define svtkCellDerivatives_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

#define SVTK_VECTOR_MODE_PASS_VECTORS 0
#define SVTK_VECTOR_MODE_COMPUTE_GRADIENT 1
#define SVTK_VECTOR_MODE_COMPUTE_VORTICITY 2

#define SVTK_TENSOR_MODE_PASS_TENSORS 0
#define SVTK_TENSOR_MODE_COMPUTE_GRADIENT 1
#define SVTK_TENSOR_MODE_COMPUTE_STRAIN 2
#define SVTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN 3

class SVTKFILTERSGENERAL_EXPORT svtkCellDerivatives : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkCellDerivatives, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct to compute the gradient of the scalars and vectors.
   */
  static svtkCellDerivatives* New();

  //@{
  /**
   * Control how the filter works to generate vector cell data. You
   * can choose to pass the input cell vectors, compute the gradient
   * of the input scalars, or extract the vorticity of the computed
   * vector gradient tensor. By default (VectorModeToComputeGradient),
   * the filter will take the gradient of the input scalar data.
   */
  svtkSetMacro(VectorMode, int);
  svtkGetMacro(VectorMode, int);
  void SetVectorModeToPassVectors() { this->SetVectorMode(SVTK_VECTOR_MODE_PASS_VECTORS); }
  void SetVectorModeToComputeGradient() { this->SetVectorMode(SVTK_VECTOR_MODE_COMPUTE_GRADIENT); }
  void SetVectorModeToComputeVorticity() { this->SetVectorMode(SVTK_VECTOR_MODE_COMPUTE_VORTICITY); }
  const char* GetVectorModeAsString();
  //@}

  //@{
  /**
   * Control how the filter works to generate tensor cell data. You can
   * choose to pass the input cell tensors, compute the gradient of
   * the input vectors, or compute the strain tensor (linearized or
   * Green-Lagrange strain)of the vector gradient tensor. By default
   * (TensorModeToComputeGradient), the filter will take the gradient
   * of the vector data to construct a tensor.
   */
  svtkSetMacro(TensorMode, int);
  svtkGetMacro(TensorMode, int);
  void SetTensorModeToPassTensors() { this->SetTensorMode(SVTK_TENSOR_MODE_PASS_TENSORS); }
  void SetTensorModeToComputeGradient() { this->SetTensorMode(SVTK_TENSOR_MODE_COMPUTE_GRADIENT); }
  void SetTensorModeToComputeStrain() { this->SetTensorMode(SVTK_TENSOR_MODE_COMPUTE_STRAIN); }
  void SetTensorModeToComputeGreenLagrangeStrain()
  {
    this->SetTensorMode(SVTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN);
  }
  const char* GetTensorModeAsString();
  //@}

protected:
  svtkCellDerivatives();
  ~svtkCellDerivatives() override {}
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int VectorMode;
  int TensorMode;

private:
  svtkCellDerivatives(const svtkCellDerivatives&) = delete;
  void operator=(const svtkCellDerivatives&) = delete;
};

#endif
