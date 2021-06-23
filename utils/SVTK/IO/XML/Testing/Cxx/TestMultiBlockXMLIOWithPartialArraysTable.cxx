#include <svtkDataSetAttributes.h>
#include <svtkFloatArray.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkNew.h>
#include <svtkTable.h>
#include <svtkTesting.h>
#include <svtkXMLMultiBlockDataReader.h>
#include <svtkXMLMultiBlockDataWriter.h>

int TestMultiBlockXMLIOWithPartialArraysTable(int argc, char* argv[])
{
  // Create a table with some points in it...
  svtkNew<svtkTable> table;
  svtkNew<svtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  svtkNew<svtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  svtkNew<svtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  int numPoints = 69;
  float inc = 7.5 / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
  }

  svtkNew<svtkTable> table1;
  table1->DeepCopy(table);
  table1->GetRowData()->GetArray("Sine")->SetName("NewSine");

  svtkNew<svtkMultiBlockDataSet> outMB;
  outMB->SetBlock(0, table);
  outMB->SetBlock(1, table1);

  svtkNew<svtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::ostringstream filename_stream;
  filename_stream << testing->GetTempDirectory()
                  << "/TestMultiBlockXMLIOWithPartialArraysTable.vtm";

  svtkNew<svtkXMLMultiBlockDataWriter> writer;
  writer->SetFileName(filename_stream.str().c_str());
  writer->SetInputDataObject(outMB);
  writer->Write();

  svtkNew<svtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(filename_stream.str().c_str());
  reader->Update();

  auto inMB = svtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0));
  if (inMB->GetNumberOfBlocks() != 2 || svtkTable::SafeDownCast(inMB->GetBlock(0)) == nullptr ||
    svtkTable::SafeDownCast(inMB->GetBlock(0))->GetRowData()->GetArray("Sine") == nullptr ||
    svtkTable::SafeDownCast(inMB->GetBlock(0))->GetRowData()->GetArray("NewSine") != nullptr ||
    svtkTable::SafeDownCast(inMB->GetBlock(1)) == nullptr ||
    svtkTable::SafeDownCast(inMB->GetBlock(1))->GetRowData()->GetArray("Sine") != nullptr ||
    svtkTable::SafeDownCast(inMB->GetBlock(1))->GetRowData()->GetArray("NewSine") == nullptr)
  {
    cerr << "ERROR: In/out data mismatched!" << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
