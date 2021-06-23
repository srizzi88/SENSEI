//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Math.h>
#include <svtkm/VectorAnalysis.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleMultiplexer.h>
#include <svtkm/cont/ArrayHandleVirtual.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/ImplicitFunctionHandle.h>
#include <svtkm/cont/Initialize.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include "Benchmarker.h"
#include <svtkm/cont/testing/Testing.h>

#include <cctype>
#include <random>
#include <string>
#include <utility>

namespace
{

//==============================================================================
// Benchmark Parameters

#define ARRAY_SIZE (1 << 22)
#define CUBE_SIZE 256

using ValueTypes = svtkm::List<svtkm::Float32, svtkm::Float64>;
using InterpValueTypes = svtkm::List<svtkm::Float32, svtkm::Vec3f_32>;

//==============================================================================
// Worklets and helpers

// Hold configuration state (e.g. active device)
svtkm::cont::InitializeResult Config;

template <typename T>
class BlackScholes : public svtkm::worklet::WorkletMapField
{
  T Riskfree;
  T Volatility;

public:
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  BlackScholes(T risk, T volatility)
    : Riskfree(risk)
    , Volatility(volatility)
  {
  }

  SVTKM_EXEC
  T CumulativeNormalDistribution(T d) const
  {
    const svtkm::Float32 A1 = 0.31938153f;
    const svtkm::Float32 A2 = -0.356563782f;
    const svtkm::Float32 A3 = 1.781477937f;
    const svtkm::Float32 A4 = -1.821255978f;
    const svtkm::Float32 A5 = 1.330274429f;
    const svtkm::Float32 RSQRT2PI = 0.39894228040143267793994605993438f;

    const svtkm::Float32 df = static_cast<svtkm::Float32>(d);
    const svtkm::Float32 K = 1.0f / (1.0f + 0.2316419f * svtkm::Abs(df));

    svtkm::Float32 cnd =
      RSQRT2PI * svtkm::Exp(-0.5f * df * df) * (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

    if (df > 0.0f)
    {
      cnd = 1.0f - cnd;
    }

    return static_cast<T>(cnd);
  }

  template <typename U, typename V, typename W>
  SVTKM_EXEC void operator()(const U& sp, const V& os, const W& oy, T& callResult, T& putResult)
    const
  {
    const T stockPrice = static_cast<T>(sp);
    const T optionStrike = static_cast<T>(os);
    const T optionYears = static_cast<T>(oy);

    // Black-Scholes formula for both call and put
    const T sqrtYears = svtkm::Sqrt(optionYears);
    const T volMultSqY = this->Volatility * sqrtYears;

    const T d1 = (svtkm::Log(stockPrice / optionStrike) +
                  (this->Riskfree + 0.5f * Volatility * Volatility) * optionYears) /
      (volMultSqY);
    const T d2 = d1 - volMultSqY;
    const T CNDD1 = CumulativeNormalDistribution(d1);
    const T CNDD2 = CumulativeNormalDistribution(d2);

    //Calculate Call and Put simultaneously
    T expRT = svtkm::Exp(-this->Riskfree * optionYears);
    callResult = stockPrice * CNDD1 - optionStrike * expRT * CNDD2;
    putResult = optionStrike * expRT * (1.0f - CNDD2) - stockPrice * (1.0f - CNDD1);
  }
};

class Mag : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  SVTKM_EXEC void operator()(const svtkm::Vec<T, 3>& vec, U& result) const
  {
    result = static_cast<U>(svtkm::Magnitude(vec));
  }
};

class Square : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  SVTKM_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(input * input);
  }
};

class Sin : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  SVTKM_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(svtkm::Sin(input));
  }
};

class Cos : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  SVTKM_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(svtkm::Cos(input));
  }
};

class FusedMath : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  SVTKM_EXEC void operator()(const svtkm::Vec<T, 3>& vec, T& result) const
  {
    const T m = svtkm::Magnitude(vec);
    result = svtkm::Cos(svtkm::Sin(m) * svtkm::Sin(m));
  }

  template <typename T, typename U>
  SVTKM_EXEC void operator()(const svtkm::Vec<T, 3>&, U&) const
  {
    this->RaiseError("Mixed types unsupported.");
  }
};

