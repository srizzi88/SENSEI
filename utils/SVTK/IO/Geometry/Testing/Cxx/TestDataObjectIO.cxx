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
#include "svtkCellData.h"
#include "svtkCubeSource.h"
#include "svtkDataObjectWriter.h"
#include "svtkDelaunay3D.h"
#include "svtkEdgeListIterator.h"
#include "svtkGenericDataObjectReader.h"
#include "svtkGenericDataObjectWriter.h"
#include "svtkIntArray.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkTable.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVariant.h"

void InitializeData(svtkPolyData* Data)
{
  svtkCubeSource* const source = svtkCubeSource::New();
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();
}

bool CompareData(svtkPolyData* Output, svtkPolyData* Input)
{
  if (Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if (Input->GetNumberOfPolys() != Output->GetNumberOfPolys())
    return false;

  return true;
}

void InitializeData(svtkRectilinearGrid* Data)
{
  Data->SetDimensions(2, 3, 4);
}

bool CompareData(svtkRectilinearGrid* Output, svtkRectilinearGrid* Input)
{
  if (memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  return true;
}

void InitializeData(svtkStructuredGrid* Data)
{
  Data->SetDimensions(2, 3, 4);
}

bool CompareData(svtkStructuredGrid* Output, svtkStructuredGrid* Input)
{
  if (memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  return true;
}

void InitializeData(svtkTable* Data)
{
  svtkIntArray* const column1 = svtkIntArray::New();
  Data->AddColumn(column1);
  column1->Delete();
  column1->SetName("column1");

  svtkIntArray* const column2 = svtkIntArray::New();
  Data->AddColumn(column2);
  column2->Delete();
  column2->SetName("column2");

  Data->InsertNextBlankRow();
  Data->InsertNextBlankRow();
  Data->InsertNextBlankRow();

  Data->SetValue(0, 0, 1);
  Data->SetValue(0, 1, 2);
  Data->SetValue(1, 0, 3);
  Data->SetValue(1, 1, 4);
  Data->SetValue(2, 0, 5);
  Data->SetValue(2, 1, 6);
}

bool CompareData(svtkTable* Output, svtkTable* Input)
{
  if (Input->GetNumberOfColumns() != Output->GetNumberOfColumns())
    return false;
  if (Input->GetNumberOfRows() != Output->GetNumberOfRows())
    return false;

  for (int column = 0; column != Input->GetNumberOfColumns(); ++column)
  {
    for (int row = 0; row != Input->GetNumberOfRows(); ++row)
    {
      if (Input->GetValue(row, column).ToDouble() != Output->GetValue(row, column).ToDouble())
      {
        return false;
      }
    }
  }

  return true;
}

void InitializeData(svtkUnstructuredGrid* Data)
{
  svtkCubeSource* const source = svtkCubeSource::New();
  svtkDelaunay3D* const delaunay = svtkDelaunay3D::New();
  delaunay->AddInputConnection(source->GetOutputPort());
  delaunay->Update();

  Data->ShallowCopy(delaunay->GetOutput());

  delaunay->Delete();
  source->Delete();
}

bool CompareData(svtkUnstructuredGrid* Output, svtkUnstructuredGrid* Input)
{
  if (Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if (Input->GetNumberOfCells() != Output->GetNumberOfCells())
    return false;

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

  if (!TestDataObjectSerialization<svtkPolyData>())
  {
    cerr << "Error: failure serializing svtkPolyData" << endl;
    result = 1;
  }
  if (!TestDataObjectSerialization<svtkRectilinearGrid>())
  {
    cerr << "Error: failure serializing svtkRectilinearGrid" << endl;
    result = 1;
  }
  if (!TestDataObjectSerialization<svtkStructuredGrid>())
  {
    cerr << "Error: failure serializing svtkStructuredGrid" << endl;
    result = 1;
  }
  if (!TestDataObjectSerialization<svtkTable>())
  {
    cerr << "Error: failure serializing svtkTable" << endl;
    result = 1;
  }
  if (!TestDataObjectSerialization<svtkUnstructuredGrid>())
  {
    cerr << "Error: failure serializaing svtkUnstructuredGrid" << endl;
    result = 1;
  }

  return result;
}
