/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetToDataObjectFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetToDataObjectFilter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkUnstructuredGrid.h"

#include <sstream>
#include <string>

svtkStandardNewMacro(svtkDataSetToDataObjectFilter);

//----------------------------------------------------------------------------
// Instantiate object.
svtkDataSetToDataObjectFilter::svtkDataSetToDataObjectFilter()
{
  this->Geometry = 1;
  this->Topology = 1;
  this->LegacyTopology = 1;
  this->ModernTopology = 1;
  this->PointData = 1;
  this->CellData = 1;
  this->FieldData = 1;
}

svtkDataSetToDataObjectFilter::~svtkDataSetToDataObjectFilter() = default;

//----------------------------------------------------------------------------
int svtkDataSetToDataObjectFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkFieldData* fd = svtkFieldData::New();
  svtkPoints* pts;
  svtkDataArray* da;
  int i;

  svtkDebugMacro(<< "Generating field data from data set");

  if (this->Geometry)
  {
    if (input->GetDataObjectType() == SVTK_POLY_DATA)
    {
      pts = static_cast<svtkPolyData*>(input)->GetPoints();
      if (pts)
      {
        da = pts->GetData();
        da->SetName("Points");
        fd->AddArray(da);
      }
    }

    else if (input->GetDataObjectType() == SVTK_STRUCTURED_POINTS)
    {
      svtkStructuredPoints* spts = static_cast<svtkStructuredPoints*>(input);

      svtkFloatArray* origin = svtkFloatArray::New();
      origin->SetNumberOfValues(3);
      double org[3];
      spts->GetOrigin(org);
      origin->SetValue(0, org[0]);
      origin->SetValue(1, org[1]);
      origin->SetValue(2, org[2]);
      origin->SetName("Origin");
      fd->AddArray(origin);
      origin->Delete();

      svtkFloatArray* spacing = svtkFloatArray::New();
      spacing->SetNumberOfValues(3);
      double sp[3];
      spts->GetSpacing(sp);
      spacing->SetValue(0, sp[0]);
      spacing->SetValue(1, sp[1]);
      spacing->SetValue(2, sp[2]);
      spacing->SetName("Spacing");
      fd->AddArray(spacing);
      spacing->Delete();
    }

    else if (input->GetDataObjectType() == SVTK_STRUCTURED_GRID)
    {
      pts = static_cast<svtkStructuredGrid*>(input)->GetPoints();
      if (pts)
      {
        da = pts->GetData();
        da->SetName("Points");
        fd->AddArray(da);
      }
    }

    else if (input->GetDataObjectType() == SVTK_RECTILINEAR_GRID)
    {
      svtkRectilinearGrid* rgrid = static_cast<svtkRectilinearGrid*>(input);
      da = rgrid->GetXCoordinates();
      if (da != nullptr)
      {
        da->SetName("XCoordinates");
        fd->AddArray(da);
      }
      da = rgrid->GetYCoordinates();
      if (da != nullptr)
      {
        da->SetName("YCoordinates");
        fd->AddArray(da);
      }
      da = rgrid->GetZCoordinates();
      if (da != nullptr)
      {
        da->SetName("ZCoordinates");
        fd->AddArray(da);
      }
    }

    else if (input->GetDataObjectType() == SVTK_UNSTRUCTURED_GRID)
    {
      pts = static_cast<svtkUnstructuredGrid*>(input)->GetPoints();
      if (pts)
      {
        da = pts->GetData();
        da->SetName("Points");
        fd->AddArray(da);
      }
    }

    else
    {
      svtkErrorMacro(<< "Unsupported dataset type!");
      fd->Delete();
      return 1;
    }
  }

  if (this->Topology)
  {
    // Helper lambda to add cell arrays to the field data:
    auto addCellConnArrays = [&](svtkCellArray* ca, const std::string& name) {
      if (!ca || ca->GetNumberOfCells() == 0)
      {
        return;
      }

      // For backwards compatibility:
      if (this->LegacyTopology)
      {
        svtkNew<svtkIdTypeArray> legacy;
        ca->ExportLegacyFormat(legacy);
        legacy->SetName(name.c_str());
        fd->AddArray(legacy);
      }

      // For modern cell storage:
      if (this->ModernTopology)
      {
        {
          const std::string connName = name + ".Connectivity";
          auto conn = svtk::TakeSmartPointer(ca->GetConnectivityArray()->NewInstance());
          conn->ShallowCopy(ca->GetConnectivityArray());
          conn->SetName(connName.c_str());
          fd->AddArray(conn);
        }

        {
          const std::string offsetsName = name + ".Offsets";
          auto offsets = svtk::TakeSmartPointer(ca->GetOffsetsArray()->NewInstance());
          offsets->ShallowCopy(ca->GetOffsetsArray());
          offsets->SetName(offsetsName.c_str());
          fd->AddArray(offsets);
        }
      }
    };

    if (input->GetDataObjectType() == SVTK_POLY_DATA)
    {
      svtkPolyData* pd = static_cast<svtkPolyData*>(input);

      addCellConnArrays(pd->GetVerts(), "Verts");
      addCellConnArrays(pd->GetLines(), "Lines");
      addCellConnArrays(pd->GetPolys(), "Polys");
      addCellConnArrays(pd->GetStrips(), "Strips");
    }

    else if (input->GetDataObjectType() == SVTK_STRUCTURED_POINTS)
    {
      svtkIntArray* dimensions = svtkIntArray::New();
      dimensions->SetNumberOfValues(3);
      int dims[3];
      static_cast<svtkStructuredPoints*>(input)->GetDimensions(dims);
      dimensions->SetValue(0, dims[0]);
      dimensions->SetValue(1, dims[1]);
      dimensions->SetValue(2, dims[2]);
      dimensions->SetName("Dimensions");
      fd->AddArray(dimensions);
      dimensions->Delete();
    }

    else if (input->GetDataObjectType() == SVTK_STRUCTURED_GRID)
    {
      svtkIntArray* dimensions = svtkIntArray::New();
      dimensions->SetNumberOfValues(3);
      int dims[3];
      static_cast<svtkStructuredGrid*>(input)->GetDimensions(dims);
      dimensions->SetValue(0, dims[0]);
      dimensions->SetValue(1, dims[1]);
      dimensions->SetValue(2, dims[2]);
      dimensions->SetName("Dimensions");
      fd->AddArray(dimensions);
      dimensions->Delete();
    }

    else if (input->GetDataObjectType() == SVTK_RECTILINEAR_GRID)
    {
      svtkIntArray* dimensions = svtkIntArray::New();
      dimensions->SetNumberOfValues(3);
      int dims[3];
      static_cast<svtkRectilinearGrid*>(input)->GetDimensions(dims);
      dimensions->SetValue(0, dims[0]);
      dimensions->SetValue(1, dims[1]);
      dimensions->SetValue(2, dims[2]);
      dimensions->SetName("Dimensions");
      fd->AddArray(dimensions);
      dimensions->Delete();
    }

    else if (input->GetDataObjectType() == SVTK_UNSTRUCTURED_GRID)
    {
      svtkCellArray* ca = static_cast<svtkUnstructuredGrid*>(input)->GetCells();
      if (ca != nullptr && ca->GetNumberOfCells() > 0)
      {
        addCellConnArrays(ca, "Cells");

        svtkIdType numCells = input->GetNumberOfCells();
        svtkIntArray* types = svtkIntArray::New();
        types->SetNumberOfValues(numCells);
        for (svtkIdType cellId = 0; cellId < numCells; ++cellId)
        {
          types->SetValue(cellId, input->GetCellType(cellId));
        }
        types->SetName("CellTypes");
        fd->AddArray(types);
        types->Delete();
      }
    }

    else
    {
      svtkErrorMacro(<< "Unsupported dataset type!");
      fd->Delete();
      return 1;
    }
  }

  svtkFieldData* fieldData;

  if (this->FieldData)
  {
    fieldData = input->GetFieldData();

    for (i = 0; i < fieldData->GetNumberOfArrays(); i++)
    {
      fd->AddArray(fieldData->GetArray(i));
    }
  }

  if (this->PointData)
  {
    fieldData = input->GetPointData();

    for (i = 0; i < fieldData->GetNumberOfArrays(); i++)
    {
      fd->AddArray(fieldData->GetArray(i));
    }
  }

  if (this->CellData)
  {
    fieldData = input->GetCellData();

    for (i = 0; i < fieldData->GetNumberOfArrays(); i++)
    {
      fd->AddArray(fieldData->GetArray(i));
    }
  }

  output->SetFieldData(fd);
  fd->Delete();
  return 1;
}

//----------------------------------------------------------------------------
int svtkDataSetToDataObjectFilter::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int svtkDataSetToDataObjectFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkDataSetToDataObjectFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Geometry: " << (this->Geometry ? "On\n" : "Off\n");
  os << indent << "Topology: " << (this->Topology ? "On\n" : "Off\n");
  os << indent << "Field Data: " << (this->FieldData ? "On\n" : "Off\n");
  os << indent << "Point Data: " << (this->PointData ? "On\n" : "Off\n");
  os << indent << "Cell Data: " << (this->CellData ? "On\n" : "Off\n");
}
