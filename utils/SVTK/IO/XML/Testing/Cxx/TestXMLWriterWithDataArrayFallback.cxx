/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLWriterWithDataArrayFallback.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkXMLWriter with data array dispatch fallback
// .SECTION Description
//

#include "svtkImageData.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkTestDataArray.h"
#include "svtkTestUtilities.h"
#include "svtkXMLImageDataReader.h"
#include "svtkXMLImageDataWriter.h"

#include <string>

int TestXMLWriterWithDataArrayFallback(int argc, char* argv[])
{
  char* temp_dir_c =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  std::string temp_dir = std::string(temp_dir_c);
  delete[] temp_dir_c;

  if (temp_dir.empty())
  {
    cerr << "Could not determine temporary directory." << endl;
    return EXIT_FAILURE;
  }

  std::string filename = temp_dir + "/testXMLWriterWithDataArrayFallback.vti";

  {
    svtkNew<svtkImageData> imageData;
    imageData->SetDimensions(2, 3, 1);

    svtkNew<svtkTestDataArray<svtkIntArray> > data;
    data->SetName("test_data");
    data->SetNumberOfTuples(6);
    for (svtkIdType i = 0; i < 6; i++)
    {
      data->SetValue(i, static_cast<int>(i));
    }

    imageData->GetPointData()->AddArray(data);

    svtkNew<svtkXMLImageDataWriter> writer;
    writer->SetFileName(filename.c_str());
    writer->SetInputData(imageData);
    writer->Write();
  }

  {
    svtkNew<svtkXMLImageDataReader> reader;
    reader->SetFileName(filename.c_str());
    reader->Update();

    svtkImageData* imageData = reader->GetOutput();
    svtkIntArray* data = svtkIntArray::SafeDownCast(imageData->GetPointData()->GetArray("test_data"));

    if (!data || data->GetNumberOfTuples() != 6)
    {
      cerr << "Could not read data array." << endl;
      return EXIT_FAILURE;
    }

    for (svtkIdType i = 0; i < data->GetNumberOfTuples(); i++)
    {
      if (data->GetValue(i) != i)
      {
        cerr << "Incorrect value from data array." << endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
