//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Quad_Extractor_h
#define svtk_m_rendering_raytracing_Quad_Extractor_h

#include <svtkm/cont/DataSet.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class QuadExtractor
{
protected:
  svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Id, 5>> QuadIds;
  svtkm::cont::ArrayHandle<svtkm::Float32> Radii;

public:
  void ExtractCells(const svtkm::cont::DynamicCellSet& cells);

  svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Id, 5>> GetQuadIds();

  svtkm::Id GetNumberOfQuads() const;

protected:
  void SetQuadIdsFromCells(const svtkm::cont::DynamicCellSet& cells);

}; // class ShapeIntersector
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_Shape_Extractor_h
