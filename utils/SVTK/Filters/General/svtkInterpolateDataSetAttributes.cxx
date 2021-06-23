/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInterpolateDataSetAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInterpolateDataSetAttributes.h"

#include "svtkCellData.h"
#include "svtkDataSetCollection.h"
#include "svtkExecutive.h"
#include "svtkGarbageCollector.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkInterpolateDataSetAttributes);

// Create object with no input or output.
svtkInterpolateDataSetAttributes::svtkInterpolateDataSetAttributes()
{
  this->InputList = svtkDataSetCollection::New();

  this->T = 0.0;
}

svtkInterpolateDataSetAttributes::~svtkInterpolateDataSetAttributes()
{
  if (this->InputList)
  {
    this->InputList->Delete();
    this->InputList = nullptr;
  }
}

svtkDataSetCollection* svtkInterpolateDataSetAttributes::GetInputList()
{
  int i;
  this->InputList->RemoveAllItems();

  for (i = 0; i < this->GetNumberOfInputConnections(0); i++)
  {
    this->InputList->AddItem(static_cast<svtkDataSet*>(this->GetExecutive()->GetInputData(0, i)));
  }
  return this->InputList;
}

// Interpolate the data
int svtkInterpolateDataSetAttributes::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numPts, numCells, i;
  int numInputs = this->GetNumberOfInputConnections(0);
  int lowDS, highDS;
  svtkDataSet *ds, *ds2;
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkPointData *inputPD, *input2PD;
  svtkCellData *inputCD, *input2CD;
  double t;

  if (numInputs < 2)
  {
    svtkErrorMacro(<< "Need at least two inputs to interpolate!");
    return 1;
  }

  svtkDebugMacro(<< "Interpolating data...");

  // Check input and determine between which data sets the interpolation
  // is to occur.
  if (this->T > static_cast<double>(numInputs))
  {
    svtkErrorMacro(<< "Bad interpolation parameter");
    return 1;
  }

  lowDS = static_cast<int>(this->T);
  if (lowDS >= (numInputs - 1))
  {
    lowDS = numInputs - 2;
  }

  highDS = lowDS + 1;
  t = this->T - static_cast<double>(lowDS);
  if (t > 1.0)
  {
    t = 1.0;
  }

  svtkInformation* dsInfo = inputVector[0]->GetInformationObject(lowDS);
  svtkInformation* ds2Info = inputVector[0]->GetInformationObject(highDS);
  ds = svtkDataSet::SafeDownCast(dsInfo->Get(svtkDataObject::DATA_OBJECT()));
  ds2 = svtkDataSet::SafeDownCast(ds2Info->Get(svtkDataObject::DATA_OBJECT()));

  numPts = ds->GetNumberOfPoints();
  numCells = ds->GetNumberOfCells();

  if (numPts != ds2->GetNumberOfPoints() || numCells != ds2->GetNumberOfCells())
  {
    svtkErrorMacro(<< "Data sets not consistent!");
    return 1;
  }

  output->CopyStructure(ds);
  inputPD = ds->GetPointData();
  inputCD = ds->GetCellData();
  input2PD = ds2->GetPointData();
  input2CD = ds2->GetCellData();

  // Allocate the data set attributes
  outputPD->CopyAllOff();
  if (inputPD->GetScalars() && input2PD->GetScalars())
  {
    outputPD->CopyScalarsOn();
  }
  if (inputPD->GetVectors() && input2PD->GetVectors())
  {
    outputPD->CopyVectorsOn();
  }
  if (inputPD->GetNormals() && input2PD->GetNormals())
  {
    outputPD->CopyNormalsOn();
  }
  if (inputPD->GetTCoords() && input2PD->GetTCoords())
  {
    outputPD->CopyTCoordsOn();
  }
  if (inputPD->GetTensors() && input2PD->GetTensors())
  {
    outputPD->CopyTensorsOn();
  }
  // *TODO* Fix
  // if ( inputPD->GetFieldData() && input2PD->GetFieldData() )
  //{
  // outputPD->CopyFieldDataOn();
  //}
  outputPD->InterpolateAllocate(inputPD);

  outputCD->CopyAllOff();
  if (inputCD->GetScalars() && input2CD->GetScalars())
  {
    outputCD->CopyScalarsOn();
  }
  if (inputCD->GetVectors() && input2CD->GetVectors())
  {
    outputCD->CopyVectorsOn();
  }
  if (inputCD->GetNormals() && input2CD->GetNormals())
  {
    outputCD->CopyNormalsOn();
  }
  if (inputCD->GetTCoords() && input2CD->GetTCoords())
  {
    outputCD->CopyTCoordsOn();
  }
  if (inputCD->GetTensors() && input2CD->GetTensors())
  {
    outputCD->CopyTensorsOn();
  }
  // *TODO* Fix
  // if ( inputCD->GetFieldData() && input2CD->GetFieldData() )
  //{
  // outputCD->CopyFieldDataOn();
  //  }
  outputCD->InterpolateAllocate(inputCD);

  // Interpolate point data. We'll assume that it takes 50% of the time
  for (i = 0; i < numPts; i++)
  {
    if (!(i % 10000))
    {
      this->UpdateProgress(static_cast<double>(i) / numPts * 0.50);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    outputPD->InterpolateTime(inputPD, input2PD, i, t);
  }

  // Interpolate cell data. We'll assume that it takes 50% of the time
  for (i = 0; i < numCells; i++)
  {
    if (!(i % 10000))
    {
      this->UpdateProgress(0.5 + static_cast<double>(i) / numCells * 0.50);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    outputCD->InterpolateTime(inputCD, input2CD, i, t);
  }

  return 1;
}

int svtkInterpolateDataSetAttributes::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

void svtkInterpolateDataSetAttributes::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "T: " << this->T << endl;
}

//----------------------------------------------------------------------------
void svtkInterpolateDataSetAttributes ::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->InputList, "InputList");
}
