/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkPLYWriterString
// .SECTION Description
// Tests reading and writing to std::string

#include "svtkPLYWriter.h"

#include "svtkFloatArray.h"
#include "svtkPLYReader.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkTestUtilities.h"
#include "svtksys/FStream.hxx"

#include <cmath>
#include <fstream>
#include <limits>
#include <streambuf>

int TestPLYWriterString(int argc, char* argv[])
{
  // Read file name.
  const char* tempFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/squareTextured.ply");
  std::string filename = tempFileName;
  delete[] tempFileName;

  svtksys::ifstream ifs;

  ifs.open(filename.c_str(), std::ios::in | std::ios::binary);
  if (!ifs.is_open())
  {
    std::cout << "Can not read the input file." << std::endl;
    return EXIT_FAILURE;
  }
  std::string inputString{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };

  // Create the reader.
  svtkNew<svtkPLYReader> reader;
  reader->ReadFromInputStringOn();
  reader->SetInputString(inputString);
  reader->Update();

  // Data to compare.
  svtkSmartPointer<svtkPolyData> data = svtkSmartPointer<svtkPolyData>::New();
  data->DeepCopy(reader->GetOutput());

  int options[3][2] = { { SVTK_ASCII, 0 }, { SVTK_BINARY, SVTK_BIG_ENDIAN },
    { SVTK_BINARY, SVTK_LITTLE_ENDIAN } };

  for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i)
  {
    int* option = options[i];
    // Create the writer.
    svtkNew<svtkPLYWriter> writer;
    writer->WriteToOutputStringOn();
    writer->SetFileType(option[0]);
    writer->SetDataByteOrder(option[1]);
    writer->SetTextureCoordinatesNameToTextureUV();
    writer->SetInputConnection(reader->GetOutputPort());
    writer->AddComment("TextureFile svtk.png");
    writer->Write();
    // read the written output string
    reader->SetInputString(writer->GetOutputString());
    reader->Update();

    svtkPolyData* newData = reader->GetOutput();

    const svtkIdType nbrPoints = newData->GetNumberOfPoints();
    if (nbrPoints != data->GetNumberOfPoints())
    {
      std::cout << "Different number of points." << std::endl;
      return EXIT_FAILURE;
    }

    svtkDataArray* tCoords = newData->GetPointData()->GetTCoords();
    if (!tCoords || !data->GetPointData()->GetTCoords())
    {
      std::cout << "Texture coordinates are not present." << std::endl;
      return EXIT_FAILURE;
    }

    const svtkIdType nbrCoords = tCoords->GetNumberOfTuples() * tCoords->GetNumberOfComponents();
    if (nbrCoords != (2 * nbrPoints))
    {
      std::cout << "Number of texture coordinates is not coherent." << std::endl;
      return EXIT_FAILURE;
    }

    svtkFloatArray* inputArray = svtkArrayDownCast<svtkFloatArray>(data->GetPointData()->GetTCoords());
    svtkFloatArray* outputArray = svtkArrayDownCast<svtkFloatArray>(tCoords);
    if (!inputArray || !outputArray)
    {
      std::cout << "Texture coordinates are not of float type." << std::endl;
      return EXIT_FAILURE;
    }

    float* input = inputArray->GetPointer(0);
    float* output = outputArray->GetPointer(0);
    for (svtkIdType id = 0; id < nbrCoords; ++id)
    {
      if (std::abs(*input++ - *output++) > std::numeric_limits<float>::epsilon())
      {
        std::cout << "Texture coordinates are not identical." << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}
