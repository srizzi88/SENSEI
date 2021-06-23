/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageLogic.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageLogic
 * @brief   And, or, xor, nand, nor, not.
 *
 * svtkImageLogic implements basic logic operations.
 * SetOperation is used to select the filter's behavior.
 * The filter can take two or one input. Inputs must have the same type.
 */

#ifndef svtkImageLogic_h
#define svtkImageLogic_h

// Operation options.
#define SVTK_AND 0
#define SVTK_OR 1
#define SVTK_XOR 2
#define SVTK_NAND 3
#define SVTK_NOR 4
#define SVTK_NOT 5
#define SVTK_NOP 6

#include "svtkImagingMathModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGMATH_EXPORT svtkImageLogic : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageLogic* New();
  svtkTypeMacro(svtkImageLogic, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the Operation to perform.
   */
  svtkSetMacro(Operation, int);
  svtkGetMacro(Operation, int);
  void SetOperationToAnd() { this->SetOperation(SVTK_AND); }
  void SetOperationToOr() { this->SetOperation(SVTK_OR); }
  void SetOperationToXor() { this->SetOperation(SVTK_XOR); }
  void SetOperationToNand() { this->SetOperation(SVTK_NAND); }
  void SetOperationToNor() { this->SetOperation(SVTK_NOR); }
  void SetOperationToNot() { this->SetOperation(SVTK_NOT); }
  //@}

  //@{
  /**
   * Set the value to use for true in the output.
   */
  svtkSetMacro(OutputTrueValue, double);
  svtkGetMacro(OutputTrueValue, double);
  //@}

  /**
   * Set the Input1 of this filter.
   */
  virtual void SetInput1Data(svtkDataObject* input) { this->SetInputData(0, input); }

  /**
   * Set the Input2 of this filter.
   */
  virtual void SetInput2Data(svtkDataObject* input) { this->SetInputData(1, input); }

protected:
  svtkImageLogic();
  ~svtkImageLogic() override {}

  int Operation;
  double OutputTrueValue;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkImageLogic(const svtkImageLogic&) = delete;
  void operator=(const svtkImageLogic&) = delete;
};

#endif
