//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/filter/FieldToColors.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
namespace
{
void TestFieldToColors()
{
  //faux input field
  constexpr svtkm::Id nvals = 8;
  constexpr int data[nvals] = { -1, 0, 10, 20, 30, 40, 50, 60 };

  //build a color table with clamping off and verify that sampling works
  svtkm::Range range{ 0.0, 50.0 };
  svtkm::cont::ColorTable table(svtkm::cont::ColorTable::Preset::COOL_TO_WARM);
  table.RescaleToRange(range);
  table.SetClampingOff();
  table.SetAboveRangeColor(svtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
  table.SetBelowRangeColor(svtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green

  svtkm::cont::DataSet ds = svtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();
  svtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(ds, "faux", data, nvals);

  svtkm::filter::FieldToColors ftc(table);
  ftc.SetOutputToRGBA();
  ftc.SetActiveField("faux");
  ftc.SetOutputFieldName("colors");

  auto rgbaResult = ftc.Execute(ds);
  SVTKM_TEST_ASSERT(rgbaResult.HasPointField("colors"), "Field missing.");
  svtkm::cont::Field Result = rgbaResult.GetPointField("colors");
  svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> resultRGBAHandle;
  Result.GetData().CopyTo(resultRGBAHandle);

  //values confirmed with ParaView 5.4
  const svtkm::Vec4ui_8 correct_diverging_rgba_values[nvals] = {
    { 0, 0, 255, 255 },     { 59, 76, 192, 255 },   { 122, 157, 248, 255 }, { 191, 211, 246, 255 },
    { 241, 204, 184, 255 }, { 238, 134, 105, 255 }, { 180, 4, 38, 255 },    { 255, 0, 0, 255 }
  };
  auto portalRGBA = resultRGBAHandle.GetPortalConstControl();
  for (std::size_t i = 0; i < nvals; ++i)
  {
    auto result = portalRGBA.Get(static_cast<svtkm::Id>(i));
    SVTKM_TEST_ASSERT(result == correct_diverging_rgba_values[i],
                     "incorrect value when interpolating between values");
  }

  //Now verify that we can switching our output mode
  ftc.SetOutputToRGB();
  auto rgbResult = ftc.Execute(ds);
  SVTKM_TEST_ASSERT(rgbResult.HasPointField("colors"), "Field missing.");
  Result = rgbResult.GetPointField("colors");
  svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> resultRGBHandle;
  Result.GetData().CopyTo(resultRGBHandle);

  //values confirmed with ParaView 5.4
  const svtkm::Vec3ui_8 correct_diverging_rgb_values[nvals] = { { 0, 0, 255 },     { 59, 76, 192 },
                                                               { 122, 157, 248 }, { 191, 211, 246 },
                                                               { 241, 204, 184 }, { 238, 134, 105 },
                                                               { 180, 4, 38 },    { 255, 0, 0 } };
  auto portalRGB = resultRGBHandle.GetPortalConstControl();
  for (std::size_t i = 0; i < nvals; ++i)
  {
    auto result = portalRGB.Get(static_cast<svtkm::Id>(i));
    SVTKM_TEST_ASSERT(result == correct_diverging_rgb_values[i],
                     "incorrect value when interpolating between values");
  }
}
}

int UnitTestFieldToColors(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestFieldToColors, argc, argv);
}
