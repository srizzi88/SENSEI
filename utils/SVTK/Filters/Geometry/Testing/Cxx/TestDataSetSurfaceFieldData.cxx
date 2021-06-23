/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetSurfaceMultiBlockFieldData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cstdio>

#include <svtkDataSetSurfaceFilter.h>
#include <svtkFieldData.h>
#include <svtkFloatArray.h>
#include <svtkImageData.h>
#include <svtkIntArray.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkStructuredGrid.h>
#include <svtkUnstructuredGrid.h>

// Test to ensure that field data is copied for different data types in svtkDataSetSurface

namespace
{

//----------------------------------------------------------------------------
int TestDataSet(svtkDataSet* ds, int expectedValue)
{
  svtkNew<svtkDataSetSurfaceFilter> surfacer;
  surfacer->SetInputData(ds);
  surfacer->Update();
  if (!surfacer->GetOutput())
  {
    std::cout << "No output!\n";
    return EXIT_FAILURE;
  }

  svtkFieldData* fieldData = surfacer->GetOutput()->GetFieldData();
  const char* className = ds->GetClassName();
  if (fieldData == nullptr || fieldData->GetNumberOfArrays() == 0)
  {
    std::cerr << "No field data was associated with data set type " << className << "\n";
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Have field data for surface from data set type " << className << "\n";

    svtkIntArray* array = svtkArrayDownCast<svtkIntArray>(fieldData->GetArray(0));
    if (!array)
    {
      std::cerr << "Field data array was not of type svtkIntArray for data set type" << className
                << "\n";
      return EXIT_FAILURE;
    }
    else if (array->GetNumberOfTuples() < 1)
    {
      std::cerr << "No tuples in field data array for surface from data set type " << className
                << "\n";
      return EXIT_FAILURE;
    }
    else
    {
      int value = 0;
      array->GetTypedTuple(0, &value);

      std::cout << "Block value " << value << "\n";
      if (value != expectedValue)
      {
        std::cerr << "Unexpected block field array value " << value
                  << " for surface from data set type " << className << ". Expected "
                  << expectedValue << "\n";
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
void AddFieldData(svtkDataSet* ds, int id)
{
  svtkNew<svtkIntArray> array;
  array->SetName("ID");
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(1);
  array->SetTypedTuple(0, &id);

  ds->GetFieldData()->AddArray(array);
}

//----------------------------------------------------------------------------
int TestImageData()
{
  // Create image data
  svtkNew<svtkImageData> imageData;
  imageData->Initialize();
  imageData->SetSpacing(1, 1, 1);
  imageData->SetOrigin(0, 0, 0);
  imageData->SetDimensions(10, 10, 10);

  int id = 1;
  AddFieldData(imageData, id);

  // Add point data
  svtkNew<svtkFloatArray> pa;
  pa->SetName("pd");
  pa->SetNumberOfComponents(1);
  pa->SetNumberOfTuples(10 * 10 * 10);

  imageData->GetPointData()->AddArray(pa);

  return TestDataSet(imageData, id);
}

//----------------------------------------------------------------------------
int TestPolyData()
{
  // Create polydata
  svtkNew<svtkPolyData> polyData;
  polyData->Initialize();

  int id = 2;
  AddFieldData(polyData, id);

  return TestDataSet(polyData, id);
}

//----------------------------------------------------------------------------
int TestStructuredGrid()
{
  // Create structured grid data
  svtkNew<svtkStructuredGrid> structuredGrid;
  structuredGrid->Initialize();

  int id = 3;
  AddFieldData(structuredGrid, id);

  return TestDataSet(structuredGrid, id);
}

//----------------------------------------------------------------------------
int TestUnstructuredGrid()
{
  // Create unstructured grid data
  svtkNew<svtkUnstructuredGrid> unstructuredGrid;
  unstructuredGrid->Initialize();

  int id = 4;
  AddFieldData(unstructuredGrid, id);

  return TestDataSet(unstructuredGrid, id);
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int TestDataSetSurfaceFieldData(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  if (TestImageData() != EXIT_SUCCESS)
  {
    std::cerr << "TestImageData failed\n";
    return EXIT_FAILURE;
  }

  if (TestPolyData() != EXIT_SUCCESS)
  {
    std::cerr << "TestPolyData failed\n";
    return EXIT_FAILURE;
  }

  if (TestStructuredGrid() != EXIT_SUCCESS)
  {
    std::cerr << "TestStructuredGrid failed\n";
    return EXIT_FAILURE;
  }

  if (TestUnstructuredGrid() != EXIT_SUCCESS)
  {
    std::cerr << "TestUnstructuredGrid failed\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
