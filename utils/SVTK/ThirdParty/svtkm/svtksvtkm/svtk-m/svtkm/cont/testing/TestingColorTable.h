//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingColorTable_h
#define svtk_m_cont_testing_TestingColorTable_h

#include <svtkm/Types.h>
#include <svtkm/cont/ColorTable.h>
#include <svtkm/cont/ColorTableSamples.h>
#include <svtkm/cont/testing/Testing.h>

// Required for implementation of ArrayRangeCompute for "odd" arrays
#include <svtkm/cont/ArrayRangeCompute.hxx>

// Required for implementation of ColorTable
#include <svtkm/cont/ColorTable.hxx>

#include <algorithm>
#include <iostream>

namespace svtkm
{
namespace cont
{
namespace testing
{

template <typename DeviceAdapterTag>
class TestingColorTable
{

public:
  static void TestConstructors()
  {
    svtkm::Range inValidRange{ 1.0, 0.0 };
    svtkm::Range range{ 0.0, 1.0 };
    svtkm::Vec<float, 3> rgb1{ 0.0f, 0.0f, 0.0f };
    svtkm::Vec<float, 3> rgb2{ 1.0f, 1.0f, 1.0f };
    auto rgbspace = svtkm::cont::ColorSpace::RGB;
    auto hsvspace = svtkm::cont::ColorSpace::HSV;
    auto diverging = svtkm::cont::ColorSpace::DIVERGING;

    svtkm::cont::ColorTable table(rgbspace);
    SVTKM_TEST_ASSERT(table.GetColorSpace() == rgbspace, "color space not saved");
    SVTKM_TEST_ASSERT(table.GetRange() == inValidRange, "default range incorrect");

    svtkm::cont::ColorTable tableRGB(range, rgb1, rgb2, hsvspace);
    SVTKM_TEST_ASSERT(tableRGB.GetColorSpace() == hsvspace, "color space not saved");
    SVTKM_TEST_ASSERT(tableRGB.GetRange() == range, "color range not saved");

    svtkm::Vec<float, 4> rgba1{ 0.0f, 0.0f, 0.0f, 1.0f };
    svtkm::Vec<float, 4> rgba2{ 1.0f, 1.0f, 1.0f, 0.0f };
    svtkm::cont::ColorTable tableRGBA(range, rgba1, rgba2, diverging);
    SVTKM_TEST_ASSERT(tableRGBA.GetColorSpace() == diverging, "color space not saved");
    SVTKM_TEST_ASSERT(tableRGBA.GetRange() == range, "color range not saved");

    //verify we can store a vector of tables
    std::vector<svtkm::cont::ColorTable> tables;
    tables.push_back(table);
    tables.push_back(tableRGB);
    tables.push_back(tableRGBA);
    tables.push_back(tableRGBA);
    tables.push_back(tableRGB);
    tables.push_back(table);
  }

