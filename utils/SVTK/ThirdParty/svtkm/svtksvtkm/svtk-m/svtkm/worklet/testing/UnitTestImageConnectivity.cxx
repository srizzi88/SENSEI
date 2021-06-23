//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/filter/Contour.h>

#include <svtkm/worklet/connectivities/ImageConnectivity.h>

class TestImageConnectivity
{
public:
  using Algorithm = svtkm::cont::Algorithm;

  void operator()() const
  {
    CCL_CUDA8x4();
    CCL_CUDA8x8();
  }

  void CCL_CUDA8x4() const
  {
    // example image from Connected Component Labeling in CUDA by OndˇrejˇŚtava,
    // Bedˇrich Beneˇ
    std::vector<svtkm::UInt8> pixels(8 * 4, 0);
    pixels[3] = pixels[4] = pixels[10] = pixels[11] = 1;
    pixels[1] = pixels[9] = pixels[16] = pixels[17] = pixels[24] = pixels[25] = 1;
    pixels[7] = pixels[15] = pixels[21] = pixels[23] = pixels[28] = pixels[29] = pixels[30] =
      pixels[31] = 1;

    svtkm::cont::DataSetBuilderUniform builder;
    svtkm::cont::DataSet data = builder.Create(svtkm::Id3(8, 4, 1));

    auto colorField = svtkm::cont::make_FieldPoint("color", svtkm::cont::make_ArrayHandle(pixels));
    data.AddField(colorField);

    svtkm::cont::ArrayHandle<svtkm::Id> component;
    svtkm::worklet::connectivity::ImageConnectivity().Run(
      data.GetCellSet().Cast<svtkm::cont::CellSetStructured<2>>(), colorField.GetData(), component);

    std::vector<svtkm::Id> componentExpected = { 0, 1, 2, 1, 1, 3, 3, 4, 0, 1, 1, 1, 3, 3, 3, 4,
                                                1, 1, 3, 3, 3, 4, 3, 4, 1, 1, 3, 3, 4, 4, 4, 4 };


    std::size_t i = 0;
    for (svtkm::Id index = 0; index < component.GetNumberOfValues(); index++, i++)
    {
      SVTKM_TEST_ASSERT(component.GetPortalConstControl().Get(index) == componentExpected[i],
                       "Components has unexpected value.");
    }
  }

  void CCL_CUDA8x8() const
  {
    // example from Figure 35.7 of Connected Component Labeling in CUDA by OndˇrejˇŚtava,
    // Bedˇrich Beneˇ
    std::vector<svtkm::UInt8> pixels{
      0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1,
      1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1,
      1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
    };

    svtkm::cont::DataSetBuilderUniform builder;
    svtkm::cont::DataSet data = builder.Create(svtkm::Id3(8, 8, 1));

    auto colorField =
      svtkm::cont::make_Field("color", svtkm::cont::Field::Association::POINTS, pixels);
    data.AddField(colorField);

    svtkm::cont::ArrayHandle<svtkm::Id> component;
    svtkm::worklet::connectivity::ImageConnectivity().Run(
      data.GetCellSet().Cast<svtkm::cont::CellSetStructured<2>>(), colorField.GetData(), component);

    std::vector<svtkm::UInt8> componentExpected = { 0, 1, 1, 1, 0, 1, 1, 2, 0, 0, 0, 1, 0, 1, 1, 2,
                                                   0, 1, 1, 0, 0, 1, 1, 2, 0, 1, 0, 0, 0, 1, 1, 2,
                                                   0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
                                                   0, 1, 0, 1, 1, 1, 3, 3, 0, 1, 1, 1, 1, 1, 3, 3 };

    for (svtkm::Id i = 0; i < component.GetNumberOfValues(); ++i)
    {
      SVTKM_TEST_ASSERT(component.GetPortalConstControl().Get(i) == componentExpected[size_t(i)],
                       "Components has unexpected value.");
    }
  }
};


int UnitTestImageConnectivity(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestImageConnectivity(), argc, argv);
}
