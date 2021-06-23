//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_tbb_internal_ParallelSort_h
#define svtk_m_cont_tbb_internal_ParallelSort_h

#include <svtkm/BinaryPredicates.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleZip.h>
#include <svtkm/cont/internal/ParallelRadixSortInterface.h>

#include <svtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>
#include <svtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <svtkm/cont/tbb/internal/FunctorsTBB.h>
#include <svtkm/cont/tbb/internal/ParallelSortTBB.hxx>

#include <type_traits>

namespace svtkm
{
namespace cont
{
namespace tbb
{
namespace sort
{

// Declare the compiled radix sort specializations:
SVTKM_DECLARE_RADIX_SORT()

// Forward declare entry points (See stack overflow discussion 7255281 --
// templated overloads of template functions are not specialization, and will
// be resolved during the first phase of two part lookup).
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(svtkm::cont::ArrayHandle<T, Container>&, BinaryCompare);
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(svtkm::cont::ArrayHandle<T, StorageT>&,
                         svtkm::cont::ArrayHandle<U, StorageU>&,
                         BinaryCompare);

// Quicksort values:
template <typename HandleType, class BinaryCompare>
void parallel_sort(HandleType& values,
                   BinaryCompare binary_compare,
                   svtkm::cont::internal::radix::PSortTag)
{
  auto arrayPortal = values.PrepareForInPlace(svtkm::cont::DeviceAdapterTagTBB());

  using IteratorsType = svtkm::cont::ArrayPortalToIterators<decltype(arrayPortal)>;
  IteratorsType iterators(arrayPortal);

  internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
  ::tbb::parallel_sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
}

// Radix sort values:
template <typename T, typename StorageT, class BinaryCompare>
void parallel_sort(svtkm::cont::ArrayHandle<T, StorageT>& values,
                   BinaryCompare binary_compare,
                   svtkm::cont::internal::radix::RadixSortTag)
{
  using namespace svtkm::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  parallel_radix_sort(
    values.GetStorage().GetArray(), static_cast<std::size_t>(values.GetNumberOfValues()), c);
}

// Value sort -- static switch between quicksort and radix sort
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(svtkm::cont::ArrayHandle<T, Container>& values, BinaryCompare binary_compare)
{
  using namespace svtkm::cont::internal::radix;
  using SortAlgorithmTag = typename sort_tag_type<T, Container, BinaryCompare>::type;
  parallel_sort(values, binary_compare, SortAlgorithmTag{});
}


// Quicksort by key
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                         svtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         svtkm::cont::internal::radix::PSortTag)
{
  using namespace svtkm::cont::internal::radix;
  using KeyType = svtkm::cont::ArrayHandle<T, StorageT>;
  constexpr bool larger_than_64bits = sizeof(U) > sizeof(svtkm::Int64);
  if (larger_than_64bits)
  {
    /// More efficient sort:
    /// Move value indexes when sorting and reorder the value array at last

    using ValueType = svtkm::cont::ArrayHandle<U, StorageU>;
    using IndexType = svtkm::cont::ArrayHandle<svtkm::Id>;
    using ZipHandleType = svtkm::cont::ArrayHandleZip<KeyType, IndexType>;

    IndexType indexArray;
    ValueType valuesScattered;
    const svtkm::Id size = values.GetNumberOfValues();

    {
      auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
      auto inputPortal = handle.PrepareForInput(DeviceAdapterTagTBB());
      auto outputPortal =
        indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagTBB());
      tbb::CopyPortals(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
    }

    ZipHandleType zipHandle = svtkm::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(zipHandle,
                  svtkm::cont::internal::KeyCompare<T, svtkm::Id, BinaryCompare>(binary_compare),
                  PSortTag());

    tbb::ScatterPortal(values.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()),
                       indexArray.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()),
                       valuesScattered.PrepareForOutput(size, svtkm::cont::DeviceAdapterTagTBB()));

    {
      auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagTBB());
      auto outputPortal =
        values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagTBB());
      tbb::CopyPortals(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
    }
  }
  else
  {
    using ValueType = svtkm::cont::ArrayHandle<U, StorageU>;
    using ZipHandleType = svtkm::cont::ArrayHandleZip<KeyType, ValueType>;

    ZipHandleType zipHandle = svtkm::cont::make_ArrayHandleZip(keys, values);
    parallel_sort(
      zipHandle, svtkm::cont::internal::KeyCompare<T, U, BinaryCompare>(binary_compare), PSortTag{});
  }
}

// Radix sort by key -- Specialize for svtkm::Id values:
template <typename T, typename StorageT, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                         svtkm::cont::ArrayHandle<svtkm::Id, StorageU>& values,
                         BinaryCompare binary_compare,
                         svtkm::cont::internal::radix::RadixSortTag)
{
  using namespace svtkm::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  parallel_radix_sort_key_values(keys.GetStorage().GetArray(),
                                 values.GetStorage().GetArray(),
                                 static_cast<std::size_t>(keys.GetNumberOfValues()),
                                 c);
}

// Radix sort by key -- Generic impl:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                         svtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         svtkm::cont::internal::radix::RadixSortTag)
{
  using KeyType = svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>;
  using ValueType = svtkm::cont::ArrayHandle<U, svtkm::cont::StorageTagBasic>;
  using IndexType = svtkm::cont::ArrayHandle<svtkm::Id, svtkm::cont::StorageTagBasic>;
  using ZipHandleType = svtkm::cont::ArrayHandleZip<KeyType, IndexType>;

  IndexType indexArray;
  ValueType valuesScattered;
  const svtkm::Id size = values.GetNumberOfValues();

  {
    auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
    auto inputPortal = handle.PrepareForInput(DeviceAdapterTagTBB());
    auto outputPortal =
      indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagTBB());
    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
  }

  if (static_cast<svtkm::Id>(sizeof(T)) * keys.GetNumberOfValues() > 400000)
  {
    parallel_sort_bykey(keys, indexArray, binary_compare);
  }
  else
  {
    ZipHandleType zipHandle = svtkm::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(zipHandle,
                  svtkm::cont::internal::KeyCompare<T, svtkm::Id, BinaryCompare>(binary_compare),
                  svtkm::cont::internal::radix::PSortTag{});
  }

  tbb::ScatterPortal(values.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()),
                     indexArray.PrepareForInput(svtkm::cont::DeviceAdapterTagTBB()),
                     valuesScattered.PrepareForOutput(size, svtkm::cont::DeviceAdapterTagTBB()));

  {
    auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagTBB());
    auto outputPortal =
      values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagTBB());
    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
  }
}

// Sort by key -- static switch between radix and quick sort:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                         svtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare)
{
  using namespace svtkm::cont::internal::radix;
  using SortAlgorithmTag =
    typename sortbykey_tag_type<T, U, StorageT, StorageU, BinaryCompare>::type;
  parallel_sort_bykey(keys, values, binary_compare, SortAlgorithmTag{});
}
}
}
}
} // end namespace svtkm::cont::tbb::sort

#endif // svtk_m_cont_tbb_internal_ParallelSort_h
