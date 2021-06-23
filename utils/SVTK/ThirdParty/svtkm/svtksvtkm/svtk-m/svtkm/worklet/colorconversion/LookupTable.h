//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_colorconversion_LookupTable_h
#define svtk_m_worklet_colorconversion_LookupTable_h

#include <svtkm/cont/ColorTableSamples.h>

#include <svtkm/exec/ColorTable.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/colorconversion/Conversions.h>

#include <float.h>

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{

using LookupTableTypes = svtkm::List<svtkm::Vec3ui_8, svtkm::Vec4ui_8, svtkm::Vec3f_32, svtkm::Vec4f_64>;

struct LookupTable : public svtkm::worklet::WorkletMapField
{
  svtkm::Float32 Shift;
  svtkm::Float32 Scale;
  svtkm::Range TableRange;
  svtkm::Int32 NumberOfSamples;

  //needs to support Nan, Above, Below Range colors
  SVTKM_CONT
  template <typename T>
  LookupTable(const T& colorTableSamples)
  {
    this->Shift = static_cast<svtkm::Float32>(-colorTableSamples.SampleRange.Min);
    double rangeDelta = colorTableSamples.SampleRange.Length();
    if (rangeDelta < DBL_MIN * colorTableSamples.NumberOfSamples)
    {
      // if the range is tiny, anything within the range will map to the bottom
      // of the color scale.
      this->Scale = 0.0;
    }
    else
    {
      this->Scale = static_cast<svtkm::Float32>(colorTableSamples.NumberOfSamples / rangeDelta);
    }
    this->TableRange = colorTableSamples.SampleRange;
    this->NumberOfSamples = colorTableSamples.NumberOfSamples;
  }

  using ControlSignature = void(FieldIn in, WholeArrayIn lookup, FieldOut color);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename T, typename WholeFieldIn, typename U, int N>
  SVTKM_EXEC void operator()(const T& in,
                            const WholeFieldIn lookupTable,
                            svtkm::Vec<U, N>& output) const
  {
    svtkm::Float64 v = (static_cast<svtkm::Float64>(in));
    svtkm::Int32 idx = 1;

    //This logic uses how ColorTableSamples is constructed. See
    //svtkm/cont/ColorTableSamples to see why we use these magic offset values
    if (svtkm::IsNan(v))
    {
      idx = this->NumberOfSamples + 3;
    }
    else if (v < this->TableRange.Min)
    { //If we are below the color range
      idx = 0;
    }
    else if (v == this->TableRange.Min)
    { //If we are at the ranges min value
      idx = 1;
    }
    else if (v > this->TableRange.Max)
    { //If we are above the ranges max value
      idx = this->NumberOfSamples + 2;
    }
    else if (v == this->TableRange.Max)
    { //If we are at the ranges min value
      idx = this->NumberOfSamples;
    }
    else
    {
      v = (v + this->Shift) * this->Scale;
      // When v is very close to p.Range[1], the floating point calculation giving
      // idx may map above the highest value in the lookup table. That is why it
      // is padded
      idx = static_cast<svtkm::Int32>(v);
    }
    output = lookupTable.Get(idx);
  }
};
}
}
}
#endif
