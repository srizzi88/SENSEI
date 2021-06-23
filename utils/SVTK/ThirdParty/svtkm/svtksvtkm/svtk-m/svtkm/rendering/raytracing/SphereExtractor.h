//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Sphere_Extractor_h
#define svtk_m_rendering_raytracing_Sphere_Extractor_h

#include <svtkm/cont/DataSet.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class SphereExtractor
{
protected:
  svtkm::cont::ArrayHandle<svtkm::Id> PointIds;
  svtkm::cont::ArrayHandle<svtkm::Float32> Radii;

public:
  //
  // Extract all nodes using a constant radius
  //
  void ExtractCoordinates(const svtkm::cont::CoordinateSystem& coords, const svtkm::Float32 radius);

  //
  // Set radius based on scalar field values. Each is interpolated from min to max
  //
  void ExtractCoordinates(const svtkm::cont::CoordinateSystem& coords,
                          const svtkm::cont::Field& field,
                          const svtkm::Float32 minRadius,
                          const svtkm::Float32 maxRadius);

  //
  // Extract all vertex shapes with constant radius
  //
  void ExtractCells(const svtkm::cont::DynamicCellSet& cells, svtkm::Float32 radius);

  //
  // Extract all vertex elements with radius based on scalar values
  //
  void ExtractCells(const svtkm::cont::DynamicCellSet& cells,
                    const svtkm::cont::Field& field,
                    const svtkm::Float32 minRadius,
                    const svtkm::Float32 maxRadius);


  svtkm::cont::ArrayHandle<svtkm::Id> GetPointIds();
  svtkm::cont::ArrayHandle<svtkm::Float32> GetRadii();
  svtkm::Id GetNumberOfSpheres() const;

protected:
  void SetUniformRadius(const svtkm::Float32 radius);
  void SetVaryingRadius(const svtkm::Float32 minRadius,
                        const svtkm::Float32 maxRadius,
                        const svtkm::cont::Field& field);

  void SetPointIdsFromCoords(const svtkm::cont::CoordinateSystem& coords);
  void SetPointIdsFromCells(const svtkm::cont::DynamicCellSet& cells);

}; // class ShapeIntersector
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_Shape_Extractor_h
