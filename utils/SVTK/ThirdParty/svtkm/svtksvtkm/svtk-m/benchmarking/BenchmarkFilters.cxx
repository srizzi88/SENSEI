//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "Benchmarker.h"

#include <svtkm/Math.h>
#include <svtkm/Range.h>
#include <svtkm/VecTraits.h>

#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/ErrorInternal.h>
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/StorageBasic.h>
#include <svtkm/cont/Timer.h>

#include <svtkm/cont/internal/OptionParser.h>

#include <svtkm/filter/CellAverage.h>
#include <svtkm/filter/Contour.h>
#include <svtkm/filter/ExternalFaces.h>
#include <svtkm/filter/FieldSelection.h>
#include <svtkm/filter/Gradient.h>
#include <svtkm/filter/PointAverage.h>
#include <svtkm/filter/PolicyBase.h>
#include <svtkm/filter/Tetrahedralize.h>
#include <svtkm/filter/Threshold.h>
#include <svtkm/filter/ThresholdPoints.h>
#include <svtkm/filter/VectorMagnitude.h>
#include <svtkm/filter/VertexClustering.h>
#include <svtkm/filter/WarpScalar.h>
#include <svtkm/filter/WarpVector.h>

#include <svtkm/io/reader/SVTKDataSetReader.h>

#include <svtkm/source/Wavelet.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <cctype> // for std::tolower
#include <sstream>
#include <type_traits>

#ifdef SVTKM_ENABLE_TBB
#include <tbb/task_scheduler_init.h>
#endif
#ifdef SVTKM_ENABLE_OPENMP
#include <omp.h>
#endif

// A specific svtk dataset can be used during benchmarking using the
// "Filename [filename]" argument.

// Otherwise a wavelet dataset will be used. The size of the wavelet can be
// specified via "WaveletDim [integer]" argument. The default is 256, resulting
// in a 256x256x256 (cell extent) dataset.

// Passing in the "Tetra" option will pass the input dataset through the
// Tetrahedralize filter to generate an unstructured, single cell type dataset.

// For the filters that require fields, the desired fields may be specified
// using these arguments:
//
// PointScalars [fieldname]
// CellScalars [fieldname]
// PointVectors [fieldname]
//
// If the fields are not specified, the first field with the correct association
// is used. If no such field exists, one will be generated from the data.

// For the TBB/OpenMP implementations, the number of threads can be customized
// using a "NumThreads [numThreads]" argument.

namespace
{

// Hold configuration state (e.g. active device):
svtkm::cont::InitializeResult Config;

// The input dataset we'll use on the filters:
static svtkm::cont::DataSet InputDataSet;
// The point scalars to use:
static std::string PointScalarsName;
// The cell scalars to use:
static std::string CellScalarsName;
// The point vectors to use:
static std::string PointVectorsName;

bool InputIsStructured()
{
  return InputDataSet.GetCellSet().IsType<svtkm::cont::CellSetStructured<3>>() ||
    InputDataSet.GetCellSet().IsType<svtkm::cont::CellSetStructured<2>>() ||
    InputDataSet.GetCellSet().IsType<svtkm::cont::CellSetStructured<1>>();
}

// Limit the filter executions to only consider the following types, otherwise
// compile times and binary sizes are nuts.
using FieldTypes = svtkm::List<svtkm::Float32, svtkm::Float64, svtkm::Vec3f_32, svtkm::Vec3f_64>;

using StructuredCellList = svtkm::List<svtkm::cont::CellSetStructured<3>>;

using UnstructuredCellList =
  svtkm::List<svtkm::cont::CellSetExplicit<>, svtkm::cont::CellSetSingleType<>>;

using AllCellList = svtkm::ListAppend<StructuredCellList, UnstructuredCellList>;

class BenchmarkFilterPolicy : public svtkm::filter::PolicyBase<BenchmarkFilterPolicy>
{
public:
  using FieldTypeList = FieldTypes;

