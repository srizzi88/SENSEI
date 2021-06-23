//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_openmp_internal_FunctorsOpenMP_h
#define svtk_m_cont_openmp_internal_FunctorsOpenMP_h

#include <svtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#include <svtkm/cont/internal/FunctorsGeneral.h>

#include <svtkm/BinaryOperators.h>
#include <svtkm/BinaryPredicates.h>
#include <svtkm/Pair.h>
#include <svtkm/Types.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ErrorExecution.h>

#include <omp.h>

#include <algorithm>
#include <type_traits>
#include <vector>

// Wrap all '#pragma omp ...' calls in this macro so we can disable them in
// non-omp builds and avoid a multitude of 'ignoring pragma..." warnings.
#ifdef _OPENMP
#define SVTKM_OPENMP_DIRECTIVE_IMPL(fullDir) _Pragma(#fullDir)
#define SVTKM_OPENMP_DIRECTIVE(dir) SVTKM_OPENMP_DIRECTIVE_IMPL(omp dir)
#else // _OPENMP
#define SVTKM_OPENMP_DIRECTIVE(directive)
#endif // _OPENMP

// See "OpenMP data sharing" section of
// https://www.gnu.org/software/gcc/gcc-9/porting_to.html. OpenMP broke
// backwards compatibility regarding const variable handling.
// tl;dr, put all const variables accessed from openmp blocks in a
// SVTKM_OPENMP_SHARED_CONST(var1, var2, ...) macro. This will do The Right Thing
// on all gcc.
#if defined(SVTKM_GCC) && (__GNUC__ < 9)
#define SVTKM_OPENMP_SHARED_CONST(...)
#else
#define SVTKM_OPENMP_SHARED_CONST(...) shared(__VA_ARGS__)
#endif

// When defined, supported type / operator combinations will use the OpenMP
// reduction(...) clause. Otherwise, all reductions use the general
// implementation with a manual reduction once the threads complete.
// I don't know how, but the benchmarks currently perform better without the
// specializations.
//#define SVTKM_OPENMP_USE_NATIVE_REDUCTION

namespace svtkm
{
namespace cont
{
namespace openmp
{

constexpr static svtkm::Id SVTKM_CACHE_LINE_SIZE = 64;
constexpr static svtkm::Id SVTKM_PAGE_SIZE = 4096;

// Returns ceil(num/den) for integral types
template <typename T>
static constexpr T CeilDivide(const T& numerator, const T& denominator)
{
  return (numerator + denominator - 1) / denominator;
}

// Computes the number of values per chunk. Note that numChunks + chunkSize may
// exceed numVals, so be sure to check upper limits.
static void ComputeChunkSize(const svtkm::Id numVals,
                             const svtkm::Id numThreads,
                             const svtkm::Id chunksPerThread,
                             const svtkm::Id bytesPerValue,
                             svtkm::Id& numChunks,
                             svtkm::Id& valuesPerChunk)
{
  // try to evenly distribute pages across chunks:
  const svtkm::Id bytesIn = numVals * bytesPerValue;
  const svtkm::Id pagesIn = CeilDivide(bytesIn, SVTKM_PAGE_SIZE);
  // If we don't have enough pages to honor chunksPerThread, ignore it:
  numChunks = (pagesIn > numThreads * chunksPerThread) ? numThreads * chunksPerThread : numThreads;
  const svtkm::Id pagesPerChunk = CeilDivide(pagesIn, numChunks);
  valuesPerChunk = CeilDivide(pagesPerChunk * SVTKM_PAGE_SIZE, bytesPerValue);
}

template <typename T, typename U>
static void DoCopy(T src, U dst, svtkm::Id numVals, std::true_type)
{
  if (numVals)
  {
    std::copy(src, src + numVals, dst);
  }
}

// Don't use std::copy when type conversion is required because MSVC.
template <typename InIterT, typename OutIterT>
static void DoCopy(InIterT inIter, OutIterT outIter, svtkm::Id numVals, std::false_type)
{
  using ValueType = typename std::iterator_traits<OutIterT>::value_type;

  for (svtkm::Id i = 0; i < numVals; ++i)
  {
    *(outIter++) = static_cast<ValueType>(*(inIter++));
  }
}

template <typename InIterT, typename OutIterT>
static void DoCopy(InIterT inIter, OutIterT outIter, svtkm::Id numVals)
{
  using InValueType = typename std::iterator_traits<InIterT>::value_type;
  using OutValueType = typename std::iterator_traits<OutIterT>::value_type;

  DoCopy(inIter, outIter, numVals, std::is_same<InValueType, OutValueType>());
}


template <typename InPortalT, typename OutPortalT>
static void CopyHelper(InPortalT inPortal,
                       OutPortalT outPortal,
                       svtkm::Id inStart,
                       svtkm::Id outStart,
                       svtkm::Id numVals)
{
  using InValueT = typename InPortalT::ValueType;
  using OutValueT = typename OutPortalT::ValueType;
  constexpr auto isSame = std::is_same<InValueT, OutValueT>();

  auto inIter = svtkm::cont::ArrayPortalToIteratorBegin(inPortal) + inStart;
  auto outIter = svtkm::cont::ArrayPortalToIteratorBegin(outPortal) + outStart;
  svtkm::Id valuesPerChunk;

  SVTKM_OPENMP_DIRECTIVE(parallel default(none) shared(inIter, outIter, valuesPerChunk, numVals)
                          SVTKM_OPENMP_SHARED_CONST(isSame))
  {

    SVTKM_OPENMP_DIRECTIVE(single)
    {
      // Evenly distribute full pages to all threads. We manually chunk the
      // data here so that we can exploit std::copy's memmove optimizations.
      svtkm::Id numChunks;
      ComputeChunkSize(
        numVals, omp_get_num_threads(), 8, sizeof(InValueT), numChunks, valuesPerChunk);
    }

    SVTKM_OPENMP_DIRECTIVE(for schedule(static))
    for (svtkm::Id i = 0; i < numVals; i += valuesPerChunk)
    {
      svtkm::Id chunkSize = std::min(numVals - i, valuesPerChunk);
      DoCopy(inIter + i, outIter + i, chunkSize, isSame);
    }
  }
}

struct CopyIfHelper
{
  svtkm::Id NumValues;
  svtkm::Id NumThreads;
  svtkm::Id ValueSize;

