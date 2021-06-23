/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOutlineFilterInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPOutlineFilterInternals.h"

#include "svtkAMRInformation.h"
#include "svtkAppendPolyData.h"
#include "svtkBoundingBox.h"
#include "svtkCompositeDataIterator.h"
#include "svtkDataObjectTree.h"
#include "svtkGraph.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineCornerSource.h"
#include "svtkOutlineSource.h"
#include "svtkOverlappingAMR.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkUniformGrid.h"

// ----------------------------------------------------------------------------
class AddBoundsListOperator : public svtkCommunicator::Operation
{
  // Description:
  // Performs a "B.AddBounds(A)" operation.
  void Function(const void* A, void* B, svtkIdType length, int datatype) override
  {
    (void)datatype;
    assert((datatype == SVTK_DOUBLE) && (length % 6 == 0));
    assert("pre: A vector is nullptr" && (A != nullptr));
    assert("pre: B vector is nullptr" && (B != nullptr));
    svtkBoundingBox box;
    const double* aPtr = reinterpret_cast<const double*>(A);
    double* bPtr = reinterpret_cast<double*>(B);
    for (svtkIdType idx = 0; idx < length; idx += 6)
    {
      box.SetBounds(&bPtr[idx]);
      box.AddBounds(&aPtr[idx]);
      box.GetBounds(&bPtr[idx]);
    }
  }

  // Description:
  // Sets Commutative to true for this operation
  int Commutative() override { return 1; }
};

// ----------------------------------------------------------------------------
svtkPOutlineFilterInternals::svtkPOutlineFilterInternals()
{
  this->Controller = nullptr;
  this->IsCornerSource = false;
  this->CornerFactor = 0.2;
}

// ----------------------------------------------------------------------------
svtkPOutlineFilterInternals::~svtkPOutlineFilterInternals()
{
  this->Controller = nullptr;
}

// ----------------------------------------------------------------------------
void svtkPOutlineFilterInternals::SetController(svtkMultiProcessController* controller)
{
  this->Controller = controller;
}

// ----------------------------------------------------------------------------
void svtkPOutlineFilterInternals::SetCornerFactor(double cornerFactor)
{
  this->CornerFactor = cornerFactor;
}

// ----------------------------------------------------------------------------
void svtkPOutlineFilterInternals::SetIsCornerSource(bool value)
{
  this->IsCornerSource = value;
}

// ----------------------------------------------------------------------------
int svtkPOutlineFilterInternals::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataObject* input = svtkDataObject::GetData(inputVector[0], 0);
  svtkPolyData* output = svtkPolyData::GetData(outputVector, 0);

  if (input == nullptr || output == nullptr)
  {
    svtkGenericWarningMacro("Missing input or output.");
    return 0;
  }

  if (this->Controller == nullptr)
  {
    svtkGenericWarningMacro("Missing Controller.");
    return 0;
  }

  svtkOverlappingAMR* oamr = svtkOverlappingAMR::SafeDownCast(input);
  if (oamr)
  {
    return this->RequestData(oamr, output);
  }

  svtkUniformGridAMR* amr = svtkUniformGridAMR::SafeDownCast(input);
  if (amr)
  {
    return this->RequestData(amr, output);
  }

  svtkDataObjectTree* cd = svtkDataObjectTree::SafeDownCast(input);
  if (cd)
  {
    return this->RequestData(cd, output);
  }

  svtkDataSet* ds = svtkDataSet::SafeDownCast(input);
  if (ds)
  {
    return this->RequestData(ds, output);
  }

  svtkGraph* graph = svtkGraph::SafeDownCast(input);
  if (graph)
  {
    return this->RequestData(graph, output);
  }
  return 0;
}

