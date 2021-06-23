/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSlab.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSlab
 * @brief   combine image slices to form a slab image
 *
 * svtkImageSlab will combine all of the slices of an image to
 * create a single slice.  The slices can be combined with the
 * following operations: averaging, summation, minimum, maximum.
 * If you require an arbitrary angle of projection, you can use
 * svtkImageReslice.
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to SVTK.
 */

#ifndef svtkImageSlab_h
#define svtkImageSlab_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageSlab : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageSlab* New();
  svtkTypeMacro(svtkImageSlab, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the slice direction: zero for x, 1 for y, 2 for z.
   * The default is the Z direction.
   */
  svtkSetClampMacro(Orientation, int, 0, 2);
  void SetOrientationToX() { this->SetOrientation(0); }
  void SetOrientationToY() { this->SetOrientation(1); }
  void SetOrientationToZ() { this->SetOrientation(2); }
  svtkGetMacro(Orientation, int);
  //@}

  //@{
  /**
   * Set the range of slices to combine. The default is to project
   * through all slices.
   */
  svtkSetVector2Macro(SliceRange, int);
  svtkGetVector2Macro(SliceRange, int);
  //@}

  //@{
  /**
   * Set the operation to use when combining slices.  The choices are
   * "Mean", "Sum", "Min", "Max".  The default is "Mean".
   */
  svtkSetClampMacro(Operation, int, SVTK_IMAGE_SLAB_MIN, SVTK_IMAGE_SLAB_SUM);
  void SetOperationToMin() { this->SetOperation(SVTK_IMAGE_SLAB_MIN); }
  void SetOperationToMax() { this->SetOperation(SVTK_IMAGE_SLAB_MAX); }
  void SetOperationToMean() { this->SetOperation(SVTK_IMAGE_SLAB_MEAN); }
  void SetOperationToSum() { this->SetOperation(SVTK_IMAGE_SLAB_SUM); }
  svtkGetMacro(Operation, int);
  const char* GetOperationAsString();
  //@}

  //@{
  /**
   * Use trapezoid integration for slab computation.  This weighs the
   * first and last slices by half when doing sum and mean, as compared
   * to the default midpoint integration that weighs all slices equally.
   * It is off by default.
   */
  svtkSetMacro(TrapezoidIntegration, svtkTypeBool);
  svtkBooleanMacro(TrapezoidIntegration, svtkTypeBool);
  svtkGetMacro(TrapezoidIntegration, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on multi-slice output.  Each slice of the output will be
   * a projection through the specified range of input slices, e.g.
   * if the SliceRange is [0,3] then slice 'i' of the output will
   * be a projection through slices 'i' through '3+i' of the input.
   * This flag is off by default.
   */
  svtkSetMacro(MultiSliceOutput, svtkTypeBool);
  svtkBooleanMacro(MultiSliceOutput, svtkTypeBool);
  svtkGetMacro(MultiSliceOutput, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the output scalar type to float or double, to avoid
   * potential overflow when doing a summation operation.
   * The default is to use the scalar type of the input data,
   * and clamp the output to the range of the input scalar type.
   */
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(SVTK_FLOAT); }
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(SVTK_DOUBLE); }
  void SetOutputScalarTypeToInputScalarType() { this->SetOutputScalarType(0); }
  svtkGetMacro(OutputScalarType, int);
  //@}

protected:
  svtkImageSlab();
  ~svtkImageSlab() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData, int ext[6],
    int id) override;

  svtkSetMacro(OutputScalarType, int);

  int Operation;
  int Orientation;
  int SliceRange[2];
  int OutputScalarType;
  svtkTypeBool MultiSliceOutput;
  svtkTypeBool TrapezoidIntegration;

private:
  svtkImageSlab(const svtkImageSlab&) = delete;
  void operator=(const svtkImageSlab&) = delete;
};

#endif
