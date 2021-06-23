//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingImplicitFunction_h
#define svtk_m_cont_testing_TestingImplicitFunction_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DeviceAdapterList.h>
#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/cont/ImplicitFunctionHandle.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/internal/Configure.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <array>

namespace svtkm
{
namespace cont
{
namespace testing
{

namespace implicit_function_detail
{

class EvaluateImplicitFunction : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  EvaluateImplicitFunction(const svtkm::ImplicitFunction* function)
    : Function(function)
  {
  }

  template <typename VecType, typename ScalarType>
  SVTKM_EXEC void operator()(const VecType& point, ScalarType& val, VecType& gradient) const
  {
    val = this->Function->Value(point);
    gradient = this->Function->Gradient(point);
  }

private:
  const svtkm::ImplicitFunction* Function;
};

template <typename DeviceAdapter>
void EvaluateOnCoordinates(svtkm::cont::CoordinateSystem points,
                           const svtkm::cont::ImplicitFunctionHandle& function,
                           svtkm::cont::ArrayHandle<svtkm::FloatDefault>& values,
                           svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::FloatDefault, 3>>& gradients,
                           DeviceAdapter device)
{
  using EvalDispatcher = svtkm::worklet::DispatcherMapField<EvaluateImplicitFunction>;

  EvaluateImplicitFunction eval(function.PrepareForExecution(device));
  EvalDispatcher dispatcher(eval);
  dispatcher.SetDevice(DeviceAdapter());
  dispatcher.Invoke(points, values, gradients);
}

template <typename ItemType, std::size_t N>
bool TestArrayEqual(const svtkm::cont::ArrayHandle<ItemType>& result,
                    const std::array<ItemType, N>& expected)
{
  bool success = false;
  auto portal = result.GetPortalConstControl();
  svtkm::Id count = portal.GetNumberOfValues();

  if (static_cast<std::size_t>(count) == N)
  {
    success = true;
    for (svtkm::Id i = 0; i < count; ++i)
    {
      if (!test_equal(portal.Get(i), expected[static_cast<std::size_t>(i)]))
      {
        success = false;
        break;
      }
    }
  }
  if (!success)
  {
    if (count == 0)
    {
      std::cout << "result: <empty>\n";
    }
    else
    {
      std::cout << "result:   " << portal.Get(0);
      for (svtkm::Id i = 1; i < count; ++i)
      {
        std::cout << ", " << portal.Get(i);
      }
      std::cout << "\n";
      std::cout << "expected: " << expected[0];
      for (svtkm::Id i = 1; i < count; ++i)
      {
        std::cout << ", " << expected[static_cast<std::size_t>(i)];
      }
      std::cout << "\n";
    }
  }

  return success;
}

} // anonymous namespace

class TestingImplicitFunction
{
public:
  TestingImplicitFunction()
    : Input(svtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet2())
  {
  }

  template <typename DeviceAdapter>
  void Run(DeviceAdapter device)
  {
    this->TestBox(device);
    this->TestCylinder(device);
    this->TestFrustum(device);
    this->TestPlane(device);
    this->TestSphere(device);
  }

private:
  template <typename DeviceAdapter>
  void Try(svtkm::cont::ImplicitFunctionHandle& function,
           const std::array<svtkm::FloatDefault, 8>& expectedValues,
           const std::array<svtkm::Vec3f, 8>& expectedGradients,
           DeviceAdapter device)
  {
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> values;
    svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::FloatDefault, 3>> gradients;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0), function, values, gradients, device);

    SVTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expectedValues),
                     "Result does not match expected values");
    SVTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(gradients, expectedGradients),
                     "Result does not match expected gradients values");
  }

  template <typename DeviceAdapter>
  void TestBox(DeviceAdapter device)
  {
    std::cout << "Testing svtkm::Box on "
              << svtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  default box" << std::endl;
    svtkm::Box box;
    svtkm::cont::ImplicitFunctionHandle boxHandle(&box, false);
    this->Try(boxHandle,
              { { -0.5f, 0.5f, 0.707107f, 0.5f, 0.5f, 0.707107f, 0.866025f, 0.707107f } },
              { { svtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.707107f, 0.0f, 0.707107f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
                  svtkm::Vec3f{ 0.707107f, 0.707107f, 0.0f },
                  svtkm::Vec3f{ 0.57735f, 0.57735f, 0.57735f },
                  svtkm::Vec3f{ 0.0f, 0.707107f, 0.707107f } } },
              device);

    std::cout << "  Specified min/max box" << std::endl;
    box.SetMinPoint({ 0.0f, -0.5f, -0.5f });
    box.SetMaxPoint({ 1.5f, 1.5f, 0.5f });
    this->Try(boxHandle,
              { { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f } },
              { { svtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);

    std::cout << "  Specified bounds box" << std::endl;
    box.SetBounds({ svtkm::Range(0.0, 1.5), svtkm::Range(-0.5, 1.5), svtkm::Range(-0.5, 0.5) });
    this->Try(boxHandle,
              { { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f } },
              { { svtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);
  }

  template <typename DeviceAdapter>
  void TestCylinder(DeviceAdapter device)
  {
    std::cout << "Testing svtkm::Cylinder on "
              << svtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  Default cylinder" << std::endl;
    svtkm::Cylinder cylinder;
    svtkm::cont::ImplicitFunctionHandle cylinderHandle(&cylinder, false);
    this->Try(cylinderHandle,
              { { -0.25f, 0.75f, 1.75f, 0.75f, -0.25f, 0.75f, 1.75f, 0.75f } },
              { { svtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f } } },
              device);

    std::cout << "  Translated, scaled cylinder" << std::endl;
    cylinder.SetCenter({ 0.0f, 0.0f, 1.0f });
    cylinder.SetAxis({ 0.0f, 1.0f, 0.0f });
    cylinder.SetRadius(1.0f);
    this->Try(cylinderHandle,
              { { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f } },
              { { svtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, -2.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, -2.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 0.0f } } },
              device);

    std::cout << "  Non-unit axis" << std::endl;
    cylinder.SetCenter({ 0.0f, 0.0f, 0.0f });
    cylinder.SetAxis({ 1.0f, 1.0f, 0.0f });
    cylinder.SetRadius(1.0f);
    this->Try(cylinderHandle,
              { { -1.0f, -0.5f, 0.5f, 0.0f, -0.5f, -1.0f, 0.0f, 0.5f } },
              { { svtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 1.0f, -1.0f, 0.0f },
                  svtkm::Vec3f{ 1.0f, -1.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ -1.0f, 1.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ -1.0f, 1.0f, 2.0f } } },
              device);
  }

  template <typename DeviceAdapter>
  void TestFrustum(DeviceAdapter device)
  {
    std::cout << "Testing svtkm::Frustum on "
              << svtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  With corner points" << std::endl;
    svtkm::Vec3f cornerPoints[8] = {
      { -0.5f, 0.0f, -0.5f }, // 0
      { -0.5f, 0.0f, 0.5f },  // 1
      { 0.5f, 0.0f, 0.5f },   // 2
      { 0.5f, 0.0f, -0.5f },  // 3
      { -0.5f, 1.0f, -0.5f }, // 4
      { -0.5f, 1.0f, 0.5f },  // 5
      { 1.5f, 1.0f, 0.5f },   // 6
      { 1.5f, 1.0f, -0.5f }   // 7
    };
    svtkm::cont::ImplicitFunctionHandle frustumHandle =
      svtkm::cont::make_ImplicitFunctionHandle<svtkm::Frustum>(cornerPoints);
    svtkm::Frustum* frustum = static_cast<svtkm::Frustum*>(frustumHandle.Get());
    this->Try(frustumHandle,
              { { 0.0f, 0.353553f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f } },
              { { svtkm::Vec3f{ 0.0f, -1.0f, 0.0f },
                  svtkm::Vec3f{ 0.707107f, -0.707107f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);

    std::cout << "  With 6 planes" << std::endl;
    svtkm::Vec3f planePoints[6] = { { 0.0f, 0.0f, 0.0f },  { 1.0f, 1.0f, 0.0f },
                                   { -0.5f, 0.0f, 0.0f }, { 0.5f, 0.0f, 0.0f },
                                   { 0.0f, 0.0f, -0.5f }, { 0.0f, 0.0f, 0.5f } };
    svtkm::Vec3f planeNormals[6] = { { 0.0f, -1.0f, 0.0f }, { 0.707107f, 0.707107f, 0.0f },
                                    { -1.0f, 0.0f, 0.0f }, { 0.707107f, -0.707107f, 0.0f },
                                    { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } };
    frustum->SetPlanes(planePoints, planeNormals);
    this->Try(frustumHandle,
              { { 0.0f, 0.353553f, 0.5f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f } },
              { { svtkm::Vec3f{ 0.0f, -1.0f, 0.0f },
                  svtkm::Vec3f{ 0.707107f, -0.707107f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 0.707107f, 0.707107f, 0.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);
  }

  template <typename DeviceAdapter>
  void TestPlane(DeviceAdapter device)
  {
    std::cout << "Testing svtkm::Plane on "
              << svtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  Default plane" << std::endl;
    svtkm::cont::ImplicitFunctionHandle planeHandle =
      svtkm::cont::make_ImplicitFunctionHandle(svtkm::Plane());
    svtkm::Plane* plane = static_cast<svtkm::Plane*>(planeHandle.Get());
    this->Try(planeHandle,
              { { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f } },
              { { svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);

    std::cout << "  Normal of length 2" << std::endl;
    plane->SetOrigin({ 1.0f, 1.0f, 1.0f });
    plane->SetNormal({ 0.0f, 0.0f, 2.0f });
    this->Try(planeHandle,
              { { -2.0f, -2.0f, 0.0f, 0.0f, -2.0f, -2.0f, 0.0f, 0.0f } },
              { { svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f } } },
              device);

    std::cout << "  Oblique plane" << std::endl;
    plane->SetOrigin({ 0.5f, 0.5f, 0.5f });
    plane->SetNormal({ 1.0f, 0.0f, 1.0f });
    this->Try(planeHandle,
              { { -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f } },
              { { svtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  svtkm::Vec3f{ 1.0f, 0.0f, 1.0f } } },
              device);

    std::cout << "  Another oblique plane" << std::endl;
    plane->SetNormal({ -1.0f, 0.0f, -1.0f });
    this->Try(planeHandle,
              { { 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f } },
              { { svtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  svtkm::Vec3f{ -1.0f, 0.0f, -1.0f } } },
              device);
  }

  template <typename DeviceAdapter>
  void TestSphere(DeviceAdapter device)
  {
    std::cout << "Testing svtkm::Sphere on "
              << svtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  Default sphere" << std::endl;
    svtkm::Sphere sphere;
    svtkm::cont::ImplicitFunctionHandle sphereHandle(&sphere, false);
    this->Try(sphereHandle,
              { { -0.25f, 0.75f, 1.75f, 0.75f, 0.75f, 1.75f, 2.75f, 1.75f } },
              { { svtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 2.0f, 0.0f },
                  svtkm::Vec3f{ 2.0f, 2.0f, 0.0f },
                  svtkm::Vec3f{ 2.0f, 2.0f, 2.0f },
                  svtkm::Vec3f{ 0.0f, 2.0f, 2.0f } } },
              device);

    std::cout << "  Shifted and scaled sphere" << std::endl;
    sphere.SetCenter({ 1.0f, 1.0f, 1.0f });
    sphere.SetRadius(1.0f);
    this->Try(sphereHandle,
              { { 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f } },
              { { svtkm::Vec3f{ -2.0f, -2.0f, -2.0f },
                  svtkm::Vec3f{ 0.0f, -2.0f, -2.0f },
                  svtkm::Vec3f{ 0.0f, -2.0f, 0.0f },
                  svtkm::Vec3f{ -2.0f, -2.0f, 0.0f },
                  svtkm::Vec3f{ -2.0f, 0.0f, -2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
                  svtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  svtkm::Vec3f{ -2.0f, 0.0f, 0.0f } } },
              device);
  }

  svtkm::cont::DataSet Input;
};
}
}
} // vtmk::cont::testing

#endif //svtk_m_cont_testing_TestingImplicitFunction_h