// ----------------------------------------------------------------------------
void svtkPOutlineFilterInternals::CollectCompositeBounds(svtkDataObject* input)
{
  svtkDataSet* ds = svtkDataSet::SafeDownCast(input);
  svtkCompositeDataSet* compInput = svtkCompositeDataSet::SafeDownCast(input);
  if (ds != nullptr)
  {
    double bounds[6];
    ds->GetBounds(bounds);
    this->BoundsList.push_back(svtkBoundingBox(bounds));
  }
  else if (compInput != nullptr)
  {
    svtkCompositeDataIterator* iter = compInput->NewIterator();
    iter->SkipEmptyNodesOff();
    iter->GoToFirstItem();
    for (; !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      this->CollectCompositeBounds(iter->GetCurrentDataObject());
    }
    iter->Delete();
  }
  else
  {
    double bounds[6];
    svtkMath::UninitializeBounds(bounds);
    this->BoundsList.push_back(bounds);
  }
}

// ----------------------------------------------------------------------------
int svtkPOutlineFilterInternals::RequestData(svtkDataObjectTree* input, svtkPolyData* output)
{
  // Check Output and Input

  // Collect local bounds.
  this->CollectCompositeBounds(input);

  // Make an array of bounds from collected bounds
  std::vector<double> boundsList;
  boundsList.resize(6 * this->BoundsList.size());

  for (size_t i = 0; i < this->BoundsList.size(); ++i)
  {
    this->BoundsList[i].GetBounds(&boundsList[i * 6]);
  }

  // Collect global bounds and copy into the array
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    AddBoundsListOperator operation;
    double* temp = new double[6 * this->BoundsList.size()];
    this->Controller->Reduce(
      &boundsList[0], temp, static_cast<svtkIdType>(6 * this->BoundsList.size()), &operation, 0);
    memcpy(&boundsList[0], temp, 6 * this->BoundsList.size() * sizeof(double));
    delete[] temp;

    if (this->Controller->GetLocalProcessId() > 0)
    {
      // only root node will produce the output.
      return 1;
    }
  }

  // Make output with collected bounds
  svtkNew<svtkAppendPolyData> appender;
  for (size_t i = 0; i < 6 * this->BoundsList.size(); i += 6)
  {
    svtkBoundingBox box(&boundsList[i]);
    if (box.IsValid())
    {
      if (this->IsCornerSource)
      {
        svtkNew<svtkOutlineCornerSource> corner;
        corner->SetBounds(&boundsList[i]);
        corner->SetCornerFactor(this->CornerFactor);
        corner->Update();
        appender->AddInputData(corner->GetOutput());
      }
      else
      {
        svtkNew<svtkOutlineSource> corner;
        corner->SetBounds(&boundsList[i]);
        corner->Update();
        appender->AddInputData(corner->GetOutput());
      }
    }
  }

  if (appender->GetNumberOfInputConnections(0) > 1)
  {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
  }
  return 1;
}

// ----------------------------------------------------------------------------
int svtkPOutlineFilterInternals::RequestData(svtkOverlappingAMR* input, svtkPolyData* output)
{
  // For Overlapping AMR, we have meta-data on all processes for the complete
  // AMR structure. Hence root node can build the outlines using that
  // meta-data, itself.
  int procid = this->Controller->GetLocalProcessId();
  if (procid != 0)
  {
    // we only generate output on the root node.
    return 1;
  }

  svtkNew<svtkAppendPolyData> appender;
  for (unsigned int level = 0; level < input->GetNumberOfLevels(); ++level)
  {
    unsigned int num_datasets = input->GetNumberOfDataSets(level);
    for (unsigned int dataIdx = 0; dataIdx < num_datasets; ++dataIdx)
    {
      double bounds[6];
      input->GetAMRInfo()->GetBounds(level, dataIdx, bounds);

      // Check if the bounds received are not default bounding box
      if (svtkBoundingBox::IsValid(bounds))
      {
        if (this->IsCornerSource)
        {
          svtkNew<svtkOutlineCornerSource> corner;
          corner->SetBounds(bounds);
          corner->SetCornerFactor(this->CornerFactor);
          corner->Update();
          appender->AddInputData(corner->GetOutput());
        }
        else
        {
          svtkNew<svtkOutlineSource> corner;
          corner->SetBounds(bounds);
          corner->Update();
          appender->AddInputData(corner->GetOutput());
        }
      }
    }
  }
  if (appender->GetNumberOfInputConnections(0) > 1)
  {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
  }
  return 1;
}