  svtkm::Id NumChunks;
  svtkm::Id ChunkSize;
  std::vector<svtkm::Id> EndIds;

  CopyIfHelper() = default;

  void Initialize(svtkm::Id numValues, svtkm::Id valueSize)
  {
    this->NumValues = numValues;
    this->NumThreads = static_cast<svtkm::Id>(omp_get_num_threads());
    this->ValueSize = valueSize;

    // Evenly distribute pages across the threads. We manually chunk the
    // data here so that we can exploit std::copy's memmove optimizations.
    ComputeChunkSize(
      this->NumValues, this->NumThreads, 8, valueSize, this->NumChunks, this->ChunkSize);

    this->EndIds.resize(static_cast<std::size_t>(this->NumChunks));
  }

  template <typename InIterT, typename StencilIterT, typename OutIterT, typename PredicateT>
  void CopyIf(InIterT inIter,
              StencilIterT stencilIter,
              OutIterT outIter,
              PredicateT pred,
              svtkm::Id chunk)
  {
    svtkm::Id startPos = std::min(chunk * this->ChunkSize, this->NumValues);
    svtkm::Id endPos = std::min((chunk + 1) * this->ChunkSize, this->NumValues);

    svtkm::Id outPos = startPos;
    for (svtkm::Id inPos = startPos; inPos < endPos; ++inPos)
    {
      if (pred(stencilIter[inPos]))
      {
        outIter[outPos++] = inIter[inPos];
      }
    }

    this->EndIds[static_cast<std::size_t>(chunk)] = outPos;
  }

