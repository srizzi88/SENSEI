//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_GhostCellRemove_hxx
#define svtk_m_filter_GhostCellRemove_hxx
#include <svtkm/filter/GhostCellRemove.h>

#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DynamicCellSet.h>

#include <svtkm/RangeId3.h>
#include <svtkm/filter/ExtractStructured.h>
#include <svtkm/worklet/CellDeepCopy.h>

namespace
{

class RemoveAllGhosts
{
public:
  SVTKM_CONT
  RemoveAllGhosts() {}

  SVTKM_EXEC bool operator()(const svtkm::UInt8& value) const { return (value == 0); }
};

class RemoveGhostByType
{
public:
  SVTKM_CONT
  RemoveGhostByType()
    : RemoveType(0)
  {
  }

  SVTKM_CONT
  RemoveGhostByType(const svtkm::UInt8& val)
    : RemoveType(static_cast<svtkm::UInt8>(~val))
  {
  }

  SVTKM_EXEC bool operator()(const svtkm::UInt8& value) const
  {
    return value == 0 || (value & RemoveType);
  }

private:
  svtkm::UInt8 RemoveType;
};

template <int DIMS>
SVTKM_EXEC_CONT svtkm::Id3 getLogical(const svtkm::Id& index, const svtkm::Id3& cellDims);

template <>
SVTKM_EXEC_CONT svtkm::Id3 getLogical<3>(const svtkm::Id& index, const svtkm::Id3& cellDims)
{
  svtkm::Id3 res(0, 0, 0);
  res[0] = index % cellDims[0];
  res[1] = (index / (cellDims[0])) % (cellDims[1]);
  res[2] = index / ((cellDims[0]) * (cellDims[1]));
  return res;
}

template <>
SVTKM_EXEC_CONT svtkm::Id3 getLogical<2>(const svtkm::Id& index, const svtkm::Id3& cellDims)
{
  svtkm::Id3 res(0, 0, 0);
  res[0] = index % cellDims[0];
  res[1] = index / cellDims[0];
  return res;
}

template <>
SVTKM_EXEC_CONT svtkm::Id3 getLogical<1>(const svtkm::Id& index, const svtkm::Id3&)
{
  svtkm::Id3 res(0, 0, 0);
  res[0] = index;
  return res;
}

template <int DIMS>
class RealMinMax : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  RealMinMax(svtkm::Id3 cellDims, bool removeAllGhost, svtkm::UInt8 removeType)
    : CellDims(cellDims)
    , RemoveAllGhost(removeAllGhost)
    , RemoveType(removeType)
  {
  }

  typedef void ControlSignature(FieldIn, AtomicArrayInOut);
  typedef void ExecutionSignature(_1, InputIndex, _2);

  template <typename Atomic>
  SVTKM_EXEC void Max(Atomic& atom, const svtkm::Id& val, const svtkm::Id& index) const
  {
    svtkm::Id old = -1;
    do
    {
      old = atom.CompareAndSwap(index, val, old);
    } while (old < val);
  }

  template <typename Atomic>
  SVTKM_EXEC void Min(Atomic& atom, const svtkm::Id& val, const svtkm::Id& index) const
  {
    svtkm::Id old = 1000000000;
    do
    {
      old = atom.CompareAndSwap(index, val, old);
    } while (old > val);
  }

  template <typename T, typename AtomicType>
  SVTKM_EXEC void operator()(const T& value, const svtkm::Id& index, AtomicType& atom) const
  {
    // we are finding the logical min max of valid cells
    if ((RemoveAllGhost && value != 0) || (!RemoveAllGhost && (value != 0 && value | RemoveType)))
      return;

    svtkm::Id3 logical = getLogical<DIMS>(index, CellDims);

    Min(atom, logical[0], 0);
    Min(atom, logical[1], 1);
    Min(atom, logical[2], 2);

    Max(atom, logical[0], 3);
    Max(atom, logical[1], 4);
    Max(atom, logical[2], 5);
  }

private:
  svtkm::Id3 CellDims;
  bool RemoveAllGhost;
  svtkm::UInt8 RemoveType;
};

template <int DIMS>
SVTKM_EXEC_CONT bool checkRange(const svtkm::RangeId3& range, const svtkm::Id3& p);