  static void TestLoadPresets()
  {
    svtkm::Range range{ 0.0, 1.0 };
    auto rgbspace = svtkm::cont::ColorSpace::RGB;
    auto hsvspace = svtkm::cont::ColorSpace::HSV;
    auto labspace = svtkm::cont::ColorSpace::LAB;
    auto diverging = svtkm::cont::ColorSpace::DIVERGING;

    {
      svtkm::cont::ColorTable table(rgbspace);
      SVTKM_TEST_ASSERT(table.LoadPreset("Cool to Warm"));
      SVTKM_TEST_ASSERT(table.GetColorSpace() == diverging,
                       "color space not switched when loading preset");
      SVTKM_TEST_ASSERT(table.GetRange() == range, "color range not correct after loading preset");
      SVTKM_TEST_ASSERT(table.GetNumberOfPoints() == 3);

      SVTKM_TEST_ASSERT(table.LoadPreset(svtkm::cont::ColorTable::Preset::COOL_TO_WARM_EXTENDED));
      SVTKM_TEST_ASSERT(table.GetColorSpace() == labspace,
                       "color space not switched when loading preset");
      SVTKM_TEST_ASSERT(table.GetRange() == range, "color range not correct after loading preset");
      SVTKM_TEST_ASSERT(table.GetNumberOfPoints() == 35);

      table.SetColorSpace(hsvspace);
      SVTKM_TEST_ASSERT((table.LoadPreset("no table with this name") == false),
                       "failed to error out on bad preset table name");
      //verify that after a failure we still have the previous preset loaded
      SVTKM_TEST_ASSERT(table.GetColorSpace() == hsvspace,
                       "color space not switched when loading preset");
      SVTKM_TEST_ASSERT(table.GetRange() == range, "color range not correct after failing preset");
      SVTKM_TEST_ASSERT(table.GetNumberOfPoints() == 35);
    }


    //verify that we can get the presets
    std::set<std::string> names = svtkm::cont::ColorTable::GetPresets();
    SVTKM_TEST_ASSERT(names.size() == 18, "incorrect number of names in preset set");

    SVTKM_TEST_ASSERT(names.count("Inferno") == 1, "names should contain inferno");
    SVTKM_TEST_ASSERT(names.count("Black-Body Radiation") == 1,
                     "names should contain black-body radiation");
    SVTKM_TEST_ASSERT(names.count("Viridis") == 1, "names should contain viridis");
    SVTKM_TEST_ASSERT(names.count("Black - Blue - White") == 1,
                     "names should contain black, blue and white");
    SVTKM_TEST_ASSERT(names.count("Blue to Orange") == 1, "names should contain samsel fire");
    SVTKM_TEST_ASSERT(names.count("Jet") == 1, "names should contain jet");

    // verify that we can load in all the listed color tables
    for (auto&& name : names)
    {
      svtkm::cont::ColorTable table(name);
      SVTKM_TEST_ASSERT(table.GetNumberOfPoints() > 0, "Issue loading preset ", name);
    }

    auto presetEnum = { svtkm::cont::ColorTable::Preset::DEFAULT,
                        svtkm::cont::ColorTable::Preset::COOL_TO_WARM,
                        svtkm::cont::ColorTable::Preset::COOL_TO_WARM_EXTENDED,
                        svtkm::cont::ColorTable::Preset::VIRIDIS,
                        svtkm::cont::ColorTable::Preset::INFERNO,
                        svtkm::cont::ColorTable::Preset::PLASMA,
                        svtkm::cont::ColorTable::Preset::BLACK_BODY_RADIATION,
                        svtkm::cont::ColorTable::Preset::X_RAY,
                        svtkm::cont::ColorTable::Preset::GREEN,
                        svtkm::cont::ColorTable::Preset::BLACK_BLUE_WHITE,
                        svtkm::cont::ColorTable::Preset::BLUE_TO_ORANGE,
                        svtkm::cont::ColorTable::Preset::GRAY_TO_RED,
                        svtkm::cont::ColorTable::Preset::COLD_AND_HOT,
                        svtkm::cont::ColorTable::Preset::BLUE_GREEN_ORANGE,
                        svtkm::cont::ColorTable::Preset::YELLOW_GRAY_BLUE,
                        svtkm::cont::ColorTable::Preset::RAINBOW_UNIFORM,
                        svtkm::cont::ColorTable::Preset::JET,
                        svtkm::cont::ColorTable::Preset::RAINBOW_DESATURATED };
    for (svtkm::cont::ColorTable::Preset preset : presetEnum)
    {
      svtkm::cont::ColorTable table(preset);
      SVTKM_TEST_ASSERT(table.GetNumberOfPoints() > 0, "Issue loading preset");
    }
  }

  static void TestClamping()
  {
    svtkm::Range range{ 0.0, 1.0 };
    svtkm::Vec<float, 3> rgb1{ 0.0f, 1.0f, 0.0f };
    svtkm::Vec<float, 3> rgb2{ 1.0f, 0.0f, 1.0f };
    auto rgbspace = svtkm::cont::ColorSpace::RGB;

    svtkm::cont::ColorTable table(range, rgb1, rgb2, rgbspace);
    SVTKM_TEST_ASSERT(table.GetClamping() == true, "clamping not setup properly");

    constexpr svtkm::Id nvals = 4;
    constexpr int data[nvals] = { -1, 0, 1, 2 };
    auto field = svtkm::cont::make_ArrayHandle(data, nvals);

    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> colors;
    const bool ran = table.Map(field, colors);
    SVTKM_TEST_ASSERT(ran, "color table failed to execute");

    //verify that we clamp the values to the expected range
    const svtkm::Vec3ui_8 correct[nvals] = {
      { 0, 255, 0 }, { 0, 255, 0 }, { 255, 0, 255 }, { 255, 0, 255 }
    };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct[i], "incorrect value in color from clamp test");
    }
  }

