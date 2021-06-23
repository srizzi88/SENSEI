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
// .NAME Test of svtkPLYWriter
// .SECTION Description
//

#include "svtkPLYWriter.h"

#include "svtkFloatArray.h"
#include "svtkPLYReader.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkTestUtilities.h"

#include <cmath>
#include <limits>

int TestPLYWriter(int argc, char* argv[])
{
  // Test temporary directory
  char* tempDir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string outputfile = testDirectory + std::string("/") + std::string("tmp.ply");

  // Read file name.
  const char* inputfile =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/squareTextured.ply");

  // Test if the reader thinks it can open the input file.
  if (0 == svtkPLYReader::CanReadFile(inputfile))
  {
    std::cout << "The PLY reader can not read the input file." << std::endl;
    return EXIT_FAILURE;
  }

  // Create the reader.
  svtkSmartPointer<svtkPLYReader> reader = svtkSmartPointer<svtkPLYReader>::New();
  reader->SetFileName(inputfile);
  reader->Update();
  delete[] inputfile;

  // Data to compare.
  svtkSmartPointer<svtkPolyData> data = svtkSmartPointer<svtkPolyData>::New();
  data->DeepCopy(reader->GetOutput());

  // Create the writer.
  svtkSmartPointer<svtkPLYWriter> writer = svtkSmartPointer<svtkPLYWriter>::New();
  writer->SetFileName(outputfile.c_str());
  writer->SetFileTypeToASCII();
  writer->SetTextureCoordinatesNameToTextureUV();
  writer->SetInputConnection(reader->GetOutputPort());
  writer->AddComment("TextureFile svtk.png");
  writer->Write();

  // Test if the reader thinks it can open the written file.
  if (0 == svtkPLYReader::CanReadFile(outputfile.c_str()))
  {
    std::cout << "The PLY reader can not read the written file." << std::endl;
    return EXIT_FAILURE;
  }
  reader->SetFileName(outputfile.c_str());
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

  return EXIT_SUCCESS;
}
