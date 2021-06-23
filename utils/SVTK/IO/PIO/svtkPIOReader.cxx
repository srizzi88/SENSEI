/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPIOReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPIOReader.h"

#include <iostream>

#include "svtkCallbackCommand.h"
#include "svtkCellData.h"
#include "svtkDataArraySelection.h"
#include "svtkDataObject.h"
#include "svtkErrorCode.h"
#include "svtkFloatArray.h"
#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkToolkits.h"
#include "svtkUnstructuredGrid.h"

#include "PIOAdaptor.h"

svtkStandardNewMacro(svtkPIOReader);

//----------------------------------------------------------------------------
// Constructor for PIO Reader
//----------------------------------------------------------------------------
svtkPIOReader::svtkPIOReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->FileName = nullptr;
  this->HyperTreeGrid = false;
  this->Tracers = false;
  this->Float64 = false;
  this->NumberOfVariables = 0;
  this->CurrentTimeStep = -1;
  this->LastTimeStep = -1;
  this->TimeSteps = 0;
  this->CellDataArraySelection = svtkDataArraySelection::New();

  // Setup selection callback to modify this object when array selection changes
  this->SelectionObserver = svtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&svtkPIOReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->CellDataArraySelection->AddObserver(svtkCommand::ModifiedEvent, this->SelectionObserver);
  // External PIO_DATA for actually reading files
  this->pioAdaptor = 0;

  this->MPIController = svtkMultiProcessController::GetGlobalController();
  if (this->MPIController)
  {
    this->Rank = this->MPIController->GetLocalProcessId();
    this->TotalRank = this->MPIController->GetNumberOfProcesses();
  }
  else
  {
    this->Rank = 0;
    this->TotalRank = 1;
  }
}

//----------------------------------------------------------------------------
// Destructor for PIO Reader
//----------------------------------------------------------------------------
svtkPIOReader::~svtkPIOReader()
{
  delete[] this->FileName;

  delete this->pioAdaptor;
  delete[] this->TimeSteps;

  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();

  // Do not delete the MPIContoroller which is a singleton
  this->MPIController = nullptr;
}

