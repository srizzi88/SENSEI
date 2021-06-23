/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProgrammableSource.h"

#include "svtkCommand.h"
#include "svtkExecutive.h"
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

svtkStandardNewMacro(svtkProgrammableSource);

// Construct programmable filter with empty execute method.
svtkProgrammableSource::svtkProgrammableSource()
{
  this->ExecuteMethod = nullptr;
  this->ExecuteMethodArg = nullptr;
  this->ExecuteMethodArgDelete = nullptr;
  this->RequestInformationMethod = nullptr;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(8);

  svtkDataObject* output;
  output = svtkPolyData::New();
  this->GetExecutive()->SetOutputData(0, output);
  output->Delete();

  output = svtkStructuredPoints::New();
  this->GetExecutive()->SetOutputData(1, output);
  output->Delete();

  output = svtkStructuredGrid::New();
  this->GetExecutive()->SetOutputData(2, output);
  output->Delete();

  output = svtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(3, output);
  output->Delete();

  output = svtkRectilinearGrid::New();
  this->GetExecutive()->SetOutputData(4, output);
  output->Delete();

  output = svtkGraph::New();
  this->GetExecutive()->SetOutputData(5, output);
  output->Delete();

  output = svtkMolecule::New();
  this->GetExecutive()->SetOutputData(6, output);
  output->Delete();

  output = svtkTable::New();
  this->GetExecutive()->SetOutputData(7, output);
  output->Delete();

  this->RequestedDataType = SVTK_POLY_DATA;
}

svtkProgrammableSource::~svtkProgrammableSource()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg) && (this->ExecuteMethodArgDelete))
  {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
  }
}

// Specify the function to use to generate the source data. Note
// that the function takes a single (void *) argument.
void svtkProgrammableSource::SetExecuteMethod(void (*f)(void*), void* arg)
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
void svtkProgrammableSource::SetExecuteMethodArgDelete(void (*f)(void*))
{
  if (f != this->ExecuteMethodArgDelete)
  {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
  }
}

void svtkProgrammableSource::SetRequestInformationMethod(void (*f)(void*))
{
  if (f != this->RequestInformationMethod)
  {
    this->RequestInformationMethod = f;
    this->Modified();
  }
}

// Get the output as a concrete type. This method is typically used by the
// writer of the source function to get the output as a particular type (i.e.,
// it essentially does type casting). It is the users responsibility to know
// the correct type of the output data.
svtkPolyData* svtkProgrammableSource::GetPolyDataOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = SVTK_POLY_DATA;
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(0));
}

// Get the output as a concrete type.
svtkStructuredPoints* svtkProgrammableSource::GetStructuredPointsOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = SVTK_STRUCTURED_POINTS;
  return svtkStructuredPoints::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

// Get the output as a concrete type.
svtkStructuredGrid* svtkProgrammableSource::GetStructuredGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 5)
  {
    return nullptr;
  }

  this->RequestedDataType = SVTK_STRUCTURED_GRID;
  return svtkStructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(2));
}

// Get the output as a concrete type.
svtkUnstructuredGrid* svtkProgrammableSource::GetUnstructuredGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = SVTK_UNSTRUCTURED_GRID;
  return svtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(3));
}

// Get the output as a concrete type.
svtkRectilinearGrid* svtkProgrammableSource::GetRectilinearGridOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = SVTK_RECTILINEAR_GRID;
  return svtkRectilinearGrid::SafeDownCast(this->GetExecutive()->GetOutputData(4));
}

// Get the output as a concrete type.
svtkGraph* svtkProgrammableSource::GetGraphOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = SVTK_GRAPH;
  return svtkGraph::SafeDownCast(this->GetExecutive()->GetOutputData(5));
}

// Get the output as a concrete type.
svtkMolecule* svtkProgrammableSource::GetMoleculeOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = SVTK_MOLECULE;
  return svtkMolecule::SafeDownCast(this->GetExecutive()->GetOutputData(6));
}

// Get the output as a concrete type.
svtkTable* svtkProgrammableSource::GetTableOutput()
{
  if (this->GetNumberOfOutputPorts() < 8)
  {
    return nullptr;
  }

  this->RequestedDataType = SVTK_TABLE;
  return svtkTable::SafeDownCast(this->GetExecutive()->GetOutputData(7));
}

int svtkProgrammableSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkDebugMacro(<< "Executing programmable source");

  // Now invoke the procedure, if specified.
  if (this->ExecuteMethod != nullptr)
  {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
  }

  return 1;
}

int svtkProgrammableSource::RequestDataObject(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo;
  svtkDataObject* output = nullptr;
  switch (this->RequestedDataType)
  {
    case SVTK_POLY_DATA:
      outInfo = outputVector->GetInformationObject(0);
      if (!outInfo)
      {
        output = svtkPolyData::New();
      }
      else
      {
        output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = svtkPolyData::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(0, output);
      output->Delete();
      break;
    case SVTK_STRUCTURED_POINTS:
      outInfo = outputVector->GetInformationObject(1);
      if (!outInfo)
      {
        output = svtkStructuredPoints::New();
      }
      else
      {
        output = svtkStructuredPoints::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = svtkStructuredPoints::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(1, output);
      output->Delete();
      break;
    case SVTK_STRUCTURED_GRID:
      outInfo = outputVector->GetInformationObject(2);
      if (!outInfo)
      {
        output = svtkStructuredGrid::New();
      }
      else
      {
        output = svtkStructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = svtkStructuredGrid::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(2, output);
      output->Delete();
      break;
    case SVTK_UNSTRUCTURED_GRID:
      outInfo = outputVector->GetInformationObject(3);
      if (!outInfo)
      {
        output = svtkUnstructuredGrid::New();
      }
      else
      {
        output = svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = svtkUnstructuredGrid::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(3, output);
      output->Delete();
      break;
    case SVTK_RECTILINEAR_GRID:
      outInfo = outputVector->GetInformationObject(4);
      if (!outInfo)
      {
        output = svtkRectilinearGrid::New();
      }
      else
      {
        output = svtkRectilinearGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = svtkRectilinearGrid::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(4, output);
      output->Delete();
      break;
    case SVTK_GRAPH:
      outInfo = outputVector->GetInformationObject(5);
      if (!outInfo)
      {
        output = svtkGraph::New();
      }
      else
      {
        output = svtkGraph::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = svtkGraph::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(5, output);
      output->Delete();
      break;
    case SVTK_MOLECULE:
      outInfo = outputVector->GetInformationObject(6);
      if (!outInfo)
      {
        output = svtkMolecule::New();
      }
      else
      {
        output = svtkMolecule::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = svtkMolecule::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(6, output);
      output->Delete();
      break;
    case SVTK_TABLE:
      outInfo = outputVector->GetInformationObject(7);
      if (!outInfo)
      {
        output = svtkTable::New();
      }
      else
      {
        output = svtkTable::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
        if (!output)
        {
          output = svtkTable::New();
        }
        else
        {
          return 1;
        }
      }
      this->GetExecutive()->SetOutputData(7, output);
      output->Delete();
      break;
    default:
      return 0;
  }
  return 1;
}

int svtkProgrammableSource::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  svtkDebugMacro(<< "requesting information");

  // Now invoke the procedure, if specified.
  if (this->RequestInformationMethod != nullptr)
  {
    (*this->RequestInformationMethod)(nullptr);
  }

  return 1;
}
