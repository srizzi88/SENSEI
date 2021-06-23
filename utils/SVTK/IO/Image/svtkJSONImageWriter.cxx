/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkJSONImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkJSONImageWriter.h"

#include "svtkCharArray.h"
#include "svtkCommand.h"
#include "svtkDataArray.h"
#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationExecutivePortKey.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <svtksys/FStream.hxx>
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkJSONImageWriter);

//----------------------------------------------------------------------------
svtkJSONImageWriter::svtkJSONImageWriter()
{
  this->FileName = nullptr;
  this->ArrayName = nullptr;
  this->Slice = -1;
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
svtkJSONImageWriter::~svtkJSONImageWriter()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void svtkJSONImageWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------

int svtkJSONImageWriter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  this->SetErrorCode(svtkErrorCode::NoError);

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Error checking
  if (input == nullptr)
  {
    svtkErrorMacro(<< "Write:Please specify an input!");
    return 0;
  }
  if (!this->FileName)
  {
    svtkErrorMacro(<< "Write:Please specify either a FileName or a file prefix and pattern");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return 0;
  }

  // Write
  this->InvokeEvent(svtkCommand::StartEvent);
  svtkCharArray* validMask =
    svtkArrayDownCast<svtkCharArray>(input->GetPointData()->GetArray("svtkValidPointMask"));

  svtksys::ofstream file(this->FileName, ios::out);
  if (file.fail())
  {
    svtkErrorMacro("RecursiveWrite: Could not open file " << this->FileName);
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    return 0;
  }
  file << "{"
       << "\"filename\" : \"" << this->FileName << "\""
       << ",\n\"dimensions\": [" << input->GetDimensions()[0] << ", " << input->GetDimensions()[1]
       << ", " << input->GetDimensions()[2] << "]"
       << ",\n\"origin\": [" << input->GetOrigin()[0] << ", " << input->GetOrigin()[1] << ", "
       << input->GetOrigin()[2] << "]"
       << ",\n\"spacing\": [" << input->GetSpacing()[0] << ", " << input->GetSpacing()[1] << ", "
       << input->GetSpacing()[2] << "]";

  // Write all arrays
  int nbArrays = input->GetPointData()->GetNumberOfArrays();
  for (int i = 0; i < nbArrays; ++i)
  {
    svtkDataArray* array = input->GetPointData()->GetArray(i);
    // We only dump scalar values
    if (array->GetNumberOfComponents() != 1 || !strcmp(array->GetName(), "svtkValidPointMask"))
    {
      continue;
    }
    if (this->ArrayName && strlen(this->ArrayName) > 0 && strcmp(array->GetName(), this->ArrayName))
    {
      continue;
    }
    file << ",\n\"" << array->GetName() << "\": [";
    svtkIdType startIdx = 0;
    svtkIdType endIdx = array->GetNumberOfTuples();
    if (this->Slice >= 0)
    {
      svtkIdType sliceSize = input->GetDimensions()[0] * input->GetDimensions()[1];
      startIdx = sliceSize * this->Slice;
      endIdx = startIdx + sliceSize;
    }
    for (svtkIdType idx = startIdx; idx < endIdx; ++idx)
    {
      if (idx % 50 == 0)
      {
        file << "\n";
        file.flush();
      }
      if (idx != startIdx)
      {
        file << ", ";
      }
      if ((validMask == nullptr) || (validMask && validMask->GetValue(idx)))
      {
        file << array->GetVariantValue(idx).ToString();
      }
      else
      {
        file << "null";
      }
    }
    file << "]";
  }

  file << "\n}" << endl;
  file.close();
  file.flush();

  this->InvokeEvent(svtkCommand::EndEvent);
  return 1;
}

//----------------------------------------------------------------------------
void svtkJSONImageWriter::Write()
{
  this->Modified();
  this->UpdateWholeExtent();
}
