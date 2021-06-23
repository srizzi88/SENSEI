//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_filter_ClipWithImplicitFunction_h
#define svtk_m_filter_ClipWithImplicitFunction_h

#include <svtkm/filter/svtkm_filter_export.h>

#include <svtkm/cont/ImplicitFunctionHandle.h>
#include <svtkm/filter/FilterDataSet.h>
#include <svtkm/worklet/Clip.h>

namespace svtkm
{
namespace filter
{

/// \brief Clip a dataset using an implicit function
///
/// Clip a dataset using a given implicit function value, such as svtkm::Sphere
/// or svtkm::Frustum.
/// The resulting geometry will not be water tight.
class SVTKM_ALWAYS_EXPORT ClipWithImplicitFunction
  : public svtkm::filter::FilterDataSet<ClipWithImplicitFunction>
{
public:
  void SetImplicitFunction(const svtkm::cont::ImplicitFunctionHandle& func)
  {
    this->Function = func;
  }

  void SetInvertClip(bool invert) { this->Invert = invert; }

  const svtkm::cont::ImplicitFunctionHandle& GetImplicitFunction() const { return this->Function; }

  template <typename DerivedPolicy>
  svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                svtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter.
  //This call is only valid after Execute has been called.
  template <typename T, typename StorageType, typename DerivedPolicy>
  bool DoMapField(svtkm::cont::DataSet& result,
                  const svtkm::cont::ArrayHandle<T, StorageType>& input,
                  const svtkm::filter::FieldMetadata& fieldMeta,
                  svtkm::filter::PolicyBase<DerivedPolicy>)
  {
    svtkm::cont::ArrayHandle<T> output;

    if (fieldMeta.IsPointField())
    {
      output = this->Worklet.ProcessPointField(input);
    }
    else if (fieldMeta.IsCellField())
    {
      output = this->Worklet.ProcessCellField(input);
    }
    else
    {
      return false;
    }

    //use the same meta data as the input so we get the same field name, etc.
    result.AddField(fieldMeta.AsField(output));

    return true;
  }

private:
  svtkm::cont::ImplicitFunctionHandle Function;
  svtkm::worklet::Clip Worklet;
  bool Invert = false;
};

#ifndef svtkm_filter_ClipWithImplicitFunction_cxx
SVTKM_FILTER_EXPORT_EXECUTE_METHOD(ClipWithImplicitFunction);
#endif
}
} // namespace svtkm::filter

#include <svtkm/filter/ClipWithImplicitFunction.hxx>

#endif // svtk_m_filter_ClipWithImplicitFunction_h