class GenerateEdges : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, WholeArrayOut edgeIds);
  using ExecutionSignature = void(PointIndices, ThreadIndices, _2);
  using InputDomain = _1;

  template <typename ConnectivityInVec, typename ThreadIndicesType, typename IdPairTableType>
  SVTKM_EXEC void operator()(const ConnectivityInVec& connectivity,
                            const ThreadIndicesType threadIndices,
                            const IdPairTableType& edgeIds) const
  {
    const svtkm::Id writeOffset = (threadIndices.GetInputIndex() * 12);

    const svtkm::IdComponent edgeTable[24] = { 0, 1, 1, 2, 3, 2, 0, 3, 4, 5, 5, 6,
                                              7, 6, 4, 7, 0, 4, 1, 5, 2, 6, 3, 7 };

    for (svtkm::Id i = 0; i < 12; ++i)
    {
      const svtkm::Id offset = (i * 2);
      const svtkm::Id2 edge(connectivity[edgeTable[offset]], connectivity[edgeTable[offset + 1]]);
      edgeIds.Set(writeOffset + i, edge);
    }
  }
};

class InterpolateField : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn interpolation_ids,
                                FieldIn interpolation_weights,
                                WholeArrayIn inputField,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename WeightType, typename T, typename S, typename D>
  SVTKM_EXEC void operator()(const svtkm::Id2& low_high,
                            const WeightType& weight,
                            const svtkm::exec::ExecutionWholeArrayConst<T, S, D>& inPortal,
                            T& result) const
  {
    //fetch the low / high values from inPortal
    result = svtkm::Lerp(inPortal.Get(low_high[0]), inPortal.Get(low_high[1]), weight);
  }

  template <typename WeightType, typename T, typename S, typename D, typename U>
  SVTKM_EXEC void operator()(const svtkm::Id2&,
                            const WeightType&,
                            const svtkm::exec::ExecutionWholeArrayConst<T, S, D>&,
                            U&) const
  {
    //the inPortal and result need to be the same type so this version only
    //exists to generate code when using dynamic arrays
    this->RaiseError("Mixed types unsupported.");
  }
};

template <typename ImplicitFunction>
class EvaluateImplicitFunction : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  EvaluateImplicitFunction(const ImplicitFunction* function)
    : Function(function)
  {
  }

  template <typename VecType, typename ScalarType>
  SVTKM_EXEC void operator()(const VecType& point, ScalarType& val) const
  {
    val = this->Function->Value(point);
  }

private:
  const ImplicitFunction* Function;
};

template <typename T1, typename T2>
class Evaluate2ImplicitFunctions : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  Evaluate2ImplicitFunctions(const T1* f1, const T2* f2)
    : Function1(f1)
    , Function2(f2)
  {
  }

  template <typename VecType, typename ScalarType>
  SVTKM_EXEC void operator()(const VecType& point, ScalarType& val) const
  {
    val = this->Function1->Value(point) + this->Function2->Value(point);
  }

private:
  const T1* Function1;
  const T2* Function2;
};

struct PassThroughFunctor
{
  template <typename T>
  SVTKM_EXEC_CONT T operator()(const T& x) const
  {
    return x;
  }
};

template <typename ArrayHandleType>
using ArrayHandlePassThrough =
  svtkm::cont::ArrayHandleTransform<ArrayHandleType, PassThroughFunctor, PassThroughFunctor>;

template <typename ValueType, svtkm::IdComponent>
struct JunkArrayHandle : svtkm::cont::ArrayHandle<ValueType>
{
};

template <typename ArrayHandleType>
using BMArrayHandleMultiplexer =
  svtkm::cont::ArrayHandleMultiplexer<ArrayHandleType,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 0>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 1>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 2>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 3>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 4>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 5>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 6>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 7>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 8>,
                                     JunkArrayHandle<typename ArrayHandleType::ValueType, 9>,
                                     ArrayHandlePassThrough<ArrayHandleType>>;

template <typename ArrayHandleType>
BMArrayHandleMultiplexer<ArrayHandleType> make_ArrayHandleMultiplexer0(const ArrayHandleType& array)
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  return BMArrayHandleMultiplexer<ArrayHandleType>(array);
}

template <typename ArrayHandleType>
BMArrayHandleMultiplexer<ArrayHandleType> make_ArrayHandleMultiplexerN(const ArrayHandleType& array)
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  return BMArrayHandleMultiplexer<ArrayHandleType>(ArrayHandlePassThrough<ArrayHandleType>(array));
}


//==============================================================================
// Benchmark implementations:

