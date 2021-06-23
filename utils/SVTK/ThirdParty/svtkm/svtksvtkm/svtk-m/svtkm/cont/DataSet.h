//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_DataSet_h
#define svtk_m_cont_DataSet_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Field.h>
#include <svtkm/cont/VariantArrayHandle.h>

namespace svtkm
{
namespace cont
{

class SVTKM_CONT_EXPORT DataSet
{
public:
  SVTKM_CONT void Clear();

  /// Get the number of cells contained in this DataSet
  SVTKM_CONT svtkm::Id GetNumberOfCells() const;

  /// Get the number of points contained in this DataSet
  ///
  /// Note: All coordinate systems for a DataSet are expected
  /// to have the same number of points.
  SVTKM_CONT svtkm::Id GetNumberOfPoints() const;

  SVTKM_CONT void AddField(const Field& field) { this->Fields.push_back(field); }

  SVTKM_CONT
  const svtkm::cont::Field& GetField(svtkm::Id index) const;

  SVTKM_CONT
  svtkm::cont::Field& GetField(svtkm::Id index);

  SVTKM_CONT
  bool HasField(const std::string& name,
                svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::ANY) const
  {
    bool found = false;
    this->FindFieldIndex(name, assoc, found);
    return found;
  }

  SVTKM_CONT
  bool HasCellField(const std::string& name) const
  {
    bool found = false;
    this->FindFieldIndex(name, svtkm::cont::Field::Association::CELL_SET, found);
    return found;
  }

  SVTKM_CONT
  bool HasPointField(const std::string& name) const
  {
    bool found = false;
    this->FindFieldIndex(name, svtkm::cont::Field::Association::POINTS, found);
    return found;
  }


  /// Returns the first field that matches the provided name and association
  /// Will return -1 if no match is found
  SVTKM_CONT
  svtkm::Id GetFieldIndex(
    const std::string& name,
    svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::ANY) const;

  /// Returns the first field that matches the provided name and association
  /// Will throw an exception if no match is found
  //@{
  SVTKM_CONT
  const svtkm::cont::Field& GetField(
    const std::string& name,
    svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::ANY) const
  {
    return this->GetField(this->GetFieldIndex(name, assoc));
  }

  SVTKM_CONT
  svtkm::cont::Field& GetField(
    const std::string& name,
    svtkm::cont::Field::Association assoc = svtkm::cont::Field::Association::ANY)
  {
    return this->GetField(this->GetFieldIndex(name, assoc));
  }
  //@}

  /// Returns the first cell field that matches the provided name.
  /// Will throw an exception if no match is found
  //@{
  SVTKM_CONT
  const svtkm::cont::Field& GetCellField(const std::string& name) const
  {
    return this->GetField(name, svtkm::cont::Field::Association::CELL_SET);
  }

  SVTKM_CONT
  svtkm::cont::Field& GetCellField(const std::string& name)
  {
    return this->GetField(name, svtkm::cont::Field::Association::CELL_SET);
  }
  //@}

  /// Returns the first point field that matches the provided name.
  /// Will throw an exception if no match is found
  //@{
  SVTKM_CONT
  const svtkm::cont::Field& GetPointField(const std::string& name) const
  {
    return this->GetField(name, svtkm::cont::Field::Association::POINTS);
  }

  SVTKM_CONT
  svtkm::cont::Field& GetPointField(const std::string& name)
  {
    return this->GetField(name, svtkm::cont::Field::Association::POINTS);
  }
  //@}

  SVTKM_CONT
  void AddCoordinateSystem(const svtkm::cont::CoordinateSystem& cs)
  {
    this->CoordSystems.push_back(cs);
  }

  SVTKM_CONT
  bool HasCoordinateSystem(const std::string& name) const
  {
    return this->GetCoordinateSystemIndex(name) >= 0;
  }

  SVTKM_CONT
  const svtkm::cont::CoordinateSystem& GetCoordinateSystem(svtkm::Id index = 0) const;

  SVTKM_CONT
  svtkm::cont::CoordinateSystem& GetCoordinateSystem(svtkm::Id index = 0);

  /// Returns the index for the first CoordinateSystem whose
  /// name matches the provided string.
  /// Will return -1 if no match is found
  SVTKM_CONT
  svtkm::Id GetCoordinateSystemIndex(const std::string& name) const;

  /// Returns the first CoordinateSystem that matches the provided name.
  /// Will throw an exception if no match is found
  //@{
  SVTKM_CONT
  const svtkm::cont::CoordinateSystem& GetCoordinateSystem(const std::string& name) const;

