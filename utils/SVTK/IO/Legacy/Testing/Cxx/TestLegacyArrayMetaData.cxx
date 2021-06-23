/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegacyArrayMetaData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Roundtrip test for array metadata in legacy readers.

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkInformationKey.h"
#include "svtkNew.h"
#include "svtkPoints.h"
#include "svtkTesting.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridReader.h"
#include "svtkUnstructuredGridWriter.h"

// Serializable keys to test:
#include "svtkInformationDoubleKey.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIdTypeKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationStringKey.h"
#include "svtkInformationStringVectorKey.h"
#include "svtkInformationUnsignedLongKey.h"

#include <svtksys/SystemTools.hxx>

namespace
{

static svtkInformationDoubleKey* TestDoubleKey =
  svtkInformationDoubleKey::MakeKey("Double", "TestKey");
// Test restricted keys with this one -- must be a vector of length 3, can NOT
// be constructed using Append():
static svtkInformationDoubleVectorKey* TestDoubleVectorKey =
  svtkInformationDoubleVectorKey::MakeKey("DoubleVector", "TestKey", 3);
static svtkInformationIdTypeKey* TestIdTypeKey =
  svtkInformationIdTypeKey::MakeKey("IdType", "TestKey");
static svtkInformationIntegerKey* TestIntegerKey =
  svtkInformationIntegerKey::MakeKey("Integer", "TestKey");
static svtkInformationIntegerVectorKey* TestIntegerVectorKey =
  svtkInformationIntegerVectorKey::MakeKey("IntegerVector", "TestKey");
static svtkInformationStringKey* TestStringKey =
  svtkInformationStringKey::MakeKey("String", "TestKey");
static svtkInformationStringVectorKey* TestStringVectorKey =
  svtkInformationStringVectorKey::MakeKey("StringVector", "TestKey");
static svtkInformationUnsignedLongKey* TestUnsignedLongKey =
  svtkInformationUnsignedLongKey::MakeKey("UnsignedLong", "TestKey");

bool stringEqual(const std::string& expect, const std::string& actual)
{
  if (expect != actual)
  {
    std::cerr << "Strings do not match! Expected: '" << expect << "', got: '" << actual << "'.\n";
    return false;
  }
  return true;
}

bool stringEqual(const std::string& expect, const char* actual)
{
  return stringEqual(expect, std::string(actual ? actual : ""));
}

template <typename T>
bool compareValues(const std::string& desc, T expect, T actual)
{
  if (expect != actual)
  {
    std::cerr << "Failed comparison for '" << desc << "'. Expected '" << expect << "', got '"
              << actual << "'.\n";
    return false;
  }
  return true;
}

bool verify(svtkUnstructuredGrid* grid)
{
  svtkDataArray* array = grid->GetPoints()->GetData();
  svtkInformation* info = array->GetInformation();
  if (!info)
  {
    std::cerr << "Missing information object!\n";
    return false;
  }

  if (!stringEqual("X coordinates", array->GetComponentName(0)) ||
    !stringEqual("Y coordinates", array->GetComponentName(1)) ||
    !stringEqual("Z coordinates", array->GetComponentName(2)))

  {
    return false;
  }

  if (!compareValues("double key", 1., info->Get(TestDoubleKey)) ||
    !compareValues("double vector key length", 3, info->Length(TestDoubleVectorKey)) ||
    !compareValues("double vector key @0", 1., info->Get(TestDoubleVectorKey, 0)) ||
    !compareValues("double vector key @1", 90., info->Get(TestDoubleVectorKey, 1)) ||
    !compareValues("double vector key @2", 260., info->Get(TestDoubleVectorKey, 2)) ||
    !compareValues<svtkIdType>("idtype key", 5, info->Get(TestIdTypeKey)) ||
    !compareValues("integer key", 408, info->Get(TestIntegerKey)) ||
    !compareValues("integer vector key length", 3, info->Length(TestIntegerVectorKey)) ||
    !compareValues("integer vector key @0", 1, info->Get(TestIntegerVectorKey, 0)) ||
    !compareValues("integer vector key @1", 5, info->Get(TestIntegerVectorKey, 1)) ||
    !compareValues("integer vector key @2", 45, info->Get(TestIntegerVectorKey, 2)) ||
    !stringEqual("Test String!\nLine2", info->Get(TestStringKey)) ||
    !compareValues("string vector key length", 3, info->Length(TestStringVectorKey)) ||
    !stringEqual("First", info->Get(TestStringVectorKey, 0)) ||
    !stringEqual("Second (with whitespace!)", info->Get(TestStringVectorKey, 1)) ||
    !stringEqual("Third (with\nnewline!)", info->Get(TestStringVectorKey, 2)) ||
    !compareValues("unsigned long key", 9ul, info->Get(TestUnsignedLongKey)))
  {
    return false;
  }

  array = grid->GetCellData()->GetArray("svtkGhostType");
  info = array->GetInformation();
  if (!info)
  {
    std::cerr << "Missing information object!\n";
    return false;
  }
  if (!stringEqual("Ghost level information", array->GetComponentName(0)) ||
    !stringEqual("N/A", info->Get(svtkDataArray::UNITS_LABEL())))
  {
    return false;
  }

  return true;
}
} // end anon namespace

