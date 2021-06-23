//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/openmp/internal/ArrayManagerExecutionOpenMP.h>
#include <svtkm/cont/openmp/internal/FunctorsOpenMP.h>
#include <svtkm/cont/openmp/internal/ParallelQuickSortOpenMP.h>
#include <svtkm/cont/openmp/internal/ParallelRadixSortOpenMP.h>

#include <svtkm/BinaryPredicates.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayHandleZip.h>

#include <omp.h>

namespace svtkm
{
namespace cont
{
namespace openmp
{
namespace sort
{

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
  auto portal = values.PrepareForInPlace(DeviceAdapterTagOpenMP());
  auto iter = svtkm::cont::ArrayPortalToIteratorBegin(portal);
  svtkm::Id2 range(0, values.GetNumberOfValues());

  using IterType = typename std::decay<decltype(iter)>::type;
  using Sorter = quick::QuickSorter<IterType, BinaryCompare>;

  Sorter sorter(iter, binary_compare);
  sorter.Execute(range);
}

// Radix sort values:
template <typename T, typename StorageT, class BinaryCompare>
void parallel_sort(svtkm::cont::ArrayHandle<T, StorageT>& values,
                   BinaryCompare binary_compare,
                   svtkm::cont::internal::radix::RadixSortTag)
{
  auto c = svtkm::cont::internal::radix::get_std_compare(binary_compare, T{});
  radix::parallel_radix_sort(
    values.GetStorage().GetArray(), static_cast<std::size_t>(values.GetNumberOfValues()), c);
}

// Value sort -- static switch between quicksort & radix sort
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(svtkm::cont::ArrayHandle<T, Container>& values, BinaryCompare binary_compare)
{
  using namespace svtkm::cont::internal::radix;
  using SortAlgorithmTag = typename sort_tag_type<T, Container, BinaryCompare>::type;

  parallel_sort(values, binary_compare, SortAlgorithmTag{});
}

// Quicksort by key:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                         svtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         svtkm::cont::internal::radix::PSortTag)
{
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

    // Generate an in-memory index array:
    {
      auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
      auto inputPortal = handle.PrepareForInput(DeviceAdapterTagOpenMP());
      auto outputPortal =
        indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagOpenMP());
      openmp::CopyHelper(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
    }

    // Sort the keys and indices:
    ZipHandleType zipHandle = svtkm::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(zipHandle,
                  svtkm::cont::internal::KeyCompare<T, svtkm::Id, BinaryCompare>(binary_compare),
                  svtkm::cont::internal::radix::PSortTag());

    // Permute the values to their sorted locations:
    {
      auto valuesInPortal = values.PrepareForInput(DeviceAdapterTagOpenMP());
      auto indexPortal = indexArray.PrepareForInput(DeviceAdapterTagOpenMP());
      auto valuesOutPortal = valuesScattered.PrepareForOutput(size, DeviceAdapterTagOpenMP());

      SVTKM_OPENMP_DIRECTIVE(parallel for
                            default(none)
                            firstprivate(valuesInPortal, indexPortal, valuesOutPortal)
                            schedule(static)
                            SVTKM_OPENMP_SHARED_CONST(size))
      for (svtkm::Id i = 0; i < size; ++i)
      {
        valuesOutPortal.Set(i, valuesInPortal.Get(indexPortal.Get(i)));
      }
    }

    // Copy the values back into the input array:
    {
      auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagOpenMP());
      auto outputPortal =
        values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagOpenMP());
      openmp::CopyHelper(inputPortal, outputPortal, 0, 0, size);
    }
  }
  else
  {
    using ValueType = svtkm::cont::ArrayHandle<U, StorageU>;
    using ZipHandleType = svtkm::cont::ArrayHandleZip<KeyType, ValueType>;

    ZipHandleType zipHandle = svtkm::cont::make_ArrayHandleZip(keys, values);
    parallel_sort(zipHandle,
                  svtkm::cont::internal::KeyCompare<T, U, BinaryCompare>(binary_compare),
                  svtkm::cont::internal::radix::PSortTag{});
  }
}

// Radix sort by key:
template <typename T, typename StorageT, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(svtkm::cont::ArrayHandle<T, StorageT>& keys,
                         svtkm::cont::ArrayHandle<svtkm::Id, StorageU>& values,
                         BinaryCompare binary_compare,
                         svtkm::cont::internal::radix::RadixSortTag)
{
  using namespace svtkm::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  radix::parallel_radix_sort_key_values(keys.GetStorage().GetArray(),
                                        values.GetStorage().GetArray(),
                                        static_cast<std::size_t>(keys.GetNumberOfValues()),
                                        c);
}
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
    auto inputPortal = handle.PrepareForInput(DeviceAdapterTagOpenMP());
    auto outputPortal =
      indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagOpenMP());
    openmp::CopyHelper(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
  }

  const svtkm::Id valuesBytes = static_cast<svtkm::Id>(sizeof(T)) * keys.GetNumberOfValues();
  if (valuesBytes > static_cast<svtkm::Id>(svtkm::cont::internal::radix::MIN_BYTES_FOR_PARALLEL))
  {
    parallel_sort_bykey(keys, indexArray, binary_compare);
  }
  else
  {
    ZipHandleType zipHandle = svtkm::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(zipHandle,
                  svtkm::cont::internal::KeyCompare<T, svtkm::Id, BinaryCompare>(binary_compare),
                  svtkm::cont::internal::radix::PSortTag());
  }

  // Permute the values to their sorted locations:
  {
    auto valuesInPortal = values.PrepareForInput(DeviceAdapterTagOpenMP());
    auto indexPortal = indexArray.PrepareForInput(DeviceAdapterTagOpenMP());
    auto valuesOutPortal = valuesScattered.PrepareForOutput(size, DeviceAdapterTagOpenMP());

    SVTKM_OPENMP_DIRECTIVE(parallel for
                          default(none)
                          firstprivate(valuesInPortal, indexPortal, valuesOutPortal)
                          SVTKM_OPENMP_SHARED_CONST(size)
                          schedule(static))
    for (svtkm::Id i = 0; i < size; ++i)
    {
      valuesOutPortal.Set(i, valuesInPortal.Get(indexPortal.Get(i)));
    }
  }

  {
    auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagOpenMP());
    auto outputPortal =
      values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagOpenMP());
    openmp::CopyHelper(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
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
} // end namespace svtkm::cont::openmp::sort
