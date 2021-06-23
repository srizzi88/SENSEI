/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageProgressIterator.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkImageProgressIterator_txx
#define svtkImageProgressIterator_txx

#include "svtkAlgorithm.h"
#include "svtkImageData.h"
#include "svtkImageProgressIterator.h"

template <class DType>
svtkImageProgressIterator<DType>::svtkImageProgressIterator(
  svtkImageData* imgd, int* ext, svtkAlgorithm* po, int id)
  : svtkImageIterator<DType>(imgd, ext)
{
  this->Target = static_cast<unsigned long>((ext[5] - ext[4] + 1) * (ext[3] - ext[2] + 1) / 50.0);
  this->Target++;
  this->Count = 0;
  this->Count2 = 0;
  this->Algorithm = po;
  this->ID = id;
}

template <class DType>
void svtkImageProgressIterator<DType>::NextSpan()
{
  this->Pointer += this->Increments[1];
  this->SpanEndPointer += this->Increments[1];
  if (this->Pointer >= this->SliceEndPointer)
  {
    this->Pointer += this->ContinuousIncrements[2];
    this->SpanEndPointer += this->ContinuousIncrements[2];
    this->SliceEndPointer += this->Increments[2];
  }
  if (!this->ID)
  {
    if (this->Count2 == this->Target)
    {
      this->Count += this->Count2;
      this->Algorithm->UpdateProgress(this->Count / (50.0 * this->Target));
      this->Count2 = 0;
    }
    this->Count2++;
  }
}

template <class DType>
svtkTypeBool svtkImageProgressIterator<DType>::IsAtEnd()
{
  if (this->Algorithm->GetAbortExecute())
  {
    return 1;
  }
  else
  {
    return this->Superclass::IsAtEnd();
  }
}

#endif