template <>
SVTKM_EXEC_CONT bool checkRange<1>(const svtkm::RangeId3& range, const svtkm::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max;
}
template <>
SVTKM_EXEC_CONT bool checkRange<2>(const svtkm::RangeId3& range, const svtkm::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max && p[1] >= range.Y.Min && p[1] <= range.Y.Max;
}
template <>
SVTKM_EXEC_CONT bool checkRange<3>(const svtkm::RangeId3& range, const svtkm::Id3& p)
{
  return p[0] >= range.X.Min && p[0] <= range.X.Max && p[1] >= range.Y.Min && p[1] <= range.Y.Max &&
    p[2] >= range.Z.Min && p[2] <= range.Z.Max;
}

template <int DIMS>
class Validate : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  Validate(const svtkm::Id3& cellDims,
           bool removeAllGhost,
           svtkm::UInt8 removeType,
           const svtkm::RangeId3& range)
    : CellDims(cellDims)
    , RemoveAll(removeAllGhost)
    , RemoveVal(removeType)
    , Range(range)
  {
  }

  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, InputIndex, _2);

  template <typename T>
  SVTKM_EXEC void operator()(const T& value, const svtkm::Id& index, svtkm::UInt8& valid) const
  {
    valid = 0;
    if (RemoveAll && value == 0)
      return;
    else if (!RemoveAll && (value == 0 || (value & RemoveVal)))
      return;

    if (checkRange<DIMS>(Range, getLogical<DIMS>(index, CellDims)))
      valid = static_cast<svtkm::UInt8>(1);
  }

private:
  svtkm::Id3 CellDims;
  bool RemoveAll;
  svtkm::UInt8 RemoveVal;
  svtkm::RangeId3 Range;
};

template <int DIMS, typename T, typename StorageType>
bool CanStrip(const svtkm::cont::ArrayHandle<T, StorageType>& ghostField,
              const svtkm::cont::Invoker& invoke,
              bool removeAllGhost,
              svtkm::UInt8 removeType,
              svtkm::RangeId3& range,
              const svtkm::Id3& cellDims,
              svtkm::Id size)
{
  svtkm::cont::ArrayHandle<svtkm::Id> minmax;
  minmax.Allocate(6);
  minmax.GetPortalControl().Set(0, std::numeric_limits<svtkm::Id>::max());
  minmax.GetPortalControl().Set(1, std::numeric_limits<svtkm::Id>::max());
  minmax.GetPortalControl().Set(2, std::numeric_limits<svtkm::Id>::max());
  minmax.GetPortalControl().Set(3, std::numeric_limits<svtkm::Id>::min());
  minmax.GetPortalControl().Set(4, std::numeric_limits<svtkm::Id>::min());
  minmax.GetPortalControl().Set(5, std::numeric_limits<svtkm::Id>::min());

  invoke(RealMinMax<3>(cellDims, removeAllGhost, removeType), ghostField, minmax);

  auto portal = minmax.GetPortalConstControl();
  range = svtkm::RangeId3(
    portal.Get(0), portal.Get(3), portal.Get(1), portal.Get(4), portal.Get(2), portal.Get(5));

  svtkm::cont::ArrayHandle<svtkm::UInt8> validFlags;
  validFlags.Allocate(size);

  invoke(Validate<DIMS>(cellDims, removeAllGhost, removeType, range), ghostField, validFlags);

  svtkm::UInt8 res = svtkm::cont::Algorithm::Reduce(validFlags, svtkm::UInt8(0), svtkm::Maximum());
  return res == 0;
}