  SVTKM_CONT
  svtkm::cont::CoordinateSystem& GetCoordinateSystem(const std::string& name);
  //@}

  SVTKM_CONT
  void SetCellSet(const svtkm::cont::DynamicCellSet& cellSet) { this->CellSet = cellSet; }

  template <typename CellSetType>
  SVTKM_CONT void SetCellSet(const CellSetType& cellSet)
  {
    SVTKM_IS_CELL_SET(CellSetType);
    this->CellSet = svtkm::cont::DynamicCellSet(cellSet);
  }

  SVTKM_CONT
  const svtkm::cont::DynamicCellSet& GetCellSet() const { return this->CellSet; }

  SVTKM_CONT
  svtkm::cont::DynamicCellSet& GetCellSet() { return this->CellSet; }

  SVTKM_CONT
  svtkm::IdComponent GetNumberOfFields() const
  {
    return static_cast<svtkm::IdComponent>(this->Fields.size());
  }

  SVTKM_CONT
  svtkm::IdComponent GetNumberOfCoordinateSystems() const
  {
    return static_cast<svtkm::IdComponent>(this->CoordSystems.size());
  }

  /// Copies the structure i.e. coordinates systems and cellset from the source
  /// dataset. The fields are left unchanged.
  SVTKM_CONT
  void CopyStructure(const svtkm::cont::DataSet& source);

  SVTKM_CONT
  void PrintSummary(std::ostream& out) const;

private:
  std::vector<svtkm::cont::CoordinateSystem> CoordSystems;
  std::vector<svtkm::cont::Field> Fields;
  svtkm::cont::DynamicCellSet CellSet;

  SVTKM_CONT
  svtkm::Id FindFieldIndex(const std::string& name,
                          svtkm::cont::Field::Association association,
                          bool& found) const;
};

} // namespace cont
} // namespace svtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename FieldTypeList = SVTKM_DEFAULT_TYPE_LIST,
          typename CellSetTypesList = SVTKM_DEFAULT_CELL_SET_LIST>
struct SerializableDataSet
{
  SerializableDataSet() = default;

  explicit SerializableDataSet(const svtkm::cont::DataSet& dataset)
    : DataSet(dataset)
  {
  }

  svtkm::cont::DataSet DataSet;
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename FieldTypeList, typename CellSetTypesList>
struct Serialization<svtkm::cont::SerializableDataSet<FieldTypeList, CellSetTypesList>>
{
private:
  using Type = svtkm::cont::SerializableDataSet<FieldTypeList, CellSetTypesList>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& serializable)
  {
    const auto& dataset = serializable.DataSet;

    svtkm::IdComponent numberOfCoordinateSystems = dataset.GetNumberOfCoordinateSystems();
    svtkmdiy::save(bb, numberOfCoordinateSystems);
    for (svtkm::IdComponent i = 0; i < numberOfCoordinateSystems; ++i)
    {
      svtkmdiy::save(bb, dataset.GetCoordinateSystem(i));
    }

    svtkmdiy::save(bb, dataset.GetCellSet().ResetCellSetList(CellSetTypesList{}));

    svtkm::IdComponent numberOfFields = dataset.GetNumberOfFields();
    svtkmdiy::save(bb, numberOfFields);
    for (svtkm::IdComponent i = 0; i < numberOfFields; ++i)
    {
      svtkmdiy::save(bb, svtkm::cont::SerializableField<FieldTypeList>(dataset.GetField(i)));
    }
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& serializable)
  {
    auto& dataset = serializable.DataSet;
    dataset = {}; // clear

    svtkm::IdComponent numberOfCoordinateSystems = 0;
    svtkmdiy::load(bb, numberOfCoordinateSystems);
    for (svtkm::IdComponent i = 0; i < numberOfCoordinateSystems; ++i)
    {
      svtkm::cont::CoordinateSystem coords;
      svtkmdiy::load(bb, coords);
      dataset.AddCoordinateSystem(coords);
    }

    svtkm::cont::DynamicCellSetBase<CellSetTypesList> cells;
    svtkmdiy::load(bb, cells);
    dataset.SetCellSet(svtkm::cont::DynamicCellSet(cells));

    svtkm::IdComponent numberOfFields = 0;
    svtkmdiy::load(bb, numberOfFields);
    for (svtkm::IdComponent i = 0; i < numberOfFields; ++i)
    {
      svtkm::cont::SerializableField<FieldTypeList> field;
      svtkmdiy::load(bb, field);
      dataset.AddField(field.Field);
    }
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_DataSet_h
