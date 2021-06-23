/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProgrammableFilter.h"

#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkTable.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkProgrammableFilter);

// Construct programmable filter with empty execute method.
svtkProgrammableFilter::svtkProgrammableFilter()
{
  this->ExecuteMethod = nullptr;
  this->ExecuteMethodArg = nullptr;
  this->ExecuteMethodArgDelete = nullptr;
  this->CopyArrays = false;
}

svtkProgrammableFilter::~svtkProgrammableFilter()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
  {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
  }
}

// Get the input as a concrete type. This method is typically used by the
// writer of the filter function to get the input as a particular type (i.e.,
// it essentially does type casting). It is the users responsibility to know
// the correct type of the input data.
svtkPolyData* svtkProgrammableFilter::GetPolyDataInput()
{
  return static_cast<svtkPolyData*>(this->GetInput());
}

// Get the input as a concrete type.
svtkStructuredPoints* svtkProgrammableFilter::GetStructuredPointsInput()
{
  return static_cast<svtkStructuredPoints*>(this->GetInput());
}

// Get the input as a concrete type.
svtkStructuredGrid* svtkProgrammableFilter::GetStructuredGridInput()
{
  return static_cast<svtkStructuredGrid*>(this->GetInput());
}

// Get the input as a concrete type.
svtkUnstructuredGrid* svtkProgrammableFilter::GetUnstructuredGridInput()
{
  return static_cast<svtkUnstructuredGrid*>(this->GetInput());
}

// Get the input as a concrete type.
svtkRectilinearGrid* svtkProgrammableFilter::GetRectilinearGridInput()
{
  return static_cast<svtkRectilinearGrid*>(this->GetInput());
}

// Get the input as a concrete type.
svtkGraph* svtkProgrammableFilter::GetGraphInput()
{
  return static_cast<svtkGraph*>(this->GetInput());
}

// Get the input as a concrete type.
svtkMolecule* svtkProgrammableFilter::GetMoleculeInput()
{
  return static_cast<svtkMolecule*>(this->GetInput());
}

// Get the input as a concrete type.
svtkTable* svtkProgrammableFilter::GetTableInput()
{
  return static_cast<svtkTable*>(this->GetInput());
}

// Specify the function to use to operate on the point attribute data. Note
// that the function takes a single (void *) argument.
void svtkProgrammableFilter::SetExecuteMethod(void (*f)(void*), void* arg)
{
  if (f != this->ExecuteMethod || arg != this->ExecuteMethodArg)
  {
    // delete the current arg if there is one and a delete meth
    if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
    {
      (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
    this->ExecuteMethod = f;
    this->ExecuteMethodArg = arg;
    this->Modified();
  }
}

// Set the arg delete method. This is used to free user memory.
void svtkProgrammableFilter::SetExecuteMethodArgDelete(void (*f)(void*))
{
  if (f != this->ExecuteMethodArgDelete)
  {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
  }
}

int svtkProgrammableFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = nullptr;
  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    inInfo = inputVector[0]->GetInformationObject(0);
  }
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  if (inInfo)
  {
    svtkDataObject* objInput = inInfo->Get(svtkDataObject::DATA_OBJECT());
    if (svtkDataSet::SafeDownCast(objInput))
    {
      svtkDataSet* dsInput = svtkDataSet::SafeDownCast(objInput);
      svtkDataSet* dsOutput = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (dsInput && dsOutput && dsInput->GetDataObjectType() == dsOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          dsOutput->ShallowCopy(dsInput);
        }
        else
        {
          dsOutput->CopyStructure(dsInput);
        }
      }
    }
    if (svtkGraph::SafeDownCast(objInput))
    {
      svtkGraph* graphInput = svtkGraph::SafeDownCast(objInput);
      svtkGraph* graphOutput = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (graphInput && graphOutput &&
        graphInput->GetDataObjectType() == graphOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          graphOutput->ShallowCopy(graphInput);
        }
        else
        {
          graphOutput->CopyStructure(graphInput);
        }
      }
    }
    if (svtkMolecule::SafeDownCast(objInput))
    {
      svtkMolecule* molInput = svtkMolecule::SafeDownCast(objInput);
      svtkMolecule* molOutput =
        svtkMolecule::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (molInput && molOutput && molInput->GetDataObjectType() == molOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          molOutput->ShallowCopy(molInput);
        }
        else
        {
          molOutput->CopyStructure(molInput);
        }
      }
    }
    if (svtkTable::SafeDownCast(objInput))
    {
      svtkTable* tableInput = svtkTable::SafeDownCast(objInput);
      svtkTable* tableOutput = svtkTable::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (tableInput && tableOutput &&
        tableInput->GetDataObjectType() == tableOutput->GetDataObjectType())
      {
        if (this->CopyArrays)
        {
          tableOutput->ShallowCopy(tableInput);
        }
      }
    }
    if (svtkCompositeDataSet::SafeDownCast(objInput))
    {
      svtkCompositeDataSet* cdsInput = svtkCompositeDataSet::SafeDownCast(objInput);
      svtkCompositeDataSet* cdsOutput =
        svtkCompositeDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
      // First, copy the input to the output as a starting point
      if (cdsInput && cdsOutput && cdsInput->GetDataObjectType() == cdsOutput->GetDataObjectType())
      {
        cdsOutput->CopyStructure(cdsInput);
        svtkCompositeDataIterator* iter = cdsInput->NewIterator();
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          svtkDataObject* iblock = iter->GetCurrentDataObject();
          svtkDataObject* oblock = iblock->NewInstance();
          if (iblock)
          {
            if (this->CopyArrays)
            {
              oblock->ShallowCopy(iblock);
            }
            else
            {
              svtkDataSet* iblockDS = svtkDataSet::SafeDownCast(iblock);
              svtkDataSet* oblockDS = svtkDataSet::SafeDownCast(oblock);
              if (iblockDS && oblockDS)
              {
                oblockDS->CopyStructure(iblockDS);
              }
            }
          }
          cdsOutput->SetDataSet(iter, oblock);
          oblock->Delete();
        }
        iter->Delete();
      }
    }
  }

  svtkDebugMacro(<< "Executing programmable filter");

  // Now invoke the procedure, if specified.
  if (this->ExecuteMethod != nullptr)
  {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
  }

  return 1;
}

int svtkProgrammableFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // This algorithm may accept a svtkDataSet or svtkGraph or svtkTable.
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMolecule");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

void svtkProgrammableFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CopyArrays: " << this->CopyArrays << endl;
}
