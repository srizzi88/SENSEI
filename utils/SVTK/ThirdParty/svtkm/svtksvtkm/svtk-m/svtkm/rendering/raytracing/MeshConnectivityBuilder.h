//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_MeshConnectivityBuilder_h
#define svtk_m_rendering_raytracing_MeshConnectivityBuilder_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/raytracing/MeshConnectivityContainers.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class MeshConnectivityBuilder
{
public:
  MeshConnectivityBuilder();
  ~MeshConnectivityBuilder();

  SVTKM_CONT
  MeshConnContainer* BuildConnectivity(const svtkm::cont::DynamicCellSet& cellset,
                                       const svtkm::cont::CoordinateSystem& coordinates);

  SVTKM_CONT
  svtkm::cont::ArrayHandle<svtkm::Id4> ExternalTrianglesStructured(
    svtkm::cont::CellSetStructured<3>& cellSetStructured);

  svtkm::cont::ArrayHandle<svtkm::Id> GetFaceConnectivity();

  svtkm::cont::ArrayHandle<svtkm::Id> GetFaceOffsets();

  svtkm::cont::ArrayHandle<svtkm::Id4> GetTriangles();

protected:
  SVTKM_CONT
  void BuildConnectivity(svtkm::cont::CellSetSingleType<>& cellSetUnstructured,
                         const svtkm::cont::ArrayHandleVirtualCoordinates& coordinates,
                         svtkm::Bounds coordsBounds);

  SVTKM_CONT
  void BuildConnectivity(svtkm::cont::CellSetExplicit<>& cellSetUnstructured,
                         const svtkm::cont::ArrayHandleVirtualCoordinates& coordinates,
                         svtkm::Bounds coordsBounds);

  svtkm::cont::ArrayHandle<svtkm::Id> FaceConnectivity;
  svtkm::cont::ArrayHandle<svtkm::Id> FaceOffsets;
  svtkm::cont::ArrayHandle<svtkm::Id4> Triangles;
};
}
}
} //namespace svtkm::rendering::raytracing
#endif