//----------------------------------------------------------------------------
// Verify that the file exists
//----------------------------------------------------------------------------
int svtkPIOReader::RequestInformation(svtkInformation* svtkNotUsed(reqInfo),
  svtkInformationVector** svtkNotUsed(inVector), svtkInformationVector* outVector)
{
  // Verify that file exists
  if (!this->FileName)
  {
    svtkErrorMacro("Reader called with no filename set");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return 0;
  }

  // Get ParaView information and output pointers
  svtkInformation* outInfo = outVector->GetInformationObject(0);
  if (this->pioAdaptor == 0)
  {

    // Create one PIOAdaptor which builds the MultiBlockDataSet
    this->pioAdaptor = new PIOAdaptor(this->Rank, this->TotalRank);

    // Initialize sizes and file reads
    // descriptor.pio file contains information
    // otherwise a basename-dmp000000 is given and defaults are used
    if (!this->pioAdaptor->initializeGlobal(this->FileName))
    {
      svtkErrorMacro("Error in pio description file");
      this->SetErrorCode(svtkErrorCode::FileFormatError);
      delete this->pioAdaptor;
      this->pioAdaptor = 0;
      return 0;
    }

    this->HyperTreeGrid = pioAdaptor->GetHyperTreeGrid();
    this->Tracers = pioAdaptor->GetTracers();
    this->Float64 = pioAdaptor->GetFloat64();

    // Get the variable names and set in the selection
    int numberOfVariables = this->pioAdaptor->GetNumberOfVariables();
    for (int i = 0; i < numberOfVariables; i++)
    {
      this->CellDataArraySelection->AddArray(this->pioAdaptor->GetVariableName(i));
    }
    this->DisableAllCellArrays();

    // Set the variable names loaded by default
    for (int i = 0; i < this->pioAdaptor->GetNumberOfDefaultVariables(); i++)
    {
      this->SetCellArrayStatus(this->pioAdaptor->GetVariableDefault(i), 1);
    }

    // Collect temporal information
    this->NumberOfTimeSteps = this->pioAdaptor->GetNumberOfTimeSteps();
    this->TimeSteps = nullptr;

    if (this->NumberOfTimeSteps > 0)
    {
      this->TimeSteps = new double[this->NumberOfTimeSteps];

      for (int step = 0; step < this->NumberOfTimeSteps; step++)
      {
        this->TimeSteps[step] = (double)this->pioAdaptor->GetTimeStep(step);
      }

      // Tell the pipeline what steps are available
      outInfo->Set(
        svtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps, this->NumberOfTimeSteps);

      // Range is required to get GUI to show things
      double tRange[2];
      tRange[0] = this->TimeSteps[0];
      tRange[1] = this->TimeSteps[this->NumberOfTimeSteps - 1];
      outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);
    }
    else
    {
      outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
      outInfo->Set(
        svtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->TimeSteps, this->NumberOfTimeSteps);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Data is read into a svtkMultiBlockDataSet
//----------------------------------------------------------------------------
int svtkPIOReader::RequestData(svtkInformation* svtkNotUsed(reqInfo),
  svtkInformationVector** svtkNotUsed(inVector), svtkInformationVector* outVector)
{
  // If no PIOAdaptor there was an earlier failure
  if (this->pioAdaptor == 0)
  {
    svtkErrorMacro("Error in pio description file");
    this->SetErrorCode(svtkErrorCode::FileFormatError);
    return 0;
  }

  svtkInformation* outInfo = outVector->GetInformationObject(0);
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Collect the time step requested
  double requestedTimeStep(0);
  svtkInformationDoubleKey* timeKey =
    static_cast<svtkInformationDoubleKey*>(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

  double dTime = 0;
  int timeStep = 0;

  // RequestData can be called from GUI pipeline or python script
  if (outInfo->Has(timeKey))
  {
    // Pipeline activated from GUI will have timeKey
    requestedTimeStep = outInfo->Get(timeKey);
    dTime = requestedTimeStep;

    // Index of the time step to request
    while (timeStep < this->NumberOfTimeSteps && this->TimeSteps[timeStep] < dTime)
    {
      timeStep++;
    }
    if (this->CurrentTimeStep != timeStep)
    {
      this->CurrentTimeStep = timeStep;
    }
  }
  else
  {
    // Pipeline actived from python script
    if (this->CurrentTimeStep < 0 || this->CurrentTimeStep >= this->NumberOfTimeSteps)
    {
      this->CurrentTimeStep = 0;
    }
    dTime = this->TimeSteps[this->CurrentTimeStep];
  }
  output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), dTime);

  // Load new geometry and data if time step has changed or selection changed
  this->LastTimeStep = this->CurrentTimeStep;

  // Initialize the PIOAdaptor for reading the requested dump file
  if (!this->pioAdaptor->initializeDump(this->CurrentTimeStep))
  {
    svtkErrorMacro("PIO dump file cannot be opened");
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    return 0;
  }

  // Set parameters for the file read
  this->pioAdaptor->SetHyperTreeGrid(this->HyperTreeGrid);
  this->pioAdaptor->SetTracers(this->Tracers);
  this->pioAdaptor->SetFloat64(this->Float64);

  // Create the geometry requested in the pio descriptor file
  this->pioAdaptor->create_geometry(output);

  // Load the requested data in the correct ordering based on PIO daughters
  this->pioAdaptor->load_variable_data(output, this->CellDataArraySelection);

  return 1;
}

//----------------------------------------------------------------------------
void svtkPIOReader::SelectionModifiedCallback(
  svtkObject*, unsigned long svtkNotUsed(eventid), void* clientdata, void* svtkNotUsed(calldata))
{
  static_cast<svtkPIOReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
svtkMultiBlockDataSet* svtkPIOReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkMultiBlockDataSet* svtkPIOReader::GetOutput(int idx)
{
  if (idx)
  {
    return nullptr;
  }
  else
  {
    return svtkMultiBlockDataSet::SafeDownCast(this->GetOutputDataObject(idx));
  }
}

//----------------------------------------------------------------------------
int svtkPIOReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkPIOReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void svtkPIOReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
const char* svtkPIOReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int svtkPIOReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void svtkPIOReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
    this->CellDataArraySelection->EnableArray(name);
  else
    this->CellDataArraySelection->DisableArray(name);
}

void svtkPIOReader::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "FileName: " << (this->FileName != nullptr ? this->FileName : "") << endl;
  os << indent << "CellDataArraySelection: " << this->CellDataArraySelection << "\n";
  this->Superclass::PrintSelf(os, indent);
}
