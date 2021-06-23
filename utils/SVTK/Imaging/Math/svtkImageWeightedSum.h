/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageWeightedSum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageWeightedSum
 * @brief    adds any number of images, weighting
 * each according to the weight set using this->SetWeights(i,w).
 *
 *
 * All weights are normalized so they will sum to 1.
 * Images must have the same extents. Output is
 *
 * @par Thanks:
 * The original author of this class is Lauren O'Donnell (MIT) for Slicer
 */

#ifndef svtkImageWeightedSum_h
#define svtkImageWeightedSum_h

#include "svtkImagingMathModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class svtkDoubleArray;
class SVTKIMAGINGMATH_EXPORT svtkImageWeightedSum : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageWeightedSum* New();
  svtkTypeMacro(svtkImageWeightedSum, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The weights control the contribution of each input to the sum.
   * They will be normalized to sum to 1 before filter execution.
   */
  virtual void SetWeights(svtkDoubleArray*);
  svtkGetObjectMacro(Weights, svtkDoubleArray);
  //@}

  /**
   * Change a specific weight. Reallocation is done
   */
  virtual void SetWeight(svtkIdType id, double weight);

  //@{
  /**
   * Setting NormalizeByWeight on will divide the
   * final result by the total weight of the component functions.
   * This process does not otherwise normalize the weighted sum
   * By default, NormalizeByWeight is on.
   */
  svtkGetMacro(NormalizeByWeight, svtkTypeBool);
  svtkSetClampMacro(NormalizeByWeight, svtkTypeBool, 0, 1);
  svtkBooleanMacro(NormalizeByWeight, svtkTypeBool);
  //@}

  /**
   * Compute the total value of all the weight
   */
  double CalculateTotalWeight();

protected:
  svtkImageWeightedSum();
  ~svtkImageWeightedSum() override;

  // Array to hold all the weights
  svtkDoubleArray* Weights;

  // Boolean flag to divide by sum or not
  svtkTypeBool NormalizeByWeight;

  int RequestInformation(svtkInformation* svtkNotUsed(request),
    svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;
  int FillInputPortInformation(int i, svtkInformation* info) override;

private:
  svtkImageWeightedSum(const svtkImageWeightedSum&) = delete;
  void operator=(const svtkImageWeightedSum&) = delete;
};

#endif
