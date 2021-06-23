//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_FieldToColors_hxx
#define svtk_m_filter_FieldToColors_hxx

#include <svtkm/VecTraits.h>
#include <svtkm/cont/ColorTable.hxx>
#include <svtkm/cont/ErrorFilterExecution.h>

namespace svtkm
{
namespace filter
{

namespace
{
struct ScalarInputMode
{
};
struct MagnitudeInputMode
{
};
struct ComponentInputMode
{
};
}

template <typename T, typename S, typename U>
inline bool execute(const svtkm::cont::ColorTable& table,
                    ScalarInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    svtkm::VecTraitsTagSingleComponent)
{
  return table.Map(input, samples, output);
}

template <typename T, typename S, typename U>
inline bool execute(const svtkm::cont::ColorTable& table,
                    MagnitudeInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    svtkm::VecTraitsTagMultipleComponents)
{
  return table.MapMagnitude(input, samples, output);
}

template <typename T, typename S, typename U>
inline bool execute(const svtkm::cont::ColorTable& table,
                    ComponentInputMode,
                    int comp,
                    const T& input,
                    const S& samples,
                    U& output,
                    svtkm::VecTraitsTagMultipleComponents)
{
  return table.MapComponent(input, comp, samples, output);
}

//error cases
template <typename T, typename S, typename U>
inline bool execute(const svtkm::cont::ColorTable& table,
                    ScalarInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    svtkm::VecTraitsTagMultipleComponents)
{ //vector input in scalar mode so do magnitude
  return table.MapMagnitude(input, samples, output);
}
template <typename T, typename S, typename U>
inline bool execute(const svtkm::cont::ColorTable& table,
                    MagnitudeInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    svtkm::VecTraitsTagSingleComponent)
{ //is a scalar array so ignore Magnitude mode
  return table.Map(input, samples, output);
}
template <typename T, typename S, typename U>
inline bool execute(const svtkm::cont::ColorTable& table,
                    ComponentInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    svtkm::VecTraitsTagSingleComponent)
{ //is a scalar array so ignore COMPONENT mode
  return table.Map(input, samples, output);
}


//-----------------------------------------------------------------------------
inline SVTKM_CONT FieldToColors::FieldToColors(const svtkm::cont::ColorTable& table)
  : svtkm::filter::FilterField<FieldToColors>()
  , Table(table)
  , InputMode(SCALAR)
  , OutputMode(RGBA)
  , SamplesRGB()
  , SamplesRGBA()
  , Component(0)
  , SampleCount(256)
  , ModifiedCount(-1)
{
}

//-----------------------------------------------------------------------------
inline SVTKM_CONT void FieldToColors::SetNumberOfSamplingPoints(svtkm::Int32 count)
{
  if (this->SampleCount != count && count > 0)
  {
    this->ModifiedCount = -1;
    this->SampleCount = count;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet FieldToColors::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& inField,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  //If the table has been modified we need to rebuild our
  //sample tables
  if (this->Table.GetModifiedCount() > this->ModifiedCount)
  {
    this->Table.Sample(this->SampleCount, this->SamplesRGB);
    this->Table.Sample(this->SampleCount, this->SamplesRGBA);
    this->ModifiedCount = this->Table.GetModifiedCount();
  }


  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input_colors.
    outputName = fieldMetadata.GetName() + "_colors";
  }
  svtkm::cont::Field outField;

  //We need to verify if the array is a svtkm::Vec

  using IsVec = typename svtkm::VecTraits<T>::HasMultipleComponents;
  if (this->OutputMode == RGBA)
  {
    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> output;

    bool ran = false;
    switch (this->InputMode)
    {
      case SCALAR:
      {
        ran = execute(this->Table,
                      ScalarInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGBA,
                      output,
                      IsVec{});
        break;
      }
      case MAGNITUDE:
      {
        ran = execute(this->Table,
                      MagnitudeInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGBA,
                      output,
                      IsVec{});
        break;
      }
      case COMPONENT:
      {
        ran = execute(this->Table,
                      ComponentInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGBA,
                      output,
                      IsVec{});
        break;
      }
    }

    if (!ran)
    {
      throw svtkm::cont::ErrorFilterExecution("Unsupported input mode.");
    }
    outField = svtkm::cont::make_FieldPoint(outputName, output);
  }
  else
  {
    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> output;

    bool ran = false;
    switch (this->InputMode)
    {
      case SCALAR:
      {
        ran = execute(this->Table,
                      ScalarInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGB,
                      output,
                      IsVec{});
        break;
      }
      case MAGNITUDE:
      {
        ran = execute(this->Table,
                      MagnitudeInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGB,
                      output,
                      IsVec{});
        break;
      }
      case COMPONENT:
      {
        ran = execute(this->Table,
                      ComponentInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGB,
                      output,
                      IsVec{});
        break;
      }
    }

    if (!ran)
    {
      throw svtkm::cont::ErrorFilterExecution("Unsupported input mode.");
    }
    outField = svtkm::cont::make_FieldPoint(outputName, output);
  }


  return CreateResult(input, outField);
}
}
} // namespace svtkm::filter

#endif