template <typename Value>
struct BenchBlackScholesImpl
{
  using ValueArrayHandle = svtkm::cont::ArrayHandle<Value>;

  ValueArrayHandle StockPrice;
  ValueArrayHandle OptionStrike;
  ValueArrayHandle OptionYears;

  ::benchmark::State& State;
  svtkm::Id ArraySize;

  svtkm::cont::Timer Timer;
  svtkm::cont::Invoker Invoker;

  SVTKM_CONT
  BenchBlackScholesImpl(::benchmark::State& state)
    : State{ state }
    , ArraySize{ ARRAY_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {

    { // Initialize arrays
      std::mt19937 rng;
      std::uniform_real_distribution<Value> price_range(Value(5.0f), Value(30.0f));
      std::uniform_real_distribution<Value> strike_range(Value(1.0f), Value(100.0f));
      std::uniform_real_distribution<Value> year_range(Value(0.25f), Value(10.0f));

      this->StockPrice.Allocate(this->ArraySize);
      this->OptionStrike.Allocate(this->ArraySize);
      this->OptionYears.Allocate(this->ArraySize);

      auto stockPricePortal = this->StockPrice.GetPortalControl();
      auto optionStrikePortal = this->OptionStrike.GetPortalControl();
      auto optionYearsPortal = this->OptionYears.GetPortalControl();

      for (svtkm::Id i = 0; i < this->ArraySize; ++i)
      {
        stockPricePortal.Set(i, price_range(rng));
        optionStrikePortal.Set(i, strike_range(rng));
        optionYearsPortal.Set(i, year_range(rng));
      }
    }

    { // Configure label:
      const svtkm::Id numBytes = this->ArraySize * static_cast<svtkm::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " (" << svtkm::cont::GetHumanReadableSize(numBytes)
           << ")";
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  SVTKM_CONT void Run(const BenchArrayType& stockPrice,
                     const BenchArrayType& optionStrike,
                     const BenchArrayType& optionYears)
  {
    static constexpr Value RISKFREE = 0.02f;
    static constexpr Value VOLATILITY = 0.30f;

    BlackScholes<Value> worklet(RISKFREE, VOLATILITY);
    svtkm::cont::ArrayHandle<Value> callResultHandle;
    svtkm::cont::ArrayHandle<Value> putResultHandle;

    for (auto _ : this->State)
    {
      (void)_;
      this->Timer.Start();
      this->Invoker(
        worklet, stockPrice, optionStrike, optionYears, callResultHandle, putResultHandle);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    const int64_t numValues = static_cast<int64_t>(this->ArraySize);
    this->State.SetItemsProcessed(numValues * iterations);
  }
};

template <typename ValueType>
void BenchBlackScholesStatic(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(impl.StockPrice, impl.OptionStrike, impl.OptionYears);
};
SVTKM_BENCHMARK_TEMPLATES(BenchBlackScholesStatic, ValueTypes);

template <typename ValueType>
void BenchBlackScholesDynamic(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(svtkm::cont::make_ArrayHandleVirtual(impl.StockPrice),
           svtkm::cont::make_ArrayHandleVirtual(impl.OptionStrike),
           svtkm::cont::make_ArrayHandleVirtual(impl.OptionYears));
};
SVTKM_BENCHMARK_TEMPLATES(BenchBlackScholesDynamic, ValueTypes);

template <typename ValueType>
void BenchBlackScholesMultiplexer0(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.StockPrice),
           make_ArrayHandleMultiplexer0(impl.OptionStrike),
           make_ArrayHandleMultiplexer0(impl.OptionYears));
};
SVTKM_BENCHMARK_TEMPLATES(BenchBlackScholesMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchBlackScholesMultiplexerN(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.StockPrice),
           make_ArrayHandleMultiplexerN(impl.OptionStrike),
           make_ArrayHandleMultiplexerN(impl.OptionYears));
};
SVTKM_BENCHMARK_TEMPLATES(BenchBlackScholesMultiplexerN, ValueTypes);

template <typename Value>
struct BenchMathImpl
{
  svtkm::cont::ArrayHandle<svtkm::Vec<Value, 3>> InputHandle;
  svtkm::cont::ArrayHandle<Value> TempHandle1;
  svtkm::cont::ArrayHandle<Value> TempHandle2;

  ::benchmark::State& State;
  svtkm::Id ArraySize;

