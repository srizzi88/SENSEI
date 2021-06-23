/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFeatureEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFeatureEdges.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTriangleStrip.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkFeatureEdges);

//----------------------------------------------------------------------------
// Construct object with feature angle = 30; all types of edges, except
// manifold edges, are extracted and colored.
svtkFeatureEdges::svtkFeatureEdges()
{
  this->FeatureAngle = 30.0;
  this->BoundaryEdges = true;
  this->FeatureEdges = true;
  this->NonManifoldEdges = true;
  this->ManifoldEdges = false;
  this->Coloring = true;
  this->Locator = nullptr;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
svtkFeatureEdges::~svtkFeatureEdges()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

//----------------------------------------------------------------------------
// Generate feature edges for mesh
int svtkFeatureEdges::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* inPts;
  svtkPoints* newPts;
  svtkFloatArray* newScalars = nullptr;
  svtkCellArray* newLines;
  svtkPolyData* Mesh;
  int i;
  svtkIdType j, numNei, cellId;
  svtkIdType numBEdges, numNonManifoldEdges, numFedges, numManifoldEdges;
  double scalar, n[3], x1[3], x2[3];
  double cosAngle = 0;
  svtkIdType lineIds[2];
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  svtkCellArray *inPolys, *inStrips, *newPolys;
  svtkFloatArray* polyNormals = nullptr;
  svtkIdType numPts, numCells, numPolys, numStrips, nei;
  svtkIdList* neighbors;
  svtkIdType p1, p2, newId;
  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();
  unsigned char* ghosts = nullptr;
  svtkDebugMacro(<< "Executing feature edges");

  svtkDataArray* temp = nullptr;
  if (cd)
  {
    temp = cd->GetArray(svtkDataSetAttributes::GhostArrayName());
  }
  if ((!temp) || (temp->GetDataType() != SVTK_UNSIGNED_CHAR) || (temp->GetNumberOfComponents() != 1))
  {
    svtkDebugMacro("No appropriate ghost levels field available.");
  }
  else
  {
    ghosts = static_cast<svtkUnsignedCharArray*>(temp)->GetPointer(0);
  }

  //  Check input
  //
  inPts = input->GetPoints();
  numCells = input->GetNumberOfCells();
  numPolys = input->GetNumberOfPolys();
  numStrips = input->GetNumberOfStrips();
  if ((numPts = input->GetNumberOfPoints()) < 1 || !inPts || (numPolys < 1 && numStrips < 1))
  {
    svtkDebugMacro(<< "No input data!");
    return 1;
  }

  if (!this->BoundaryEdges && !this->NonManifoldEdges && !this->FeatureEdges &&
    !this->ManifoldEdges)
  {
    svtkDebugMacro(<< "All edge types turned off!");
  }

  // Build cell structure.  Might have to triangulate the strips.
  Mesh = svtkPolyData::New();
  Mesh->SetPoints(inPts);
  inPolys = input->GetPolys();
  if (numStrips > 0)
  {
    newPolys = svtkCellArray::New();
    if (numPolys > 0)
    {
      newPolys->DeepCopy(inPolys);
    }
    else
    {
      newPolys->AllocateEstimate(numStrips, 5);
    }
    inStrips = input->GetStrips();
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
    {
      svtkTriangleStrip::DecomposeStrip(npts, pts, newPolys);
    }
    Mesh->SetPolys(newPolys);
    newPolys->Delete();
  }
  else
  {
    newPolys = inPolys;
    Mesh->SetPolys(newPolys);
  }
  Mesh->BuildLinks();

  // Allocate storage for lines/points (arbitrary allocation sizes)
  //
  newPts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->Allocate(numPts / 10, numPts);
  newLines = svtkCellArray::New();
  newLines->AllocateEstimate(numPts / 20, 2);
  if (this->Coloring)
  {
    newScalars = svtkFloatArray::New();
    newScalars->SetName("Edge Types");
    newScalars->Allocate(numCells / 10, numCells);
  }

  outPD->CopyAllocate(pd, numPts);
  outCD->CopyAllocate(cd, numCells);

  // Get our locator for merging points
  //
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, input->GetBounds());

  // Loop over all polygons generating boundary, non-manifold,
  // and feature edges
  //
  if (this->FeatureEdges)
  {
    polyNormals = svtkFloatArray::New();
    polyNormals->SetNumberOfComponents(3);
    polyNormals->Allocate(3 * newPolys->GetNumberOfCells());

    for (cellId = 0, newPolys->InitTraversal(); newPolys->GetNextCell(npts, pts); cellId++)
    {
      svtkPolygon::ComputeNormal(inPts, npts, pts, n);
      polyNormals->InsertTuple(cellId, n);
    }

    cosAngle = cos(svtkMath::RadiansFromDegrees(this->FeatureAngle));
  }

  neighbors = svtkIdList::New();
  neighbors->Allocate(SVTK_CELL_SIZE);

  int abort = 0;
  svtkIdType progressInterval = numCells / 20 + 1;

  numBEdges = numNonManifoldEdges = numFedges = numManifoldEdges = 0;
  for (cellId = 0, newPolys->InitTraversal(); newPolys->GetNextCell(npts, pts) && !abort; cellId++)
  {
    if (!(cellId % progressInterval)) // manage progress / early abort
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    for (i = 0; i < npts; i++)
    {
      p1 = pts[i];
      p2 = pts[(i + 1) % npts];

      Mesh->GetCellEdgeNeighbors(cellId, p1, p2, neighbors);
      numNei = neighbors->GetNumberOfIds();

      if (this->BoundaryEdges && numNei < 1)
      {
        if (ghosts && ghosts[cellId] & svtkDataSetAttributes::DUPLICATECELL)
        {
          continue;
        }
        else
        {
          numBEdges++;
          scalar = 0.0;
        }
      }

      else if (this->NonManifoldEdges && numNei > 1)
      {
        // check to make sure that this edge hasn't been created before
        for (j = 0; j < numNei; j++)
        {
          if (neighbors->GetId(j) < cellId)
          {
            break;
          }
        }
        if (j >= numNei)
        {
          if (ghosts && ghosts[cellId] & svtkDataSetAttributes::DUPLICATECELL)
          {
            continue;
          }
          else
          {
            numNonManifoldEdges++;
            scalar = 0.222222;
          }
        }
        else
        {
          continue;
        }
      }
      else if (this->FeatureEdges && numNei == 1 && (nei = neighbors->GetId(0)) > cellId)
      {
        double neiTuple[3];
        double cellTuple[3];
        polyNormals->GetTuple(nei, neiTuple);
        polyNormals->GetTuple(cellId, cellTuple);
        if (svtkMath::Dot(neiTuple, cellTuple) <= cosAngle)
        {
          if (ghosts && ghosts[cellId] & svtkDataSetAttributes::DUPLICATECELL)
          {
            continue;
          }
          else
          {
            numFedges++;
            scalar = 0.444444;
          }
        }
        else
        {
          continue;
        }
      }
      else if (this->ManifoldEdges && numNei == 1 && neighbors->GetId(0) > cellId)
      {
        if (ghosts && ghosts[cellId] & svtkDataSetAttributes::DUPLICATECELL)
        {
          continue;
        }
        else
        {
          numManifoldEdges++;
          scalar = 0.666667;
        }
      }
      else
      {
        continue;
      }

      // Add edge to output
      Mesh->GetPoint(p1, x1);
      Mesh->GetPoint(p2, x2);

      if (this->Locator->InsertUniquePoint(x1, lineIds[0]))
      {
        outPD->CopyData(pd, p1, lineIds[0]);
      }

      if (this->Locator->InsertUniquePoint(x2, lineIds[1]))
      {
        outPD->CopyData(pd, p2, lineIds[1]);
      }

      newId = newLines->InsertNextCell(2, lineIds);
      outCD->CopyData(cd, cellId, newId);
      if (this->Coloring)
      {
        newScalars->InsertTuple(newId, &scalar);
      }
    }
  }

  svtkDebugMacro(<< "Created " << numBEdges << " boundary edges, " << numNonManifoldEdges
                << " non-manifold edges, " << numFedges << " feature edges, " << numManifoldEdges
                << " manifold edges");

  //  Update ourselves.
  //
  if (this->FeatureEdges)
  {
    polyNormals->Delete();
  }

  Mesh->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
  neighbors->Delete();

  output->SetLines(newLines);
  newLines->Delete();
  this->Locator->Initialize(); // release any extra memory
  if (this->Coloring)
  {
    int idx = outCD->AddArray(newScalars);
    outCD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkFeatureEdges::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

//----------------------------------------------------------------------------
// Specify a spatial locator for merging points. By
// default an instance of svtkMergePoints is used.
void svtkFeatureEdges::SetLocator(svtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  if (locator)
  {
    locator->Register(this);
  }
  this->Locator = locator;
  this->Modified();
}

//----------------------------------------------------------------------------
svtkMTimeType svtkFeatureEdges::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//----------------------------------------------------------------------------
int svtkFeatureEdges::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int numPieces, ghostLevel;

  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1)
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel + 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkFeatureEdges::ExtractAllEdgeTypesOn()
{
  this->BoundaryEdgesOn();
  this->FeatureEdgesOn();
  this->NonManifoldEdgesOn();
  this->ManifoldEdgesOn();
}

//----------------------------------------------------------------------------
void svtkFeatureEdges::ExtractAllEdgeTypesOff()
{
  this->BoundaryEdgesOff();
  this->FeatureEdgesOff();
  this->NonManifoldEdgesOff();
  this->ManifoldEdgesOff();
}

//----------------------------------------------------------------------------
void svtkFeatureEdges::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
  os << indent << "Boundary Edges: " << (this->BoundaryEdges ? "On\n" : "Off\n");
  os << indent << "Feature Edges: " << (this->FeatureEdges ? "On\n" : "Off\n");
  os << indent << "Non-Manifold Edges: " << (this->NonManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Manifold Edges: " << (this->ManifoldEdges ? "On\n" : "Off\n");
  os << indent << "Coloring: " << (this->Coloring ? "On\n" : "Off\n");

  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
