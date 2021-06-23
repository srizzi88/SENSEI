//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/ScalarsToColors.h>

namespace
{

// data we want are valid values between 0 and 1 that represent the fraction
// of the range we want to map into.
std::vector<float> test_values = { 0.0f, 0.125f, 0.25f, 0.5f, 0.625, 0.75f, 1.0f };
std::vector<svtkm::Vec3ui_8> rgb_result = {
  { 0, 0, 0 },       { 32, 32, 32 },    { 64, 64, 64 },    { 128, 128, 128 },
  { 159, 159, 159 }, { 191, 191, 191 }, { 255, 255, 255 },
};

template <typename T>
T as_color(svtkm::Float32 v, svtkm::Float32)
{
  return static_cast<T>(v);
}

template <>
svtkm::UInt8 as_color<svtkm::UInt8>(svtkm::Float32 v, svtkm::Float32)
{
  return static_cast<svtkm::UInt8>(v * 255.0f + 0.5f);
}

template <>
svtkm::Vec2f_32 as_color<svtkm::Vec2f_32>(svtkm::Float32 v, svtkm::Float32 alpha)
{ //generate luminance+alpha values
  return svtkm::Vec2f_32(v, alpha);
}
template <>
svtkm::Vec2f_64 as_color<svtkm::Vec2f_64>(svtkm::Float32 v, svtkm::Float32 alpha)
{ //generate luminance+alpha values
  return svtkm::Vec2f_64(v, alpha);
}
template <>
svtkm::Vec2ui_8 as_color<svtkm::Vec2ui_8>(svtkm::Float32 v, svtkm::Float32 alpha)
{ //generate luminance+alpha values
  return svtkm::Vec2ui_8(static_cast<svtkm::UInt8>(v * 255.0f + 0.5f),
                        static_cast<svtkm::UInt8>(alpha * 255.0f + 0.5f));
}

template <>
svtkm::Vec3ui_8 as_color<svtkm::Vec3ui_8>(svtkm::Float32 v, svtkm::Float32)
{ //vec 3 are always rgb
  return svtkm::Vec3ui_8(static_cast<svtkm::UInt8>(v * 255.0f + 0.5f));
}

template <>
svtkm::Vec4f_32 as_color<svtkm::Vec4f_32>(svtkm::Float32 v, svtkm::Float32 alpha)
{ //generate rgba
  return svtkm::Vec4f_32(v, v, v, alpha);
}
template <>
svtkm::Vec4f_64 as_color<svtkm::Vec4f_64>(svtkm::Float32 v, svtkm::Float32 alpha)
{ //generate rgba
  return svtkm::Vec4f_64(v, v, v, alpha);
}
template <>
svtkm::Vec4ui_8 as_color<svtkm::Vec4ui_8>(svtkm::Float32 v, svtkm::Float32 alpha)
{ //generate rgba
  return svtkm::Vec4ui_8(static_cast<svtkm::UInt8>(v * 255.0f + 0.5f),
                        static_cast<svtkm::UInt8>(v * 255.0f + 0.5f),
                        static_cast<svtkm::UInt8>(v * 255.0f + 0.5f),
                        static_cast<svtkm::UInt8>(alpha * 255.0f + 0.5f));
}


template <typename T>
svtkm::cont::ArrayHandle<T> make_data(const svtkm::Range& r)
{
  using BaseT = typename svtkm::VecTraits<T>::BaseComponentType;
  svtkm::Float32 shift, scale;
  svtkm::worklet::colorconversion::ComputeShiftScale(r, shift, scale);
  const bool shiftscale = svtkm::worklet::colorconversion::needShiftScale(BaseT{}, shift, scale);



  svtkm::cont::ArrayHandle<T> handle;
  handle.Allocate(static_cast<svtkm::Id>(test_values.size()));

  auto portal = handle.GetPortalControl();
  svtkm::Id index = 0;
  if (shiftscale)
  {
    const svtkm::Float32 alpha = static_cast<svtkm::Float32>(r.Max);
    for (const auto& i : test_values)
    {
      //we want to apply the shift and scale, and than clamp to the allowed
      //range of the data. The problem is that we might need to shift and scale the alpha value
      //for the color
      //
      const float c = (i * static_cast<float>(r.Length())) - shift;
      portal.Set(index++, as_color<T>(c, alpha));
    }
  }
  else
  {
    const svtkm::Float32 alpha = 1.0f;
    //no shift or scale required
    for (const auto& i : test_values)
    {
      portal.Set(index++, as_color<T>(i, alpha));
    }
  }
  return handle;
}

bool verify(svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> output)
{
  auto portal = output.GetPortalConstControl();
  svtkm::Id index = 0;
  for (auto i : rgb_result)
  {
    auto v = portal.Get(index);
    if (v != i)
    {
      std::cerr << "failed comparison at index: " << index
                << " found: " << static_cast<svtkm::Vec<int, 3>>(v)
                << " was expecting: " << static_cast<svtkm::Vec<int, 3>>(i) << std::endl;
      break;
    }
    index++;
  }
  bool valid = static_cast<std::size_t>(index) == rgb_result.size();
  return valid;
}

bool verify(svtkm::Float32 alpha, svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> output)
{
  const svtkm::UInt8 a = svtkm::worklet::colorconversion::ColorToUChar(alpha);
  auto portal = output.GetPortalConstControl();
  svtkm::Id index = 0;
  for (auto i : rgb_result)
  {
    auto v = portal.Get(index);
    auto e = svtkm::make_Vec(i[0], i[1], i[2], a);
    if (v != e)
    {
      std::cerr << "failed comparison at index: " << index
                << " found: " << static_cast<svtkm::Vec<int, 4>>(v)
                << " was expecting: " << static_cast<svtkm::Vec<int, 4>>(e) << std::endl;
      break;
    }
    index++;
  }
  bool valid = static_cast<std::size_t>(index) == rgb_result.size();
  return valid;
}

#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif

struct TestToRGB
{
  svtkm::worklet::ScalarsToColors Worklet;

