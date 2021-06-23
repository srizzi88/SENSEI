//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <typeinfo>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/DataSetBuilderRectilinear.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/ParticleAdvection.h>
#include <svtkm/worklet/particleadvection/Integrators.h>
#include <svtkm/worklet/particleadvection/Particles.h>
#include <svtkm/worklet/particleadvection/TemporalGridEvaluators.h>

#include <svtkm/io/writer/SVTKDataSetWriter.h>

template <typename ScalarType>
svtkm::cont::DataSet CreateUniformDataSet(const svtkm::Bounds& bounds, const svtkm::Id3& dims)
{
  svtkm::Vec<ScalarType, 3> origin(static_cast<ScalarType>(bounds.X.Min),
                                  static_cast<ScalarType>(bounds.Y.Min),
                                  static_cast<ScalarType>(bounds.Z.Min));
  svtkm::Vec<ScalarType, 3> spacing(
    static_cast<ScalarType>(bounds.X.Length()) / static_cast<ScalarType>((dims[0] - 1)),
    static_cast<ScalarType>(bounds.Y.Length()) / static_cast<ScalarType>((dims[1] - 1)),
    static_cast<ScalarType>(bounds.Z.Length()) / static_cast<ScalarType>((dims[2] - 1)));

  svtkm::cont::DataSetBuilderUniform dataSetBuilder;
  svtkm::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);
  return ds;
}

class TestEvaluatorWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputPoint,
                                ExecObject evaluator,
                                FieldOut validity,
                                FieldOut outputPoint);

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename EvaluatorType>
  SVTKM_EXEC void operator()(svtkm::Particle& pointIn,
                            const EvaluatorType& evaluator,
                            svtkm::worklet::particleadvection::GridEvaluatorStatus& status,
                            svtkm::Vec3f& pointOut) const
  {
    status = evaluator.Evaluate(pointIn.Pos, 0.5f, pointOut);
  }
};

template <typename EvalType>
void ValidateEvaluator(const EvalType& eval,
                       const svtkm::cont::ArrayHandle<svtkm::Particle>& pointIns,
                       const svtkm::cont::ArrayHandle<svtkm::Vec3f>& validity,
                       const std::string& msg)
{
  using EvalTester = TestEvaluatorWorklet;
  using EvalTesterDispatcher = svtkm::worklet::DispatcherMapField<EvalTester>;
  using Status = svtkm::worklet::particleadvection::GridEvaluatorStatus;

  EvalTester evalTester;
  EvalTesterDispatcher evalTesterDispatcher(evalTester);
  svtkm::Id numPoints = pointIns.GetNumberOfValues();
  svtkm::cont::ArrayHandle<Status> evalStatus;
  svtkm::cont::ArrayHandle<svtkm::Vec3f> evalResults;
  evalTesterDispatcher.Invoke(pointIns, eval, evalStatus, evalResults);
  auto statusPortal = evalStatus.GetPortalConstControl();
  auto resultsPortal = evalResults.GetPortalConstControl();
  auto validityPortal = validity.GetPortalConstControl();
  for (svtkm::Id index = 0; index < numPoints; index++)
  {
    Status status = statusPortal.Get(index);
    svtkm::Vec3f result = resultsPortal.Get(index);
    svtkm::Vec3f expected = validityPortal.Get(index);
    SVTKM_TEST_ASSERT(status.CheckOk(), "Error in evaluator for " + msg);
    SVTKM_TEST_ASSERT(result == expected, "Error in evaluator result for " + msg);
  }
  evalStatus.ReleaseResources();
  evalResults.ReleaseResources();
}

template <typename ScalarType>
void CreateConstantVectorField(svtkm::Id num,
                               const svtkm::Vec<ScalarType, 3>& vec,
                               svtkm::cont::ArrayHandle<svtkm::Vec<ScalarType, 3>>& vecField)
{
  svtkm::cont::ArrayHandleConstant<svtkm::Vec<ScalarType, 3>> vecConst;
  vecConst = svtkm::cont::make_ArrayHandleConstant(vec, num);
  svtkm::cont::ArrayCopy(vecConst, vecField);
}