  template <typename OutIterT>
  svtkm::Id Reduce(OutIterT data)
  {
    svtkm::Id endPos = this->EndIds.front();
    for (svtkm::Id i = 1; i < this->NumChunks; ++i)
    {
      svtkm::Id chunkStart = std::min(i * this->ChunkSize, this->NumValues);
      svtkm::Id chunkEnd = this->EndIds[static_cast<std::size_t>(i)];
      svtkm::Id numValuesToCopy = chunkEnd - chunkStart;
      if (numValuesToCopy > 0 && chunkStart != endPos)
      {
        std::copy(data + chunkStart, data + chunkEnd, data + endPos);
      }
      endPos += numValuesToCopy;
    }
    return endPos;
  }
};

#ifdef SVTKM_OPENMP_USE_NATIVE_REDUCTION
// OpenMP only declares reduction operations for primitive types. This utility
// detects if a type T is supported.
template <typename T>
struct OpenMPReductionSupported : std::false_type
{
};
template <>
struct OpenMPReductionSupported<Int8> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<UInt8> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<Int16> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<UInt16> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<Int32> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<UInt32> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<Int64> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<UInt64> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<Float32> : std::true_type
{
};
template <>
struct OpenMPReductionSupported<Float64> : std::true_type
{
};
#else
template <typename T>
using OpenMPReductionSupported = std::false_type;
#endif // SVTKM_OPENMP_USE_NATIVE_REDUCTION

struct ReduceHelper
{
  // std::is_integral, but adapted to see through vecs and pairs.
  template <typename T>
  struct IsIntegral : public std::is_integral<T>
  {
  };

  template <typename T, svtkm::IdComponent Size>
  struct IsIntegral<svtkm::Vec<T, Size>> : public std::is_integral<T>
  {
  };

  template <typename T, typename U>
  struct IsIntegral<svtkm::Pair<T, U>>
    : public std::integral_constant<bool, std::is_integral<T>::value && std::is_integral<U>::value>
  {
  };

  // Generic implementation:
  template <typename PortalT, typename ReturnType, typename Functor>
  static ReturnType Execute(PortalT portal, ReturnType init, Functor functorIn, std::false_type)
  {
    internal::WrappedBinaryOperator<ReturnType, Functor> f(functorIn);

    const svtkm::Id numVals = portal.GetNumberOfValues();
    auto data = svtkm::cont::ArrayPortalToIteratorBegin(portal);

    bool doParallel = false;
    int numThreads = 0;
    std::unique_ptr<ReturnType[]> threadData;

    SVTKM_OPENMP_DIRECTIVE(parallel default(none) firstprivate(f) shared(
      data, doParallel, numThreads, threadData) SVTKM_OPENMP_SHARED_CONST(numVals))
    {

      int tid = omp_get_thread_num();

      SVTKM_OPENMP_DIRECTIVE(single)
      {
        numThreads = omp_get_num_threads();
        if (numVals >= numThreads * 2)
        {
          doParallel = true;
          threadData.reset(new ReturnType[static_cast<std::size_t>(numThreads)]);
        }
      }

      if (doParallel)
      {
        // Static dispatch to unroll non-integral types:
        const ReturnType localResult = ReduceHelper::DoParallelReduction<ReturnType>(
          data, numVals, tid, numThreads, f, IsIntegral<ReturnType>{});

        threadData[static_cast<std::size_t>(tid)] = localResult;
      }
    } // end parallel

    if (doParallel)
    {
      // do the final reduction serially:
      for (size_t i = 0; i < static_cast<size_t>(numThreads); ++i)
      {
        init = f(init, threadData[i]);
      }
    }
    else
    {
      // Not enough threads. Do the entire reduction in serial:
      for (svtkm::Id i = 0; i < numVals; ++i)
      {
        init = f(init, data[i]);
      }
    }

    return init;
  }

  // non-integer reduction: unroll loop manually.
  // This gives faster code for floats and non-trivial types.
  template <typename ReturnType, typename IterType, typename FunctorType>
  static ReturnType DoParallelReduction(IterType data,
                                        svtkm::Id numVals,
                                        int tid,
                                        int numThreads,
                                        FunctorType f,
                                        std::false_type /* isIntegral */)
  {
    // Use the first (numThreads*2) values for initializing:
    ReturnType accum = f(data[2 * tid], data[2 * tid + 1]);

    svtkm::Id i = numThreads * 2;
    const svtkm::Id unrollEnd = ((numVals / 4) * 4) - 4;
    SVTKM_OPENMP_DIRECTIVE(for schedule(static))
    for (i = numThreads * 2; i < unrollEnd; i += 4)
    {
      const auto t1 = f(data[i], data[i + 1]);
      const auto t2 = f(data[i + 2], data[i + 3]);
      accum = f(accum, t1);
      accum = f(accum, t2);
    }
    // Let the last thread mop up any remaining values as it would
    // have just accessed the adjacent data
    if (tid == numThreads - 1)
    {
      for (i = unrollEnd; i < numVals; ++i)
      {
        accum = f(accum, data[i]);
      }
    }

    return accum;
  }

