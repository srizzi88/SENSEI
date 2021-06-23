/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEdgeCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEdgeCenters.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataSetAttributes.h"
#include "svtkEdgeListIterator.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkEdgeCenters);

// Construct object with vertex cell generation turned off.
svtkEdgeCenters::svtkEdgeCenters()
{
  this->VertexCells = 0;
}

// Generate points
int svtkEdgeCenters::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numEdges;
  svtkDataSetAttributes* inED;
  svtkDataSetAttributes* outPD;
  svtkPoints* newPts;
  double x[3];

  inED = input->GetEdgeData();
  outPD = output->GetPointData();

  if ((numEdges = input->GetNumberOfEdges()) < 1)
  {
    svtkDebugMacro(<< "No cells to generate center points for");
    return 1;
  }

  newPts = svtkPoints::New();
  newPts->SetNumberOfPoints(numEdges);

  int abort = 0;
  svtkIdType progressInterval = numEdges / 10 + 1;
  svtkEdgeListIterator* edges = svtkEdgeListIterator::New();
  input->GetEdges(edges);
  svtkIdType processed = 0;
  while (edges->HasNext() && !abort)
  {
    svtkEdgeType e = edges->Next();
    if (!(processed % progressInterval))
    {
      svtkDebugMacro(<< "Processing #" << processed);
      this->UpdateProgress(0.5 * processed / numEdges);
      abort = this->GetAbortExecute();
    }
    double p1[3];
    double p2[3];
    input->GetPoint(e.Source, p1);
    input->GetPoint(e.Target, p2);
    svtkIdType npts = 0;
    double* pts = nullptr;
    input->GetEdgePoints(e.Id, npts, pts);
    // Find the length of the edge
    if (npts == 0)
    {
      for (int c = 0; c < 3; ++c)
      {
        x[c] = (p1[c] + p2[c]) / 2.0;
      }
    }
    else
    {
      svtkIdType nptsFull = npts + 2;
      double* ptsFull = new double[3 * (nptsFull)];
      ptsFull[0] = p1[0];
      ptsFull[1] = p1[1];
      ptsFull[2] = p1[2];
      memcpy(ptsFull + 3, pts, sizeof(double) * 3 * npts);
      ptsFull[3 * (nptsFull - 1) + 0] = p2[0];
      ptsFull[3 * (nptsFull - 1) + 1] = p2[1];
      ptsFull[3 * (nptsFull - 1) + 2] = p2[2];

      double len = 0.0;
      double* curPt = ptsFull;
      for (svtkIdType i = 0; i < nptsFull - 1; ++i, curPt += 3)
      {
        len += sqrt(svtkMath::Distance2BetweenPoints(curPt, curPt + 3));
      }
      double half = len / 2;
      len = 0.0;
      curPt = ptsFull;
      for (svtkIdType i = 0; i < nptsFull - 1; ++i, curPt += 3)
      {
        double curDist = sqrt(svtkMath::Distance2BetweenPoints(curPt, curPt + 3));
        if (len + curDist > half)
        {
          double alpha = (half - len) / curDist;
          for (int c = 0; c < 3; ++c)
          {
            x[c] = (1.0 - alpha) * curPt[c] + alpha * curPt[c + 3];
          }
          break;
        }
        len += curDist;
      }
      delete[] ptsFull;
    }
    newPts->SetPoint(e.Id, x);
    ++processed;
  }
  edges->Delete();

  if (this->VertexCells)
  {
    svtkIdType pts[1];
    svtkCellData* outCD = output->GetCellData();
    svtkCellArray* verts = svtkCellArray::New();
    verts->AllocateEstimate(numEdges, 2);

    processed = 0;
    edges = svtkEdgeListIterator::New();
    input->GetEdges(edges);
    while (edges->HasNext() && !abort)
    {
      svtkEdgeType e = edges->Next();
      if (!(processed % progressInterval))
      {
        svtkDebugMacro(<< "Processing #" << processed);
        this->UpdateProgress(0.5 + 0.5 * processed / numEdges);
        abort = this->GetAbortExecute();
      }

      pts[0] = e.Id;
      verts->InsertNextCell(1, pts);
      ++processed;
    }
    edges->Delete();

    output->SetVerts(verts);
    verts->Delete();
    outCD->PassData(inED); // only if verts are generated
  }

  // clean up and update output
  output->SetPoints(newPts);
  newPts->Delete();

  outPD->PassData(inED); // because number of points = number of cells

  return 1;
}

int svtkEdgeCenters::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

void svtkEdgeCenters::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Vertex Cells: " << (this->VertexCells ? "On\n" : "Off\n");
}
