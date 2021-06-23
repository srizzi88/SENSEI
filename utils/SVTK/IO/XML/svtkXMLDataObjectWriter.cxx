/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLDataObjectWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLDataObjectWriter.h"

#include "svtkAlgorithmOutput.h"
#include "svtkCallbackCommand.h"
#include "svtkDataSet.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLHyperTreeGridWriter.h"
#include "svtkXMLImageDataWriter.h"
#include "svtkXMLPolyDataWriter.h"
#include "svtkXMLRectilinearGridWriter.h"
#include "svtkXMLStructuredGridWriter.h"
#include "svtkXMLTableWriter.h"
#include "svtkXMLUnstructuredGridWriter.h"

svtkStandardNewMacro(svtkXMLDataObjectWriter);

//----------------------------------------------------------------------------
svtkXMLDataObjectWriter::svtkXMLDataObjectWriter()
{
  // Setup a callback for the internal writer to report progress.
  this->InternalProgressObserver = svtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(&svtkXMLDataObjectWriter::ProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);
}

//----------------------------------------------------------------------------
svtkXMLDataObjectWriter::~svtkXMLDataObjectWriter()
{
  this->InternalProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void svtkXMLDataObjectWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkDataSet* svtkXMLDataObjectWriter::GetInput()
{
  return static_cast<svtkDataSet*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
svtkXMLWriter* svtkXMLDataObjectWriter::NewWriter(int dataset_type)
{
  // Create a writer based on the data set type.
  switch (dataset_type)
  {
    case SVTK_UNIFORM_GRID:
    case SVTK_IMAGE_DATA:
    case SVTK_STRUCTURED_POINTS:
      return svtkXMLImageDataWriter::New();
    case SVTK_STRUCTURED_GRID:
      return svtkXMLStructuredGridWriter::New();
    case SVTK_RECTILINEAR_GRID:
      return svtkXMLRectilinearGridWriter::New();
    case SVTK_UNSTRUCTURED_GRID:
      return svtkXMLUnstructuredGridWriter::New();
    case SVTK_POLY_DATA:
      return svtkXMLPolyDataWriter::New();
    case SVTK_TABLE:
      return svtkXMLTableWriter::New();
    case SVTK_HYPER_TREE_GRID:
      return svtkXMLHyperTreeGridWriter::New();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkXMLDataObjectWriter::WriteInternal()
{
  // Create a writer based on the data set type.
  svtkXMLWriter* writer = svtkXMLDataObjectWriter::NewWriter(this->GetInput()->GetDataObjectType());
  if (writer)
  {
    writer->SetInputConnection(this->GetInputConnection(0, 0));

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
    writer->AddObserver(svtkCommand::ProgressEvent, this->InternalProgressObserver);

    // Try to write.
    int result = writer->Write();

    // Cleanup.
    writer->RemoveObserver(this->InternalProgressObserver);
    writer->Delete();
    return result;
  }

  // Make sure we got a valid writer for the data set.
  svtkErrorMacro("Cannot write dataset type: "
    << this->GetInput()->GetDataObjectType() << " which is a " << this->GetInput()->GetClassName());
  return 0;
}

//----------------------------------------------------------------------------
const char* svtkXMLDataObjectWriter::GetDataSetName()
{
  return "DataSet";
}

//----------------------------------------------------------------------------
const char* svtkXMLDataObjectWriter::GetDefaultFileExtension()
{
  return "svtk";
}

//----------------------------------------------------------------------------
void svtkXMLDataObjectWriter::ProgressCallbackFunction(
  svtkObject* caller, unsigned long, void* clientdata, void*)
{
  svtkAlgorithm* w = svtkAlgorithm::SafeDownCast(caller);
  if (w)
  {
    reinterpret_cast<svtkXMLDataObjectWriter*>(clientdata)->ProgressCallback(w);
  }
}

//----------------------------------------------------------------------------
void svtkXMLDataObjectWriter::ProgressCallback(svtkAlgorithm* w)
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  float internalProgress = w->GetProgress();
  float progress = this->ProgressRange[0] + internalProgress * width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
  {
    w->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
int svtkXMLDataObjectWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}