template <typename T, typename StorageType>
bool CanDoStructuredStrip(const svtkm::cont::DynamicCellSet& cells,
                          const svtkm::cont::ArrayHandle<T, StorageType>& ghostField,
                          const svtkm::cont::Invoker& invoke,
                          bool removeAllGhost,
                          svtkm::UInt8 removeType,
                          svtkm::RangeId3& range)
{
  bool canDo = false;
  svtkm::Id3 cellDims(1, 1, 1);

  if (cells.IsSameType(svtkm::cont::CellSetStructured<1>()))
  {
    auto cells1D = cells.Cast<svtkm::cont::CellSetStructured<1>>();
    svtkm::Id d = cells1D.GetCellDimensions();
    cellDims[0] = d;
    svtkm::Id sz = d;

    canDo = CanStrip<1>(ghostField, invoke, removeAllGhost, removeType, range, cellDims, sz);
  }
  else if (cells.IsSameType(svtkm::cont::CellSetStructured<2>()))
  {
    auto cells2D = cells.Cast<svtkm::cont::CellSetStructured<2>>();
    svtkm::Id2 d = cells2D.GetCellDimensions();
    cellDims[0] = d[0];
    cellDims[1] = d[1];
    svtkm::Id sz = cellDims[0] * cellDims[1];
    canDo = CanStrip<2>(ghostField, invoke, removeAllGhost, removeType, range, cellDims, sz);
  }
  else if (cells.IsSameType(svtkm::cont::CellSetStructured<3>()))
  {
    auto cells3D = cells.Cast<svtkm::cont::CellSetStructured<3>>();
    cellDims = cells3D.GetCellDimensions();
    svtkm::Id sz = cellDims[0] * cellDims[1] * cellDims[2];
    canDo = CanStrip<3>(ghostField, invoke, removeAllGhost, removeType, range, cellDims, sz);
  }

  return canDo;
}

} // end anon namespace

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT GhostCellRemove::GhostCellRemove()
  : svtkm::filter::FilterDataSetWithField<GhostCellRemove>()
  , RemoveAll(false)
  , RemoveField(false)
  , RemoveVals(0)
{
  this->SetActiveField("svtkmGhostCells");
  this->SetFieldsToPass("svtkmGhostCells", svtkm::filter::FieldSelection::MODE_EXCLUDE);
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet GhostCellRemove::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //get the cells and coordinates of the dataset
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  svtkm::cont::DynamicCellSet cellOut;

  //Preserve structured output where possible.
  if (cells.IsSameType(svtkm::cont::CellSetStructured<1>()) ||
      cells.IsSameType(svtkm::cont::CellSetStructured<2>()) ||
      cells.IsSameType(svtkm::cont::CellSetStructured<3>()))
  {
    svtkm::RangeId3 range;
    if (CanDoStructuredStrip(
          cells, field, this->Invoke, this->GetRemoveAllGhost(), this->GetRemoveType(), range))
    {
      svtkm::filter::ExtractStructured extract;
      extract.SetInvoker(this->Invoke);
      svtkm::RangeId3 erange(
        range.X.Min, range.X.Max + 2, range.Y.Min, range.Y.Max + 2, range.Z.Min, range.Z.Max + 2);
      svtkm::Id3 sample(1, 1, 1);
      extract.SetVOI(erange);
      extract.SetSampleRate(sample);
      if (this->GetRemoveGhostField())
        extract.SetFieldsToPass(this->GetActiveFieldName(),
                                svtkm::filter::FieldSelection::MODE_EXCLUDE);

      auto output = extract.Execute(input);
      return output;
    }
  }

  if (this->GetRemoveAllGhost())
  {
    cellOut = this->Worklet.Run(svtkm::filter::ApplyPolicyCellSet(cells, policy),
                                field,
                                fieldMeta.GetAssociation(),
                                RemoveAllGhosts());
  }
  else if (this->GetRemoveByType())
  {
    cellOut = this->Worklet.Run(svtkm::filter::ApplyPolicyCellSet(cells, policy),
                                field,
                                fieldMeta.GetAssociation(),
                                RemoveGhostByType(this->GetRemoveType()));
  }
  else
  {
    throw svtkm::cont::ErrorFilterExecution("Unsupported ghost cell removal type");
  }

  svtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.SetCellSet(cellOut);

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool GhostCellRemove::DoMapField(
  svtkm::cont::DataSet& result,
  const svtkm::cont::ArrayHandle<T, StorageType>& input,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (fieldMeta.IsPointField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(fieldMeta.AsField(input));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    svtkm::cont::ArrayHandle<T> out = this->Worklet.ProcessCellField(input);
    result.AddField(fieldMeta.AsField(out));
    return true;
  }
  else
  {
    return false;
  }
}
}
}

#endif //svtk_m_filter_GhostCellRemove_hxx
