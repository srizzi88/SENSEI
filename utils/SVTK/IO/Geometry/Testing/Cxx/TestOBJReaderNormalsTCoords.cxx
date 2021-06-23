/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOBJReaderNormalsTCoords.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkOBJReader
// .SECTION Description
//

#include "svtkDebugLeaks.h"
#include "svtkOBJReader.h"

#include "svtkCellArray.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"

int TestOBJReaderNormalsTCoords(int argc, char* argv[])
{
  int retVal = 0;

  // Create the reader.
  char* fname =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/obj_with_normals_and_tcoords.obj");
  svtkSmartPointer<svtkOBJReader> reader = svtkSmartPointer<svtkOBJReader>::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkPolyData* data = reader->GetOutput();

  if (!data)
  {
    std::cerr << "Could not read data" << std::endl;
    return 1;
  }
  if (data->GetNumberOfPoints() != 4)
  {
    std::cerr << "Invalid number of points" << std::endl;
    return 1;
  }
  if (data->GetPointData()->GetNumberOfArrays() != 2)
  {
    std::cerr << "Invalid number of arrays" << std::endl;
    return 1;
  }
  if (!data->GetPointData()->HasArray("TCoords"))
  {
    std::cerr << "Could not find TCoords array" << std::endl;
    return 1;
  }
  if (!data->GetPointData()->HasArray("Normals"))
  {
    std::cerr << "Could not find TCoords array" << std::endl;
    return 1;
  }

  return retVal;
}
