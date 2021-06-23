/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageProgressIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageProgressIterator
 * @brief   a simple image iterator with progress
 *
 * This is a simple image iterator that can be used to iterate over an
 * image. Typically used to iterate over the output image
 *
 * @sa
 * svtkImageData svtkImageIterator
 */

#ifndef svtkImageProgressIterator_h
#define svtkImageProgressIterator_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkImageIterator.h"
class svtkAlgorithm;

template <class DType>
class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkImageProgressIterator : public svtkImageIterator<DType>
{
public:
  typedef svtkImageIterator<DType> Superclass;

  /**
   * Create a progress iterator for the provided image data
   * and extent to iterate over. The passes progress object will
   * receive any UpdateProgress calls if the thread id is zero
   */
  svtkImageProgressIterator(svtkImageData* imgd, int* ext, svtkAlgorithm* po, int id);

  /**
   * Move the iterator to the next span, may call UpdateProgress on the
   * filter (svtkAlgorithm)
   */
  void NextSpan();

  /**
   * Overridden from svtkImageIterator to check AbortExecute on the
   * filter (svtkAlgorithm).
   */
  svtkTypeBool IsAtEnd();

protected:
  svtkAlgorithm* Algorithm;
  unsigned long Count;
  unsigned long Count2;
  unsigned long Target;
  int ID;
};

#ifndef svtkImageProgressIterator_cxx
svtkExternTemplateMacro(
  extern template class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkImageProgressIterator);
#endif

#endif
// SVTK-HeaderTest-Exclude: svtkImageProgressIterator.h
