//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_Field_h
#define svtk_m_cont_Field_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/Range.h>
#include <svtkm/Types.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/ArrayRangeCompute.h>
#include <svtkm/cont/ArrayRangeCompute.hxx>
#include <svtkm/cont/VariantArrayHandle.h>

namespace svtkm
{
namespace cont
{

namespace internal
{

struct ComputeRange
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& input, svtkm::cont::ArrayHandle<svtkm::Range>& range) const
  {
    range = svtkm::cont::ArrayRangeCompute(input);
  }
};

} // namespace internal


/// A \c Field encapsulates an array on some piece of the mesh, such as
/// the points, a cell set, a point logical dimension, or the whole mesh.
///
class SVTKM_CONT_EXPORT Field
{
public:
  enum struct Association
  {
    ANY,
    WHOLE_MESH,
    POINTS,
    CELL_SET
  };

  SVTKM_CONT
  Field() = default;

  SVTKM_CONT
  Field(std::string name, Association association, const svtkm::cont::VariantArrayHandle& data);

  template <typename T, typename Storage>
  SVTKM_CONT Field(std::string name,
                  Association association,
                  const svtkm::cont::ArrayHandle<T, Storage>& data)
    : Field(name, association, svtkm::cont::VariantArrayHandle{ data })
  {
  }

  Field(const svtkm::cont::Field& src);
  Field(svtkm::cont::Field&& src) noexcept;

  SVTKM_CONT virtual ~Field();

  SVTKM_CONT Field& operator=(const svtkm::cont::Field& src);
  SVTKM_CONT Field& operator=(svtkm::cont::Field&& src) noexcept;

  SVTKM_CONT const std::string& GetName() const { return this->Name; }
  SVTKM_CONT Association GetAssociation() const { return this->FieldAssociation; }
  const svtkm::cont::VariantArrayHandle& GetData() const;
  svtkm::cont::VariantArrayHandle& GetData();

  SVTKM_CONT bool IsFieldCell() const { return this->FieldAssociation == Association::CELL_SET; }
  SVTKM_CONT bool IsFieldPoint() const { return this->FieldAssociation == Association::POINTS; }

  SVTKM_CONT svtkm::Id GetNumberOfValues() const { return this->Data.GetNumberOfValues(); }

  template <typename TypeList>
  SVTKM_CONT void GetRange(svtkm::Range* range, TypeList) const
  {
    this->GetRangeImpl(TypeList());
    const svtkm::Id length = this->Range.GetNumberOfValues();
    for (svtkm::Id i = 0; i < length; ++i)
    {
      range[i] = this->Range.GetPortalConstControl().Get(i);
    }
  }

  template <typename TypeList>
  SVTKM_CONT const svtkm::cont::ArrayHandle<svtkm::Range>& GetRange(TypeList) const
  {
    return this->GetRangeImpl(TypeList());
  }

  SVTKM_CONT
  const svtkm::cont::ArrayHandle<svtkm::Range>& GetRange() const
  {
    return this->GetRangeImpl(SVTKM_DEFAULT_TYPE_LIST());
  }

  SVTKM_CONT void GetRange(svtkm::Range* range) const
  {
    return this->GetRange(range, SVTKM_DEFAULT_TYPE_LIST());
  }

  template <typename T, typename StorageTag>
  SVTKM_CONT void SetData(const svtkm::cont::ArrayHandle<T, StorageTag>& newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  SVTKM_CONT
  void SetData(const svtkm::cont::VariantArrayHandle& newdata)
  {
    this->Data = newdata;
    this->ModifiedFlag = true;
  }

  SVTKM_CONT
  virtual void PrintSummary(std::ostream& out) const;

  SVTKM_CONT
  virtual void ReleaseResourcesExecution()
  {
    this->Data.ReleaseResourcesExecution();
    this->Range.ReleaseResourcesExecution();
  }

private:
  std::string Name; ///< name of field

  Association FieldAssociation = Association::ANY;
  svtkm::cont::VariantArrayHandle Data;
  mutable svtkm::cont::ArrayHandle<svtkm::Range> Range;
  mutable bool ModifiedFlag = true;

  template <typename TypeList>
  SVTKM_CONT const svtkm::cont::ArrayHandle<svtkm::Range>& GetRangeImpl(TypeList) const
  {
    SVTKM_IS_LIST(TypeList);

    if (this->ModifiedFlag)
    {
      svtkm::cont::CastAndCall(
        this->Data.ResetTypes(TypeList()), internal::ComputeRange{}, this->Range);
      this->ModifiedFlag = false;
    }

    return this->Range;
  }
};

template <typename Functor, typename... Args>
void CastAndCall(const svtkm::cont::Field& field, Functor&& f, Args&&... args)
{
  svtkm::cont::CastAndCall(field.GetData(), std::forward<Functor>(f), std::forward<Args>(args)...);
}


//@{
/// Convenience functions to build fields from C style arrays and std::vector
template <typename T>
svtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const T* data,
                             svtkm::Id size,
                             svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  return svtkm::cont::Field(name, association, svtkm::cont::make_ArrayHandle(data, size, copy));
}

