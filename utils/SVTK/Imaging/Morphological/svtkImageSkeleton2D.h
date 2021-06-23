/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSkeleton2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSkeleton2D
 * @brief   Skeleton of 2D images.
 *
 * svtkImageSkeleton2D should leave only single pixel width lines
 * of non-zero-valued pixels (values of 1 are not allowed).
 * It works by erosion on a 3x3 neighborhood with special rules.
 * The number of iterations determines how far the filter can erode.
 * There are three pruning levels:
 *  prune == 0 will leave traces on all angles...
 *  prune == 1 will not leave traces on 135 degree angles, but will on 90.
 *  prune == 2 does not leave traces on any angles leaving only closed loops.
 * Prune defaults to zero. The output scalar type is the same as the input.
 */

#ifndef svtkImageSkeleton2D_h
#define svtkImageSkeleton2D_h

#include "svtkImageIterateFilter.h"
#include "svtkImagingMorphologicalModule.h" // For export macro

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageSkeleton2D : public svtkImageIterateFilter
{
public:
  static svtkImageSkeleton2D* New();
  svtkTypeMacro(svtkImageSkeleton2D, svtkImageIterateFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * When prune is on, only closed loops are left unchanged.
   */
  svtkSetMacro(Prune, svtkTypeBool);
  svtkGetMacro(Prune, svtkTypeBool);
  svtkBooleanMacro(Prune, svtkTypeBool);
  //@}

  /**
   * Sets the number of cycles in the erosion.
   */
  void SetNumberOfIterations(int num) override;

protected:
  svtkImageSkeleton2D();
  ~svtkImageSkeleton2D() override {}

  svtkTypeBool Prune;

  int IterativeRequestUpdateExtent(svtkInformation* in, svtkInformation* out) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inDataV, svtkImageData** outDataV,
    int outExt[6], int id) override;

private:
  svtkImageSkeleton2D(const svtkImageSkeleton2D&) = delete;
  void operator=(const svtkImageSkeleton2D&) = delete;
};

#endif
