/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOBJReaderMultiTexture.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDebugLeaks.h"
#include "svtkOBJReader.h"

#include "svtkCellArray.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkTestUtilities.h"

//------------------------------------------------------------------------------
int TestOBJReaderSingleTexture(int argc, char* argv[])
{
  // Create the reader.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/obj_singletexture.obj");

  svtkNew<svtkOBJReader> reader;
  reader->SetFileName(fname);
  reader->Update();

  svtkPolyData* data = reader->GetOutput();

  delete[] fname;

  if (!data)
  {
    std::cerr << "Could not read data" << std::endl;
    return EXIT_FAILURE;
  }

  // The OBJ file has 4 points and 2 cells.
  if (data->GetNumberOfPoints() != 4 && data->GetNumberOfCells() == 2)
  {
    std::cerr << "Invalid number of points or cells" << std::endl;
    return EXIT_FAILURE;
  }

  svtkDataArray* tCoords = data->GetPointData()->GetTCoords();

  if (!tCoords)
  {
    std::cerr << "Could not find texture coordinates array" << std::endl;
    return EXIT_FAILURE;
  }

  if (strcmp(tCoords->GetName(), "Material0"))
  {
    std::cerr << "Invalid texture coordinates array name" << std::endl;
    return EXIT_FAILURE;
  }

  // Check the values
  for (int i = 0; i < 4; ++i)
  {
    double* currentTCoord = tCoords->GetTuple2(i);
    if (i == 2)
    {
      if (currentTCoord[0] != 1.0 && currentTCoord[1] != 1.0)
      {
        std::cerr << "Unexpected texture values" << std::endl;
        return EXIT_FAILURE;
      }
    }
    if (currentTCoord[0] < 0. || currentTCoord[1] > 1.0)
    {
      std::cerr << "Unexpected texture values" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
