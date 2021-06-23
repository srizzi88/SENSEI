//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_colorconversion_TransferFunction_h
#define svtk_m_worklet_colorconversion_TransferFunction_h

#include <svtkm/exec/ColorTable.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/colorconversion/Conversions.h>

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{

struct TransferFunction : public svtkm::worklet::WorkletMapField
{
  TransferFunction(const svtkm::exec::ColorTableBase* table)
    : ColorTable(table)
  {
  }

  using ControlSignature = void(FieldIn in, FieldOut color);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  SVTKM_EXEC void operator()(const T& in, svtkm::Vec3ui_8& output) const
  {
    svtkm::Vec<float, 3> rgb = this->ColorTable->MapThroughColorSpace(static_cast<double>(in));
    output[0] = colorconversion::ColorToUChar(rgb[0]);
    output[1] = colorconversion::ColorToUChar(rgb[1]);
    output[2] = colorconversion::ColorToUChar(rgb[2]);
  }

  template <typename T>
  SVTKM_EXEC void operator()(const T& in, svtkm::Vec4ui_8& output) const
  {
    svtkm::Vec<float, 3> rgb = this->ColorTable->MapThroughColorSpace(static_cast<double>(in));
    float alpha = this->ColorTable->MapThroughOpacitySpace(static_cast<double>(in));
    output[0] = colorconversion::ColorToUChar(rgb[0]);
    output[1] = colorconversion::ColorToUChar(rgb[1]);
    output[2] = colorconversion::ColorToUChar(rgb[2]);
    output[3] = colorconversion::ColorToUChar(alpha);
  }

  template <typename T>
  SVTKM_EXEC void operator()(const T& in, svtkm::Vec<float, 3>& output) const
  {
    output = this->ColorTable->MapThroughColorSpace(static_cast<double>(in));
  }

  template <typename T>
  SVTKM_EXEC void operator()(const T& in, svtkm::Vec<float, 4>& output) const
  {
    svtkm::Vec<float, 3> rgb = this->ColorTable->MapThroughColorSpace(static_cast<double>(in));
    float alpha = this->ColorTable->MapThroughOpacitySpace(static_cast<double>(in));
    output[0] = rgb[0];
    output[1] = rgb[1];
    output[2] = rgb[2];
    output[3] = alpha;
  }

  const svtkm::exec::ColorTableBase* ColorTable;
};
}
}
}
#endif