  static void TestRangeColors()
  {
    svtkm::Range range{ -1.0, 2.0 };
    svtkm::Vec<float, 3> rgb1{ 0.0f, 1.0f, 0.0f };
    svtkm::Vec<float, 3> rgb2{ 1.0f, 0.0f, 1.0f };
    auto rgbspace = svtkm::cont::ColorSpace::RGB;

    svtkm::cont::ColorTable table(range, rgb1, rgb2, rgbspace);
    table.SetClampingOff();
    SVTKM_TEST_ASSERT(table.GetClamping() == false, "clamping not setup properly");

    constexpr svtkm::Id nvals = 4;
    constexpr int data[nvals] = { -2, -1, 2, 3 };
    auto field = svtkm::cont::make_ArrayHandle(data, nvals);

    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> colors;
    const bool ran = table.Map(field, colors);
    SVTKM_TEST_ASSERT(ran, "color table failed to execute");

    //verify that both the above and below range colors are used,
    //and that the default value of both is 0,0,0
    const svtkm::Vec3ui_8 correct_range_defaults[nvals] = {
      { 0, 0, 0 }, { 0, 255, 0 }, { 255, 0, 255 }, { 0, 0, 0 }
    };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_range_defaults[i],
                       "incorrect value in color from default range color test");
    }