int TestLegacyArrayMetaData(int argc, char* argv[])
{
  // Load the initial dataset:
  svtkNew<svtkTesting> testing;
  testing->AddArguments(argc, argv);

  std::string filename = testing->GetDataRoot();
  filename += "/Data/ghost_cells.svtk";

  svtkNew<svtkUnstructuredGridReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();
  svtkUnstructuredGrid* grid = reader->GetOutput();

  // Set component names on points:
  svtkDataArray* array = grid->GetPoints()->GetData();
  array->SetComponentName(0, "X coordinates");
  array->SetComponentName(1, "Y coordinates");
  array->SetComponentName(2, "Z coordinates");

  // Test information keys that can be serialized
  svtkInformation* info = array->GetInformation();
  info->Set(TestDoubleKey, 1.0);
  // Set the double vector using an array / Set instead of appending. Appending
  // doesn't work when RequiredLength is set.
  double doubleVecData[3] = { 1., 90., 260. };
  info->Set(TestDoubleVectorKey, doubleVecData, 3);
  info->Set(TestIdTypeKey, 5);
  info->Set(TestIntegerKey, 408);
  info->Append(TestIntegerVectorKey, 1);
  info->Append(TestIntegerVectorKey, 5);
  info->Append(TestIntegerVectorKey, 45);
  info->Set(TestStringKey, "Test String!\nLine2");
  info->Append(TestStringVectorKey, "First");
  info->Append(TestStringVectorKey, "Second (with whitespace!)");
  info->Append(TestStringVectorKey, "Third (with\nnewline!)");
  info->Set(TestUnsignedLongKey, 9);

  // And on the svtkGhostLevels array:
  array = grid->GetCellData()->GetArray("svtkGhostType");
  info = array->GetInformation();
  info->Set(svtkDataArray::UNITS_LABEL(), "N/A");
  array->SetComponentName(0, "Ghost level information");

  // Check that the input grid passes our test:
  if (!verify(grid))
  {
    std::cerr << "Sanity check failed.\n";
    return EXIT_FAILURE;
  }

  // Now roundtrip the dataset through the readers/writers:
  svtkNew<svtkUnstructuredGridWriter> testWriter;
  svtkNew<svtkUnstructuredGridReader> testReader;

  testWriter->SetInputData(grid);
  testWriter->WriteToOutputStringOn();
  testReader->ReadFromInputStringOn();

  // Test ASCII mode: (string).
  testWriter->SetFileTypeToASCII();
  if (!testWriter->Write())
  {
    std::cerr << "Write failed!" << std::endl;
    return EXIT_FAILURE;
  }

  testReader->SetInputString(testWriter->GetOutputStdString());
  testReader->Update();
  grid = testReader->GetOutput();

  if (!verify(grid))
  {
    std::cerr << "ASCII mode test failed.\n"
              << "Error while parsing:\n"
              << testWriter->GetOutputStdString() << "\n";
    return EXIT_FAILURE;
  }

  // Test binary mode: (string)
  testWriter->SetFileTypeToBinary();
  if (!testWriter->Write())
  {
    std::cerr << "Write failed!" << std::endl;
    return EXIT_FAILURE;
  }

  testReader->SetInputString(testWriter->GetOutputStdString());
  testReader->Update();
  grid = testReader->GetOutput();

  if (!verify(grid))
  {
    std::cerr << "Binary mode test failed.\n"
              << "Error while parsing:\n"
              << testWriter->GetOutputStdString() << "\n";
    return EXIT_FAILURE;
  }

  if (!testing->GetTempDirectory())
  {
    std::cout << "No temporary directory specified. Skipping testing read/write from files."
              << std::endl;
    return EXIT_SUCCESS;
  }

  const std::string temp = testing->GetTempDirectory();
  const std::string tfilename = temp + "/TestLegacyArrayMetaData.svtk";

  testWriter->WriteToOutputStringOff();
  testWriter->SetFileName(tfilename.c_str());
  testReader->ReadFromInputStringOff();
  testReader->SetFileName(tfilename.c_str());

  svtksys::SystemTools::RemoveFile(tfilename);
  // Test ASCII mode: (file).
  testWriter->SetFileTypeToASCII();
  if (!testWriter->Write())
  {
    std::cerr << "Write to file (ASCII) failed!" << std::endl;
    return EXIT_FAILURE;
  }

  testReader->Update();
  grid = testReader->GetOutput();
  if (!verify(grid))
  {
    std::cerr << "ASCII mode test (file i/o) failed.\n";
    return EXIT_FAILURE;
  }

  // Test binary mode: (file)
  svtksys::SystemTools::RemoveFile(tfilename);
  testWriter->SetFileTypeToBinary();
  if (!testWriter->Write())
  {
    std::cerr << "Write to file (binary) failed!" << std::endl;
    return EXIT_FAILURE;
  }

  testReader->Update();
  grid = testReader->GetOutput();

  if (!verify(grid))
  {
    std::cerr << "Binary mode test (file i/o) failed.\n";
    return EXIT_FAILURE;
  }

  svtksys::SystemTools::RemoveFile(tfilename);
  return EXIT_SUCCESS;
}
