/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOBBDicer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOBBDicer.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkOBBTree.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkShortArray.h"

svtkStandardNewMacro(svtkOBBDicer);

void svtkOBBDicer::BuildTree(svtkIdList* ptIds, svtkOBBNode* OBBptr, svtkDataSet* input)
{
  svtkIdType i, numPts = ptIds->GetNumberOfIds();
  svtkIdType ptId;
  svtkOBBTree* OBB = svtkOBBTree::New();

  double size[3];

  // Gather all the points into a single list
  //
  for (this->PointsList->Reset(), i = 0; i < numPts; i++)
  {
    ptId = ptIds->GetId(i);
    this->PointsList->InsertNextPoint(input->GetPoint(ptId));
  }

  // Now compute the OBB
  //
  OBB->ComputeOBB(
    this->PointsList, OBBptr->Corner, OBBptr->Axes[0], OBBptr->Axes[1], OBBptr->Axes[2], size);
  OBB->Delete();

  // Check whether to continue recursing; if so, create two children and
  // assign cells to appropriate child.
  //
  if (numPts > this->NumberOfPointsPerPiece)
  {
    svtkOBBNode* LHnode = new svtkOBBNode;
    svtkOBBNode* RHnode = new svtkOBBNode;
    OBBptr->Kids = new svtkOBBNode*[2];
    OBBptr->Kids[0] = LHnode;
    OBBptr->Kids[1] = RHnode;
    svtkIdList* LHlist = svtkIdList::New();
    LHlist->Allocate(numPts / 2);
    svtkIdList* RHlist = svtkIdList::New();
    RHlist->Allocate(numPts / 2);
    LHnode->Parent = OBBptr;
    RHnode->Parent = OBBptr;
    double n[3], p[3], x[3], val;

    // split the longest axis down the middle
    for (i = 0; i < 3; i++) // compute split point
    {
      p[i] = OBBptr->Corner[i] + OBBptr->Axes[0][i] / 2.0 + OBBptr->Axes[1][i] / 2.0 +
        OBBptr->Axes[2][i] / 2.0;
    }

    // compute split normal
    for (i = 0; i < 3; i++)
    {
      n[i] = OBBptr->Axes[0][i];
    }
    svtkMath::Normalize(n);

    // traverse cells, assigning to appropriate child list as necessary
    for (i = 0; i < numPts; i++)
    {
      ptId = ptIds->GetId(i);
      input->GetPoint(ptId, x);
      val = n[0] * (x[0] - p[0]) + n[1] * (x[1] - p[1]) + n[2] * (x[2] - p[2]);

      if (val < 0.0)
      {
        LHlist->InsertNextId(ptId);
      }
      else
      {
        RHlist->InsertNextId(ptId);
      }

    } // for all points

    ptIds->Delete(); // don't need to keep anymore
    this->BuildTree(LHlist, LHnode, input);
    this->BuildTree(RHlist, RHnode, input);
  } // if should build tree

  else // terminate recursion
  {
    ptIds->Squeeze();
    OBBptr->Cells = ptIds;
  }
}

// Current implementation uses an OBBTree to split up the dataset.
int svtkOBBDicer::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkIdType ptId, numPts;
  svtkIdList* ptIds;
  svtkShortArray* groupIds;
  svtkOBBNode* root;
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Dicing object");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if ((numPts = input->GetNumberOfPoints()) < 1)
  {
    svtkErrorMacro(<< "No data to dice!");
    return 1;
  }

  // The superclass computes piece size limits based on filter ivars
  this->UpdatePieceMeasures(input);

  // Create list of points
  //
  this->PointsList = svtkPoints::New();
  this->PointsList->Allocate(numPts);
  ptIds = svtkIdList::New();
  ptIds->SetNumberOfIds(numPts);
  for (ptId = 0; ptId < numPts; ptId++)
  {
    ptIds->SetId(ptId, ptId);
  }

  root = new svtkOBBNode;
  this->BuildTree(ptIds, root, input);

  // Generate scalar values
  //
  this->PointsList->Delete();
  this->PointsList = nullptr;
  groupIds = svtkShortArray::New();
  groupIds->SetNumberOfTuples(numPts);
  groupIds->SetName("svtkOBBDicer_GroupIds");
  this->NumberOfActualPieces = 0;
  this->MarkPoints(root, groupIds);
  this->DeleteTree(root);
  delete root;

  svtkDebugMacro(<< "Created " << this->NumberOfActualPieces << " pieces");

  // Update self
  //
  if (this->FieldData)
  {
    output->GetPointData()->AddArray(groupIds);
    output->GetPointData()->CopyFieldOff("svtkOBBDicer_GroupIds");
    output->GetPointData()->PassData(input->GetPointData());
  }
  else
  {
    output->GetPointData()->AddArray(groupIds);
    output->GetPointData()->SetActiveScalars(groupIds->GetName());
    output->GetPointData()->CopyScalarsOff();
    output->GetPointData()->PassData(input->GetPointData());
  }

  output->GetCellData()->PassData(input->GetCellData());

  groupIds->Delete();

  return 1;
}

void svtkOBBDicer::MarkPoints(svtkOBBNode* OBBptr, svtkShortArray* groupIds)
{
  if (OBBptr->Kids == nullptr) // leaf OBB
  {
    svtkIdList* ptIds;
    svtkIdType i, ptId, numIds;

    ptIds = OBBptr->Cells;
    if ((numIds = ptIds->GetNumberOfIds()) > 0)
    {
      for (i = 0; i < numIds; i++)
      {
        ptId = ptIds->GetId(i);
        groupIds->SetValue(ptId, this->NumberOfActualPieces);
      }
      this->NumberOfActualPieces++;
    } // if any points in this leaf OBB
  }
  else
  {
    this->MarkPoints(OBBptr->Kids[0], groupIds);
    this->MarkPoints(OBBptr->Kids[1], groupIds);
  }
}

void svtkOBBDicer::DeleteTree(svtkOBBNode* OBBptr)
{
  if (OBBptr->Kids != nullptr)
  {
    this->DeleteTree(OBBptr->Kids[0]);
    this->DeleteTree(OBBptr->Kids[1]);
    delete OBBptr->Kids[0];
    delete OBBptr->Kids[1];
  }
}

void svtkOBBDicer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
