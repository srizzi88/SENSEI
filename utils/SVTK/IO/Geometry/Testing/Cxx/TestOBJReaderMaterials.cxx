/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOBJReaderMaterials.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Verifies that svtkOBJReader does something sensible w/rt materials.
// .SECTION Description
//

#include "svtkOBJReader.h"

#include "svtkCellData.h"
#include "svtkStringArray.h"
#include "svtkTestUtilities.h"

int TestOBJReaderMaterials(int argc, char* argv[])
{
  int retVal = 0;

  // Create the reader.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/obj_with_materials.obj");
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
  svtkStringArray* mna =
    svtkStringArray::SafeDownCast(data->GetFieldData()->GetAbstractArray("MaterialNames"));
  if (!mna)
  {
    std::cerr << "missing material names array" << std::endl;
    return 1;
  }
  svtkIntArray* mia =
    svtkIntArray::SafeDownCast(data->GetCellData()->GetAbstractArray("MaterialIds"));
  if (!mia)
  {
    std::cerr << "missing material id array" << std::endl;
    return 1;
  }
  if (data->GetNumberOfCells() != 2)
  {
    std::cerr << "wrong number of cells" << std::endl;
    return 1;
  }
  int matid = mia->GetVariantValue(1).ToInt();
  std::string matname = mna->GetVariantValue(matid).ToString();
  if (matname != "Air")
  {
    std::cerr << "wrong material for" << std::endl;
    return 1;
  }

  return retVal;
}