  // Integer reduction: no unrolling. Ints vectorize easily and unrolling can
  // hurt performance.
  template <typename ReturnType, typename IterType, typename FunctorType>
  static ReturnType DoParallelReduction(IterType data,
                                        svtkm::Id numVals,
                                        int tid,
                                        int numThreads,
                                        FunctorType f,
                                        std::true_type /* isIntegral */)
  {
    // Use the first (numThreads*2) values for initializing:
    ReturnType accum = f(data[2 * tid], data[2 * tid + 1]);

    // Assign each thread chunks of the remaining values for local reduction
    SVTKM_OPENMP_DIRECTIVE(for schedule(static))
    for (svtkm::Id i = numThreads * 2; i < numVals; i++)
    {
      accum = f(accum, data[i]);
    }

    return accum;
  }

#ifdef SVTKM_OPENMP_USE_NATIVE_REDUCTION

// Specialize for svtkm functors with OpenMP special cases:
#define SVTKM_OPENMP_SPECIALIZE_REDUCE1(FunctorType, PragmaString)                                  \
  template <typename PortalT, typename ReturnType>                                                 \
  static ReturnType Execute(                                                                       \
    PortalT portal, ReturnType value, FunctorType functorIn, std::true_type)                       \
  {                                                                                                \
    const svtkm::Id numValues = portal.GetNumberOfValues();                                         \
    internal::WrappedBinaryOperator<ReturnType, FunctorType> f(functorIn);                         \
    _Pragma(#PragmaString) for (svtkm::Id i = 0; i < numValues; ++i)                                \
    {                                                                                              \
      value = f(value, portal.Get(i));                                                             \
    }                                                                                              \
    return value;                                                                                  \
  }

// Constructing the pragma string inside the _Pragma call doesn't work so
// we jump through a hoop:
#define SVTKM_OPENMP_SPECIALIZE_REDUCE(FunctorType, Operator)                                       \
  SVTKM_OPENMP_SPECIALIZE_REDUCE1(FunctorType, "omp parallel for reduction(" #Operator ":value)")

  // + (Add, Sum)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::Add, +)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::Sum, +)
  // * (Multiply, Product)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::Multiply, *)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::Product, *)
  // - (Subtract)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::Subtract, -)
  // & (BitwiseAnd)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::BitwiseAnd, &)
  // | (BitwiseOr)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::BitwiseOr, |)
  // ^ (BitwiseXor)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::BitwiseXor, ^)
  // && (LogicalAnd)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::LogicalAnd, &&)
  // || (LogicalOr)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::LogicalOr, ||)
  // min (Minimum)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::Minimum, min)
  // max (Maximum)
  SVTKM_OPENMP_SPECIALIZE_REDUCE(svtkm::Maximum, max)

#undef SVTKM_OPENMP_SPECIALIZE_REDUCE
#undef SVTKM_OPENMP_SPECIALIZE_REDUCE1

#endif // SVTKM_OPENMP_USE_NATIVE_REDUCTION
};

template <typename KeysInArray,
          typename ValuesInArray,
          typename KeysOutArray,
          typename ValuesOutArray,
          typename BinaryFunctor>
void ReduceByKeyHelper(KeysInArray keysInArray,
                       ValuesInArray valuesInArray,
                       KeysOutArray keysOutArray,
                       ValuesOutArray valuesOutArray,
                       BinaryFunctor functor)
{
  using KeyType = typename KeysInArray::ValueType;
  using ValueType = typename ValuesInArray::ValueType;

  const svtkm::Id numValues = keysInArray.GetNumberOfValues();
  auto keysInPortal = keysInArray.PrepareForInput(DeviceAdapterTagOpenMP());
  auto valuesInPortal = valuesInArray.PrepareForInput(DeviceAdapterTagOpenMP());
  auto keysIn = svtkm::cont::ArrayPortalToIteratorBegin(keysInPortal);
  auto valuesIn = svtkm::cont::ArrayPortalToIteratorBegin(valuesInPortal);

  auto keysOutPortal = keysOutArray.PrepareForOutput(numValues, DeviceAdapterTagOpenMP());
  auto valuesOutPortal = valuesOutArray.PrepareForOutput(numValues, DeviceAdapterTagOpenMP());
  auto keysOut = svtkm::cont::ArrayPortalToIteratorBegin(keysOutPortal);
  auto valuesOut = svtkm::cont::ArrayPortalToIteratorBegin(valuesOutPortal);

  internal::WrappedBinaryOperator<ValueType, BinaryFunctor> f(functor);
  svtkm::Id outIdx = 0;

  SVTKM_OPENMP_DIRECTIVE(parallel default(none) firstprivate(keysIn, valuesIn, keysOut, valuesOut, f)
                          shared(outIdx) SVTKM_OPENMP_SHARED_CONST(numValues))
  {
    int tid = omp_get_thread_num();
    int numThreads = omp_get_num_threads();

    // Determine bounds for this thread's scan operation:
    svtkm::Id chunkSize = (numValues + numThreads - 1) / numThreads;
    svtkm::Id scanIdx = std::min(tid * chunkSize, numValues);
    svtkm::Id scanEnd = std::min(scanIdx + chunkSize, numValues);

    auto threadKeysBegin = keysOut + scanIdx;
    auto threadValuesBegin = valuesOut + scanIdx;
    auto threadKey = threadKeysBegin;
    auto threadValue = threadValuesBegin;

    // Reduce each thread's partition:
    KeyType rangeKey;
    ValueType rangeValue;
    for (;;)
    {
      if (scanIdx < scanEnd)
      {
        rangeKey = keysIn[scanIdx];
        rangeValue = valuesIn[scanIdx];
        ++scanIdx;

        // Locate end of current range:
        while (scanIdx < scanEnd && static_cast<KeyType>(keysIn[scanIdx]) == rangeKey)
        {
          rangeValue = f(rangeValue, valuesIn[scanIdx]);
          ++scanIdx;
        }

        *threadKey = rangeKey;
        *threadValue = rangeValue;
        ++threadKey;
        ++threadValue;
      }
      else
      {
        break;
      }
    }

    if (tid == 0)
    {
      outIdx = static_cast<svtkm::Id>(threadKey - threadKeysBegin);
    }

    // Combine the reduction results. Skip tid == 0, since it's already in
    // the correct location:
    for (int i = 1; i < numThreads; ++i)
    {

      // This barrier ensures that:
      // 1) Threads remain synchronized through this final reduction loop.
      // 2) The outIdx variable is initialized by thread 0.
      // 3) All threads have reduced their partitions.
      SVTKM_OPENMP_DIRECTIVE(barrier)

      if (tid == i)
      {
        // Check if the previous thread's last key matches our first:
        if (outIdx > 0 && threadKeysBegin < threadKey && keysOut[outIdx - 1] == *threadKeysBegin)
        {
          valuesOut[outIdx - 1] = f(valuesOut[outIdx - 1], *threadValuesBegin);
          ++threadKeysBegin;
          ++threadValuesBegin;
        }

        // Copy reduced partition to final location (if needed)
        if (threadKeysBegin < threadKey && threadKeysBegin != keysOut + outIdx)
        {
          std::copy(threadKeysBegin, threadKey, keysOut + outIdx);
          std::copy(threadValuesBegin, threadValue, valuesOut + outIdx);
        }

        outIdx += static_cast<svtkm::Id>(threadKey - threadKeysBegin);

      } // end tid == i
    }   // end combine reduction
  }     // end parallel

  keysOutArray.Shrink(outIdx);
  valuesOutArray.Shrink(outIdx);
}

template <typename IterT, typename RawPredicateT>
struct UniqueHelper
{
  using ValueType = typename std::iterator_traits<IterT>::value_type;
  using PredicateT = internal::WrappedBinaryOperator<bool, RawPredicateT>;

  struct Node
  {
    svtkm::Id2 InputRange{ -1, -1 };
    svtkm::Id2 OutputRange{ -1, -1 };

    // Pad the node out to the size of a cache line to prevent false sharing:
    static constexpr size_t DataSize = 2 * sizeof(svtkm::Id2);
    static constexpr size_t NumCacheLines = CeilDivide<size_t>(DataSize, SVTKM_CACHE_LINE_SIZE);
    static constexpr size_t PaddingSize = NumCacheLines * SVTKM_CACHE_LINE_SIZE - DataSize;
    unsigned char Padding[PaddingSize];
  };

  IterT Data;
  svtkm::Id NumValues;
  PredicateT Predicate;
  svtkm::Id LeafSize;
  std::vector<Node> Nodes;
  size_t NextNode;

  UniqueHelper(IterT iter, svtkm::Id numValues, RawPredicateT pred)
    : Data(iter)
    , NumValues(numValues)
    , Predicate(pred)
    , LeafSize(0)
    , NextNode(0)
  {
  }

  svtkm::Id Execute()
  {
    svtkm::Id outSize = 0;

    SVTKM_OPENMP_DIRECTIVE(parallel default(shared))
    {
      SVTKM_OPENMP_DIRECTIVE(single)
      {
        this->Prepare();

        // Kick off task-based divide-and-conquer uniquification:
        Node* rootNode = this->AllocNode();
        rootNode->InputRange = svtkm::Id2(0, this->NumValues);
        this->Uniquify(rootNode);
        outSize = rootNode->OutputRange[1] - rootNode->OutputRange[0];
      }
    }

    return outSize;
  }

private:
  void Prepare()
  {
    // Figure out how many values each thread should handle:
    int numThreads = omp_get_num_threads();
    svtkm::Id chunksPerThread = 8;
    svtkm::Id numChunks;
    ComputeChunkSize(
      this->NumValues, numThreads, chunksPerThread, sizeof(ValueType), numChunks, this->LeafSize);

    // Compute an upper-bound of the number of nodes in the tree:
    std::size_t numNodes = static_cast<std::size_t>(numChunks);
    while (numChunks > 1)
    {
      numChunks = (numChunks + 1) / 2;
      numNodes += static_cast<std::size_t>(numChunks);
    }
    this->Nodes.resize(numNodes);
    this->NextNode = 0;
  }

  Node* AllocNode()
  {
    size_t nodeIdx;

// GCC emits a false positive "value computed but not used" for this block:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

    SVTKM_OPENMP_DIRECTIVE(atomic capture)
    {
      nodeIdx = this->NextNode;
      ++this->NextNode;
    }

#pragma GCC diagnostic pop

    SVTKM_ASSERT(nodeIdx < this->Nodes.size());

    return &this->Nodes[nodeIdx];
  }

  bool IsLeaf(const svtkm::Id2& range) { return (range[1] - range[0]) <= this->LeafSize; }

  // Not an strict midpoint, but ensures that the first range will always be
  // a multiple of the leaf size.
  svtkm::Id ComputeMidpoint(const svtkm::Id2& range)
  {
    const svtkm::Id n = range[1] - range[0];
    const svtkm::Id np = this->LeafSize;

    return CeilDivide(n / 2, np) * np + range[0];
  }

  void Uniquify(Node* node)
  {
    if (!this->IsLeaf(node->InputRange))
    {
      svtkm::Id midpoint = this->ComputeMidpoint(node->InputRange);

      Node* right = this->AllocNode();
      Node* left = this->AllocNode();

      right->InputRange = svtkm::Id2(midpoint, node->InputRange[1]);

      // Intel compilers seem to have trouble following the 'this' pointer
      // when launching tasks, resulting in a corrupt task environment.
      // Explicitly copying the pointer into a local variable seems to fix this.
      auto explicitThis = this;

      SVTKM_OPENMP_DIRECTIVE(taskgroup)
      {
        SVTKM_OPENMP_DIRECTIVE(task) { explicitThis->Uniquify(right); }

        left->InputRange = svtkm::Id2(node->InputRange[0], midpoint);
        this->Uniquify(left);

      } // end taskgroup. Both sides of the tree will be completed here.

      // Combine the ranges in the left side:
      if (this->Predicate(this->Data[left->OutputRange[1] - 1], this->Data[right->OutputRange[0]]))
      {
        ++right->OutputRange[0];
      }

      svtkm::Id numVals = right->OutputRange[1] - right->OutputRange[0];
      DoCopy(this->Data + right->OutputRange[0], this->Data + left->OutputRange[1], numVals);

      node->OutputRange[0] = left->OutputRange[0];
      node->OutputRange[1] = left->OutputRange[1] + numVals;
    }
    else
    {
      auto start = this->Data + node->InputRange[0];
      auto end = this->Data + node->InputRange[1];
      end = std::unique(start, end, this->Predicate);
      node->OutputRange[0] = node->InputRange[0];
      node->OutputRange[1] = node->InputRange[0] + static_cast<svtkm::Id>(end - start);
    }
  }
};
}
}
} // end namespace svtkm::cont::openmp

#endif // svtk_m_cont_openmp_internal_FunctorsOpenMP_h
