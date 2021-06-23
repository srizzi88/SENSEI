//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetSingleType_h
#define svtk_m_cont_CellSetSingleType_h

#include <svtkm/CellShape.h>
#include <svtkm/CellTraits.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/CellSetExplicit.h>

#include <map>
#include <utility>

namespace svtkm
{
namespace cont
{

//Only works with fixed sized cell sets

template <typename ConnectivityStorageTag = SVTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG>
class SVTKM_ALWAYS_EXPORT CellSetSingleType
  : public svtkm::cont::CellSetExplicit<
      typename svtkm::cont::ArrayHandleConstant<svtkm::UInt8>::StorageTag, //ShapesStorageTag
      ConnectivityStorageTag,
      typename svtkm::cont::ArrayHandleCounting<svtkm::Id>::StorageTag //OffsetsStorageTag
      >
{
  using Thisclass = svtkm::cont::CellSetSingleType<ConnectivityStorageTag>;
  using Superclass =
    svtkm::cont::CellSetExplicit<typename svtkm::cont::ArrayHandleConstant<svtkm::UInt8>::StorageTag,
                                ConnectivityStorageTag,
                                typename svtkm::cont::ArrayHandleCounting<svtkm::Id>::StorageTag>;

public:
  SVTKM_CONT
  CellSetSingleType()
    : Superclass()
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(CellShapeTagEmpty::Id)
    , NumberOfPointsPerCell(0)
  {
  }

  SVTKM_CONT
  CellSetSingleType(const Thisclass& src)
    : Superclass(src)
    , ExpectedNumberOfCellsAdded(-1)
    , CellShapeAsId(src.CellShapeAsId)
    , NumberOfPointsPerCell(src.NumberOfPointsPerCell)
  {
  }

  SVTKM_CONT
  CellSetSingleType(Thisclass&& src) noexcept : Superclass(std::forward<Superclass>(src)),
                                                ExpectedNumberOfCellsAdded(-1),
                                                CellShapeAsId(src.CellShapeAsId),
                                                NumberOfPointsPerCell(src.NumberOfPointsPerCell)
  {
  }


  SVTKM_CONT
  Thisclass& operator=(const Thisclass& src)
  {
    this->Superclass::operator=(src);
    this->CellShapeAsId = src.CellShapeAsId;
    this->NumberOfPointsPerCell = src.NumberOfPointsPerCell;
    return *this;
  }

  SVTKM_CONT
  Thisclass& operator=(Thisclass&& src) noexcept
  {
    this->Superclass::operator=(std::forward<Superclass>(src));
    this->CellShapeAsId = src.CellShapeAsId;
    this->NumberOfPointsPerCell = src.NumberOfPointsPerCell;
    return *this;
  }

  virtual ~CellSetSingleType() override {}

  /// First method to add cells -- one at a time.
  SVTKM_CONT
  void PrepareToAddCells(svtkm::Id numCells, svtkm::Id connectivityMaxLen)
  {
    this->CellShapeAsId = svtkm::CELL_SHAPE_EMPTY;

    this->Data->CellPointIds.Connectivity.Allocate(connectivityMaxLen);

    this->Data->NumberOfCellsAdded = 0;
    this->Data->ConnectivityAdded = 0;
    this->ExpectedNumberOfCellsAdded = numCells;
  }

  /// Second method to add cells -- one at a time.
  template <typename IdVecType>
  SVTKM_CONT void AddCell(svtkm::UInt8 shapeId, svtkm::IdComponent numVertices, const IdVecType& ids)
  {
    using Traits = svtkm::VecTraits<IdVecType>;
    SVTKM_STATIC_ASSERT_MSG((std::is_same<typename Traits::ComponentType, svtkm::Id>::value),
                           "CellSetSingleType::AddCell requires svtkm::Id for indices.");

    if (Traits::GetNumberOfComponents(ids) < numVertices)
    {
      throw svtkm::cont::ErrorBadValue("Not enough indices given to CellSetSingleType::AddCell.");
    }

    if (this->Data->ConnectivityAdded + numVertices >
        this->Data->CellPointIds.Connectivity.GetNumberOfValues())
    {
      throw svtkm::cont::ErrorBadValue(
        "Connectivity increased past estimated maximum connectivity.");
    }

    if (this->CellShapeAsId == svtkm::CELL_SHAPE_EMPTY)
    {
      if (shapeId == svtkm::CELL_SHAPE_EMPTY)
      {
        throw svtkm::cont::ErrorBadValue("Cannot create cells of type empty.");
      }
      this->CellShapeAsId = shapeId;
      this->CheckNumberOfPointsPerCell(numVertices);
      this->NumberOfPointsPerCell = numVertices;
    }
    else
    {
      if (shapeId != this->GetCellShape(0))
      {
        throw svtkm::cont::ErrorBadValue("Cannot have differing shapes in CellSetSingleType.");
      }
      if (numVertices != this->NumberOfPointsPerCell)
      {
        throw svtkm::cont::ErrorBadValue(
          "Inconsistent number of points in cells for CellSetSingleType.");
      }
    }
    auto conn = this->Data->CellPointIds.Connectivity.GetPortalControl();
    for (svtkm::IdComponent iVert = 0; iVert < numVertices; ++iVert)
    {
      conn.Set(this->Data->ConnectivityAdded + iVert, Traits::GetComponent(ids, iVert));
    }
    this->Data->NumberOfCellsAdded++;
    this->Data->ConnectivityAdded += numVertices;
  }

  /// Third and final method to add cells -- one at a time.
  SVTKM_CONT
  void CompleteAddingCells(svtkm::Id numPoints)
  {
    this->Data->NumberOfPoints = numPoints;
    this->CellPointIds.Connectivity.Shrink(this->ConnectivityAdded);

    svtkm::Id numCells = this->NumberOfCellsAdded;

    this->CellPointIds.Shapes =
      svtkm::cont::make_ArrayHandleConstant(this->GetCellShape(0), numCells);
    this->CellPointIds.IndexOffsets = svtkm::cont::make_ArrayHandleCounting(
      svtkm::Id(0), static_cast<svtkm::Id>(this->NumberOfPointsPerCell), numCells);

    this->CellPointIds.ElementsValid = true;

    if (this->ExpectedNumberOfCellsAdded != this->GetNumberOfCells())
    {
      throw svtkm::cont::ErrorBadValue("Did not add the expected number of cells.");
    }

    this->NumberOfCellsAdded = -1;
    this->ConnectivityAdded = -1;
    this->ExpectedNumberOfCellsAdded = -1;
  }

  //This is the way you can fill the memory from another system without copying
  SVTKM_CONT
  void Fill(svtkm::Id numPoints,
            svtkm::UInt8 shapeId,
            svtkm::IdComponent numberOfPointsPerCell,
            const svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityStorageTag>& connectivity)
  {
    this->Data->NumberOfPoints = numPoints;
    this->CellShapeAsId = shapeId;
    this->CheckNumberOfPointsPerCell(numberOfPointsPerCell);

    const svtkm::Id numCells = connectivity.GetNumberOfValues() / numberOfPointsPerCell;
    SVTKM_ASSERT((connectivity.GetNumberOfValues() % numberOfPointsPerCell) == 0);

    this->Data->CellPointIds.Shapes = svtkm::cont::make_ArrayHandleConstant(shapeId, numCells);

    this->Data->CellPointIds.Offsets = svtkm::cont::make_ArrayHandleCounting(
      svtkm::Id(0), static_cast<svtkm::Id>(numberOfPointsPerCell), numCells + 1);

    this->Data->CellPointIds.Connectivity = connectivity;

    this->Data->CellPointIds.ElementsValid = true;

    this->ResetConnectivity(TopologyElementTagPoint{}, TopologyElementTagCell{});
  }

  SVTKM_CONT
  svtkm::Id GetCellShapeAsId() const { return this->CellShapeAsId; }

  SVTKM_CONT
  svtkm::UInt8 GetCellShape(svtkm::Id svtkmNotUsed(cellIndex)) const override
  {
    return static_cast<svtkm::UInt8>(this->CellShapeAsId);
  }

  SVTKM_CONT
  std::shared_ptr<CellSet> NewInstance() const override
  {
    return std::make_shared<CellSetSingleType>();
  }

  SVTKM_CONT
  void DeepCopy(const CellSet* src) override
  {
    const auto* other = dynamic_cast<const CellSetSingleType*>(src);
    if (!other)
    {
      throw svtkm::cont::ErrorBadType("CellSetSingleType::DeepCopy types don't match");
    }

    this->Superclass::DeepCopy(other);
    this->CellShapeAsId = other->CellShapeAsId;
    this->NumberOfPointsPerCell = other->NumberOfPointsPerCell;
  }

  virtual void PrintSummary(std::ostream& out) const override
  {
    out << "   CellSetSingleType: Type=" << this->CellShapeAsId << std::endl;
    out << "   CellPointIds:" << std::endl;
    this->Data->CellPointIds.PrintSummary(out);
    out << "   PointCellIds:" << std::endl;
    this->Data->PointCellIds.PrintSummary(out);
  }

private:
  template <typename CellShapeTag>
  void CheckNumberOfPointsPerCell(CellShapeTag,
                                  svtkm::CellTraitsTagSizeFixed,
                                  svtkm::IdComponent numVertices) const
  {
    if (numVertices != svtkm::CellTraits<CellShapeTag>::NUM_POINTS)
    {
      throw svtkm::cont::ErrorBadValue("Passed invalid number of points for cell shape.");
    }
  }

  template <typename CellShapeTag>
  void CheckNumberOfPointsPerCell(CellShapeTag,
                                  svtkm::CellTraitsTagSizeVariable,
                                  svtkm::IdComponent svtkmNotUsed(numVertices)) const
  {
    // Technically, a shape with a variable number of points probably has a
    // minimum number of points, but we are not being sophisticated enough to
    // check that. Instead, just pass the check by returning without error.
  }

  void CheckNumberOfPointsPerCell(svtkm::IdComponent numVertices) const
  {
    switch (this->CellShapeAsId)
    {
      svtkmGenericCellShapeMacro(this->CheckNumberOfPointsPerCell(
        CellShapeTag(), svtkm::CellTraits<CellShapeTag>::IsSizeFixed(), numVertices));
      default:
        throw svtkm::cont::ErrorBadValue("CellSetSingleType unable to determine the cell type");
    }
  }

  svtkm::Id ExpectedNumberOfCellsAdded;
  svtkm::Id CellShapeAsId;
  svtkm::IdComponent NumberOfPointsPerCell;
};
}
} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename ConnectivityST>
struct SerializableTypeString<svtkm::cont::CellSetSingleType<ConnectivityST>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Single<" +
      SerializableTypeString<svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityST>>::Get() + "_ST>";

    return name;
  }
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename ConnectivityST>
struct Serialization<svtkm::cont::CellSetSingleType<ConnectivityST>>
{
private:
  using Type = svtkm::cont::CellSetSingleType<ConnectivityST>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    svtkmdiy::save(bb, cs.GetNumberOfPoints());
    svtkmdiy::save(bb, cs.GetCellShape(0));
    svtkmdiy::save(bb, cs.GetNumberOfPointsInCell(0));
    svtkmdiy::save(
      bb, cs.GetConnectivityArray(svtkm::TopologyElementTagCell{}, svtkm::TopologyElementTagPoint{}));
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    svtkm::Id numberOfPoints = 0;
    svtkmdiy::load(bb, numberOfPoints);
    svtkm::UInt8 shape;
    svtkmdiy::load(bb, shape);
    svtkm::IdComponent count;
    svtkmdiy::load(bb, count);
    svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityST> connectivity;
    svtkmdiy::load(bb, connectivity);

    cs = Type{};
    cs.Fill(numberOfPoints, shape, count, connectivity);
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_CellSetSingleType_h
