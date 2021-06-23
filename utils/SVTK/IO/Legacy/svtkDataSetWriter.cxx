/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetWriter.h"

#include "svtkAlgorithmOutput.h"
#include "svtkDataSet.h"
#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataWriter.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridWriter.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridWriter.h"
#include "svtkStructuredPointsWriter.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridWriter.h"

svtkStandardNewMacro(svtkDataSetWriter);

void svtkDataSetWriter::WriteData()
{
  int type;
  svtkDataWriter* writer;
  svtkAlgorithmOutput* input = this->GetInputConnection(0, 0);

  svtkDebugMacro(<< "Writing svtk dataset...");

  type = this->GetInput()->GetDataObjectType();
  if (type == SVTK_POLY_DATA)
  {
    svtkPolyDataWriter* pwriter = svtkPolyDataWriter::New();
    pwriter->SetInputConnection(input);
    writer = pwriter;
  }

  else if (type == SVTK_STRUCTURED_POINTS || type == SVTK_IMAGE_DATA || type == SVTK_UNIFORM_GRID)
  {
    svtkStructuredPointsWriter* spwriter = svtkStructuredPointsWriter::New();
    spwriter->SetInputConnection(input);
    writer = spwriter;
  }

  else if (type == SVTK_STRUCTURED_GRID)
  {
    svtkStructuredGridWriter* sgwriter = svtkStructuredGridWriter::New();
    sgwriter->SetInputConnection(input);
    writer = sgwriter;
  }

  else if (type == SVTK_UNSTRUCTURED_GRID)
  {
    svtkUnstructuredGridWriter* ugwriter = svtkUnstructuredGridWriter::New();
    ugwriter->SetInputConnection(input);
    writer = ugwriter;
  }

  else if (type == SVTK_RECTILINEAR_GRID)
  {
    svtkRectilinearGridWriter* rgwriter = svtkRectilinearGridWriter::New();
    rgwriter->SetInputConnection(input);
    writer = rgwriter;
  }

  else
  {
    svtkErrorMacro(<< "Cannot write dataset type: " << type);
    return;
  }

  writer->SetFileName(this->FileName);
  writer->SetScalarsName(this->ScalarsName);
  writer->SetVectorsName(this->VectorsName);
  writer->SetNormalsName(this->NormalsName);
  writer->SetTensorsName(this->TensorsName);
  writer->SetTCoordsName(this->TCoordsName);
  writer->SetHeader(this->Header);
  writer->SetLookupTableName(this->LookupTableName);
  writer->SetFieldDataName(this->FieldDataName);
  writer->SetFileType(this->FileType);
  writer->SetDebug(this->Debug);
  writer->SetWriteToOutputString(this->WriteToOutputString);
  writer->Write();
  if (writer->GetErrorCode() == svtkErrorCode::OutOfDiskSpaceError)
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
  }
  if (this->WriteToOutputString)
  {
    delete[] this->OutputString;
    this->OutputStringLength = writer->GetOutputStringLength();
    this->OutputString = writer->RegisterAndGetOutputString();
  }
  writer->Delete();
}

int svtkDataSetWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

svtkDataSet* svtkDataSetWriter::GetInput()
{
  return svtkDataSet::SafeDownCast(this->Superclass::GetInput());
}

svtkDataSet* svtkDataSetWriter::GetInput(int port)
{
  return svtkDataSet::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkDataSetWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