  svtkm::cont::Timer Timer;
  svtkm::cont::Invoker Invoker;

  SVTKM_CONT
  BenchMathImpl(::benchmark::State& state)
    : State{ state }
    , ArraySize{ ARRAY_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    { // Initialize input
      std::mt19937 rng;
      std::uniform_real_distribution<Value> range;

      this->InputHandle.Allocate(this->ArraySize);
      auto portal = this->InputHandle.GetPortalControl();
      for (svtkm::Id i = 0; i < this->ArraySize; ++i)
      {
        portal.Set(i, svtkm::Vec<Value, 3>{ range(rng), range(rng), range(rng) });
      }
    }
  }

  template <typename InputArrayType, typename BenchArrayType>
  SVTKM_CONT void Run(const InputArrayType& inputHandle,
                     const BenchArrayType& tempHandle1,
                     const BenchArrayType& tempHandle2)
  {
    { // Configure label:
      const svtkm::Id numBytes = this->ArraySize * static_cast<svtkm::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " (" << svtkm::cont::GetHumanReadableSize(numBytes)
           << ")";
      this->State.SetLabel(desc.str());
    }

    for (auto _ : this->State)
    {
      (void)_;

      this->Timer.Start();
      this->Invoker(Mag{}, inputHandle, tempHandle1);
      this->Invoker(Sin{}, tempHandle1, tempHandle2);
      this->Invoker(Square{}, tempHandle2, tempHandle1);
      this->Invoker(Cos{}, tempHandle1, tempHandle2);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    const int64_t numValues = static_cast<int64_t>(this->ArraySize);
    this->State.SetItemsProcessed(numValues * iterations);
  }
};

template <typename ValueType>
void BenchMathStatic(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(impl.InputHandle, impl.TempHandle1, impl.TempHandle2);
};
SVTKM_BENCHMARK_TEMPLATES(BenchMathStatic, ValueTypes);

template <typename ValueType>
void BenchMathDynamic(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(svtkm::cont::make_ArrayHandleVirtual(impl.InputHandle),
           svtkm::cont::make_ArrayHandleVirtual(impl.TempHandle1),
           svtkm::cont::make_ArrayHandleVirtual(impl.TempHandle2));
};
SVTKM_BENCHMARK_TEMPLATES(BenchMathDynamic, ValueTypes);

template <typename ValueType>
void BenchMathMultiplexer0(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.InputHandle),
           make_ArrayHandleMultiplexer0(impl.TempHandle1),
           make_ArrayHandleMultiplexer0(impl.TempHandle2));
};
SVTKM_BENCHMARK_TEMPLATES(BenchMathMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchMathMultiplexerN(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.InputHandle),
           make_ArrayHandleMultiplexerN(impl.TempHandle1),
           make_ArrayHandleMultiplexerN(impl.TempHandle2));
};
SVTKM_BENCHMARK_TEMPLATES(BenchMathMultiplexerN, ValueTypes);

template <typename Value>
struct BenchFusedMathImpl
{
  svtkm::cont::ArrayHandle<svtkm::Vec<Value, 3>> InputHandle;

  ::benchmark::State& State;
  svtkm::Id ArraySize;

  svtkm::cont::Timer Timer;
  svtkm::cont::Invoker Invoker;

  SVTKM_CONT
  BenchFusedMathImpl(::benchmark::State& state)
    : State{ state }
    , ArraySize{ ARRAY_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    { // Initialize input
      std::mt19937 rng;
      std::uniform_real_distribution<Value> range;

      this->InputHandle.Allocate(this->ArraySize);
      auto portal = this->InputHandle.GetPortalControl();
      for (svtkm::Id i = 0; i < this->ArraySize; ++i)
      {
        portal.Set(i, svtkm::Vec<Value, 3>{ range(rng), range(rng), range(rng) });
      }
    }

    { // Configure label:
      const svtkm::Id numBytes = this->ArraySize * static_cast<svtkm::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " (" << svtkm::cont::GetHumanReadableSize(numBytes)
           << ")";
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  SVTKM_CONT void Run(const BenchArrayType& inputHandle)
  {
    svtkm::cont::ArrayHandle<Value> result;

    for (auto _ : this->State)
    {
      (void)_;

      this->Timer.Start();
      this->Invoker(FusedMath{}, inputHandle, result);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    const int64_t numValues = static_cast<int64_t>(this->ArraySize);
    this->State.SetItemsProcessed(numValues * iterations);
  }
};

template <typename ValueType>
void BenchFusedMathStatic(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(impl.InputHandle);
};
SVTKM_BENCHMARK_TEMPLATES(BenchFusedMathStatic, ValueTypes);

template <typename ValueType>
void BenchFusedMathDynamic(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(svtkm::cont::make_ArrayHandleVirtual(impl.InputHandle));
};
SVTKM_BENCHMARK_TEMPLATES(BenchFusedMathDynamic, ValueTypes);

template <typename ValueType>
void BenchFusedMathMultiplexer0(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.InputHandle));
};
SVTKM_BENCHMARK_TEMPLATES(BenchFusedMathMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchFusedMathMultiplexerN(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.InputHandle));
};
SVTKM_BENCHMARK_TEMPLATES(BenchFusedMathMultiplexerN, ValueTypes);

