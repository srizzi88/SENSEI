//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/filter/CreateResult.h>
#include <svtkm/filter/FilterField.h>

#include <svtkm/io/writer/SVTKDataSetWriter.h>

#include <svtkm/cont/Initialize.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>

#include <svtkm/VectorAnalysis.h>

namespace svtkm
{
namespace worklet
{

struct HelloWorklet : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inVector, FieldOut outMagnitude);

  SVTKM_EXEC void operator()(const svtkm::Vec3f& inVector, svtkm::FloatDefault& outMagnitude) const
  {
    outMagnitude = svtkm::Magnitude(inVector);
  }
};
}
} // namespace svtkm::worklet

namespace svtkm
{
namespace filter
{

class HelloField : public svtkm::filter::FilterField<HelloField>
{
public:
  // Specify that this filter operates on 3-vectors
  using SupportedTypes = svtkm::TypeListFieldVec3;

  template <typename FieldType, typename Policy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& inDataSet,
                                          const FieldType& inField,
                                          const svtkm::filter::FieldMetadata& fieldMetadata,
                                          svtkm::filter::PolicyBase<Policy>)
  {
    SVTKM_IS_ARRAY_HANDLE(FieldType);

    //construct our output
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> outField;

    //construct our invoker to launch worklets
    svtkm::worklet::HelloWorklet mag;
    this->Invoke(mag, inField, outField); //launch mag worklets

    //construct output field information
    if (this->GetOutputFieldName().empty())
    {
      this->SetOutputFieldName(fieldMetadata.GetName() + "_magnitude");
    }

    //return the result, which is the input data with the computed field added to it
    return svtkm::filter::CreateResult(
      inDataSet, outField, this->GetOutputFieldName(), fieldMetadata);
  }
};
}
} // svtkm::filter


int main(int argc, char** argv)
{
  svtkm::cont::Initialize(argc, argv, svtkm::cont::InitializeOptions::Strict);

  svtkm::cont::testing::MakeTestDataSet testDataMaker;
  svtkm::cont::DataSet inputData = testDataMaker.Make3DExplicitDataSetCowNose();

  svtkm::filter::HelloField helloField;
  helloField.SetActiveField("point_vectors");
  svtkm::cont::DataSet outputData = helloField.Execute(inputData);

  svtkm::io::writer::SVTKDataSetWriter writer("out_data.svtk");
  writer.WriteDataSet(outputData);

  return 0;
}