    //verify that we can specify custom above and below range colors
    table.SetAboveRangeColor(svtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
    table.SetBelowRangeColor(svtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green
    const bool ran2 = table.Map(field, colors);
    SVTKM_TEST_ASSERT(ran2, "color table failed to execute");
    const svtkm::Vec3ui_8 correct_custom_range_colors[nvals] = {
      { 0, 0, 255 }, { 0, 255, 0 }, { 255, 0, 255 }, { 255, 0, 0 }
    };
    portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_custom_range_colors[i],
                       "incorrect value in custom above/below range color test");
    }
  }

  static void TestRescaleRange()
  {
    svtkm::Range range{ -100.0, 100.0 };

    //implement a blue2yellow color table
    svtkm::Vec<float, 3> rgb1{ 0.0f, 0.0f, 1.0f };
    svtkm::Vec<float, 3> rgb2{ 1.0f, 1.0f, 0.0f };
    auto lab = svtkm::cont::ColorSpace::LAB;

    svtkm::cont::ColorTable table(range, rgb1, rgb2, lab);
    table.AddPoint(0.0, svtkm::Vec<float, 3>{ 0.5f, 0.5f, 0.5f });
    SVTKM_TEST_ASSERT(table.GetRange() == range, "custom range not saved");

    svtkm::cont::ColorTable newTable = table.MakeDeepCopy();
    SVTKM_TEST_ASSERT(newTable.GetRange() == range, "custom range not saved");

    svtkm::Range normalizedRange{ 0.0, 50.0 };
    newTable.RescaleToRange(normalizedRange);
    SVTKM_TEST_ASSERT(table.GetRange() == range, "deep copy not working properly");
    SVTKM_TEST_ASSERT(newTable.GetRange() == normalizedRange, "rescale of range failed");
    SVTKM_TEST_ASSERT(newTable.GetNumberOfPoints() == 3,
                     "rescaled has incorrect number of control points");

    //Verify that the rescaled color table generates correct colors
    constexpr svtkm::Id nvals = 6;
    constexpr int data[nvals] = { 0, 10, 20, 30, 40, 50 };
    auto field = svtkm::cont::make_ArrayHandle(data, nvals);

    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> colors;
    const bool ran = newTable.Map(field, colors);
    SVTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    const svtkm::Vec3ui_8 correct_lab_values[nvals] = { { 0, 0, 255 },     { 105, 69, 204 },
                                                       { 126, 109, 153 }, { 156, 151, 117 },
                                                       { 207, 202, 87 },  { 255, 255, 0 } };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_lab_values[i],
                       "incorrect value in color after rescaling the color table");
    }
  }

  static void TestAddPoints()
  {
    svtkm::Range range{ -20, 20.0 };
    auto rgbspace = svtkm::cont::ColorSpace::RGB;

    svtkm::cont::ColorTable table(rgbspace);
    table.AddPoint(-10.0, svtkm::Vec<float, 3>{ 0.0f, 1.0f, 1.0f });
    table.AddPoint(-20.0, svtkm::Vec<float, 3>{ 1.0f, 1.0f, 1.0f });
    table.AddPoint(20.0, svtkm::Vec<float, 3>{ 0.0f, 0.0f, 0.0f });
    table.AddPoint(0.0, svtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f });

    SVTKM_TEST_ASSERT(table.GetRange() == range, "adding points to make range expand properly");
    SVTKM_TEST_ASSERT(table.GetNumberOfPoints() == 4,
                     "adding points caused number of control points to be wrong");

    constexpr svtkm::Id nvals = 3;
    constexpr float data[nvals] = { 10.0f, -5.0f, -15.0f };

    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> colors;
    auto field = svtkm::cont::make_ArrayHandle(data, nvals);
    const bool ran = table.Map(field, colors);
    SVTKM_TEST_ASSERT(ran, "color table failed to execute");

    const svtkm::Vec3ui_8 correct_rgb_values[nvals] = { { 0, 0, 128 },
                                                       { 0, 128, 255 },
                                                       { 128, 255, 255 } };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_rgb_values[i],
                       "incorrect value when interpolating between values");
    }
  }

  static void TestAddSegments()
  {
    svtkm::Range range{ 0.0, 50.0 };
    auto diverging = svtkm::cont::ColorSpace::DIVERGING;

    svtkm::cont::ColorTable table(svtkm::cont::ColorTable::Preset::COOL_TO_WARM);
    SVTKM_TEST_ASSERT(table.GetColorSpace() == diverging,
                     "color space not switched when loading preset");


    //Opacity Ramp from 0 to 1
    table.AddSegmentAlpha(0.0, 0.0f, 1.0, 1.0f);
    SVTKM_TEST_ASSERT(table.GetNumberOfPointsAlpha() == 2, "incorrect number of alpha points");

    table.RescaleToRange(range);

    //Verify that the opacity points have moved
    svtkm::Vec<double, 4> opacityData;
    table.GetPointAlpha(1, opacityData);
    SVTKM_TEST_ASSERT(test_equal(opacityData[0], range.Max), "rescale to range failed on opacity");
    SVTKM_TEST_ASSERT(opacityData[1] == 1.0, "rescale changed opacity values");
    SVTKM_TEST_ASSERT(opacityData[2] == 0.5, "rescale modified mid/sharp of opacity");
    SVTKM_TEST_ASSERT(opacityData[3] == 0.0, "rescale modified mid/sharp of opacity");


    constexpr svtkm::Id nvals = 6;
    constexpr int data[nvals] = { 0, 10, 20, 30, 40, 50 };

    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> colors;
    auto field = svtkm::cont::make_ArrayHandle(data, nvals);
    const bool ran = table.Map(field, colors);
    SVTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    const svtkm::Vec4ui_8 correct_diverging_values[nvals] = {
      { 59, 76, 192, 0 },     { 124, 159, 249, 51 },  { 192, 212, 245, 102 },
      { 242, 203, 183, 153 }, { 238, 133, 104, 204 }, { 180, 4, 38, 255 }
    };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_diverging_values[i],
                       "incorrect value when interpolating between values");
    }
  }

  static void TestRemovePoints()
  {
    auto hsv = svtkm::cont::ColorSpace::HSV;

    svtkm::cont::ColorTable table(hsv);
    //implement Blue to Red Rainbow color table
    table.AddSegment(0,
                     svtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f },
                     1., //second points color should be replaced by following segment
                     svtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f });

    table.AddPoint(-10.0, svtkm::Vec<float, 3>{ 0.0f, 1.0f, 1.0f });
    table.AddPoint(-20.0, svtkm::Vec<float, 3>{ 1.0f, 1.0f, 1.0f });
    table.AddPoint(20.0, svtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f });

    SVTKM_TEST_ASSERT(table.RemovePoint(-10.) == true, "failed to remove a existing point");
    SVTKM_TEST_ASSERT(table.RemovePoint(-20.) == true, "failed to remove a existing point");
    SVTKM_TEST_ASSERT(table.RemovePoint(20.) == true, "failed to remove a existing point");
    SVTKM_TEST_ASSERT(table.RemovePoint(20.) == false, "can't remove a point that doesn't exist");

    SVTKM_TEST_ASSERT((table.GetRange() == svtkm::Range{ 0.0, 1.0 }),
                     "removing points didn't update range");
    table.RescaleToRange(svtkm::Range{ 0.0, 50.0 });

    constexpr svtkm::Id nvals = 6;
    constexpr float data[nvals] = { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };

    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> colors;
    auto field = svtkm::cont::make_ArrayHandle(data, nvals);
    const bool ran = table.Map(field, colors);
    SVTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    const svtkm::Vec3ui_8 correct_hsv_values[nvals] = { { 0, 0, 255 },   { 0, 204, 255 },
                                                       { 0, 255, 102 }, { 102, 255, 0 },
                                                       { 255, 204, 0 }, { 255, 0, 0 } };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_hsv_values[i],
                       "incorrect value when interpolating between values");
    }

    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> colors_rgb;
    table.SetColorSpace(svtkm::cont::ColorSpace::RGB);
    table.Map(field, colors_rgb);

    const svtkm::Vec3ui_8 correct_rgb_values[nvals] = { { 0, 0, 255 },   { 51, 0, 204 },
                                                       { 102, 0, 153 }, { 153, 0, 102 },
                                                       { 204, 0, 51 },  { 255, 0, 0 } };
    auto rgb_portal = colors_rgb.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = rgb_portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_rgb_values[i],
                       "incorrect value when interpolating between values");
    }
  }

  static void TestOpacityOnlyPoints()
  {
    auto hsv = svtkm::cont::ColorSpace::HSV;

    svtkm::cont::ColorTable table(hsv);
    //implement only a color table
    table.AddPointAlpha(0.0, 0.0f, 0.75f, 0.25f);
    table.AddPointAlpha(1.0, 1.0f);

    table.AddPointAlpha(10.0, 0.5f, 0.5f, 0.0f);
    table.AddPointAlpha(-10.0, 0.0f);
    table.AddPointAlpha(-20.0, 1.0f);
    table.AddPointAlpha(20.0, 0.5f);

    SVTKM_TEST_ASSERT(table.RemovePointAlpha(10.) == true, "failed to remove a existing point");
    SVTKM_TEST_ASSERT(table.RemovePointAlpha(-10.) == true, "failed to remove a existing point");
    SVTKM_TEST_ASSERT(table.RemovePointAlpha(-20.) == true, "failed to remove a existing point");
    SVTKM_TEST_ASSERT(table.RemovePointAlpha(20.) == true, "failed to remove a existing point");
    SVTKM_TEST_ASSERT(table.RemovePointAlpha(20.) == false,
                     "can't remove a point that doesn't exist");

    SVTKM_TEST_ASSERT((table.GetRange() == svtkm::Range{ 0.0, 1.0 }),
                     "removing points didn't update range");
    table.RescaleToRange(svtkm::Range{ 0.0, 50.0 });

    constexpr svtkm::Id nvals = 6;
    constexpr float data[nvals] = { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };

    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> colors;
    auto field = svtkm::cont::make_ArrayHandle(data, nvals);
    const bool ran = table.Map(field, colors);
    SVTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    const svtkm::Vec4ui_8 correct_opacity_values[nvals] = { { 0, 0, 0, 0 },   { 0, 0, 0, 1 },
                                                           { 0, 0, 0, 11 },  { 0, 0, 0, 52 },
                                                           { 0, 0, 0, 203 }, { 0, 0, 0, 255 } };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_opacity_values[i],
                       "incorrect value when interpolating between opacity values");
    }
  }

  static void TestWorkletTransport()
  {
    using namespace svtkm::worklet::colorconversion;

    svtkm::cont::ColorTable table(svtkm::cont::ColorTable::Preset::GREEN);
    SVTKM_TEST_ASSERT((table.GetRange() == svtkm::Range{ 0.0, 1.0 }),
                     "loading linear green table failed with wrong range");
    SVTKM_TEST_ASSERT((table.GetNumberOfPoints() == 21),
                     "loading linear green table failed with number of control points");

    constexpr svtkm::Id nvals = 3;
    constexpr double data[3] = { 0.0, 0.5, 1.0 };
    auto samples = svtkm::cont::make_ArrayHandle(data, nvals);

    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> colors;
    TransferFunction transfer(table.PrepareForExecution(DeviceAdapterTag{}));
    svtkm::worklet::DispatcherMapField<TransferFunction> dispatcher(transfer);
    dispatcher.SetDevice(DeviceAdapterTag());
    dispatcher.Invoke(samples, colors);

    const svtkm::Vec4ui_8 correct_sampling_points[nvals] = { { 14, 28, 31, 255 },
                                                            { 21, 150, 21, 255 },
                                                            { 255, 251, 230, 255 } };

    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_sampling_points[i],
                       "incorrect value when interpolating in linear green preset");
    }
  }

  static void TestSampling()
  {

    svtkm::cont::ColorTable table(svtkm::cont::ColorTable::Preset::GREEN);
    SVTKM_TEST_ASSERT((table.GetRange() == svtkm::Range{ 0.0, 1.0 }),
                     "loading linear green table failed with wrong range");
    SVTKM_TEST_ASSERT((table.GetNumberOfPoints() == 21),
                     "loading linear green table failed with number of control points");

    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> colors;
    constexpr svtkm::Id nvals = 3;
    table.Sample(3, colors);

    const svtkm::Vec4ui_8 correct_sampling_points[nvals] = { { 14, 28, 31, 255 },
                                                            { 21, 150, 21, 255 },
                                                            { 255, 251, 230, 255 } };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_sampling_points[i],
                       "incorrect value when interpolating in linear green preset");
    }
  }

  static void TestLookupTable()
  {
    //build a color table with clamping off and verify that sampling works
    svtkm::Range range{ 0.0, 50.0 };
    svtkm::cont::ColorTable table(svtkm::cont::ColorTable::Preset::COOL_TO_WARM);
    table.RescaleToRange(range);
    table.SetClampingOff();
    table.SetAboveRangeColor(svtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
    table.SetBelowRangeColor(svtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green

    svtkm::cont::ColorTableSamplesRGB samples;
    table.Sample(256, samples);
    SVTKM_TEST_ASSERT((samples.Samples.GetNumberOfValues() == 260), "invalid sample length");

    constexpr svtkm::Id nvals = 8;
    constexpr int data[nvals] = { -1, 0, 10, 20, 30, 40, 50, 60 };

    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> colors;
    auto field = svtkm::cont::make_ArrayHandle(data, nvals);
    const bool ran = table.Map(field, samples, colors);
    SVTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    const svtkm::Vec3ui_8 correct_diverging_values[nvals] = { { 0, 0, 255 },     { 59, 76, 192 },
                                                             { 122, 157, 248 }, { 191, 211, 246 },
                                                             { 241, 204, 184 }, { 238, 134, 105 },
                                                             { 180, 4, 38 },    { 255, 0, 0 } };
    auto portal = colors.GetPortalConstControl();
    for (std::size_t i = 0; i < nvals; ++i)
    {
      auto result = portal.Get(static_cast<svtkm::Id>(i));
      SVTKM_TEST_ASSERT(result == correct_diverging_values[i],
                       "incorrect value when interpolating between values");
    }
  }

  struct TestAll
  {
    SVTKM_CONT void operator()() const
    {
      TestConstructors();
      TestLoadPresets();
      TestClamping();
      TestRangeColors();

      TestRescaleRange(); //uses Lab
      TestAddPoints();    //uses RGB
      TestAddSegments();  //uses Diverging && opacity
      TestRemovePoints(); //use HSV

      TestOpacityOnlyPoints();

      TestWorkletTransport();
      TestSampling();
      TestLookupTable();
    }
  };

  static int Run(int argc, char* argv[])
  {
    //We need to verify the color table runs on this specific device
    //so we need to force our single device
    svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return svtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
}
#endif