template <typename Value>
struct BenchEdgeInterpImpl
{
  svtkm::cont::ArrayHandle<svtkm::Float32> WeightHandle;
  svtkm::cont::ArrayHandle<Value> FieldHandle;
  svtkm::cont::ArrayHandle<svtkm::Id2> EdgePairHandle;

  ::benchmark::State& State;
  svtkm::Id CubeSize;

  svtkm::cont::Timer Timer;
  svtkm::cont::Invoker Invoker;

  SVTKM_CONT
  BenchEdgeInterpImpl(::benchmark::State& state)
    : State{ state }
    , CubeSize{ CUBE_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    { // Initialize arrays
      using CT = typename svtkm::VecTraits<Value>::ComponentType;

      std::mt19937 rng;
      std::uniform_real_distribution<svtkm::Float32> weight_range(0.0f, 1.0f);
      std::uniform_real_distribution<CT> field_range;

      //basically the core challenge is to generate an array whose
      //indexing pattern matches that of a edge based algorithm.
      //
      //So for this kind of problem we generate the 12 edges of each
      //cell and place them into array.
      svtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(svtkm::Id3{ this->CubeSize, this->CubeSize, this->CubeSize });

      const svtkm::Id numberOfEdges = cellSet.GetNumberOfCells() * 12;

      this->EdgePairHandle.Allocate(numberOfEdges);
      this->Invoker(GenerateEdges{}, cellSet, this->EdgePairHandle);

      { // Per-edge weights
        this->WeightHandle.Allocate(numberOfEdges);
        auto portal = this->WeightHandle.GetPortalControl();
        for (svtkm::Id i = 0; i < numberOfEdges; ++i)
        {
          portal.Set(i, weight_range(rng));
        }
      }

      { // Point field
        this->FieldHandle.Allocate(cellSet.GetNumberOfPoints());
        auto portal = this->FieldHandle.GetPortalControl();
        for (svtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
        {
          portal.Set(i, field_range(rng));
        }
      }
    }

    { // Configure label:
      const svtkm::Id numValues = this->FieldHandle.GetNumberOfValues();
      const svtkm::Id numBytes = numValues * static_cast<svtkm::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "FieldValues:" << numValues << " (" << svtkm::cont::GetHumanReadableSize(numBytes)
           << ") | CubeSize: " << this->CubeSize;
      this->State.SetLabel(desc.str());
    }
  }

  template <typename EdgePairArrayType, typename WeightArrayType, typename FieldArrayType>
  SVTKM_CONT void Run(const EdgePairArrayType& edgePairs,
                     const WeightArrayType& weights,
                     const FieldArrayType& field)
  {
    svtkm::cont::ArrayHandle<Value> result;

    for (auto _ : this->State)
    {
      (void)_;
      this->Timer.Start();
      this->Invoker(InterpolateField{}, edgePairs, weights, field, result);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }
  }
};

template <typename ValueType>
void BenchEdgeInterpStatic(::benchmark::State& state)
{
  BenchEdgeInterpImpl<ValueType> impl{ state };
  impl.Run(impl.EdgePairHandle, impl.WeightHandle, impl.FieldHandle);
};
SVTKM_BENCHMARK_TEMPLATES(BenchEdgeInterpStatic, InterpValueTypes);

template <typename ValueType>
void BenchEdgeInterpDynamic(::benchmark::State& state)
{
  BenchEdgeInterpImpl<ValueType> impl{ state };
  impl.Run(svtkm::cont::make_ArrayHandleVirtual(impl.EdgePairHandle),
           svtkm::cont::make_ArrayHandleVirtual(impl.WeightHandle),
           svtkm::cont::make_ArrayHandleVirtual(impl.FieldHandle));
};
SVTKM_BENCHMARK_TEMPLATES(BenchEdgeInterpDynamic, InterpValueTypes);

struct ImplicitFunctionBenchData
{
  svtkm::cont::ArrayHandle<svtkm::Vec3f> Points;
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> Result;
  svtkm::Sphere Sphere1;
  svtkm::Sphere Sphere2;
};

static ImplicitFunctionBenchData MakeImplicitFunctionBenchData()
{
  svtkm::Id count = ARRAY_SIZE;
  svtkm::FloatDefault bounds[6] = { -2.0f, 2.0f, -2.0f, 2.0f, -2.0f, 2.0f };

  ImplicitFunctionBenchData data;
  data.Points.Allocate(count);
  data.Result.Allocate(count);

  std::default_random_engine rangen;
  std::uniform_real_distribution<svtkm::FloatDefault> distx(bounds[0], bounds[1]);
  std::uniform_real_distribution<svtkm::FloatDefault> disty(bounds[2], bounds[3]);
  std::uniform_real_distribution<svtkm::FloatDefault> distz(bounds[4], bounds[5]);

  auto portal = data.Points.GetPortalControl();
  for (svtkm::Id i = 0; i < count; ++i)
  {
    portal.Set(i, svtkm::make_Vec(distx(rangen), disty(rangen), distz(rangen)));
  }

  data.Sphere1 = svtkm::Sphere({ 0.22f, 0.33f, 0.44f }, 0.55f);
  data.Sphere2 = svtkm::Sphere({ 0.22f, 0.33f, 0.11f }, 0.77f);

  return data;
}

void BenchImplicitFunction(::benchmark::State& state)
{
  using EvalWorklet = EvaluateImplicitFunction<svtkm::Sphere>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  auto handle = svtkm::cont::make_ImplicitFunctionHandle(data.Sphere1);
  auto function = static_cast<const svtkm::Sphere*>(handle.PrepareForExecution(device));
  EvalWorklet eval(function);

  svtkm::cont::Timer timer{ device };
  svtkm::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchImplicitFunction);

void BenchVirtualImplicitFunction(::benchmark::State& state)
{
  using EvalWorklet = EvaluateImplicitFunction<svtkm::ImplicitFunction>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  auto sphere = svtkm::cont::make_ImplicitFunctionHandle(data.Sphere1);
  EvalWorklet eval(sphere.PrepareForExecution(device));

  svtkm::cont::Timer timer{ device };
  svtkm::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchVirtualImplicitFunction);

void Bench2ImplicitFunctions(::benchmark::State& state)
{
  using EvalWorklet = Evaluate2ImplicitFunctions<svtkm::Sphere, svtkm::Sphere>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  auto h1 = svtkm::cont::make_ImplicitFunctionHandle(data.Sphere1);
  auto h2 = svtkm::cont::make_ImplicitFunctionHandle(data.Sphere2);
  auto f1 = static_cast<const svtkm::Sphere*>(h1.PrepareForExecution(device));
  auto f2 = static_cast<const svtkm::Sphere*>(h2.PrepareForExecution(device));
  EvalWorklet eval(f1, f2);

  svtkm::cont::Timer timer{ device };
  svtkm::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(Bench2ImplicitFunctions);

void Bench2VirtualImplicitFunctions(::benchmark::State& state)
{
  using EvalWorklet = Evaluate2ImplicitFunctions<svtkm::ImplicitFunction, svtkm::ImplicitFunction>;

  const svtkm::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  auto s1 = svtkm::cont::make_ImplicitFunctionHandle(data.Sphere1);
  auto s2 = svtkm::cont::make_ImplicitFunctionHandle(data.Sphere2);
  EvalWorklet eval(s1.PrepareForExecution(device), s2.PrepareForExecution(device));

  svtkm::cont::Timer timer{ device };
  svtkm::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(Bench2VirtualImplicitFunctions);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse SVTK-m options:
  auto opts = svtkm::cont::InitializeOptions::RequireDevice | svtkm::cont::InitializeOptions::AddHelp;
  Config = svtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

  // handle benchmarking related args and run benchmarks:
  SVTKM_EXECUTE_BENCHMARKS(argc, argv);
}
