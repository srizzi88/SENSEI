//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_tool_h
#define svtk_m_worklet_zfp_tool_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/AtomicArray.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/worklet/DispatcherMapField.h>

#include <svtkm/worklet/zfp/ZFPEncode3.h>

using ZFPWord = svtkm::UInt64;

#include <stdio.h>

namespace svtkm
{
namespace worklet
{
namespace zfp
{
namespace detail
{

class MemTransfer : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  MemTransfer() {}
  using ControlSignature = void(FieldIn, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2);

  template <typename PortalType>
  SVTKM_EXEC void operator()(const svtkm::Id id, PortalType& outValue) const
  {
    (void)id;
    (void)outValue;
  }
}; //class MemTransfer

inline size_t CalcMem3d(const svtkm::Id3 dims, const svtkm::UInt32 bits_per_block)
{
  const size_t vals_per_block = 64;
  const size_t size = static_cast<size_t>(dims[0] * dims[1] * dims[2]);
  size_t total_blocks = size / vals_per_block;
  const size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}

inline size_t CalcMem2d(const svtkm::Id2 dims, const svtkm::UInt32 bits_per_block)
{
  constexpr size_t vals_per_block = 16;
  const size_t size = static_cast<size_t>(dims[0] * dims[1]);
  size_t total_blocks = size / vals_per_block;
  constexpr size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}

inline size_t CalcMem1d(const svtkm::Id dims, const svtkm::UInt32 bits_per_block)
{
  constexpr size_t vals_per_block = 4;
  const size_t size = static_cast<size_t>(dims);
  size_t total_blocks = size / vals_per_block;
  constexpr size_t bits_per_word = sizeof(ZFPWord) * 8;
  const size_t total_bits = bits_per_block * total_blocks;
  const size_t alloc_size = total_bits / bits_per_word;
  return alloc_size * sizeof(ZFPWord);
}


template <typename T>
T* GetSVTKMPointer(svtkm::cont::ArrayHandle<T>& handle)
{
  typedef typename svtkm::cont::ArrayHandle<T> HandleType;
  typedef typename HandleType::template ExecutionTypes<svtkm::cont::DeviceAdapterTagSerial>::Portal
    PortalType;
  typedef typename svtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType IteratorType;
  IteratorType iter =
    svtkm::cont::ArrayPortalToIterators<PortalType>(handle.GetPortalControl()).GetBegin();
  return &(*iter);
}

template <typename T, typename S>
void DataDump(svtkm::cont::ArrayHandle<T, S> handle, std::string fileName)
{

  T* ptr = GetSVTKMPointer(handle);
  svtkm::Id osize = handle.GetNumberOfValues();
  FILE* fp = fopen(fileName.c_str(), "wb");
  ;
  if (fp != NULL)
  {
    fwrite(ptr, sizeof(T), static_cast<size_t>(osize), fp);
  }

  fclose(fp);
}


} // namespace detail
} // namespace zfp
} // namespace worklet
} // namespace svtkm
#endif //  svtk_m_worklet_zfp_tools_h
