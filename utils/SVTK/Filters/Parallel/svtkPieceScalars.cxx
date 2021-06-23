/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPieceScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPieceScalars.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPieceScalars);

//----------------------------------------------------------------------------
svtkPieceScalars::svtkPieceScalars()
{
  this->CellScalarsFlag = 0;
  this->RandomMode = 0;
}

//----------------------------------------------------------------------------
svtkPieceScalars::~svtkPieceScalars() = default;

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int svtkPieceScalars::RequestData(svtkInformation* svtkNotUsed(request),
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

  int piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  if (this->RandomMode)
  {
    pieceColors = this->MakeRandomScalars(piece, num);
  }
  else
  {
    pieceColors = this->MakePieceScalars(piece, num);
  }

  output->ShallowCopy(input);
  pieceColors->SetName("Piece");
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
svtkIntArray* svtkPieceScalars::MakePieceScalars(int piece, svtkIdType num)
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
svtkFloatArray* svtkPieceScalars::MakeRandomScalars(int piece, svtkIdType num)
{
  svtkMath::RandomSeed(piece);
  float randomValue = static_cast<float>(svtkMath::Random());

  svtkFloatArray* pieceColors = svtkFloatArray::New();
  pieceColors->SetNumberOfTuples(num);

  for (svtkIdType i = 0; i < num; ++i)
  {
    pieceColors->SetValue(i, randomValue);
  }

  return pieceColors;
}

//----------------------------------------------------------------------------
void svtkPieceScalars::PrintSelf(ostream& os, svtkIndent indent)
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
}
