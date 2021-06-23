#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkFieldData.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPassInputTypeAlgorithm.h"
#include "svtkPolyData.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkUnsignedIntArray.h"
#include "svtkXMLGenericDataObjectReader.h"

#define SVTK_SUCCESS 0
#define SVTK_FAILURE 1
namespace
{

class svtkTestAlgorithm : public svtkPassInputTypeAlgorithm
{
public:
  static svtkTestAlgorithm* New();
  svtkTestAlgorithm(const svtkTestAlgorithm&) = delete;
  void operator=(const svtkTestAlgorithm&) = delete;

  svtkTypeMacro(svtkTestAlgorithm, svtkPassInputTypeAlgorithm);

protected:
  svtkTestAlgorithm()
    : Superclass()
  {
    this->SetNumberOfOutputPorts(2);
  }

  int FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info) override
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    return 1;
  }

  int FillOutputPortInformation(int port, svtkInformation* info) override
  {
    if (port == 0)
    {
      info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
      return 1;
    }
    else
    {
      return Superclass::FillOutputPortInformation(port, info);
    }
  }

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override
  {
    int success = Superclass::RequestDataObject(request, inputVector, outputVector);
    svtkDataObject* output = svtkDataObject::GetData(outputVector, 0);
    if (!output || !svtkPolyData::SafeDownCast(output))
    {
      svtkNew<svtkPolyData> newOutput;
      outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), newOutput);
    }
    return success;
  }

  int RequestData(svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override
  {
    svtkDataSet* input = svtkDataSet::GetData(inputVector[0], 0);
    double bounds[6];
    input->GetBounds(bounds);
    svtkNew<svtkSphereSource> sphere;
    sphere->SetCenter(bounds[0], bounds[2], bounds[4]);
    sphere->SetRadius(bounds[1] - bounds[0]);
    sphere->Update();

    svtkPolyData* polyOut = svtkPolyData::GetData(outputVector, 0);
    polyOut->ShallowCopy(sphere->GetOutput());

    polyOut->GetFieldData()->PassData(input->GetFieldData());

    svtkDataSet* output = svtkDataSet::GetData(outputVector, 1);
    output->ShallowCopy(input);
    return 1;
  }
};

svtkStandardNewMacro(svtkTestAlgorithm);

void AddPerBlockFieldData(svtkCompositeDataSet* data)
{
  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(data->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataObject* currentData = iter->GetCurrentDataObject();
    if (svtkDataSet::SafeDownCast(currentData))
    {
      svtkFieldData* fd = currentData->GetFieldData();
      if (!fd)
      {
        svtkNew<svtkFieldData> fieldData;
        currentData->SetFieldData(fieldData);
        fd = fieldData;
      }
      svtkNew<svtkUnsignedIntArray> array;
      array->SetNumberOfComponents(1);
      array->SetNumberOfTuples(1);
      array->SetValue(0, iter->GetCurrentFlatIndex());
      array->SetName("compositeIndexBasedData");
      fd->AddArray(array);
      std::cout << "Assinging field data " << iter->GetCurrentFlatIndex() << std::endl;
    }
  }
}

bool CheckPerBlockFieldData(svtkCompositeDataSet* data)
{
  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(data->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataObject* currentData = iter->GetCurrentDataObject();
    if (svtkDataSet::SafeDownCast(currentData))
    {
      svtkFieldData* fd = currentData->GetFieldData();
      if (!fd)
      {
        // field data didn't propagate
        std::cout << "No field data!" << std::endl;
        return false;
      }
      svtkUnsignedIntArray* array =
        svtkUnsignedIntArray::SafeDownCast(fd->GetArray("compositeIndexBasedData"));
      if (!array)
      {
        // array type changed
        std::cout << "Expected field data array not found!" << std::endl;
        return false;
      }
      unsigned value = array->GetValue(0);
      if (value != iter->GetCurrentFlatIndex())
      {
        // data changed
        std::cout << "Field data didn't match, should be " << iter->GetCurrentFlatIndex()
                  << " but was " << value << std::endl;
        return false;
      }
    }
  }
  return true;
}

int TestComposite(std::string& inputDataFile, bool isAMR)
{
  svtkNew<svtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(inputDataFile.c_str());
  reader->Update();

  svtkCompositeDataSet* data = svtkCompositeDataSet::SafeDownCast(reader->GetOutput());

  AddPerBlockFieldData(data);

  svtkNew<svtkTestAlgorithm> testAlg;
  testAlg->SetInputData(data);
  testAlg->Update();

  int retVal = SVTK_SUCCESS;

  svtkDataObject* data0 = testAlg->GetOutputDataObject(0);
  svtkDataObject* data1 = testAlg->GetOutputDataObject(1);
  if (!svtkMultiBlockDataSet::SafeDownCast(data0))
  {
    std::cout << "Error: output 0 is not multiblock after composite data pipeline run" << std::endl;
    std::cout << "instead it is " << data0->GetClassName() << std::endl;
    retVal = SVTK_FAILURE;
  }
  if (!isAMR)
  {
    // This test doesn't work on AMR data... only the root block has field data and that field data
    // is copied to
    // all output blocks.
    if (retVal == SVTK_SUCCESS && !CheckPerBlockFieldData(svtkCompositeDataSet::SafeDownCast(data0)))
    {
      std::cout << "Per block field data for the first output port changed" << std::endl;
      retVal = SVTK_FAILURE;
    }
    if (!svtkMultiBlockDataSet::SafeDownCast(data1))
    {
      std::cout << "Error: output 1 is not multiblock after composite data pipeline run"
                << std::endl;
      std::cout << "instead it is " << data1->GetClassName() << std::endl;
      retVal = SVTK_FAILURE;
    }
  }
  else
  {
    if (!svtkHierarchicalBoxDataSet::SafeDownCast(data1))
    {
      std::cout << "Error: output 1 is not an AMR dataset after composite data pipeline run"
                << std::endl;
      std::cout << "instead it is " << data1->GetClassName() << std::endl;
      retVal = SVTK_FAILURE;
    }
  }
  if (retVal == SVTK_SUCCESS && !CheckPerBlockFieldData(svtkCompositeDataSet::SafeDownCast(data1)))
  {
    std::cout << "Per block field data for the second output port changed" << std::endl;
    retVal = SVTK_FAILURE;
  }

  // Exercise NewInstance for coverage.
  auto dummy = testAlg->NewInstance();
  dummy->Delete();

  return retVal;
}
}

int TestMultiOutputSimpleFilter(int argc, char* argv[])
{
  char const* tmp =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/AMR/HierarchicalBoxDataset.v1.1.vthb");
  std::string inputAMR = tmp;
  delete[] tmp;

  tmp = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/many_blocks/many_blocks.vtm");
  std::string inputMultiblock = tmp;
  delete[] tmp;

  int retVal = TestComposite(inputAMR, true);

  retVal |= TestComposite(inputMultiblock, false);

  return retVal;
}