  using StructuredCellSetList = StructuredCellList;
  using UnstructuredCellSetList = UnstructuredCellList;
  using AllCellSetList = AllCellList;
};

enum GradOpts : int
{
  Gradient = 1,
  PointGradient = 1 << 1,
  Divergence = 1 << 2,
  Vorticity = 1 << 3,
  QCriterion = 1 << 4,
  RowOrdering = 1 << 5,
  ScalarInput = 1 << 6
};

void BenchGradient(::benchmark::State& state, int options)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;

  svtkm::filter::Gradient filter;

  if (options & ScalarInput)
  {
    // Some outputs require vectors:
    if (options & Divergence || options & Vorticity || options & QCriterion)
    {
      throw svtkm::cont::ErrorInternal("A requested gradient output is "
                                      "incompatible with scalar input.");
    }
    filter.SetActiveField(PointScalarsName, svtkm::cont::Field::Association::POINTS);
  }
  else
  {
    filter.SetActiveField(PointVectorsName, svtkm::cont::Field::Association::POINTS);
  }

  filter.SetComputeGradient(static_cast<bool>(options & Gradient));
  filter.SetComputePointGradient(static_cast<bool>(options & PointGradient));
  filter.SetComputeDivergence(static_cast<bool>(options & Divergence));
  filter.SetComputeVorticity(static_cast<bool>(options & Vorticity));
  filter.SetComputeQCriterion(static_cast<bool>(options & QCriterion));

