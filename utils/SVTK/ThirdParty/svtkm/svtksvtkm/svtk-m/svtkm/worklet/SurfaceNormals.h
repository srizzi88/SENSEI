//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_SurfaceNormals_h
#define svtk_m_worklet_SurfaceNormals_h


#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/CellTraits.h>
#include <svtkm/TypeTraits.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/cont/VariantArrayHandle.h>

namespace svtkm
{
namespace worklet
{

namespace detail
{
struct PassThrough
{
  template <typename T>
  SVTKM_EXEC svtkm::Vec<T, 3> operator()(const svtkm::Vec<T, 3>& in) const
  {
    return in;
  }
};

struct Normal
{
  template <typename T>
  SVTKM_EXEC svtkm::Vec<T, 3> operator()(const svtkm::Vec<T, 3>& in) const
  {
    return svtkm::Normal(in);
  }
};
} // detail

class FacetedSurfaceNormals
{
public:
  template <typename NormalFnctr = detail::Normal>
  class Worklet : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint points, FieldOutCell normals);
    using ExecutionSignature = void(CellShape, _2, _3);

    using InputDomain = _1;

    template <typename CellShapeTag, typename PointsVecType, typename T>
    SVTKM_EXEC void operator()(CellShapeTag,
                              const PointsVecType& points,
                              svtkm::Vec<T, 3>& normal) const
    {
      using CTraits = svtkm::CellTraits<CellShapeTag>;
      const auto tag = typename CTraits::TopologicalDimensionsTag();
      this->Compute(tag, points, normal);
    }

    template <svtkm::IdComponent Dim, typename PointsVecType, typename T>
    SVTKM_EXEC void Compute(svtkm::CellTopologicalDimensionsTag<Dim>,
                           const PointsVecType&,
                           svtkm::Vec<T, 3>& normal) const
    {
      normal = svtkm::TypeTraits<svtkm::Vec<T, 3>>::ZeroInitialization();
    }

    template <typename PointsVecType, typename T>
    SVTKM_EXEC void Compute(svtkm::CellTopologicalDimensionsTag<2>,
                           const PointsVecType& points,
                           svtkm::Vec<T, 3>& normal) const
    {
      normal = this->Normal(svtkm::Cross(points[2] - points[1], points[0] - points[1]));
    }



    template <typename PointsVecType, typename T>
    SVTKM_EXEC void operator()(svtkm::CellShapeTagGeneric shape,
                              const PointsVecType& points,
                              svtkm::Vec<T, 3>& normal) const
    {
      switch (shape.Id)
      {
        svtkmGenericCellShapeMacro(this->operator()(CellShapeTag(), points, normal));
        default:
          this->RaiseError("unknown cell type");
          break;
      }
    }

  private:
    NormalFnctr Normal;
  };

  FacetedSurfaceNormals()
    : Normalize(true)
  {
  }

  /// Set/Get if the results should be normalized
  void SetNormalize(bool value) { this->Normalize = value; }
  bool GetNormalize() const { return this->Normalize; }

  template <typename CellSetType,
            typename CoordsCompType,
            typename CoordsStorageType,
            typename NormalCompType>
  void Run(const CellSetType& cellset,
           const svtkm::cont::ArrayHandle<svtkm::Vec<CoordsCompType, 3>, CoordsStorageType>& points,
           svtkm::cont::ArrayHandle<svtkm::Vec<NormalCompType, 3>>& normals)
  {
    if (this->Normalize)
    {
      svtkm::worklet::DispatcherMapTopology<Worklet<>>().Invoke(cellset, points, normals);
    }
    else
    {
      svtkm::worklet::DispatcherMapTopology<Worklet<detail::PassThrough>>().Invoke(
        cellset, points, normals);
    }
  }

  template <typename CellSetType, typename NormalCompType>
  void Run(const CellSetType& cellset,
           const svtkm::cont::VariantArrayHandleBase<svtkm::TypeListFieldVec3>& points,
           svtkm::cont::ArrayHandle<svtkm::Vec<NormalCompType, 3>>& normals)
  {
    if (this->Normalize)
    {
      svtkm::worklet::DispatcherMapTopology<Worklet<>>().Invoke(cellset, points, normals);
    }
    else
    {
      svtkm::worklet::DispatcherMapTopology<Worklet<detail::PassThrough>>().Invoke(
        cellset, points, normals);
    }
  }

private:
  bool Normalize;
};

class SmoothSurfaceNormals
{
public:
  class Worklet : public svtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInCell faceNormals,
                                  FieldOutPoint pointNormals);
    using ExecutionSignature = void(CellCount, _2, _3);

    using InputDomain = _1;

    template <typename FaceNormalsVecType, typename T>
    SVTKM_EXEC void operator()(svtkm::IdComponent numCells,
                              const FaceNormalsVecType& faceNormals,
                              svtkm::Vec<T, 3>& pointNormal) const
    {
      if (numCells == 0)
      {
        pointNormal = svtkm::TypeTraits<svtkm::Vec<T, 3>>::ZeroInitialization();
      }
      else
      {
        auto result = faceNormals[0];
        for (svtkm::IdComponent i = 1; i < numCells; ++i)
        {
          result += faceNormals[i];
        }
        pointNormal = svtkm::Normal(result);
      }
    }
  };

  template <typename CellSetType, typename NormalCompType, typename FaceNormalStorageType>
  void Run(
    const CellSetType& cellset,
    const svtkm::cont::ArrayHandle<svtkm::Vec<NormalCompType, 3>, FaceNormalStorageType>& faceNormals,
    svtkm::cont::ArrayHandle<svtkm::Vec<NormalCompType, 3>>& pointNormals)
  {
    svtkm::worklet::DispatcherMapTopology<Worklet>().Invoke(cellset, faceNormals, pointNormals);
  }

  template <typename CellSetType, typename FaceNormalTypeList, typename NormalCompType>
  void Run(const CellSetType& cellset,
           const svtkm::cont::VariantArrayHandleBase<FaceNormalTypeList>& faceNormals,
           svtkm::cont::ArrayHandle<svtkm::Vec<NormalCompType, 3>>& pointNormals)
  {
    svtkm::worklet::DispatcherMapTopology<Worklet>().Invoke(cellset, faceNormals, pointNormals);
  }
};
}
} // svtkm::worklet

#endif // svtk_m_worklet_SurfaceNormals_h