  TestToRGB()
    : Worklet()
  {
  }

  TestToRGB(svtkm::Float32 minR, svtkm::Float32 maxR)
    : Worklet(svtkm::Range(minR, maxR))
  {
  }

  template <typename T>
  SVTKM_CONT void operator()(T) const
  {
    //use each component to generate the output
    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> output;
    this->Worklet.Run(make_data<T>(this->Worklet.GetRange()), output);
    bool valid = verify(output);
    SVTKM_TEST_ASSERT(valid, "scalar RGB failed");
  }

  template <typename U, int N>
  SVTKM_CONT void operator()(svtkm::Vec<U, N>) const
  {
    bool valid = false;
    using T = svtkm::Vec<U, N>;

    auto input = make_data<T>(this->Worklet.GetRange());
    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> output;

    //use all components to generate the output
    this->Worklet.Run(input, output);
    valid = verify(output);
    SVTKM_TEST_ASSERT(valid, "all components RGB failed");

    //use the magnitude of the vector if vector is 3 components
    if (N == 3)
    {
      //compute the range needed for the magnitude, since the range can span
      //negative/positive space we need to find the magnitude of each value
      //and them to the range to get the correct range
      svtkm::worklet::colorconversion::MagnitudePortal wrapper;
      svtkm::Range magr;
      auto portal = input.GetPortalControl();
      for (svtkm::Id i = 0; i < input.GetNumberOfValues(); ++i)
      {
        const auto magv = wrapper(portal.Get(i));
        magr.Include(static_cast<double>(magv));
      }

      svtkm::worklet::ScalarsToColors magWorklet(magr);
      magWorklet.RunMagnitude(input, output);
      // svtkm::cont::printSummary_ArrayHandle(output, std::cout, true);

      auto portal2 = output.GetPortalControl();
      for (svtkm::Id i = 0; i < input.GetNumberOfValues(); ++i)
      {
        const auto expected = wrapper(portal.Get(i));
        const auto percent = (portal2.Get(i)[0] / 255.0f);
        const auto result = (percent * magr.Length()) + magr.Min;
        if (!test_equal(expected, result, 0.005))
        {
          std::cerr << "failed comparison at index: " << i << " found: " << result
                    << " was expecting: " << expected << std::endl;
          SVTKM_TEST_ASSERT(test_equal(expected, result), "magnitude RGB failed");
        }
      }
    }

    //use the components of the vector, if the vector is 2 or 4 we need
    //to ignore the last component as it is alpha
    int end = (N % 2 == 0) ? (N - 1) : N;
    for (int i = 0; i < end; ++i)
    {
      this->Worklet.RunComponent(input, i, output);
      valid = verify(output);
      SVTKM_TEST_ASSERT(valid, "per component RGB failed");
    }
  }
};

struct TestToRGBA
{
  svtkm::worklet::ScalarsToColors Worklet;

  TestToRGBA()
    : Worklet()
  {
  }

  TestToRGBA(svtkm::Float32 minR, svtkm::Float32 maxR, svtkm::Float32 alpha)
    : Worklet(svtkm::Range(minR, maxR), alpha)
  {
  }

  template <typename T>
  SVTKM_CONT void operator()(T) const
  {
    //use each component to generate the output
    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> output;
    this->Worklet.Run(make_data<T>(this->Worklet.GetRange()), output);

    bool valid = verify(this->Worklet.GetAlpha(), output);
    SVTKM_TEST_ASSERT(valid, "scalar RGBA failed");
  }

