/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFacetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFacetWriter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkErrorCode.h"
#include "svtkGarbageCollector.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"

#include "svtkDoubleArray.h"
#include "svtkSmartPointer.h"
#include "svtkUnsignedIntArray.h"
#include "svtksys/FStream.hxx"

#include <string>
#include <sys/stat.h>
#include <vector>

svtkStandardNewMacro(svtkFacetWriter);

//----------------------------------------------------------------------------
svtkFacetWriter::svtkFacetWriter()
{
  this->FileName = nullptr;
  this->OutputStream = nullptr;
}

//----------------------------------------------------------------------------
svtkFacetWriter::~svtkFacetWriter()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void svtkFacetWriter::Write()
{
  this->WriteToStream(nullptr);
}

//----------------------------------------------------------------------------
int svtkFacetWriter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  this->SetErrorCode(svtkErrorCode::NoError);

  int cleanStream = 0;
  if (!this->OutputStream)
  {
    if (!this->FileName)
    {
      svtkErrorMacro("File name not specified");
      return 0;
    }

    this->OutputStream = new svtksys::ofstream(this->FileName);
    if (!this->OutputStream)
    {
      svtkErrorMacro("Error opening file: " << this->FileName << " for writing");
      return 0;
    }
    cleanStream = 1;
  }

  if (!this->OutputStream)
  {
    svtkErrorMacro("No output stream");
    return 0;
  }

  int cc;
  int len = inputVector[0]->GetNumberOfInformationObjects();
  *this->OutputStream << "FACET FILE FROM SVTK" << endl << len << endl;

  for (cc = 0; cc < len; cc++)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(cc);
    svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    if (!this->WriteDataToStream(this->OutputStream, input))
    {
      if (cleanStream)
      {
        delete this->OutputStream;
        this->OutputStream = nullptr;
      }
      return 0;
    }
  }
  if (cleanStream)
  {
    delete this->OutputStream;
    this->OutputStream = nullptr;
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkFacetWriter::WriteToStream(ostream* ost)
{
  this->OutputStream = ost;
  // we always write, even if nothing has changed, so send a modified
  this->Modified();
  this->UpdateInformation();
  svtkInformation* inInfo = this->GetInputInformation(0, 0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  this->Update();
  this->OutputStream = nullptr;
}

//----------------------------------------------------------------------------
int svtkFacetWriter::WriteDataToStream(ostream* ost, svtkPolyData* data)
{
  *ost << "Element" << data << endl << "0" << endl << data->GetNumberOfPoints() << " 0 0" << endl;
  svtkIdType point;
  for (point = 0; point < data->GetNumberOfPoints(); point++)
  {
    double xyz[3];
    data->GetPoint(point, xyz);
    *ost << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
  }
  *ost << "1" << endl << "Element" << data << endl;
  int written = 0;
  svtkCellArray* ca;
  svtkIdType numCells;
  svtkIdType totalCells = 0;
  int material = 0;
  int part = 0;
  svtkIdType cc;

  if (data->GetVerts()->GetNumberOfCells())
  {
    // This test is needed if another cell type is written above this
    // block.  We must remove it here because it produces an
    // unreachable code warning.
    //
    // if ( written )
    //  {
    //  svtkErrorMacro("Multiple different cells in the poly data");
    //  return 0;
    //  }
    ca = data->GetVerts();
    numCells = ca->GetNumberOfCells();
    svtkIdType numPts = 0;
    const svtkIdType* pts = nullptr;
    ca->InitTraversal();
    while (ca->GetNextCell(numPts, pts))
    {
      // Each vertex is one cell
      for (cc = 0; cc < numPts; cc++)
      {
        totalCells++;
      }
    }
    *ost << totalCells << " 1" << endl;
    ca->InitTraversal();
    while (ca->GetNextCell(numPts, pts))
    {
      for (cc = 0; cc < numPts; cc++)
      {
        // Indices of point starts with 1
        svtkIdType pointIndex = pts[cc] + 1;
        *ost << pointIndex << " " << material << " " << part << endl;
      }
    }
    written = 1;
  }

  if (data->GetLines()->GetNumberOfCells())
  {
    if (written)
    {
      svtkErrorMacro("Multiple different cells in the poly data");
      return 0;
    }
    ca = data->GetLines();
    numCells = ca->GetNumberOfCells();
    svtkIdType numPts = 0;
    const svtkIdType* pts = nullptr;
    ca->InitTraversal();
    while (ca->GetNextCell(numPts, pts))
    {
      // One line per cell
      for (cc = 1; cc < numPts; cc++)
      {
        totalCells++;
      }
    }
    *ost << totalCells << " 2" << endl;
    ca->InitTraversal();
    while (ca->GetNextCell(numPts, pts))
    {
      for (cc = 1; cc < numPts; cc++)
      {
        svtkIdType point1 = pts[cc - 1] + 1;
        svtkIdType point2 = pts[cc] + 1;
        *ost << point1 << " " << point2 << " " << material << " " << part << endl;
      }
    }
    written = 1;
  }

  if (data->GetPolys()->GetNumberOfCells())
  {
    if (written)
    {
      svtkErrorMacro("Multiple different cells in the poly data");
      return 0;
    }
    ca = data->GetPolys();
    numCells = ca->GetNumberOfCells();
    svtkIdType numPts = 0;
    const svtkIdType* pts = nullptr;
    ca->InitTraversal();
    ca->GetNextCell(numPts, pts);
    totalCells++;
    svtkIdType numPoints = numPts;
    while (ca->GetNextCell(numPts, pts))
    {
      if (numPts != numPoints)
      {
        svtkErrorMacro("Found polygons with different order");
        return 0;
      }
      totalCells++;
    }
    *ost << totalCells << " " << numPoints << endl;
    ca->InitTraversal();
    int cnt = 0;
    while (ca->GetNextCell(numPts, pts))
    {
      for (cc = 0; cc < numPts; cc++)
      {
        svtkIdType pointindex = pts[cc] + 1;
        *ost << pointindex << " ";
      }
      *ost << material << " " << part << endl;
      cnt++;
    }
    cout << "Written: " << cnt << " / " << numCells << " / " << totalCells << endl;
    written = 1;
  }

  if (data->GetStrips()->GetNumberOfCells())
  {
    if (written)
    {
      svtkErrorMacro("Multiple different cells in the poly data");
      return 0;
    }
    ca = data->GetStrips();
    numCells = ca->GetNumberOfCells();
    svtkIdType numPts = 0;
    const svtkIdType* pts = nullptr;
    ca->InitTraversal();
    while (ca->GetNextCell(numPts, pts))
    {
      // One triangle per cell
      for (cc = 2; cc < numPts; cc++)
      {
        totalCells++;
      }
    }
    *ost << totalCells << " 3" << endl;
    ca->InitTraversal();
    while (ca->GetNextCell(numPts, pts))
    {
      for (cc = 2; cc < numPts; cc++)
      {
        svtkIdType point1 = pts[cc - 2] + 1;
        svtkIdType point2 = pts[cc - 1] + 1;
        svtkIdType point3 = pts[cc] + 1;
        *ost << point1 << " " << point2 << " " << point3 << " " << material << " " << part << endl;
      }
    }
    written = 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkFacetWriter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}
//----------------------------------------------------------------------------
void svtkFacetWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}
