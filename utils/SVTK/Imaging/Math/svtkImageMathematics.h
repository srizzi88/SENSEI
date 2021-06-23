/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMathematics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMathematics
 * @brief   Add, subtract, multiply, divide, invert, sin,
 * cos, exp, log.
 *
 * svtkImageMathematics implements basic mathematic operations SetOperation is
 * used to select the filters behavior.  The filter can take two or one
 * input.
 */

#ifndef svtkImageMathematics_h
#define svtkImageMathematics_h

// Operation options.
#define SVTK_ADD 0
#define SVTK_SUBTRACT 1
#define SVTK_MULTIPLY 2
#define SVTK_DIVIDE 3
#define SVTK_INVERT 4
#define SVTK_SIN 5
#define SVTK_COS 6
#define SVTK_EXP 7
#define SVTK_LOG 8
#define SVTK_ABS 9
#define SVTK_SQR 10
#define SVTK_SQRT 11
#define SVTK_MIN 12
#define SVTK_MAX 13
#define SVTK_ATAN 14
#define SVTK_ATAN2 15
#define SVTK_MULTIPLYBYK 16
#define SVTK_ADDC 17
#define SVTK_CONJUGATE 18
#define SVTK_COMPLEX_MULTIPLY 19
#define SVTK_REPLACECBYK 20

#include "svtkImagingMathModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGMATH_EXPORT svtkImageMathematics : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageMathematics* New();
  svtkTypeMacro(svtkImageMathematics, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the Operation to perform.
   */
  svtkSetMacro(Operation, int);
  svtkGetMacro(Operation, int);
  //@}

  /**
   * Set each pixel in the output image to the sum of the corresponding pixels
   * in Input1 and Input2.
   */
  void SetOperationToAdd() { this->SetOperation(SVTK_ADD); }

  /**
   * Set each pixel in the output image to the difference of the corresponding pixels
   * in Input1 and Input2 (output = Input1 - Input2).
   */
  void SetOperationToSubtract() { this->SetOperation(SVTK_SUBTRACT); }

  /**
   * Set each pixel in the output image to the product of the corresponding pixels
   * in Input1 and Input2.
   */
  void SetOperationToMultiply() { this->SetOperation(SVTK_MULTIPLY); }

  /**
   * Set each pixel in the output image to the quotient of the corresponding pixels
   * in Input1 and Input2 (Output = Input1 / Input2).
   */
  void SetOperationToDivide() { this->SetOperation(SVTK_DIVIDE); }

  void SetOperationToConjugate() { this->SetOperation(SVTK_CONJUGATE); }

  void SetOperationToComplexMultiply() { this->SetOperation(SVTK_COMPLEX_MULTIPLY); }

  /**
   * Set each pixel in the output image to 1 over the corresponding pixel
   * in Input1 and Input2 (output = 1 / Input1). Input2 is not used.
   */
  void SetOperationToInvert() { this->SetOperation(SVTK_INVERT); }

  /**
   * Set each pixel in the output image to the sine of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToSin() { this->SetOperation(SVTK_SIN); }

  /**
   * Set each pixel in the output image to the cosine of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToCos() { this->SetOperation(SVTK_COS); }

  /**
   * Set each pixel in the output image to the exponential of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToExp() { this->SetOperation(SVTK_EXP); }

  /**
   * Set each pixel in the output image to the log of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToLog() { this->SetOperation(SVTK_LOG); }

  /**
   * Set each pixel in the output image to the absolute value of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToAbsoluteValue() { this->SetOperation(SVTK_ABS); }

  /**
   * Set each pixel in the output image to the square of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToSquare() { this->SetOperation(SVTK_SQR); }

  /**
   * Set each pixel in the output image to the square root of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToSquareRoot() { this->SetOperation(SVTK_SQRT); }

  /**
   * Set each pixel in the output image to the minimum of the corresponding pixels
   * in Input1 and Input2. (Output = min(Input1, Input2))
   */
  void SetOperationToMin() { this->SetOperation(SVTK_MIN); }

  /**
   * Set each pixel in the output image to the maximum of the corresponding pixels
   * in Input1 and Input2. (Output = max(Input1, Input2))
   */
  void SetOperationToMax() { this->SetOperation(SVTK_MAX); }

  /**
   * Set each pixel in the output image to the arctangent of the corresponding pixel
   * in Input1. Input2 is not used.
   */
  void SetOperationToATAN() { this->SetOperation(SVTK_ATAN); }

  void SetOperationToATAN2() { this->SetOperation(SVTK_ATAN2); }

  /**
   * Set each pixel in the output image to the product of ConstantK with the
   * corresponding pixel in Input1. Input2 is not used.
   */
  void SetOperationToMultiplyByK() { this->SetOperation(SVTK_MULTIPLYBYK); }

  /**
   * Set each pixel in the output image to the product of ConstantC with the
   * corresponding pixel in Input1. Input2 is not used.
   */
  void SetOperationToAddConstant() { this->SetOperation(SVTK_ADDC); }

  /**
   * Find every pixel in Input1 that equals ConstantC and set the corresponding pixels
   * in the Output to ConstantK. Input2 is not used.
   */
  void SetOperationToReplaceCByK() { this->SetOperation(SVTK_REPLACECBYK); }

  //@{
  /**
   * A constant used by some operations (typically multiplicative). Default is 1.
   */
  svtkSetMacro(ConstantK, double);
  svtkGetMacro(ConstantK, double);
  //@}

  //@{
  /**
   * A constant used by some operations (typically additive). Default is 0.
   */
  svtkSetMacro(ConstantC, double);
  svtkGetMacro(ConstantC, double);
  //@}

  //@{
  /**
   * How to handle divide by zero. Default is 0.
   */
  svtkSetMacro(DivideByZeroToC, svtkTypeBool);
  svtkGetMacro(DivideByZeroToC, svtkTypeBool);
  svtkBooleanMacro(DivideByZeroToC, svtkTypeBool);
  //@}

  /**
   * Set the two inputs to this filter. For some operations, the second input
   * is not used.
   */
  virtual void SetInput1Data(svtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(svtkDataObject* in) { this->SetInputData(1, in); }

protected:
  svtkImageMathematics();
  ~svtkImageMathematics() override {}

  int Operation;
  double ConstantK;
  double ConstantC;
  svtkTypeBool DivideByZeroToC;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int threadId) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkImageMathematics(const svtkImageMathematics&) = delete;
  void operator=(const svtkImageMathematics&) = delete;
};

#endif
