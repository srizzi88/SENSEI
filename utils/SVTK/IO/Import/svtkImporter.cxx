/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImporter.h"
#include "svtkAbstractArray.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRenderWindow.h"
#include "svtkRendererCollection.h"

#include <sstream>

svtkCxxSetObjectMacro(svtkImporter, RenderWindow, svtkRenderWindow);

svtkImporter::svtkImporter()
{
  this->Renderer = nullptr;
  this->RenderWindow = nullptr;
}

svtkImporter::~svtkImporter()
{
  this->SetRenderWindow(nullptr);

  if (this->Renderer)
  {
    this->Renderer->UnRegister(nullptr);
    this->Renderer = nullptr;
  }
}

void svtkImporter::ReadData()
{
  // this->Import actors, cameras, lights and properties
  this->ImportActors(this->Renderer);
  this->ImportCameras(this->Renderer);
  this->ImportLights(this->Renderer);
  this->ImportProperties(this->Renderer);
}

void svtkImporter::Read()
{
  svtkRenderer* renderer;

  // if there is no render window, create one
  if (this->RenderWindow == nullptr)
  {
    svtkDebugMacro(<< "Creating a RenderWindow\n");
    this->RenderWindow = svtkRenderWindow::New();
  }

  // Get the first renderer in the render window
  renderer = this->RenderWindow->GetRenderers()->GetFirstRenderer();
  if (renderer == nullptr)
  {
    svtkDebugMacro(<< "Creating a Renderer\n");
    this->Renderer = svtkRenderer::New();
    renderer = this->Renderer;
    this->RenderWindow->AddRenderer(renderer);
  }
  else
  {
    if (this->Renderer)
    {
      this->Renderer->UnRegister(nullptr);
    }
    this->Renderer = renderer;
    this->Renderer->Register(this);
  }

  if (this->ImportBegin())
  {
    this->ReadData();
    this->ImportEnd();
  }
}

void svtkImporter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Render Window: ";
  if (this->RenderWindow)
  {
    os << this->RenderWindow << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << this->Renderer << "\n";
  }
  else
  {
    os << "(none)\n";
  }
}

//----------------------------------------------------------------------------
std::string svtkImporter::GetArrayDescription(svtkAbstractArray* array, svtkIndent indent)
{
  std::stringstream ss;
  ss << indent;
  if (array->GetName())
  {
    ss << array->GetName() << " : ";
  }
  ss << array->GetDataTypeAsString() << " : ";

  svtkIdType nbTuples = array->GetNumberOfTuples();

  if (nbTuples == 1)
  {
    ss << array->GetVariantValue(0).ToString();
  }
  else
  {
    int nComp = array->GetNumberOfComponents();
    double range[2];
    for (int j = 0; j < nComp; j++)
    {
      svtkDataArray* dataArray = svtkDataArray::SafeDownCast(array);
      if (dataArray)
      {
        dataArray->GetRange(range, j);
        ss << "[" << range[0] << ", " << range[1] << "] ";
      }
      else
      {
        ss << "[range unavailable] ";
      }
    }
  }
  ss << "\n";
  return ss.str();
}

//----------------------------------------------------------------------------
std::string svtkImporter::GetDataSetDescription(svtkDataSet* ds, svtkIndent indent)
{
  std::stringstream ss;
  ss << indent << "Number of points: " << ds->GetNumberOfPoints() << "\n";

  svtkPolyData* pd = svtkPolyData::SafeDownCast(ds);
  if (pd)
  {
    ss << indent << "Number of polygons: " << pd->GetNumberOfPolys() << "\n";
    ss << indent << "Number of lines: " << pd->GetNumberOfLines() << "\n";
    ss << indent << "Number of vertices: " << pd->GetNumberOfVerts() << "\n";
  }
  else
  {
    ss << indent << "Number of cells: " << ds->GetNumberOfCells() << "\n";
  }

  svtkPointData* pointData = ds->GetPointData();
  svtkCellData* cellData = ds->GetCellData();
  svtkFieldData* fieldData = ds->GetFieldData();
  int nbPointData = pointData->GetNumberOfArrays();
  int nbCellData = cellData->GetNumberOfArrays();
  int nbFieldData = fieldData->GetNumberOfArrays();

  ss << indent << nbPointData << " point data array(s):\n";
  for (svtkIdType i = 0; i < nbPointData; i++)
  {
    svtkAbstractArray* array = pointData->GetAbstractArray(i);
    ss << svtkImporter::GetArrayDescription(array, indent.GetNextIndent());
  }

  ss << indent << nbCellData << " cell data array(s):\n";
  for (svtkIdType i = 0; i < nbCellData; i++)
  {
    svtkAbstractArray* array = cellData->GetAbstractArray(i);
    ss << svtkImporter::GetArrayDescription(array, indent.GetNextIndent());
  }

  ss << indent << nbFieldData << " field data array(s):\n";
  for (svtkIdType i = 0; i < nbFieldData; i++)
  {
    svtkAbstractArray* array = fieldData->GetAbstractArray(i);
    if (array)
    {
      ss << svtkImporter::GetArrayDescription(array, indent.GetNextIndent());
    }
  }

  return ss.str();
}

//----------------------------------------------------------------------------
svtkIdType svtkImporter::GetNumberOfAnimations()
{
  return -1;
}

//----------------------------------------------------------------------------
bool svtkImporter::GetTemporalInformation(svtkIdType svtkNotUsed(animationIdx),
  int& svtkNotUsed(nbTimeSteps), double svtkNotUsed(timeRange)[2],
  svtkDoubleArray* svtkNotUsed(timeSteps))
{
  return false;
}

//----------------------------------------------------------------------------
void svtkImporter::UpdateTimeStep(double svtkNotUsed(timeStep))
{
  this->Update();
}
