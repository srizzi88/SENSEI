//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/ThresholdPoints.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/CellSet.h>

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{

// Predicate for values less than minimum
class ValuesBelow
{
public:
  SVTKM_CONT
  ValuesBelow(const svtkm::FloatDefault& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  SVTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return static_cast<svtkm::FloatDefault>(value) <= this->Value;
  }

private:
  svtkm::FloatDefault Value;
};

// Predicate for values greater than maximum
class ValuesAbove
{
public:
  SVTKM_CONT
  ValuesAbove(const svtkm::FloatDefault& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  SVTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return static_cast<svtkm::FloatDefault>(value) >= this->Value;
  }

private:
  svtkm::FloatDefault Value;
};

// Predicate for values between minimum and maximum
class ValuesBetween
{
public:
  SVTKM_CONT
  ValuesBetween(const svtkm::FloatDefault& lower, const svtkm::FloatDefault& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename ScalarType>
  SVTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return static_cast<svtkm::FloatDefault>(value) >= this->Lower &&
      static_cast<svtkm::FloatDefault>(value) <= this->Upper;
  }

private:
  svtkm::FloatDefault Lower;
  svtkm::FloatDefault Upper;
};

using svtkm::cont::testing::MakeTestDataSet;

class TestingThresholdPoints
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing threshold on 2D uniform dataset" << std::endl;

    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();

    // Output dataset contains input coordinate system and point data
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    outDataSet.AddField(dataset.GetField("pointvar"));

    // Output dataset gets new cell set of points that meet threshold predicate
    svtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet =
      threshold.Run(dataset.GetCellSet(),
                    dataset.GetField("pointvar").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
                    ValuesBetween(40.0f, 71.0f));
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 11),
                     "Wrong result for ThresholdPoints");

    svtkm::cont::Field pointField = outDataSet.GetField("pointvar");
    svtkm::cont::ArrayHandle<svtkm::Float32> pointFieldArray;
    pointField.GetData().CopyTo(pointFieldArray);
    SVTKM_TEST_ASSERT(pointFieldArray.GetPortalConstControl().Get(12) == 50.0f,
                     "Wrong point field data");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing threshold on 3D uniform dataset" << std::endl;

    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();

    // Output dataset contains input coordinate system and point data
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));
    outDataSet.AddField(dataset.GetField("pointvar"));

    // Output dataset gets new cell set of points that meet threshold predicate
    svtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet =
      threshold.Run(dataset.GetCellSet(),
                    dataset.GetField("pointvar").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
                    ValuesAbove(1.0f));
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 27),
                     "Wrong result for ThresholdPoints");
  }

  void TestExplicit3D() const
  {
    std::cout << "Testing threshold on 3D explicit dataset" << std::endl;

    using OutCellSetType = svtkm::cont::CellSetSingleType<>;

    svtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();

    // Output dataset contains input coordinate system and point data
    svtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output dataset gets new cell set of points that meet threshold predicate
    svtkm::worklet::ThresholdPoints threshold;
    OutCellSetType outCellSet;
    outCellSet =
      threshold.Run(dataset.GetCellSet(),
                    dataset.GetField("pointvar").GetData().ResetTypes(svtkm::TypeListFieldScalar()),
                    ValuesBelow(50.0f));
    outDataSet.SetCellSet(outCellSet);

    SVTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 6),
                     "Wrong result for ThresholdPoints");
  }

  void operator()() const
  {
    this->TestUniform2D();
    this->TestUniform3D();
    this->TestExplicit3D();
  }
};
}

int UnitTestThresholdPoints(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestingThresholdPoints(), argc, argv);
}
