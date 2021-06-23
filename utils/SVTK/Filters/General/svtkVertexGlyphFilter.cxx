/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVertexGlyphFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "svtkVertexGlyphFilter.h"

#include "svtkCellArray.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkStandardNewMacro(svtkVertexGlyphFilter);

//-----------------------------------------------------------------------------

svtkVertexGlyphFilter::svtkVertexGlyphFilter() = default;

svtkVertexGlyphFilter::~svtkVertexGlyphFilter() = default;

void svtkVertexGlyphFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

int svtkVertexGlyphFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//-----------------------------------------------------------------------------

int svtkVertexGlyphFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects.
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output.
  svtkPointSet* psInput = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkGraph* graphInput = svtkGraph::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* points = nullptr;
  if (psInput)
  {
    points = psInput->GetPoints();
  }
  else
  {
    points = graphInput->GetPoints();
  }

  // If no points, then nothing to do.
  if (points == nullptr)
  {
    return 1;
  }

  output->SetPoints(points);
  svtkIdType numPoints = points->GetNumberOfPoints();

  if (psInput)
  {
    output->GetPointData()->PassData(psInput->GetPointData());
  }
  else
  {
    output->GetPointData()->PassData(graphInput->GetVertexData());
  }

  SVTK_CREATE(svtkCellArray, cells);
  cells->AllocateEstimate(numPoints, 1);

  for (svtkIdType i = 0; i < numPoints; i++)
  {
    cells->InsertNextCell(1, &i);
  }
  output->SetVerts(cells);

  return 1;
}
