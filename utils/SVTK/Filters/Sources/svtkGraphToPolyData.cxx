/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphToPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkGraphToPolyData.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDirectedGraph.h"
#include "svtkDoubleArray.h"
#include "svtkEdgeListIterator.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"

#include <map>
#include <utility>
#include <vector>

svtkStandardNewMacro(svtkGraphToPolyData);

svtkGraphToPolyData::svtkGraphToPolyData()
{
  this->EdgeGlyphOutput = false;
  this->EdgeGlyphPosition = 1.0;
  this->SetNumberOfOutputPorts(2);
}

int svtkGraphToPolyData::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

int svtkGraphToPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* arrowInfo = outputVector->GetInformationObject(1);

  // get the input and output
  svtkGraph* input = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* arrowOutput =
    svtkPolyData::SafeDownCast(arrowInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* edgeGhostLevels = svtkArrayDownCast<svtkDataArray>(
    input->GetEdgeData()->GetAbstractArray(svtkDataSetAttributes::GhostArrayName()));

  if (edgeGhostLevels == nullptr)
  {
    svtkSmartPointer<svtkIdTypeArray> cells = svtkSmartPointer<svtkIdTypeArray>::New();
    svtkSmartPointer<svtkEdgeListIterator> it = svtkSmartPointer<svtkEdgeListIterator>::New();
    input->GetEdges(it);
    svtkSmartPointer<svtkPoints> newPoints = svtkSmartPointer<svtkPoints>::New();
    newPoints->DeepCopy(input->GetPoints());
    output->SetPoints(newPoints);
    svtkIdType numEdges = input->GetNumberOfEdges();
    bool noExtraPoints = true;
    for (svtkIdType e = 0; e < numEdges; ++e)
    {
      svtkIdType npts;
      double* pts;
      input->GetEdgePoints(e, npts, pts);
      svtkIdType source = input->GetSourceVertex(e);
      svtkIdType target = input->GetTargetVertex(e);
      if (npts == 0)
      {
        cells->InsertNextValue(2);
        cells->InsertNextValue(source);
        cells->InsertNextValue(target);
      }
      else
      {
        cells->InsertNextValue(2 + npts);
        cells->InsertNextValue(source);
        for (svtkIdType i = 0; i < npts; ++i, pts += 3)
        {
          noExtraPoints = false;
          svtkIdType pt = output->GetPoints()->InsertNextPoint(pts);
          cells->InsertNextValue(pt);
        }
        cells->InsertNextValue(target);
      }
    }
    svtkSmartPointer<svtkCellArray> newLines = svtkSmartPointer<svtkCellArray>::New();
    newLines->AllocateExact(numEdges, cells->GetNumberOfValues() - numEdges);
    newLines->ImportLegacyFormat(cells);

    // Send the data to output.
    output->SetLines(newLines);

    // Points only correspond to vertices if we didn't add extra points.
    if (noExtraPoints)
    {
      output->GetPointData()->PassData(input->GetVertexData());
    }

    // Cells correspond to edges, so pass the cell data along.
    output->GetCellData()->PassData(input->GetEdgeData());
  }
  else
  {
    svtkIdType numEdges = input->GetNumberOfEdges();
    svtkDataSetAttributes* inputCellData = input->GetEdgeData();
    svtkCellData* outputCellData = output->GetCellData();
    outputCellData->CopyAllocate(inputCellData);
    svtkSmartPointer<svtkCellArray> newLines = svtkSmartPointer<svtkCellArray>::New();
    newLines->AllocateEstimate(numEdges, 2);
    svtkIdType points[2];

    // Only create lines for non-ghost edges
    svtkSmartPointer<svtkEdgeListIterator> it = svtkSmartPointer<svtkEdgeListIterator>::New();
    input->GetEdges(it);
    while (it->HasNext())
    {
      svtkEdgeType e = it->Next();
      if (edgeGhostLevels->GetComponent(e.Id, 0) == 0)
      {
        points[0] = e.Source;
        points[1] = e.Target;
        svtkIdType ind = newLines->InsertNextCell(2, points);
        outputCellData->CopyData(inputCellData, e.Id, ind);
      }
    }

    // Send data to output
    output->SetPoints(input->GetPoints());
    output->SetLines(newLines);
    output->GetPointData()->PassData(input->GetVertexData());

    // Clean up
    output->Squeeze();
  }

  if (this->EdgeGlyphOutput)
  {
    svtkDataSetAttributes* inputCellData = input->GetEdgeData();

    svtkPointData* arrowPointData = arrowOutput->GetPointData();
    arrowPointData->CopyAllocate(inputCellData);
    svtkPoints* newPoints = svtkPoints::New();
    arrowOutput->SetPoints(newPoints);
    newPoints->Delete();
    svtkDoubleArray* orientArr = svtkDoubleArray::New();
    orientArr->SetNumberOfComponents(3);
    orientArr->SetName("orientation");
    arrowPointData->AddArray(orientArr);
    arrowPointData->SetVectors(orientArr);
    orientArr->Delete();
    double sourcePt[3] = { 0, 0, 0 };
    double targetPt[3] = { 0, 0, 0 };
    double pt[3] = { 0, 0, 0 };
    double orient[3] = { 0, 0, 0 };
    svtkSmartPointer<svtkEdgeListIterator> it = svtkSmartPointer<svtkEdgeListIterator>::New();
    input->GetEdges(it);
    while (it->HasNext())
    {
      svtkEdgeType e = it->Next();
      if (!edgeGhostLevels || edgeGhostLevels->GetComponent(e.Id, 0) == 0)
      {
        svtkIdType source = e.Source;
        svtkIdType target = e.Target;
        // Do not render arrows for self loops.
        if (source != target)
        {
          input->GetPoint(source, sourcePt);
          input->GetPoint(target, targetPt);
          for (int j = 0; j < 3; j++)
          {
            pt[j] =
              (1 - this->EdgeGlyphPosition) * sourcePt[j] + this->EdgeGlyphPosition * targetPt[j];
            orient[j] = targetPt[j] - sourcePt[j];
          }
          svtkIdType ind = newPoints->InsertNextPoint(pt);
          orientArr->InsertNextTuple(orient);
          arrowPointData->CopyData(inputCellData, e.Id, ind);
        }
      }
    }
  }

  return 1;
}

void svtkGraphToPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EdgeGlyphOutput: " << (this->EdgeGlyphOutput ? "on" : "off") << endl;
  os << indent << "EdgeGlyphPosition: " << this->EdgeGlyphPosition << endl;
}