  template <typename U, int N>
  SVTKM_CONT void operator()(svtkm::Vec<U, N>) const
  {
    bool valid = false;
    using T = svtkm::Vec<U, N>;
    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> output;

    auto input = make_data<T>(this->Worklet.GetRange());
    // svtkm::cont::printSummary_ArrayHandle(input, std::cout, true);

    //use all components to generate the output
    this->Worklet.Run(input, output);
    valid = verify(this->Worklet.GetAlpha(), output);
    SVTKM_TEST_ASSERT(valid, "all components RGBA failed");

    //use the magnitude of the vector if vector is 3 components
    if (N == 3)
    {
      //compute the range needed for the magnitude, since the range can span
      //negative/positive space we need to find the magnitude of each value
      //and them to the range to get the correct range
      svtkm::worklet::colorconversion::MagnitudePortal wrapper;
      svtkm::Range magr;
      auto portal = input.GetPortalControl();
      for (svtkm::Id i = 0; i < input.GetNumberOfValues(); ++i)
      {
        const auto magv = wrapper(portal.Get(i));
        magr.Include(static_cast<double>(magv));
      }

      svtkm::worklet::ScalarsToColors magWorklet(magr);
      magWorklet.RunMagnitude(input, output);
      // svtkm::cont::printSummary_ArrayHandle(output, std::cout, true);

      auto portal2 = output.GetPortalControl();
      for (svtkm::Id i = 0; i < input.GetNumberOfValues(); ++i)
      {
        const auto expected = wrapper(portal.Get(i));
        const auto percent = (portal2.Get(i)[0] / 255.0f);
        const auto result = (percent * magr.Length()) + magr.Min;
        if (!test_equal(expected, result, 0.005))
        {
          std::cerr << "failed comparison at index: " << i << " found: " << result
                    << " was expecting: " << expected << std::endl;
          SVTKM_TEST_ASSERT(test_equal(expected, result), "magnitude RGB failed");
        }
      }
    }

    //use the components of the vector, if the vector is 2 or 4 we need
    //to ignore the last component as it is alpha
    int end = (N % 2 == 0) ? (N - 1) : N;
    for (int i = 0; i < end; ++i)
    {
      this->Worklet.RunComponent(input, i, output);
      valid = verify(this->Worklet.GetAlpha(), output);
      SVTKM_TEST_ASSERT(valid, "per component RGB failed");
    }
  }
};

#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif


using TypeListScalarColorTypes = svtkm::List<svtkm::Float32,
                                            svtkm::Float64,
                                            svtkm::Vec2f_32,
                                            svtkm::Vec2f_64,
                                            svtkm::Vec3f_32,
                                            svtkm::Vec3f_64,
                                            svtkm::Vec4f_32,
                                            svtkm::Vec4f_64>;

using TypeListUIntColorTypes =
  svtkm::List<svtkm::UInt8, svtkm::Vec2ui_8, svtkm::Vec3ui_8, svtkm::Vec4ui_8>;


void TestScalarsToColors()
{
  std::cout << "Test ConvertToRGB with UInt8 types" << std::endl;
  svtkm::testing::Testing::TryTypes(TestToRGB(), TypeListUIntColorTypes());
  std::cout << "Test ConvertToRGB with Scalar types" << std::endl;
  svtkm::testing::Testing::TryTypes(TestToRGB(0.0f, 1.0f), TypeListScalarColorTypes());
  std::cout << "Test ShiftScaleToRGB with scalar types and varying range" << std::endl;
  svtkm::testing::Testing::TryTypes(TestToRGB(1024.0f, 4096.0f), TypeListScalarColorTypes());
  svtkm::testing::Testing::TryTypes(TestToRGB(-2048.0f, 1024.0f), TypeListScalarColorTypes());

  std::cout << "Test ConvertToRGBA with UInt8 types and alpha values=[1.0, 0.5, 0.0]" << std::endl;
  svtkm::testing::Testing::TryTypes(TestToRGBA(), TypeListUIntColorTypes());
  svtkm::testing::Testing::TryTypes(TestToRGBA(0.0f, 255.0f, 0.5f), TypeListUIntColorTypes());
  svtkm::testing::Testing::TryTypes(TestToRGBA(0.0f, 255.0f, 0.0f), TypeListUIntColorTypes());

  std::cout << "Test ConvertToRGBA with Scalar types and alpha values=[0.3, 0.6, 1.0]" << std::endl;
  svtkm::testing::Testing::TryTypes(TestToRGBA(0.0f, 1.0f, 0.3f), TypeListScalarColorTypes());
  svtkm::testing::Testing::TryTypes(TestToRGBA(0.0f, 1.0f, 0.6f), TypeListScalarColorTypes());
  svtkm::testing::Testing::TryTypes(TestToRGBA(0.0f, 1.0f, 1.0f), TypeListScalarColorTypes());

  std::cout
    << "Test ConvertToRGBA with Scalar types and varying range with alpha values=[0.25, 0.5, 0.75]"
    << std::endl;
  svtkm::testing::Testing::TryTypes(TestToRGBA(-0.075f, -0.025f, 0.25f), TypeListScalarColorTypes());
  svtkm::testing::Testing::TryTypes(TestToRGBA(0.0f, 2048.0f, 0.5f), TypeListScalarColorTypes());
  svtkm::testing::Testing::TryTypes(TestToRGBA(-2048.0f, 2048.0f, 0.75f),
                                   TypeListScalarColorTypes());
}
}

int UnitTestScalarsToColors(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestScalarsToColors, argc, argv);
}
