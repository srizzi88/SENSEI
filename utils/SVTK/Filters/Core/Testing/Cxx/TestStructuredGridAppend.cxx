/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStructuredGridAppend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include <svtkCellData.h>
#include <svtkDataSetAttributes.h>
#include <svtkDoubleArray.h>
#include <svtkIdList.h>
#include <svtkIntArray.h>
#include <svtkMath.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkSmartPointer.h>
#include <svtkStructuredGrid.h>
#include <svtkStructuredGridAppend.h>

namespace
{
const char arrayName[] = "coordinates";

//////////////////////////////////////////////////////////////////////////////
// Create a dataset for testing
//////////////////////////////////////////////////////////////////////////////
void CreateDataset(svtkStructuredGrid* dataset, int extent[6])
{
  dataset->SetExtent(extent);
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  // create a point data array
  svtkNew<svtkDoubleArray> pointArray;
  pointArray->SetName(arrayName);
  dataset->GetPointData()->AddArray(pointArray);
  for (int k = extent[4]; k <= extent[5]; k++)
  {
    for (int j = extent[2]; j <= extent[3]; j++)
    {
      for (int i = extent[0]; i <= extent[1]; i++)
      {
        points->InsertNextPoint((double)i, (double)j, (double)k);
        pointArray->InsertNextValue((double)i);
      }
    }
  }
  dataset->SetPoints(points);

  // create a cell data array
  svtkNew<svtkIntArray> cellArray;
  cellArray->SetName(arrayName);
  cellArray->SetNumberOfComponents(3);
  dataset->GetCellData()->AddArray(cellArray);
  int ijk[3];
  for (int k = extent[4]; k < extent[5]; k++)
  {
    ijk[2] = k;
    for (int j = extent[2]; j < extent[3]; j++)
    {
      ijk[1] = j;
      for (int i = extent[0]; i < extent[1]; i++)
      {
        ijk[0] = i;
        cellArray->InsertNextTypedTuple(ijk);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// Returns 1 on success, 0 otherwise
//////////////////////////////////////////////////////////////////////////////
int AppendDatasetsAndCheck(
  const std::vector<svtkSmartPointer<svtkStructuredGrid> >& inputs, int outputExtent[6])
{
  svtkNew<svtkStructuredGridAppend> append;
  for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex)
  {
    append->AddInputData(inputs[inputIndex]);
  }
  append->Update();

  svtkStructuredGrid* output = append->GetOutput();
  int extent[6];
  output->GetExtent(extent);
  for (int i = 0; i < 6; i++)
  {
    if (extent[i] != outputExtent[i])
    {
      svtkGenericWarningMacro("ERROR: Extent is wrong.");
      return 0;
    }
  }

  if (svtkDoubleArray* pointArray =
        svtkArrayDownCast<svtkDoubleArray>(output->GetPointData()->GetArray(arrayName)))
  {
    svtkIdType counter = 0;
    for (int k = extent[4]; k <= extent[5]; k++)
    {
      for (int j = extent[2]; j <= extent[3]; j++)
      {
        for (int i = extent[0]; i <= extent[1]; i++)
        {
          double value = pointArray->GetValue(counter);
          if (value != (double)i)
          {
            svtkGenericWarningMacro(
              "ERROR: Bad point array value " << value << " which should be " << (double)i);
            return 0;
          }
          counter++;
        }
      }
    }
  }
  else
  {
    svtkGenericWarningMacro("ERROR: Could not find point data array.");
    return 0;
  }

  if (svtkIntArray* cellArray =
        svtkArrayDownCast<svtkIntArray>(output->GetCellData()->GetArray(arrayName)))
  {
    svtkIdType counter = 0;
    for (int k = extent[4]; k < extent[5]; k++)
    {
      for (int j = extent[2]; j < extent[3]; j++)
      {
        for (int i = extent[0]; i < extent[1]; i++)
        {
          int values[3];
          cellArray->GetTypedTuple(counter, values);
          if (values[0] != i || values[1] != j || values[2] != k)
          {
            svtkGenericWarningMacro("ERROR: Bad cell array tuple value ["
              << values[0] << ", " << values[1] << ", " << values[2] << "] which should be [" << i
              << ", " << j << ", " << k << "]");
            return 0;
          }
          counter++;
        }
      }
    }
  }
  else
  {
    svtkGenericWarningMacro("ERROR: Could not find cell data array.");
    return 0;
  }

  return 1;
}

} // end anonymous namespace

//////////////////////////////////////////////////////////////////////////////
int TestStructuredGridAppend(int, char*[])
{
  std::vector<svtkSmartPointer<svtkStructuredGrid> > inputs;
  int outputExtent[6] = { -1, 19, 0, 4, 0, 5 };
  for (int i = 0; i < 3; i++)
  {
    int extent[6] = { i * 6 - 1, (i + 1) * 6 + 1, 0, 4, 0, 5 };
    svtkSmartPointer<svtkStructuredGrid> dataset = svtkSmartPointer<svtkStructuredGrid>::New();
    CreateDataset(dataset, extent);
    inputs.push_back(dataset);
  }

  if (!AppendDatasetsAndCheck(inputs, outputExtent))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