  if (options & RowOrdering)
  {
    filter.SetRowMajorOrdering();
  }
  else
  {
    filter.SetColumnMajorOrdering();
  }

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

#define SVTKM_PRIVATE_GRADIENT_BENCHMARK(Name, Opts)                                                \
  void BenchGradient##Name(::benchmark::State& state) { BenchGradient(state, Opts); }              \
  SVTKM_BENCHMARK(BenchGradient##Name)

SVTKM_PRIVATE_GRADIENT_BENCHMARK(Scalar, Gradient | ScalarInput);
SVTKM_PRIVATE_GRADIENT_BENCHMARK(Vector, Gradient);
SVTKM_PRIVATE_GRADIENT_BENCHMARK(VectorRow, Gradient | RowOrdering);
SVTKM_PRIVATE_GRADIENT_BENCHMARK(Point, PointGradient);
SVTKM_PRIVATE_GRADIENT_BENCHMARK(Divergence, Divergence);
SVTKM_PRIVATE_GRADIENT_BENCHMARK(Vorticity, Vorticity);
SVTKM_PRIVATE_GRADIENT_BENCHMARK(QCriterion, QCriterion);
SVTKM_PRIVATE_GRADIENT_BENCHMARK(All,
                                Gradient | PointGradient | Divergence | Vorticity | QCriterion);

#undef SVTKM_PRIVATE_GRADIENT_BENCHMARK

void BenchThreshold(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;

  // Lookup the point scalar range
  const auto range = []() -> svtkm::Range {
    auto ptScalarField =
      InputDataSet.GetField(PointScalarsName, svtkm::cont::Field::Association::POINTS);
    return svtkm::cont::ArrayGetValue(0, ptScalarField.GetRange());
  }();

  // Extract points with values between 25-75% of the range
  svtkm::Float64 quarter = range.Length() / 4.;
  svtkm::Float64 mid = range.Center();

  svtkm::filter::Threshold filter;
  filter.SetActiveField(PointScalarsName, svtkm::cont::Field::Association::POINTS);
  filter.SetLowerThreshold(mid - quarter);
  filter.SetUpperThreshold(mid + quarter);

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchThreshold);

void BenchThresholdPoints(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const bool compactPoints = static_cast<bool>(state.range(0));

  // Lookup the point scalar range
  const auto range = []() -> svtkm::Range {
    auto ptScalarField =
      InputDataSet.GetField(PointScalarsName, svtkm::cont::Field::Association::POINTS);
    return svtkm::cont::ArrayGetValue(0, ptScalarField.GetRange());
  }();

  // Extract points with values between 25-75% of the range
  svtkm::Float64 quarter = range.Length() / 4.;
  svtkm::Float64 mid = range.Center();

  svtkm::filter::ThresholdPoints filter;
  filter.SetActiveField(PointScalarsName, svtkm::cont::Field::Association::POINTS);
  filter.SetLowerThreshold(mid - quarter);
  filter.SetUpperThreshold(mid + quarter);
  filter.SetCompactPoints(compactPoints);

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK_OPTS(BenchThresholdPoints, ->ArgName("CompactPts")->DenseRange(0, 1));

void BenchCellAverage(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;

  svtkm::filter::CellAverage filter;
  filter.SetActiveField(PointScalarsName, svtkm::cont::Field::Association::POINTS);

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchCellAverage);

void BenchPointAverage(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;

  svtkm::filter::PointAverage filter;
  filter.SetActiveField(CellScalarsName, svtkm::cont::Field::Association::CELL_SET);

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchPointAverage);

void BenchWarpScalar(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;

  svtkm::filter::WarpScalar filter{ 2. };
  filter.SetUseCoordinateSystemAsField(true);
  filter.SetNormalField(PointVectorsName, svtkm::cont::Field::Association::POINTS);
  filter.SetScalarFactorField(PointScalarsName, svtkm::cont::Field::Association::POINTS);

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchWarpScalar);

void BenchWarpVector(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;

  svtkm::filter::WarpVector filter{ 2. };
  filter.SetUseCoordinateSystemAsField(true);
  filter.SetVectorField(PointVectorsName, svtkm::cont::Field::Association::POINTS);

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchWarpVector);

void BenchContour(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;

  const svtkm::Id numIsoVals = static_cast<svtkm::Id>(state.range(0));
  const bool mergePoints = static_cast<bool>(state.range(1));
  const bool normals = static_cast<bool>(state.range(2));
  const bool fastNormals = static_cast<bool>(state.range(3));

  svtkm::filter::Contour filter;
  filter.SetActiveField(PointScalarsName, svtkm::cont::Field::Association::POINTS);

  // Set up some equally spaced contours, with the min/max slightly inside the
  // scalar range:
  const svtkm::Range scalarRange = []() -> svtkm::Range {
    auto field = InputDataSet.GetField(PointScalarsName, svtkm::cont::Field::Association::POINTS);
    return svtkm::cont::ArrayGetValue(0, field.GetRange());
  }();
  const auto step = scalarRange.Length() / static_cast<svtkm::Float64>(numIsoVals + 1);
  const auto minIsoVal = scalarRange.Min + (step / 2.);

  filter.SetNumberOfIsoValues(numIsoVals);
  for (svtkm::Id i = 0; i < numIsoVals; ++i)
  {
    filter.SetIsoValue(i, minIsoVal + (step * static_cast<svtkm::Float64>(i)));
  }

  filter.SetMergeDuplicatePoints(mergePoints);
  filter.SetGenerateNormals(normals);
  filter.SetComputeFastNormalsForStructured(fastNormals);
  filter.SetComputeFastNormalsForUnstructured(fastNormals);

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

void BenchContourGenerator(::benchmark::internal::Benchmark* bm)
{
  bm->ArgNames({ "NIsoVals", "MergePts", "GenNormals", "FastNormals" });

  auto helper = [&](const svtkm::Id numIsoVals) {
    bm->Args({ numIsoVals, 0, 0, 0 });
    bm->Args({ numIsoVals, 1, 0, 0 });
    bm->Args({ numIsoVals, 0, 1, 0 });
    bm->Args({ numIsoVals, 0, 1, 1 });
  };

  helper(1);
  helper(3);
  helper(12);
}
SVTKM_BENCHMARK_APPLY(BenchContour, BenchContourGenerator);

void BenchExternalFaces(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const bool compactPoints = static_cast<bool>(state.range(0));

  svtkm::filter::ExternalFaces filter;
  filter.SetCompactPoints(compactPoints);

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK_OPTS(BenchExternalFaces, ->ArgName("Compact")->DenseRange(0, 1));

void BenchTetrahedralize(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;

  // This filter only supports structured datasets:
  if (!InputIsStructured())
  {
    state.SkipWithError("Tetrahedralize Filter requires structured data.");
    return;
  }

  svtkm::filter::Tetrahedralize filter;

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchTetrahedralize);

void BenchVertexClustering(::benchmark::State& state)
{
  const svtkm::cont::DeviceAdapterId device = Config.Device;
  const svtkm::Id numDivs = static_cast<svtkm::Id>(state.range(0));

  // This filter only supports unstructured datasets:
  if (InputIsStructured())
  {
    state.SkipWithError("VertexClustering Filter requires unstructured data.");
    return;
  }

  svtkm::filter::VertexClustering filter;
  filter.SetNumberOfDivisions({ numDivs });

  BenchmarkFilterPolicy policy;
  svtkm::cont::Timer timer{ device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    auto result = filter.Execute(InputDataSet, policy);
    ::benchmark::DoNotOptimize(result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK_OPTS(BenchVertexClustering,
                      ->RangeMultiplier(2)
                      ->Range(32, 1024)
                      ->ArgName("NumDivs"));

// Helper for resetting the reverse connectivity table:
struct PrepareForInput
{
  mutable svtkm::cont::Timer Timer;

  PrepareForInput()
    : Timer{ Config.Device }
  {
  }

  void operator()(const svtkm::cont::CellSet& cellSet) const
  {
    static bool warned{ false };
    if (!warned)
    {
      std::cerr << "Invalid cellset type for benchmark.\n";
      cellSet.PrintSummary(std::cerr);
      warned = true;
    }
  }

  template <typename T1, typename T2, typename T3>
  SVTKM_CONT void operator()(const svtkm::cont::CellSetExplicit<T1, T2, T3>& cellSet) const
  {
    svtkm::cont::TryExecuteOnDevice(Config.Device, *this, cellSet);
  }

  template <typename T1, typename T2, typename T3, typename DeviceTag>
  SVTKM_CONT bool operator()(DeviceTag, const svtkm::cont::CellSetExplicit<T1, T2, T3>& cellSet) const
  {
    // Why does CastAndCall insist on making the cellset const?
    using CellSetT = svtkm::cont::CellSetExplicit<T1, T2, T3>;
    CellSetT& mcellSet = const_cast<CellSetT&>(cellSet);
    mcellSet.ResetConnectivity(svtkm::TopologyElementTagPoint{}, svtkm::TopologyElementTagCell{});

    this->Timer.Start();
    auto result = cellSet.PrepareForInput(
      DeviceTag{}, svtkm::TopologyElementTagPoint{}, svtkm::TopologyElementTagCell{});
    ::benchmark::DoNotOptimize(result);
    this->Timer.Stop();

    return true;
  }
};

void BenchReverseConnectivityGen(::benchmark::State& state)
{
  if (InputIsStructured())
  {
    state.SkipWithError("ReverseConnectivityGen requires unstructured data.");
    return;
  }

  auto cellset = InputDataSet.GetCellSet();
  PrepareForInput functor;
  for (auto _ : state)
  {
    (void)_;
    cellset.CastAndCall(functor);
    state.SetIterationTime(functor.Timer.GetElapsedTime());
  }
}
SVTKM_BENCHMARK(BenchReverseConnectivityGen);

// Generates a Vec3 field from point coordinates.
struct PointVectorGenerator : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  svtkm::Bounds Bounds;
  svtkm::Vec3f_64 Center;
  svtkm::Vec3f_64 Scale;

  SVTKM_CONT
  PointVectorGenerator(const svtkm::Bounds& bounds)
    : Bounds(bounds)
    , Center(bounds.Center())
    , Scale((6. * svtkm::Pi()) / bounds.X.Length(),
            (2. * svtkm::Pi()) / bounds.Y.Length(),
            (7. * svtkm::Pi()) / bounds.Z.Length())
  {
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec<T, 3> operator()(svtkm::Vec<T, 3> val) const
  {
    using Vec3T = svtkm::Vec<T, 3>;
    using Vec3F64 = svtkm::Vec3f_64;

    Vec3F64 valF64{ val };
    Vec3F64 periodic = (valF64 - this->Center) * this->Scale;
    periodic[0] = svtkm::Sin(periodic[0]);
    periodic[1] = svtkm::Sin(periodic[1]);
    periodic[2] = svtkm::Cos(periodic[2]);

    if (svtkm::MagnitudeSquared(periodic) > 0.)
    {
      svtkm::Normalize(periodic);
    }
    if (svtkm::MagnitudeSquared(valF64) > 0.)
    {
      svtkm::Normalize(valF64);
    }
    return Vec3T{ svtkm::Normal(periodic + valF64) };
  }
};

// Get the number of components in a VariantArrayHandle, ArrayHandle, or Field's
// ValueType.
struct NumberOfComponents
{
  svtkm::IdComponent NumComponents;

  template <typename ArrayHandleT>
  SVTKM_CONT void operator()(const ArrayHandleT&)
  {
    using ValueType = typename ArrayHandleT::ValueType;
    using Traits = svtkm::VecTraits<ValueType>;
    this->NumComponents = Traits::NUM_COMPONENTS;
  }

  template <typename DynamicType>
  SVTKM_CONT static svtkm::IdComponent Check(const DynamicType& obj)
  {
    NumberOfComponents functor;
    svtkm::cont::CastAndCall(obj, functor);
    return functor.NumComponents;
  }
};

void FindFields()
{
  if (PointScalarsName.empty())
  {
    for (svtkm::Id i = 0; i < InputDataSet.GetNumberOfFields(); ++i)
    {
      auto field = InputDataSet.GetField(i);
      if (field.GetAssociation() == svtkm::cont::Field::Association::POINTS &&
          NumberOfComponents::Check(field) == 1)
      {
        PointScalarsName = field.GetName();
        std::cerr << "[FindFields] Found PointScalars: " << PointScalarsName << "\n";
        break;
      }
    }
  }

  if (CellScalarsName.empty())
  {
    for (svtkm::Id i = 0; i < InputDataSet.GetNumberOfFields(); ++i)
    {
      auto field = InputDataSet.GetField(i);
      if (field.GetAssociation() == svtkm::cont::Field::Association::CELL_SET &&
          NumberOfComponents::Check(field) == 1)
      {
        CellScalarsName = field.GetName();
        std::cerr << "[FindFields] CellScalars: " << CellScalarsName << "\n";
        break;
      }
    }
  }

  if (PointVectorsName.empty())
  {
    for (svtkm::Id i = 0; i < InputDataSet.GetNumberOfFields(); ++i)
    {
      auto field = InputDataSet.GetField(i);
      if (field.GetAssociation() == svtkm::cont::Field::Association::POINTS &&
          NumberOfComponents::Check(field) == 3)
      {
        PointVectorsName = field.GetName();
        std::cerr << "[FindFields] Found PointVectors: " << PointVectorsName << "\n";
        break;
      }
    }
  }
}

void CreateMissingFields()
{
  // Do point vectors first, so we can generate the scalars from them if needed
  if (PointVectorsName.empty())
  {
    // Construct them from the coordinates:
    auto coords = InputDataSet.GetCoordinateSystem();
    auto bounds = coords.GetBounds();
    auto points = coords.GetData();
    svtkm::cont::ArrayHandle<svtkm::Vec3f> pvecs;

    PointVectorGenerator worklet(bounds);
    svtkm::worklet::DispatcherMapField<PointVectorGenerator> dispatch(worklet);
    dispatch.Invoke(points, pvecs);
    InputDataSet.AddField(
      svtkm::cont::Field("GeneratedPointVectors", svtkm::cont::Field::Association::POINTS, pvecs));
    PointVectorsName = "GeneratedPointVectors";
    std::cerr << "[CreateFields] Generated point vectors '" << PointVectorsName
              << "' from coordinate data.\n";
  }

  if (PointScalarsName.empty())
  {
    if (!CellScalarsName.empty())
    { // Generate from found cell field:
      svtkm::filter::PointAverage avg;
      avg.SetActiveField(CellScalarsName, svtkm::cont::Field::Association::CELL_SET);
      avg.SetOutputFieldName("GeneratedPointScalars");
      auto outds = avg.Execute(InputDataSet);
      InputDataSet.AddField(
        outds.GetField("GeneratedPointScalars", svtkm::cont::Field::Association::POINTS));
      PointScalarsName = "GeneratedPointScalars";
      std::cerr << "[CreateFields] Generated point scalars '" << PointScalarsName
                << "' from cell scalars, '" << CellScalarsName << "'.\n";
    }
    else
    {
      // Compute the magnitude of the vectors:
      SVTKM_ASSERT(!PointVectorsName.empty());
      svtkm::filter::VectorMagnitude mag;
      mag.SetActiveField(PointVectorsName, svtkm::cont::Field::Association::POINTS);
      mag.SetOutputFieldName("GeneratedPointScalars");
      auto outds = mag.Execute(InputDataSet);
      InputDataSet.AddField(
        outds.GetField("GeneratedPointScalars", svtkm::cont::Field::Association::POINTS));
      PointScalarsName = "GeneratedPointScalars";
      std::cerr << "[CreateFields] Generated point scalars '" << PointScalarsName
                << "' from point vectors, '" << PointVectorsName << "'.\n";
    }
  }

  if (CellScalarsName.empty())
  { // Attempt to construct them from a point field:
    SVTKM_ASSERT(!PointScalarsName.empty());
    svtkm::filter::CellAverage avg;
    avg.SetActiveField(PointScalarsName, svtkm::cont::Field::Association::POINTS);
    avg.SetOutputFieldName("GeneratedCellScalars");
    auto outds = avg.Execute(InputDataSet);
    InputDataSet.AddField(
      outds.GetField("GeneratedCellScalars", svtkm::cont::Field::Association::CELL_SET));
    CellScalarsName = "GeneratedCellScalars";
    std::cerr << "[CreateFields] Generated cell scalars '" << CellScalarsName
              << "' from point scalars, '" << PointScalarsName << "'.\n";
  }
}

struct Arg : svtkm::cont::internal::option::Arg
{
  static svtkm::cont::internal::option::ArgStatus Number(
    const svtkm::cont::internal::option::Option& option,
    bool msg)
  {
    bool argIsNum = ((option.arg != nullptr) && (option.arg[0] != '\0'));
    const char* c = option.arg;
    while (argIsNum && (*c != '\0'))
    {
      argIsNum &= static_cast<bool>(std::isdigit(*c));
      ++c;
    }

    if (argIsNum)
    {
      return svtkm::cont::internal::option::ARG_OK;
    }
    else
    {
      if (msg)
      {
        std::cerr << "Option " << option.name << " requires a numeric argument." << std::endl;
      }

      return svtkm::cont::internal::option::ARG_ILLEGAL;
    }
  }

  static svtkm::cont::internal::option::ArgStatus Required(
    const svtkm::cont::internal::option::Option& option,
    bool msg)
  {
    if ((option.arg != nullptr) && (option.arg[0] != '\0'))
    {
      if (msg)
      {
        std::cerr << "Option " << option.name << " requires an argument." << std::endl;
      }
      return svtkm::cont::internal::option::ARG_ILLEGAL;
    }
    else
    {
      return svtkm::cont::internal::option::ARG_OK;
    }
  }
};

enum optionIndex
{
  UNKNOWN,
  HELP,
  NUM_THREADS,
  FILENAME,
  POINT_SCALARS,
  CELL_SCALARS,
  POINT_VECTORS,
  WAVELET_DIM,
  TETRA
};

void InitDataSet(int& argc, char** argv)
{
  int numThreads = 0;
  std::string filename;
  svtkm::Id waveletDim = 256;
  bool tetra = false;

  namespace option = svtkm::cont::internal::option;

  std::vector<option::Descriptor> usage;
  std::string usageHeader{ "Usage: " };
  usageHeader.append(argv[0]);
  usageHeader.append(" [input data options] [benchmark options]");
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, usageHeader.c_str() });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, "Input data options are:" });
  usage.push_back({ HELP, 0, "h", "help", Arg::None, "  -h, --help\tDisplay this help." });
  usage.push_back({ UNKNOWN, 0, "", "", Arg::None, Config.Usage.c_str() });
  usage.push_back({ NUM_THREADS,
                    0,
                    "",
                    "num-threads",
                    Arg::Number,
                    "  --num-threads <N> \tSpecify the number of threads to use." });
  usage.push_back({ FILENAME,
                    0,
                    "",
                    "file",
                    Arg::Required,
                    "  --file <filename> \tFile (in legacy svtk format) to read as input. "
                    "If not specified, a wavelet source is generated." });
  usage.push_back({ POINT_SCALARS,
                    0,
                    "",
                    "point-scalars",
                    Arg::Required,
                    "  --point-scalars <name> \tName of the point scalar field to operate on." });
  usage.push_back({ CELL_SCALARS,
                    0,
                    "",
                    "cell-scalars",
                    Arg::Required,
                    "  --cell-scalars <name> \tName of the cell scalar field to operate on." });
  usage.push_back({ POINT_VECTORS,
                    0,
                    "",
                    "point-vectors",
                    Arg::Required,
                    "  --point-vectors <name> \tName of the point vector field to operate on." });
  usage.push_back({ WAVELET_DIM,
                    0,
                    "",
                    "wavelet-dim",
                    Arg::Number,
                    "  --wavelet-dim <N> \tThe size in each dimension of the wavelet grid "
                    "(if generated)." });
  usage.push_back({ TETRA,
                    0,
                    "",
                    "tetra",
                    Arg::None,
                    "  --tetra \tTetrahedralize data set before running benchmark." });
  usage.push_back({ 0, 0, nullptr, nullptr, nullptr, nullptr });


  svtkm::cont::internal::option::Stats stats(usage.data(), argc - 1, argv + 1);
  std::unique_ptr<option::Option[]> options{ new option::Option[stats.options_max] };
  std::unique_ptr<option::Option[]> buffer{ new option::Option[stats.buffer_max] };
  option::Parser commandLineParse(usage.data(), argc - 1, argv + 1, options.get(), buffer.get());

  if (options[HELP])
  {
    // FIXME: Print google benchmark usage too
    option::printUsage(std::cerr, usage.data());
    exit(0);
  }

  if (options[NUM_THREADS])
  {
    std::istringstream parse(options[NUM_THREADS].arg);
    parse >> numThreads;
    if (Config.Device == svtkm::cont::DeviceAdapterTagTBB() ||
        Config.Device == svtkm::cont::DeviceAdapterTagOpenMP())
    {
      std::cout << "Selected " << numThreads << " " << Config.Device.GetName() << " threads."
                << std::endl;
    }
    else
    {
      std::cerr << options[NUM_THREADS].name << " not valid on this device. Ignoring." << std::endl;
    }
  }

  if (options[FILENAME])
  {
    filename = options[FILENAME].arg;
  }

  if (options[POINT_SCALARS])
  {
    PointScalarsName = options[POINT_SCALARS].arg;
  }
  if (options[CELL_SCALARS])
  {
    CellScalarsName = options[CELL_SCALARS].arg;
  }
  if (options[POINT_VECTORS])
  {
    PointVectorsName = options[POINT_VECTORS].arg;
  }

  if (options[WAVELET_DIM])
  {
    std::istringstream parse(options[WAVELET_DIM].arg);
    parse >> waveletDim;
  }

  tetra = (options[TETRA] != nullptr);

#ifdef SVTKM_ENABLE_TBB
  // Must not be destroyed as long as benchmarks are running:
  tbb::task_scheduler_init init((numThreads > 0) ? numThreads
                                                 : tbb::task_scheduler_init::automatic);
#endif
#ifdef SVTKM_ENABLE_OPENMP
  omp_set_num_threads((numThreads > 0) ? numThreads : omp_get_max_threads());
#endif

  // Now go back through the arg list and remove anything that is not in the list of
  // unknown options or non-option arguments.
  int destArg = 1;
  // This is copy/pasted from svtkm::cont::Initialize(), should probably be abstracted eventually:
  for (int srcArg = 1; srcArg < argc; ++srcArg)
  {
    std::string thisArg{ argv[srcArg] };
    bool copyArg = false;

    // Special case: "--" gets removed by optionparser but should be passed.
    if (thisArg == "--")
    {
      copyArg = true;
    }
    for (const option::Option* opt = options[UNKNOWN]; !copyArg && opt != nullptr;
         opt = opt->next())
    {
      if (thisArg == opt->name)
      {
        copyArg = true;
      }
      if ((opt->arg != nullptr) && (thisArg == opt->arg))
      {
        copyArg = true;
      }
      // Special case: optionparser sometimes removes a single "-" from an option
      if (thisArg.substr(1) == opt->name)
      {
        copyArg = true;
      }
    }
    for (int nonOpt = 0; !copyArg && nonOpt < commandLineParse.nonOptionsCount(); ++nonOpt)
    {
      if (thisArg == commandLineParse.nonOption(nonOpt))
      {
        copyArg = true;
      }
    }
    if (copyArg)
    {
      if (destArg != srcArg)
      {
        argv[destArg] = argv[srcArg];
      }
      ++destArg;
    }
  }
  argc = destArg;

  // Load / generate the dataset
  svtkm::cont::Timer inputGenTimer{ Config.Device };
  inputGenTimer.Start();

  if (!filename.empty())
  {
    std::cerr << "[InitDataSet] Loading file: " << filename << "\n";
    svtkm::io::reader::SVTKDataSetReader reader(filename);
    InputDataSet = reader.ReadDataSet();
  }
  else
  {
    std::cerr << "[InitDataSet] Generating " << waveletDim << "x" << waveletDim << "x" << waveletDim
              << " wavelet...\n";
    svtkm::source::Wavelet source;
    source.SetExtent({ 0 }, { waveletDim - 1 });

    InputDataSet = source.Execute();
  }

  if (tetra)
  {
    std::cerr << "[InitDataSet] Tetrahedralizing dataset...\n";
    svtkm::filter::Tetrahedralize tet;
    tet.SetFieldsToPass(svtkm::filter::FieldSelection(svtkm::filter::FieldSelection::MODE_ALL));
    InputDataSet = tet.Execute(InputDataSet);
  }

  FindFields();
  CreateMissingFields();

  inputGenTimer.Stop();

  std::cerr << "[InitDataSet] DataSet initialization took " << inputGenTimer.GetElapsedTime()
            << " seconds.\n\n-----------------";
}

} // end anon namespace

int main(int argc, char* argv[])
{
  auto opts = svtkm::cont::InitializeOptions::RequireDevice;
  Config = svtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

  InitDataSet(argc, argv);

  const std::string dataSetSummary = []() -> std::string {
    std::ostringstream out;
    InputDataSet.PrintSummary(out);
    return out.str();
  }();

  // handle benchmarking related args and run benchmarks:
  SVTKM_EXECUTE_BENCHMARKS_PREAMBLE(argc, argv, dataSetSummary);
}