svtkm::Vec3f RandomPt(const svtkm::Bounds& bounds)
{
  svtkm::FloatDefault rx =
    static_cast<svtkm::FloatDefault>(rand()) / static_cast<svtkm::FloatDefault>(RAND_MAX);
  svtkm::FloatDefault ry =
    static_cast<svtkm::FloatDefault>(rand()) / static_cast<svtkm::FloatDefault>(RAND_MAX);
  svtkm::FloatDefault rz =
    static_cast<svtkm::FloatDefault>(rand()) / static_cast<svtkm::FloatDefault>(RAND_MAX);

  svtkm::Vec3f p;
  p[0] = static_cast<svtkm::FloatDefault>(bounds.X.Min + rx * bounds.X.Length());
  p[1] = static_cast<svtkm::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length());
  p[2] = static_cast<svtkm::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length());
  return p;
}

void GeneratePoints(const svtkm::Id numOfEntries,
                    const svtkm::Bounds& bounds,
                    svtkm::cont::ArrayHandle<svtkm::Particle>& pointIns)
{
  pointIns.Allocate(numOfEntries);
  auto writePortal = pointIns.GetPortalControl();
  for (svtkm::Id index = 0; index < numOfEntries; index++)
  {
    svtkm::Particle particle(RandomPt(bounds), index);
    writePortal.Set(index, particle);
  }
}

void GenerateValidity(const svtkm::Id numOfEntries,
                      svtkm::cont::ArrayHandle<svtkm::Vec3f>& validity,
                      const svtkm::Vec3f& vecOne,
                      const svtkm::Vec3f& vecTwo)
{
  validity.Allocate(numOfEntries);
  auto writePortal = validity.GetPortalControl();
  for (svtkm::Id index = 0; index < numOfEntries; index++)
  {
    svtkm::Vec3f value = 0.5f * vecOne + (1.0f - 0.5f) * vecTwo;
    writePortal.Set(index, value);
  }
}

void TestTemporalEvaluators()
{
  using ScalarType = svtkm::FloatDefault;
  using PointType = svtkm::Vec<ScalarType, 3>;
  using FieldHandle = svtkm::cont::ArrayHandle<PointType>;
  using EvalType = svtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using TemporalEvalType = svtkm::worklet::particleadvection::TemporalGridEvaluator<FieldHandle>;

  // Create Datasets
  svtkm::Id3 dims(5, 5, 5);
  svtkm::Bounds bounds(0, 10, 0, 10, 0, 10);
  svtkm::cont::DataSet sliceOne = CreateUniformDataSet<ScalarType>(bounds, dims);
  svtkm::cont::DataSet sliceTwo = CreateUniformDataSet<ScalarType>(bounds, dims);

  // Create Vector Field
  PointType X(1, 0, 0);
  PointType Z(0, 0, 1);
  svtkm::cont::ArrayHandle<PointType> alongX, alongZ;
  CreateConstantVectorField(125, X, alongX);
  CreateConstantVectorField(125, Z, alongZ);

  // Send them to test
  EvalType evalOne(sliceOne.GetCoordinateSystem(), sliceOne.GetCellSet(), alongX);
  EvalType evalTwo(sliceTwo.GetCoordinateSystem(), sliceTwo.GetCellSet(), alongZ);

  // Test data : populate with meaningful values
  svtkm::Id numValues = 10;
  svtkm::cont::ArrayHandle<svtkm::Particle> pointIns;
  svtkm::cont::ArrayHandle<svtkm::Vec3f> validity;
  GeneratePoints(numValues, bounds, pointIns);
  GenerateValidity(numValues, validity, X, Z);

  svtkm::FloatDefault timeOne(0.0f), timeTwo(1.0f);
  TemporalEvalType gridEval(evalOne, timeOne, evalTwo, timeTwo);
  ValidateEvaluator(gridEval, pointIns, validity, "grid evaluator");
}

void TestTemporalAdvection()
{
  TestTemporalEvaluators();
}

int UnitTestTemporalAdvection(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestTemporalAdvection, argc, argv);
}