// ----------------------------------------------------------------------------
int svtkPOutlineFilterInternals::RequestData(svtkUniformGridAMR* input, svtkPolyData* output)
{
  // All processes simply produce the outline for the non-null blocks that exist
  // on the process.
  svtkNew<svtkAppendPolyData> appender;
  unsigned int block_id = 0;
  for (unsigned int level = 0; level < input->GetNumberOfLevels(); ++level)
  {
    unsigned int num_datasets = input->GetNumberOfDataSets(level);
    for (unsigned int dataIdx = 0; dataIdx < num_datasets; ++dataIdx, block_id++)
    {
      svtkUniformGrid* ug = input->GetDataSet(level, dataIdx);
      if (ug)
      {
        double bounds[6];
        ug->GetBounds(bounds);

        // Check if the bounds received are not default bounding box
        if (svtkBoundingBox::IsValid(bounds))
        {
          if (this->IsCornerSource)
          {
            svtkNew<svtkOutlineCornerSource> corner;
            corner->SetBounds(bounds);
            corner->SetCornerFactor(this->CornerFactor);
            corner->Update();
            appender->AddInputData(corner->GetOutput());
          }
          else
          {
            svtkNew<svtkOutlineSource> corner;
            corner->SetBounds(bounds);
            corner->Update();
            appender->AddInputData(corner->GetOutput());
          }
        }
      }
    }
  }
  if (appender->GetNumberOfInputConnections(0) > 1)
  {
    appender->Update();
    output->ShallowCopy(appender->GetOutput());
  }
  return 1;
}

// ----------------------------------------------------------------------------
int svtkPOutlineFilterInternals::RequestData(svtkDataSet* input, svtkPolyData* output)
{
  double bounds[6];
  input->GetBounds(bounds);

  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    double reduced_bounds[6];
    int procid = this->Controller->GetLocalProcessId();
    AddBoundsListOperator operation;
    this->Controller->Reduce(bounds, reduced_bounds, 6, &operation, 0);
    if (procid > 0)
    {
      // Satellite node
      return 1;
    }
    memcpy(bounds, reduced_bounds, 6 * sizeof(double));
  }

  if (svtkMath::AreBoundsInitialized(bounds))
  {
    // only output in process 0.
    if (this->IsCornerSource)
    {
      svtkNew<svtkOutlineCornerSource> corner;
      corner->SetBounds(bounds);
      corner->SetCornerFactor(this->CornerFactor);
      corner->Update();
      output->ShallowCopy(corner->GetOutput());
    }
    else
    {
      svtkNew<svtkOutlineSource> corner;
      corner->SetBounds(bounds);
      corner->Update();
      output->ShallowCopy(corner->GetOutput());
    }
  }

  return 1;
}

// ----------------------------------------------------------------------------
int svtkPOutlineFilterInternals::RequestData(svtkGraph* input, svtkPolyData* output)
{
  double bounds[6];
  input->GetBounds(bounds);

  if (this->Controller->GetNumberOfProcesses() > 1)
  {
    double reduced_bounds[6];
    int procid = this->Controller->GetLocalProcessId();
    AddBoundsListOperator operation;
    this->Controller->Reduce(bounds, reduced_bounds, 6, &operation, 0);
    if (procid > 0)
    {
      // Satellite node
      return 1;
    }
    memcpy(bounds, reduced_bounds, 6 * sizeof(double));
  }

  if (svtkMath::AreBoundsInitialized(bounds))
  {
    // only output in process 0.
    if (this->IsCornerSource)
    {
      svtkNew<svtkOutlineCornerSource> corner;
      corner->SetBounds(bounds);
      corner->SetCornerFactor(this->CornerFactor);
      corner->Update();
      output->ShallowCopy(corner->GetOutput());
    }
    else
    {
      svtkNew<svtkOutlineSource> corner;
      corner->SetBounds(bounds);
      corner->Update();
      output->ShallowCopy(corner->GetOutput());
    }
  }

  return 1;
}
