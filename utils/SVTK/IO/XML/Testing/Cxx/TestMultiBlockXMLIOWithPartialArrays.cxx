#include <svtkDataArray.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkSphereSource.h>
#include <svtkTesting.h>
#include <svtkXMLMultiBlockDataReader.h>
#include <svtkXMLMultiBlockDataWriter.h>

int TestMultiBlockXMLIOWithPartialArrays(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sp;
  sp->Update();

  svtkNew<svtkPolyData> pd0;
  pd0->DeepCopy(sp->GetOutput());

  svtkNew<svtkPolyData> pd1;
  pd1->DeepCopy(sp->GetOutput());
  pd1->GetPointData()->GetArray("Normals")->SetName("NewNormals");

  svtkNew<svtkMultiBlockDataSet> outMB;
  outMB->SetBlock(0, pd0);
  outMB->SetBlock(1, pd1);

  svtkNew<svtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::ostringstream filename_stream;
  filename_stream << testing->GetTempDirectory() << "/TestMultiBlockXMLIOWithPartialArrays.vtm";

  svtkNew<svtkXMLMultiBlockDataWriter> writer;
  writer->SetFileName(filename_stream.str().c_str());
  writer->SetInputDataObject(outMB);
  writer->Write();

  svtkNew<svtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(filename_stream.str().c_str());
  reader->Update();

  auto inMB = svtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
  if (inMB->GetNumberOfBlocks() != 2 || svtkPolyData::SafeDownCast(inMB->GetBlock(0)) == nullptr ||
    svtkPolyData::SafeDownCast(inMB->GetBlock(0))->GetPointData()->GetArray("Normals") == nullptr ||
    svtkPolyData::SafeDownCast(inMB->GetBlock(0))->GetPointData()->GetArray("NewNormals") !=
      nullptr ||
    svtkPolyData::SafeDownCast(inMB->GetBlock(1)) == nullptr ||
    svtkPolyData::SafeDownCast(inMB->GetBlock(1))->GetPointData()->GetArray("Normals") != nullptr ||
    svtkPolyData::SafeDownCast(inMB->GetBlock(1))->GetPointData()->GetArray("NewNormals") == nullptr)
  {
    cerr << "ERROR: In/out data mismatched!" << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
