/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLegacyCompositeDataReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAMRGaussianPulseSource.h"
#include "svtkGenericDataObjectReader.h"
#include "svtkGenericDataObjectWriter.h"
#include "svtkNew.h"
#include "svtkOverlappingAMR.h"
#include "svtkTesting.h"

#define TEST_SUCCESS 0
#define TEST_FAILED 1

#define svtk_assert(x)                                                                              \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "ERROR: Condition FAILED!! : " << #x << endl;                                          \
    return TEST_FAILED;                                                                            \
  }

int Validate(svtkOverlappingAMR* input, svtkOverlappingAMR* result)
{
  svtk_assert(input->GetNumberOfLevels() == result->GetNumberOfLevels());
  svtk_assert(input->GetOrigin()[0] == result->GetOrigin()[0]);
  svtk_assert(input->GetOrigin()[1] == result->GetOrigin()[1]);
  svtk_assert(input->GetOrigin()[2] == result->GetOrigin()[2]);

  for (unsigned int level = 0; level < input->GetNumberOfLevels(); level++)
  {
    svtk_assert(input->GetNumberOfDataSets(level) == result->GetNumberOfDataSets(level));
  }

  cout << "Audit Input" << endl;
  input->Audit();
  cout << "Audit Output" << endl;
  result->Audit();
  return TEST_SUCCESS;
}

int TestLegacyCompositeDataReaderWriter(int argc, char* argv[])
{
  svtkNew<svtkTesting> testing;
  testing->AddArguments(argc, argv);

  svtkNew<svtkAMRGaussianPulseSource> source;

  std::string filename = testing->GetTempDirectory();
  filename += "/amr_data.svtk";
  svtkNew<svtkGenericDataObjectWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetFileTypeToASCII();
  writer->SetInputConnection(source->GetOutputPort());
  writer->Write();

  svtkNew<svtkGenericDataObjectReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  // now valid the input and output datasets.
  svtkOverlappingAMR* input = svtkOverlappingAMR::SafeDownCast(source->GetOutputDataObject(0));
  svtkOverlappingAMR* result = svtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  if (Validate(input, result) == TEST_FAILED)
  {
    return TEST_FAILED;
  }

  cout << "Test Binary IO" << endl;

  writer->SetFileTypeToBinary();
  writer->Write();

  reader->SetFileName(nullptr);
  reader->SetFileName(filename.c_str());
  reader->Update();
  return Validate(input, svtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}
