/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkADIOS2VTXReader.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

/*
 *  svtkADIOS2VTXReader.cxx
 *
 *  Created on: May 1, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "svtkADIOS2VTXReader.h"

#include "VTX/VTXSchemaManager.h"
#include "VTX/common/VTXHelper.h"

#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkADIOS2VTXReader);

svtkADIOS2VTXReader::svtkADIOS2VTXReader()
  : FileName(nullptr)
  , SchemaManager(new vtx::VTXSchemaManager)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

int svtkADIOS2VTXReader::RequestInformation(svtkInformation* svtkNotUsed(inputVector),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  this->SchemaManager->Update(FileName); // check if FileName changed

  // set time info
  const std::vector<double> vTimes =
    vtx::helper::MapKeysToVector(this->SchemaManager->Reader->Times);

  svtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    svtkStreamingDemandDrivenPipeline::TIME_STEPS(), vTimes.data(), static_cast<int>(vTimes.size()));

  const std::vector<double> timeRange = { vTimes.front(), vTimes.back() };
  info->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange.data(),
    static_cast<int>(timeRange.size()));

  return 1;
}

int svtkADIOS2VTXReader::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  const double newTime = info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
  this->SchemaManager->Step = this->SchemaManager->Reader->Times[newTime];
  this->SchemaManager->Time = newTime;
  return 1;
}

int svtkADIOS2VTXReader::RequestData(svtkInformation* svtkNotUsed(inputVector),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkDataObject* output = info->Get(svtkDataObject::DATA_OBJECT());
  svtkMultiBlockDataSet* multiBlock = svtkMultiBlockDataSet::SafeDownCast(output);

  output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), this->SchemaManager->Time);
  this->SchemaManager->Fill(multiBlock, this->SchemaManager->Step);
  return 1;
}

void svtkADIOS2VTXReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}
