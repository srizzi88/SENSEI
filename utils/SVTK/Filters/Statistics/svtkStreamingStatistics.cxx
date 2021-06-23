/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStreamingStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkStreamingStatistics.h"

#include "svtkDataObjectCollection.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStatisticsAlgorithm.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkStreamingStatistics);

svtkCxxSetObjectMacro(svtkStreamingStatistics, StatisticsAlgorithm, svtkStatisticsAlgorithm);

// ----------------------------------------------------------------------
svtkStreamingStatistics::svtkStreamingStatistics()
{
  // Setup input/output ports
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(3);

  // Initialize internal stats algorithm to nullptr
  this->StatisticsAlgorithm = nullptr;
  this->SetStatisticsAlgorithm(nullptr);

  // Initialize internal model
  this->InternalModel = svtkMultiBlockDataSet::New();
}

// ----------------------------------------------------------------------
svtkStreamingStatistics::~svtkStreamingStatistics()
{
  // Release/delete internal stats algorithm
  this->SetStatisticsAlgorithm(nullptr);
  this->StatisticsAlgorithm = nullptr;

  // Release/delete internal model to nullptr
  this->InternalModel->Delete();
  this->InternalModel = nullptr;
}

// ----------------------------------------------------------------------
int svtkStreamingStatistics::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == INPUT_DATA)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }
  else if (port == INPUT_MODEL)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
    return 1;
  }
  else if (port == LEARN_PARAMETERS)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------
int svtkStreamingStatistics::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == OUTPUT_DATA)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTable");
    return 1;
  }
  else if (port == OUTPUT_MODEL)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
    return 1;
  }
  else if (port == OUTPUT_TEST)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkTable");
    return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------
int svtkStreamingStatistics::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Input handles
  svtkTable* inData = svtkTable::GetData(inputVector[INPUT_DATA], 0);

  // Output handles
  svtkTable* outData = svtkTable::GetData(outputVector, OUTPUT_DATA);
  svtkMultiBlockDataSet* outModel = svtkMultiBlockDataSet::GetData(outputVector, OUTPUT_MODEL);

  // These will be used later
  /*
  svtkMultiBlockDataSet* inModel      = svtkMultiBlockDataSet::GetData( inputVector[INPUT_MODEL], 0 );
  */
  svtkDataObject* inParameters = svtkDataObject::GetData(inputVector[LEARN_PARAMETERS], 0);
  svtkTable* outTest = svtkTable::GetData(outputVector, OUTPUT_TEST);

  // Note: Experimental code. Lots of use case are currently not handled in
  // any way and there are a lot of assumptions made about this or that

  // Make sure the statistics algorithm is set
  if (!this->StatisticsAlgorithm)
  {
    svtkErrorMacro("StatisticsAlgorithm not set! Punting!");
    cerr << "StatisticsAlgorithm not set! Punting!" << endl;
    return 0;
  }

  // Set the input into my stats algorithms
  this->StatisticsAlgorithm->SetInputData(inData);
  this->StatisticsAlgorithm->SetLearnOptionParameters(inParameters);
  this->StatisticsAlgorithm->SetInputModel(this->InternalModel);

  // Force an update
  this->StatisticsAlgorithm->Update();

  // Grab (DeepCopy) the model for next time
  this->InternalModel->DeepCopy(this->StatisticsAlgorithm->GetOutputDataObject(OUTPUT_MODEL));

  // Shallow copy the internal output to external output
  outData->ShallowCopy(this->StatisticsAlgorithm->GetOutput(OUTPUT_DATA));
  outModel->ShallowCopy(this->StatisticsAlgorithm->GetOutputDataObject(OUTPUT_MODEL));
  outTest->ShallowCopy(this->StatisticsAlgorithm->GetOutput(OUTPUT_TEST));

  return 1;
}

// ----------------------------------------------------------------------
void svtkStreamingStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->StatisticsAlgorithm)
  {
    os << indent << "StatisticsAlgorithm:\n";
    svtkIndent i2 = indent.GetNextIndent();
    this->StatisticsAlgorithm->PrintSelf(os, i2);
  }
  os << indent << "InternalModel: " << this->InternalModel << "\n";
}
