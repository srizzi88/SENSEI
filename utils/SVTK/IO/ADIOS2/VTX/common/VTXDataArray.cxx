/*=========================================================================

 Program:   Visualization Toolkit
 Module:    VTXDataArray.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 * VTXDataArray.cxx
 *
 *  Created on: Jun 4, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VTXDataArray.h"

#include <algorithm> //std::copy

#include "svtkDoubleArray.h"

namespace vtx
{
namespace types
{

bool DataArray::IsScalar() const noexcept
{
  if (this->VectorVariables.empty())
  {
    return true;
  }
  return false;
}

void DataArray::ConvertTo3DSVTK(const std::vector<double>& fillValues)
{
  const int components = this->Data->GetNumberOfComponents();
  if (this->Data->GetDataType() != SVTK_DOUBLE)
  {
    return;
  }

  if (components == 2 || components == 1)
  {
    // copy contents to a temporary
    double* data = svtkDoubleArray::SafeDownCast(this->Data.GetPointer())->GetPointer(0);
    const size_t size = static_cast<size_t>(this->Data->GetSize());
    // using raw memory to just use new and copy without initializing
    double* temporary = new double[size];
    std::copy(data, data + size, temporary);

    // reallocate
    const size_t tuples = static_cast<size_t>(this->Data->GetNumberOfTuples());
    const size_t newSize = 3 * tuples;

    const double fillValue = (fillValues.size() == 1) ? fillValues.front() : 0.;

    // set svtkDataArray to 3D
    this->Data->Reset();
    this->Data->Allocate(static_cast<svtkIdType>(newSize));
    this->Data->SetNumberOfComponents(3);
    this->Data->SetNumberOfTuples(static_cast<svtkIdType>(tuples));

    for (size_t t = 0; t < tuples; ++t)
    {
      const svtkIdType tSVTK = static_cast<svtkIdType>(t);
      this->Data->SetComponent(static_cast<svtkIdType>(tSVTK), 0, temporary[2 * tSVTK]);

      if (components == 2)
      {
        this->Data->SetComponent(tSVTK, 1, temporary[2 * tSVTK + 1]);
      }
      else if (components == 1)
      {
        this->Data->SetComponent(tSVTK, 1, fillValue);
      }

      this->Data->SetComponent(tSVTK, 2, fillValue);
    }

    delete[] temporary;
  }

  // swap tuples with components and swap data
  if (IsSOA)
  {
    double* data = svtkDoubleArray::SafeDownCast(this->Data.GetPointer())->GetPointer(0);
    const size_t size = static_cast<size_t>(this->Data->GetSize());
    // using raw memory to just use new and copy without initializing
    double* temporary = new double[size];
    std::copy(data, data + size, temporary);

    // set svtkDataArray to 3D
    const size_t tuples = static_cast<size_t>(this->Data->GetNumberOfComponents());
    this->Data->SetNumberOfComponents(3);
    this->Data->SetNumberOfTuples(tuples);

    for (size_t t = 0; t < tuples; ++t)
    {
      const svtkIdType tSVTK = static_cast<svtkIdType>(t);
      this->Data->SetComponent(tSVTK, 0, temporary[3 * tSVTK]);
      this->Data->SetComponent(tSVTK, 1, temporary[3 * tSVTK + 1]);
      this->Data->SetComponent(tSVTK, 2, temporary[3 * tSVTK + 2]);
    }

    delete[] temporary;
  }
}

} // end namespace types
} // end namespace vtx
