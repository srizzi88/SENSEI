/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataObjectIO.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericDataObjectReader.h"
#include "svtkGenericDataObjectWriter.h"
#include "svtkImageData.h"
#include "svtkImageNoiseSource.h"
void InitializeData(svtkImageData* Data)
{
  svtkImageNoiseSource* const source = svtkImageNoiseSource::New();
  source->SetWholeExtent(0, 15, 0, 15, 0, 0);
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

bool CompareData(svtkImageData* Output, svtkImageData* Input)
{
  if (memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  const int point_count =
    Input->GetDimensions()[0] * Input->GetDimensions()[1] * Input->GetDimensions()[2];
  for (int point = 0; point != point_count; ++point)
  {
    if (memcmp(Input->GetPoint(point), Output->GetPoint(point), 3 * sizeof(double)))
      return false;
  }

  return true;
}

template <typename DataT>
bool TestDataObjectSerialization()
{
  DataT* const output_data = DataT::New();
  InitializeData(output_data);

  const char* const filename = output_data->GetClassName();

  svtkGenericDataObjectWriter* const writer = svtkGenericDataObjectWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename);
  writer->Write();
  writer->Delete();

  svtkGenericDataObjectReader* const reader = svtkGenericDataObjectReader::New();
  reader->SetFileName(filename);
  reader->Update();

  svtkDataObject* obj = reader->GetOutput();
  DataT* const input_data = DataT::SafeDownCast(obj);
  if (!input_data)
  {
    reader->Delete();
    output_data->Delete();
    return false;
  }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  return result;
}

int TestDataObjectIO(int /*argc*/, char* /*argv*/[])
{
  int result = 0;

  if (!TestDataObjectSerialization<svtkImageData>())
  {
    cerr << "Error: failure serializing svtkImageData" << endl;
    result = 1;
  }
  return result;
}