template <typename T>
svtkm::cont::Field make_Field(std::string name,
                             Field::Association association,
                             const std::vector<T>& data,
                             svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  return svtkm::cont::Field(name, association, svtkm::cont::make_ArrayHandle(data, copy));
}

//@}

/// Convenience function to build point fields from svtkm::cont::ArrayHandle
template <typename T, typename S>
svtkm::cont::Field make_FieldPoint(std::string name, const svtkm::cont::ArrayHandle<T, S>& data)
{
  return svtkm::cont::Field(name, svtkm::cont::Field::Association::POINTS, data);
}

/// Convenience function to build point fields from svtkm::cont::VariantArrayHandle
inline svtkm::cont::Field make_FieldPoint(std::string name,
                                         const svtkm::cont::VariantArrayHandle& data)
{
  return svtkm::cont::Field(name, svtkm::cont::Field::Association::POINTS, data);
}

/// Convenience function to build cell fields from svtkm::cont::ArrayHandle
template <typename T, typename S>
svtkm::cont::Field make_FieldCell(std::string name, const svtkm::cont::ArrayHandle<T, S>& data)
{
  return svtkm::cont::Field(name, svtkm::cont::Field::Association::CELL_SET, data);
}


/// Convenience function to build cell fields from svtkm::cont::VariantArrayHandle
inline svtkm::cont::Field make_FieldCell(std::string name,
                                        const svtkm::cont::VariantArrayHandle& data)
{
  return svtkm::cont::Field(name, svtkm::cont::Field::Association::CELL_SET, data);
}

} // namespace cont
} // namespace svtkm


namespace svtkm
{
namespace cont
{
namespace internal
{
template <>
struct DynamicTransformTraits<svtkm::cont::Field>
{
  using DynamicTag = svtkm::cont::internal::DynamicTransformTagCastAndCall;
};
} // namespace internal
} // namespace cont
} // namespace svtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{
template <typename TypeList = SVTKM_DEFAULT_TYPE_LIST>
struct SerializableField
{
  SerializableField() = default;

  explicit SerializableField(const svtkm::cont::Field& field)
    : Field(field)
  {
  }

  svtkm::cont::Field Field;
};
} // namespace cont
} // namespace svtkm

namespace mangled_diy_namespace
{

template <typename TypeList>
struct Serialization<svtkm::cont::SerializableField<TypeList>>
{
private:
  using Type = svtkm::cont::SerializableField<TypeList>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const Type& serializable)
  {
    const auto& field = serializable.Field;

    svtkmdiy::save(bb, field.GetName());
    svtkmdiy::save(bb, static_cast<int>(field.GetAssociation()));
    svtkmdiy::save(bb, field.GetData().ResetTypes(TypeList{}));
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, Type& serializable)
  {
    auto& field = serializable.Field;

    std::string name;
    svtkmdiy::load(bb, name);
    int assocVal = 0;
    svtkmdiy::load(bb, assocVal);

    auto assoc = static_cast<svtkm::cont::Field::Association>(assocVal);
    svtkm::cont::VariantArrayHandleBase<TypeList> data;
    svtkmdiy::load(bb, data);
    field = svtkm::cont::Field(name, assoc, svtkm::cont::VariantArrayHandle(data));
  }
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_Field_h
