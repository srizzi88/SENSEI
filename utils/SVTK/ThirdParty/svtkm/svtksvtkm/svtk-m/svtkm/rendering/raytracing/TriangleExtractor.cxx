//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/internal/RunTriangulator.h>
#include <svtkm/rendering/raytracing/TriangleExtractor.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

void TriangleExtractor::ExtractCells(const svtkm::cont::DynamicCellSet& cells)
{
  svtkm::Id numberOfTriangles;
  svtkm::rendering::internal::RunTriangulator(cells, this->Triangles, numberOfTriangles);
}

svtkm::cont::ArrayHandle<svtkm::Id4> TriangleExtractor::GetTriangles()
{
  return this->Triangles;
}

svtkm::Id TriangleExtractor::GetNumberOfTriangles() const
{
  return this->Triangles.GetNumberOfValues();
}
}
}
} //namespace svtkm::rendering::raytracing
