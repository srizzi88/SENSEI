/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProcessIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProcessIdScalars.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkProcessIdScalars);

svtkCxxSetObjectMacro(svtkProcessIdScalars, Controller, svtkMultiProcessController);

//----------------------------------------------------------------------------
svtkProcessIdScalars::svtkProcessIdScalars()
{
  this->CellScalarsFlag = 0;
  this->RandomMode = 0;

  this->Controller = svtkMultiProcessController::GetGlobalController();
  if (this->Controller)
  {
    this->Controller->Register(this);
  }
}

//----------------------------------------------------------------------------
svtkProcessIdScalars::~svtkProcessIdScalars()
{
  if (this->Controller)
  {
    this->Controller->Delete();
    this->Controller = nullptr;
  }
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int svtkProcessIdScalars::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* pieceColors;
  svtkIdType num;

  if (this->CellScalarsFlag)
  {
    num = input->GetNumberOfCells();
  }
  else
  {
    num = input->GetNumberOfPoints();
  }

  int piece = (this->Controller ? this->Controller->GetLocalProcessId() : 0);

  if (this->RandomMode)
  {
    pieceColors = this->MakeRandomScalars(piece, num);
  }
  else
  {
    pieceColors = this->MakeProcessIdScalars(piece, num);
  }

  output->ShallowCopy(input);
  pieceColors->SetName("ProcessId");
  if (this->CellScalarsFlag)
  {
    output->GetCellData()->AddArray(pieceColors);
    output->GetCellData()->SetActiveScalars(pieceColors->GetName());
  }
  else
  {
    output->GetPointData()->AddArray(pieceColors);
    output->GetPointData()->SetActiveScalars(pieceColors->GetName());
  }

  pieceColors->Delete();

  return 1;
}

//----------------------------------------------------------------------------
svtkIntArray* svtkProcessIdScalars::MakeProcessIdScalars(int piece, svtkIdType num)
{
  svtkIntArray* pieceColors = svtkIntArray::New();
  pieceColors->SetNumberOfTuples(num);

  for (svtkIdType i = 0; i < num; ++i)
  {
    pieceColors->SetValue(i, piece);
  }

  return pieceColors;
}

//----------------------------------------------------------------------------
svtkFloatArray* svtkProcessIdScalars::MakeRandomScalars(int piece, svtkIdType num)
{
  svtkMath::RandomSeed(piece);
  float randomValue = svtkMath::Random();

  svtkFloatArray* pieceColors = svtkFloatArray::New();
  pieceColors->SetNumberOfTuples(num);

  for (svtkIdType i = 0; i < num; ++i)
  {
    pieceColors->SetValue(i, randomValue);
  }

  return pieceColors;
}

//----------------------------------------------------------------------------
void svtkProcessIdScalars::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RandomMode: " << this->RandomMode << endl;
  if (this->CellScalarsFlag)
  {
    os << indent << "ScalarMode: CellData\n";
  }
  else
  {
    os << indent << "ScalarMode: PointData\n";
  }

  os << indent << "Controller: ";
  if (this->Controller)
  {
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}
