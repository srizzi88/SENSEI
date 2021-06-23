/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPDataSetWriter.h"

#include "svtkCallbackCommand.h"
#include "svtkDataSet.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLPImageDataWriter.h"
#include "svtkXMLPPolyDataWriter.h"
#include "svtkXMLPRectilinearGridWriter.h"
#include "svtkXMLPStructuredGridWriter.h"
#include "svtkXMLPUnstructuredGridWriter.h"

svtkStandardNewMacro(svtkXMLPDataSetWriter);

//----------------------------------------------------------------------------
svtkXMLPDataSetWriter::svtkXMLPDataSetWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPDataSetWriter::~svtkXMLPDataSetWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPDataSetWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkDataSet* svtkXMLPDataSetWriter::GetInput()
{
  return static_cast<svtkDataSet*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
int svtkXMLPDataSetWriter::WriteInternal()
{
  svtkAlgorithmOutput* input = this->GetInputConnection(0, 0);
  svtkXMLPDataWriter* writer = nullptr;

  // Create a writer based on the data set type.
  switch (this->GetInput()->GetDataObjectType())
  {
    case SVTK_UNIFORM_GRID:
    case SVTK_IMAGE_DATA:
    case SVTK_STRUCTURED_POINTS:
    {
      svtkXMLPImageDataWriter* w = svtkXMLPImageDataWriter::New();
      w->SetInputConnection(input);
      writer = w;
    }
    break;
    case SVTK_STRUCTURED_GRID:
    {
      svtkXMLPStructuredGridWriter* w = svtkXMLPStructuredGridWriter::New();
      w->SetInputConnection(input);
      writer = w;
    }
    break;
    case SVTK_RECTILINEAR_GRID:
    {
      svtkXMLPRectilinearGridWriter* w = svtkXMLPRectilinearGridWriter::New();
      w->SetInputConnection(input);
      writer = w;
    }
    break;
    case SVTK_UNSTRUCTURED_GRID:
    {
      svtkXMLPUnstructuredGridWriter* w = svtkXMLPUnstructuredGridWriter::New();
      w->SetInputConnection(input);
      writer = w;
    }
    break;
    case SVTK_POLY_DATA:
    {
      svtkXMLPPolyDataWriter* w = svtkXMLPPolyDataWriter::New();
      w->SetInputConnection(input);
      writer = w;
    }
    break;
  }

  // Make sure we got a valid writer for the data set.
  if (!writer)
  {
    svtkErrorMacro("Cannot write dataset type: " << this->GetInput()->GetDataObjectType());
    return 0;
  }

  // Copy the settings to the writer.
  writer->SetDebug(this->GetDebug());
  writer->SetFileName(this->GetFileName());
  writer->SetByteOrder(this->GetByteOrder());
  writer->SetCompressor(this->GetCompressor());
  writer->SetBlockSize(this->GetBlockSize());
  writer->SetDataMode(this->GetDataMode());
  writer->SetEncodeAppendedData(this->GetEncodeAppendedData());
  writer->SetHeaderType(this->GetHeaderType());
  writer->SetIdType(this->GetIdType());
  writer->SetNumberOfPieces(this->GetNumberOfPieces());
  writer->SetGhostLevel(this->GetGhostLevel());
  writer->SetStartPiece(this->GetStartPiece());
  writer->SetEndPiece(this->GetEndPiece());
  writer->SetWriteSummaryFile(this->WriteSummaryFile);
  writer->AddObserver(svtkCommand::ProgressEvent, this->InternalProgressObserver);

  // Try to write.
  int result = writer->Write();

  // Cleanup.
  writer->RemoveObserver(this->InternalProgressObserver);
  writer->Delete();
  return result;
}

//----------------------------------------------------------------------------
const char* svtkXMLPDataSetWriter::GetDataSetName()
{
  return "DataSet";
}

//----------------------------------------------------------------------------
const char* svtkXMLPDataSetWriter::GetDefaultFileExtension()
{
  return "svtk";
}

//----------------------------------------------------------------------------
svtkXMLWriter* svtkXMLPDataSetWriter::CreatePieceWriter(int)
{
  return nullptr;
}
//----------------------------------------------------------------------------
int svtkXMLPDataSetWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}
