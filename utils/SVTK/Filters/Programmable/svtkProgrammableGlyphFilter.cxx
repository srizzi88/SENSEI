/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableGlyphFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProgrammableGlyphFilter.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkTransform.h"

svtkStandardNewMacro(svtkProgrammableGlyphFilter);

// Construct object with scaling on, scaling mode is by scalar value,
// scale factor = 1.0, the range is (0,1), orient geometry is on, and
// orientation is by vector. Clamping and indexing are turned off. No
// initial sources are defined.
svtkProgrammableGlyphFilter::svtkProgrammableGlyphFilter()
{
  this->GlyphMethod = nullptr;
  this->GlyphMethodArgDelete = nullptr;
  this->GlyphMethodArg = nullptr;

  this->Point[0] = this->Point[1] = this->Point[2] = 0.0;
  this->PointId = -1;
  this->PointData = nullptr;

  this->ColorMode = SVTK_COLOR_BY_INPUT;

  this->SetNumberOfInputPorts(2);
}

svtkProgrammableGlyphFilter::~svtkProgrammableGlyphFilter()
{
  if ((this->GlyphMethodArg) && (this->GlyphMethodArgDelete))
  {
    (*this->GlyphMethodArgDelete)(this->GlyphMethodArg);
  }
}

void svtkProgrammableGlyphFilter::SetSourceConnection(svtkAlgorithmOutput* output)
{
  this->SetInputConnection(1, output);
}

void svtkProgrammableGlyphFilter::SetSourceData(svtkPolyData* pd)
{
  this->SetInputData(1, pd);
}

// Get a pointer to a source object at a specified table location.
svtkPolyData* svtkProgrammableGlyphFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

int svtkProgrammableGlyphFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* source = svtkPolyData::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData* inputPD = input->GetPointData();
  svtkCellData* inputCD = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkPoints *newPts, *sourcePts;
  svtkFloatArray *ptScalars = nullptr, *cellScalars = nullptr;
  svtkDataArray *inPtScalars = nullptr, *inCellScalars = nullptr;
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPointData* sourcePD;
  svtkCellData* sourceCD;
  svtkIdType numSourcePts, numSourceCells, ptOffset = 0, cellId, ptId, id, idx;
  int i, npts;
  svtkIdList* pts;
  svtkIdList* cellPts;
  svtkCell* cell;

  // Initialize
  svtkDebugMacro(<< "Generating programmable glyphs!");

  if (numPts < 1)
  {
    svtkErrorMacro(<< "No input points to glyph");
  }

  pts = svtkIdList::New();
  pts->Allocate(SVTK_CELL_SIZE);
  sourcePD = source->GetPointData();
  sourceCD = source->GetCellData();
  numSourcePts = source->GetNumberOfPoints();
  numSourceCells = source->GetNumberOfCells();

  outputPD->CopyScalarsOff(); //'cause we control the coloring process
  outputCD->CopyScalarsOff();

  output->AllocateEstimate(numSourceCells * numPts, 1);
  outputPD->CopyAllocate(sourcePD, numSourcePts * numPts, numSourcePts * numPts);
  outputCD->CopyAllocate(sourceCD, numSourceCells * numPts, numSourceCells * numPts);
  newPts = svtkPoints::New();
  newPts->Allocate(numSourcePts * numPts);

  // figure out how to color the data and setup
  if (this->ColorMode == SVTK_COLOR_BY_INPUT)
  {
    if ((inPtScalars = inputPD->GetScalars()))
    {
      ptScalars = svtkFloatArray::New();
      ptScalars->Allocate(numSourcePts * numPts);
    }
    if ((inCellScalars = inputCD->GetScalars()))
    {
      cellScalars = svtkFloatArray::New();
      cellScalars->Allocate(numSourcePts * numPts);
    }
  }

  else
  {
    if (sourcePD->GetScalars())
    {
      ptScalars = svtkFloatArray::New();
      ptScalars->Allocate(numSourcePts * numPts);
    }
    if (sourceCD->GetScalars())
    {
      cellScalars = svtkFloatArray::New();
      cellScalars->Allocate(numSourcePts * numPts);
    }
  }

  // Loop over all points, invoking glyph method and Update(),
  // then append output of source to output of this filter.
  //
  //  this->Updating = 1; // to prevent infinite recursion
  this->PointData = input->GetPointData();
  for (this->PointId = 0; this->PointId < numPts; this->PointId++)
  {
    if (!(this->PointId % 10000))
    {
      this->UpdateProgress((double)this->PointId / numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    input->GetPoint(this->PointId, this->Point);

    if (this->GlyphMethod)
    {
      (*this->GlyphMethod)(this->GlyphMethodArg);

      // The GlyphMethod may have set the source connection to nullptr
      if (this->GetNumberOfInputConnections(1) == 0)
      {
        source = nullptr;
      }
      else
      {
        // Update the source connection in case the GlyphMethod changed
        // its parameters.
        this->GetInputAlgorithm(1, 0)->Update();
        // The GlyphMethod may also have changed the source.
        sourceInfo = inputVector[1]->GetInformationObject(0);
        source = svtkPolyData::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
      }
    }
    if (source)
    {
      sourcePts = source->GetPoints();
      numSourcePts = source->GetNumberOfPoints();
      numSourceCells = source->GetNumberOfCells();
      sourcePD = source->GetPointData();
      sourceCD = source->GetCellData();

      if (this->ColorMode == SVTK_COLOR_BY_SOURCE)
      {
        inPtScalars = sourcePD->GetScalars();
        inCellScalars = sourceCD->GetScalars();
      }

      // Copy all data from source to output.
      for (ptId = 0; ptId < numSourcePts; ptId++)
      {
        id = newPts->InsertNextPoint(sourcePts->GetPoint(ptId));
        outputPD->CopyData(sourcePD, ptId, id);
      }

      for (cellId = 0; cellId < numSourceCells; cellId++)
      {
        cell = source->GetCell(cellId);
        cellPts = cell->GetPointIds();
        npts = cellPts->GetNumberOfIds();
        for (pts->Reset(), i = 0; i < npts; i++)
        {
          pts->InsertId(i, cellPts->GetId(i) + ptOffset);
        }
        id = output->InsertNextCell(cell->GetCellType(), pts);
        outputCD->CopyData(sourceCD, cellId, id);
      }

      // If we're coloring the output with scalars, do that now
      if (ptScalars)
      {
        for (ptId = 0; ptId < numSourcePts; ptId++)
        {
          idx = (this->ColorMode == SVTK_COLOR_BY_INPUT ? this->PointId : ptId);
          ptScalars->InsertNextValue(inPtScalars->GetComponent(idx, 0));
        }
      }
      else if (cellScalars)
      {
        for (cellId = 0; cellId < numSourceCells; cellId++)
        {
          idx = (this->ColorMode == SVTK_COLOR_BY_INPUT ? this->PointId : cellId);
          cellScalars->InsertNextValue(inCellScalars->GetComponent(idx, 0));
        }
      }

      ptOffset += numSourcePts;

    } // if a source is available
  }   // for all input points

  //  this->Updating = 0;

  pts->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  if (ptScalars)
  {
    idx = outputPD->AddArray(ptScalars);
    outputPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    ptScalars->Delete();
  }

  if (cellScalars)
  {
    idx = outputCD->AddArray(cellScalars);
    outputCD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    cellScalars->Delete();
  }

  output->Squeeze();

  return 1;
}

// Specify function to be called before object executes.
void svtkProgrammableGlyphFilter::SetGlyphMethod(void (*f)(void*), void* arg)
{
  if (f != this->GlyphMethod || arg != this->GlyphMethodArg)
  {
    // delete the current arg if there is one and a delete meth
    if ((this->GlyphMethodArg) && (this->GlyphMethodArgDelete))
    {
      (*this->GlyphMethodArgDelete)(this->GlyphMethodArg);
    }
    this->GlyphMethod = f;
    this->GlyphMethodArg = arg;
    this->Modified();
  }
}

// Set the arg delete method. This is used to free user memory.
void svtkProgrammableGlyphFilter::SetGlyphMethodArgDelete(void (*f)(void*))
{
  if (f != this->GlyphMethodArgDelete)
  {
    this->GlyphMethodArgDelete = f;
    this->Modified();
  }
}

// Description:
// Return the method of coloring as a descriptive character string.
const char* svtkProgrammableGlyphFilter::GetColorModeAsString()
{
  if (this->ColorMode == SVTK_COLOR_BY_INPUT)
  {
    return "ColorByInput";
  }
  else
  {
    return "ColorBySource";
  }
}

int svtkProgrammableGlyphFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    return 1;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}

void svtkProgrammableGlyphFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;
  os << indent << "Point Id: " << this->PointId << "\n";
  os << indent << "Point: " << this->Point[0] << ", " << this->Point[1] << ", " << this->Point[2]
     << "\n";
  if (this->PointData)
  {
    os << indent << "PointData: " << this->PointData << "\n";
  }
  else
  {
    os << indent << "PointData: (not defined)\n";
  }

  if (this->GlyphMethod)
  {
    os << indent << "Glyph Method defined\n";
  }
  else
  {
    os << indent << "No Glyph Method\n";
  }
}
