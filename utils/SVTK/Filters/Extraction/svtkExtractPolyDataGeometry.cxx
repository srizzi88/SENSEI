/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPolyDataGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractPolyDataGeometry.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkExtractPolyDataGeometry);
svtkCxxSetObjectMacro(svtkExtractPolyDataGeometry, ImplicitFunction, svtkImplicitFunction);

//----------------------------------------------------------------------------
// Construct object with ExtractInside turned on.
svtkExtractPolyDataGeometry::svtkExtractPolyDataGeometry(svtkImplicitFunction* f)
{
  this->ImplicitFunction = f;
  if (this->ImplicitFunction)
  {
    this->ImplicitFunction->Register(this);
  }

  this->ExtractInside = 1;
  this->ExtractBoundaryCells = 0;
  this->PassPoints = 0;
}

//----------------------------------------------------------------------------
svtkExtractPolyDataGeometry::~svtkExtractPolyDataGeometry()
{
  this->SetImplicitFunction(nullptr);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
svtkMTimeType svtkExtractPolyDataGeometry::GetMTime()
{
  svtkMTimeType mTime = this->MTime.GetMTime();
  svtkMTimeType impFuncMTime;

  if (this->ImplicitFunction != nullptr)
  {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = (impFuncMTime > mTime ? impFuncMTime : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int svtkExtractPolyDataGeometry::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkPoints* inPts = input->GetPoints();
  svtkIdType numPts, i, cellId = 0, newId, ptId, *pointMap = nullptr;
  float multiplier;
  svtkCellArray *inVerts = nullptr, *inLines = nullptr, *inPolys = nullptr, *inStrips = nullptr;
  svtkCellArray *newVerts = nullptr, *newLines = nullptr, *newPolys = nullptr, *newStrips = nullptr;
  svtkPoints* newPts = nullptr;

  svtkDebugMacro(<< "Extracting poly data geometry");

  if (!this->ImplicitFunction)
  {
    svtkErrorMacro(<< "No implicit function specified");
    return 1;
  }

  numPts = input->GetNumberOfPoints();

  if (this->ExtractInside)
  {
    multiplier = 1.0;
  }
  else
  {
    multiplier = -1.0;
  }

  // Use a templated function to access the points. The points are
  // passed through, but scalar values are generated.
  svtkFloatArray* newScalars = svtkFloatArray::New();
  newScalars->SetNumberOfValues(numPts);

  for (ptId = 0; ptId < numPts; ptId++)
  {
    newScalars->SetValue(
      ptId, this->ImplicitFunction->FunctionValue(inPts->GetPoint(ptId)) * multiplier);
  }

  // Do different things with the points depending on user directive
  if (this->PassPoints)
  {
    output->SetPoints(inPts);
    outputPD->PassData(pd);
  }
  else
  {
    newPts = svtkPoints::New();
    newPts->Allocate(numPts / 4, numPts);
    pointMap = new svtkIdType[numPts]; // maps old point ids into new
    for (ptId = 0; ptId < numPts; ptId++)
    {
      if (newScalars->GetValue(ptId) <= 0.0)
      {
        newId = this->InsertPointInMap(ptId, inPts, newPts, pointMap);
      }
      else
      {
        pointMap[ptId] = -1;
      }
    }
  }
  outputCD->CopyAllocate(cd);

  // Now loop over all cells to see whether they are inside the implicit
  // function. Copy if they are. Note: there is an awful hack here, that
  // can result in bugs. The cellId is assumed to be arranged starting
  // with the verts, then lines, then polys, then strips.
  //
  int numIn;
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  if (input->GetNumberOfVerts())
  {
    inVerts = input->GetVerts();
    newVerts = svtkCellArray::New();
    newVerts->AllocateCopy(inVerts);
  }
  if (input->GetNumberOfLines())
  {
    inLines = input->GetLines();
    newLines = svtkCellArray::New();
    newLines->AllocateCopy(inLines);
  }
  if (input->GetNumberOfPolys())
  {
    inPolys = input->GetPolys();
    newPolys = svtkCellArray::New();
    newPolys->AllocateCopy(inPolys);
  }
  if (input->GetNumberOfStrips())
  {
    inStrips = input->GetStrips();
    newStrips = svtkCellArray::New();
    newStrips->AllocateCopy(inStrips);
  }

  // verts
  if (newVerts && !this->GetAbortExecute())
  {
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts, pts);)
    {
      for (numIn = 0, i = 0; i < npts; i++)
      {
        if (newScalars->GetValue(pts[i]) <= 0.0)
        {
          numIn++;
        }
      }
      if ((numIn == npts) || (this->ExtractBoundaryCells && numIn > 0))
      {
        if (this->PassPoints)
        {
          newId = newVerts->InsertNextCell(npts, pts);
        }
        else
        {
          newId = newVerts->InsertNextCell(npts);
          for (i = 0; i < npts; i++)
          {
            if (pointMap[pts[i]] < 0)
            {
              ptId = this->InsertPointInMap(pts[i], inPts, newPts, pointMap);
            }
            else
            {
              ptId = pointMap[pts[i]];
            }
            newVerts->InsertCellPoint(ptId);
          }
        }
        outputCD->CopyData(cd, cellId, newId);
      }
      cellId++;
    }
  }
  this->UpdateProgress(0.6);

  // lines
  if (newLines && !this->GetAbortExecute())
  {
    for (inLines->InitTraversal(); inLines->GetNextCell(npts, pts);)
    {
      for (numIn = 0, i = 0; i < npts; i++)
      {
        if (newScalars->GetValue(pts[i]) <= 0.0)
        {
          numIn++;
        }
      }
      if ((numIn == npts) || (this->ExtractBoundaryCells && numIn > 0))
      {
        if (this->PassPoints)
        {
          newId = newLines->InsertNextCell(npts, pts);
        }
        else
        {
          newId = newLines->InsertNextCell(npts);
          for (i = 0; i < npts; i++)
          {
            if (pointMap[pts[i]] < 0)
            {
              ptId = this->InsertPointInMap(pts[i], inPts, newPts, pointMap);
            }
            else
            {
              ptId = pointMap[pts[i]];
            }
            newLines->InsertCellPoint(ptId);
          }
        }
        outputCD->CopyData(cd, cellId, newId);
      }
      cellId++;
    }
  }
  this->UpdateProgress(0.75);

  // polys
  if (newPolys && !this->GetAbortExecute())
  {
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
    {
      for (numIn = 0, i = 0; i < npts; i++)
      {
        if (newScalars->GetValue(pts[i]) <= 0.0)
        {
          numIn++;
        }
      }
      if ((numIn == npts) || (this->ExtractBoundaryCells && numIn > 0))
      {
        if (this->PassPoints)
        {
          newId = newPolys->InsertNextCell(npts, pts);
        }
        else
        {
          newId = newPolys->InsertNextCell(npts);
          for (i = 0; i < npts; i++)
          {
            if (pointMap[pts[i]] < 0)
            {
              ptId = this->InsertPointInMap(pts[i], inPts, newPts, pointMap);
            }
            else
            {
              ptId = pointMap[pts[i]];
            }
            newPolys->InsertCellPoint(ptId);
          }
        }
        outputCD->CopyData(cd, cellId, newId);
      }
      cellId++;
    }
  }
  this->UpdateProgress(0.90);

  // strips
  if (newStrips && !this->GetAbortExecute())
  {
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
    {
      for (numIn = 0, i = 0; i < npts; i++)
      {
        if (newScalars->GetValue(pts[i]) <= 0.0)
        {
          numIn++;
        }
      }
      if ((numIn == npts) || (this->ExtractBoundaryCells && numIn > 0))
      {
        if (this->PassPoints)
        {
          newId = newStrips->InsertNextCell(npts, pts);
        }
        else
        {
          newId = newStrips->InsertNextCell(npts);
          for (i = 0; i < npts; i++)
          {
            if (pointMap[pts[i]] < 0)
            {
              ptId = this->InsertPointInMap(pts[i], inPts, newPts, pointMap);
            }
            else
            {
              ptId = pointMap[pts[i]];
            }
            newStrips->InsertCellPoint(ptId);
          }
        }
        outputCD->CopyData(cd, cellId, newId);
      }
      cellId++;
    }
  }
  this->UpdateProgress(1.0);

  // Update ourselves and release memory
  //
  newScalars->Delete();
  if (!this->PassPoints)
  {
    output->SetPoints(newPts);
    newPts->Delete();
    outputPD->CopyAllocate(pd);
    for (i = 0; i < numPts; i++)
    {
      if (pointMap[i] >= 0)
      {
        outputPD->CopyData(pd, i, pointMap[i]);
      }
    }
    delete[] pointMap;
  }

  if (newVerts)
  {
    output->SetVerts(newVerts);
    newVerts->Delete();
  }
  if (newLines)
  {
    output->SetLines(newLines);
    newLines->Delete();
  }
  if (newPolys)
  {
    output->SetPolys(newPolys);
    newPolys->Delete();
  }
  if (newStrips)
  {
    output->SetStrips(newStrips);
    newStrips->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractPolyDataGeometry::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ImplicitFunction)
  {
    os << indent << "Implicit Function: " << static_cast<void*>(this->ImplicitFunction) << "\n";
  }
  else
  {
    os << indent << "Implicit Function: (null)\n";
  }
  os << indent << "Extract Inside: " << (this->ExtractInside ? "On\n" : "Off\n");
  os << indent << "Extract Boundary Cells: " << (this->ExtractBoundaryCells ? "On\n" : "Off\n");
  os << indent << "Pass Points: " << (this->PassPoints ? "On\n" : "Off\n");
}
