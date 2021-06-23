/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWriter.h"

#include "svtkCommand.h"
#include "svtkDataObject.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"

#include <sstream>

// Construct with no start and end write methods or arguments.
svtkWriter::svtkWriter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
}

svtkWriter::~svtkWriter() = default;

void svtkWriter::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

void svtkWriter::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

svtkDataObject* svtkWriter::GetInput()
{
  return this->GetInput(0);
}

svtkDataObject* svtkWriter::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}

// Write data to output. Method executes subclasses WriteData() method, as
// well as StartMethod() and EndMethod() methods.
int svtkWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    svtkErrorMacro("No input provided!");
    return 0;
  }

  // always write even if the data hasn't changed
  this->Modified();
  this->UpdateWholeExtent();

  return (this->GetErrorCode() == svtkErrorCode::NoError);
}

svtkTypeBool svtkWriter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int svtkWriter::RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  this->SetErrorCode(svtkErrorCode::NoError);

  svtkDataObject* input = this->GetInput();

  // make sure input is available
  if (!input)
  {
    svtkErrorMacro(<< "No input!");
    return 0;
  }

  this->InvokeEvent(svtkCommand::StartEvent, nullptr);
  this->WriteData();
  this->InvokeEvent(svtkCommand::EndEvent, nullptr);

  this->WriteTime.Modified();

  return 1;
}

void svtkWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkWriter::EncodeString(char* resname, const char* name, bool doublePercent)
{
  if (!name || !resname)
  {
    return;
  }
  int cc = 0;
  std::ostringstream str;

  char buffer[10];

  while (name[cc])
  {
    // Encode spaces and %'s (and most non-printable ascii characters)
    // The reader does not support spaces in strings.
    if (name[cc] < 33 || name[cc] > 126 || name[cc] == '\"' || name[cc] == '%')
    {
      snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned char>(name[cc]));
      if (doublePercent)
      {
        str << "%%";
      }
      else
      {
        str << "%";
      }
      str << buffer;
    }
    else
    {
      str << name[cc];
    }
    cc++;
  }
  strcpy(resname, str.str().c_str());
}

void svtkWriter::EncodeWriteString(ostream* out, const char* name, bool doublePercent)
{
  if (!name)
  {
    return;
  }
  int cc = 0;

  char buffer[10];

  while (name[cc])
  {
    // Encode spaces and %'s (and most non-printable ascii characters)
    // The reader does not support spaces in strings.
    if (name[cc] < 33 || name[cc] > 126 || name[cc] == '\"' || name[cc] == '%')
    {
      snprintf(buffer, sizeof(buffer), "%02X", static_cast<unsigned char>(name[cc]));
      if (doublePercent)
      {
        *out << "%%";
      }
      else
      {
        *out << "%";
      }
      *out << buffer;
    }
    else
    {
      *out << name[cc];
    }
    cc++;
  }
}
