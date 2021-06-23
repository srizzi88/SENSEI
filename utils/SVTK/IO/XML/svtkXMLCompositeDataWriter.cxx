/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLCompositeDataWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLCompositeDataWriter.h"

#include "svtkCallbackCommand.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDoubleArray.h"
#include "svtkErrorCode.h"
#include "svtkExecutive.h"
#include "svtkFieldData.h"
#include "svtkGarbageCollector.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkHyperTreeGrid.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkTable.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLDataObjectWriter.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <svtksys/SystemTools.hxx>

//----------------------------------------------------------------------------

class svtkXMLCompositeDataWriterInternals
{
  // These are used to by GetDefaultFileExtension(). This helps us avoid
  // creating new instances repeatedly for the same dataset type.
  std::map<int, svtkSmartPointer<svtkXMLWriter> > TmpWriters;

public:
  std::vector<svtkSmartPointer<svtkXMLWriter> > Writers;
  std::string FilePath;
  std::string FilePrefix;
  svtkSmartPointer<svtkXMLDataElement> Root;
  std::vector<int> DataTypes;

  // Get the default extension for the dataset_type. Will return nullptr if an
  // extension cannot be determined.
  const char* GetDefaultFileExtensionForDataSet(int dataset_type)
  {
    std::map<int, svtkSmartPointer<svtkXMLWriter> >::iterator iter =
      this->TmpWriters.find(dataset_type);
    if (iter == this->TmpWriters.end())
    {
      svtkSmartPointer<svtkXMLWriter> writer;
      writer.TakeReference(svtkXMLDataObjectWriter::NewWriter(dataset_type));
      if (writer)
      {
        std::pair<int, svtkSmartPointer<svtkXMLWriter> > pair(dataset_type, writer);
        iter = this->TmpWriters.insert(pair).first;
      }
    }
    if (iter != this->TmpWriters.end())
    {
      return iter->second->GetDefaultFileExtension();
    }
    return nullptr;
  }
};

//----------------------------------------------------------------------------
svtkXMLCompositeDataWriter::svtkXMLCompositeDataWriter()
{
  this->Internal = new svtkXMLCompositeDataWriterInternals;
  this->GhostLevel = 0;
  this->WriteMetaFile = 1;

  // Setup a callback for the internal writers to report progress.
  this->InternalProgressObserver = svtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(&svtkXMLCompositeDataWriter::ProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);

  this->InputInformation = nullptr;
}

//----------------------------------------------------------------------------
svtkXMLCompositeDataWriter::~svtkXMLCompositeDataWriter()
{
  this->InternalProgressObserver->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
const char* svtkXMLCompositeDataWriter::GetDefaultFileExtensionForDataSet(int dataset_type)
{
  return this->Internal->GetDefaultFileExtensionForDataSet(dataset_type);
}

//----------------------------------------------------------------------------
unsigned int svtkXMLCompositeDataWriter::GetNumberOfDataTypes()
{
  return static_cast<unsigned int>(this->Internal->DataTypes.size());
}

//----------------------------------------------------------------------------
int* svtkXMLCompositeDataWriter::GetDataTypesPointer()
{
  return &this->Internal->DataTypes[0];
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "WriteMetaFile: " << this->WriteMetaFile << endl;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkXMLCompositeDataWriter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::SetWriteMetaFile(int flag)
{
  if (this->WriteMetaFile != flag)
  {
    this->WriteMetaFile = flag;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataWriter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataWriter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  this->InputInformation = inInfo;

  svtkCompositeDataSet* compositeData =
    svtkCompositeDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!compositeData)
  {
    svtkErrorMacro("No hierarchical input has been provided. Cannot write");
    this->InputInformation = nullptr;
    return 0;
  }

  // Create writers for each input.
  this->CreateWriters(compositeData);

  this->SetErrorCode(svtkErrorCode::NoError);

  // Make sure we have a file to write.
  if (!this->Stream && !this->FileName)
  {
    svtkErrorMacro("Writer called with no FileName set.");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    this->InputInformation = nullptr;
    return 0;
  }

  // We are just starting to write.  Do not call
  // UpdateProgressDiscrete because we want a 0 progress callback the
  // first time.
  this->UpdateProgress(0);

  // Initialize progress range to entire 0..1 range.
  float wholeProgressRange[2] = { 0.f, 1.f };
  this->SetProgressRange(wholeProgressRange, 0, 1);

  // Prepare file prefix for creation of internal file names.
  this->SplitFileName();

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  // Create the subdirectory for the internal files.
  std::string subdir = this->Internal->FilePath;
  subdir += this->Internal->FilePrefix;
  this->MakeDirectory(subdir.c_str());

  this->Internal->Root = svtkSmartPointer<svtkXMLDataElement>::New();
  this->Internal->Root->SetName(compositeData->GetClassName());

  int writerIdx = 0;
  if (!this->WriteComposite(compositeData, this->Internal->Root, writerIdx))
  {
    this->RemoveWrittenFiles(subdir.c_str());
    return 0;
  }

  if (this->WriteMetaFile)
  {
    this->SetProgressRange(progressRange, this->GetNumberOfInputConnections(0),
      this->GetNumberOfInputConnections(0) + this->WriteMetaFile);
    int retVal = this->WriteMetaFileIfRequested();
    this->InputInformation = nullptr;
    return retVal;
  }

  // We have finished writing.
  this->UpdateProgressDiscrete(1);

  this->InputInformation = nullptr;
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataWriter::WriteNonCompositeData(
  svtkDataObject* dObj, svtkXMLDataElement* datasetXML, int& writerIdx, const char* fileName)
{
  // Write a leaf dataset.
  int myWriterIndex = writerIdx;
  writerIdx++;

  // Locate the actual data writer for this dataset.
  svtkXMLWriter* writer = this->GetWriter(myWriterIndex);
  if (!writer)
  {
    return 0;
  }

  svtkDataSet* curDS = svtkDataSet::SafeDownCast(dObj);
  svtkTable* curTable = svtkTable::SafeDownCast(dObj);
  svtkHyperTreeGrid* curHTG = svtkHyperTreeGrid::SafeDownCast(dObj);
  if (!curDS && !curTable && !curHTG)
  {
    if (dObj)
    {
      svtkWarningMacro("This writer cannot handle sub-datasets of type: "
        << dObj->GetClassName() << " Dataset will be skipped.");
    }
    return 0;
  }

  if (datasetXML)
  {
    // Create the entry for the collection file.
    datasetXML->SetAttribute("file", fileName);
  }

  // FIXME
  // Ken's note, I do not think you can fix this, the
  // setprogress range has to be done in the loop that calls
  // this function.
  // this->SetProgressRange(progressRange, myWriterIndex,
  //                       GetNumberOfInputConnections(0)+writeCollection);

  std::string full = this->Internal->FilePath;
  full += fileName;

  writer->SetFileName(full.c_str());

  // Write the data.
  writer->AddObserver(svtkCommand::ProgressEvent, this->InternalProgressObserver);
  writer->Write();
  writer->RemoveObserver(this->InternalProgressObserver);

  if (writer->GetErrorCode() == svtkErrorCode::OutOfDiskSpaceError)
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataWriter::WriteData()
{
  // Write the collection file.
  this->StartFile();
  svtkIndent indent = svtkIndent().GetNextIndent();

  // Open the primary element.
  ostream& os = *(this->Stream);

  if (this->Internal->Root)
  {
    this->Internal->Root->PrintXML(os, indent);
  }

  // We want to avoid using appended data mode as it
  // is not supported in meta formats.
  int dataMode = this->DataMode;
  if (dataMode == svtkXMLWriter::Appended)
  {
    this->DataMode = svtkXMLWriter::Binary;
  }

  svtkDataObject* input = this->GetInput();
  svtkFieldData* fieldData = input->GetFieldData();

  svtkInformation* meta = input->GetInformation();
  bool hasTime = meta->Has(svtkDataObject::DATA_TIME_STEP()) ? true : false;
  if ((fieldData && fieldData->GetNumberOfArrays()) || hasTime)
  {
    svtkNew<svtkFieldData> fieldDataCopy;
    fieldDataCopy->ShallowCopy(fieldData);
    if (hasTime)
    {
      svtkNew<svtkDoubleArray> time;
      time->SetNumberOfTuples(1);
      time->SetTypedComponent(0, 0, meta->Get(svtkDataObject::DATA_TIME_STEP()));
      time->SetName("TimeValue");
      fieldDataCopy->AddArray(time);
    }
    this->WriteFieldDataInline(fieldDataCopy, indent);
  }
  this->DataMode = dataMode;

  return this->EndFile();
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataWriter::WriteMetaFileIfRequested()
{
  if (this->WriteMetaFile)
  {
    if (!this->Superclass::WriteInternal())
    {
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::MakeDirectory(const char* name)
{
  if (!svtksys::SystemTools::MakeDirectory(name))
  {
    svtkErrorMacro(<< "Sorry unable to create directory: " << name << endl
                  << "Last system error was: "
                  << svtksys::SystemTools::GetLastSystemError().c_str());
  }
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::RemoveADirectory(const char* name)
{
  if (!svtksys::SystemTools::RemoveADirectory(name))
  {
    svtkErrorMacro(<< "Sorry unable to remove a directory: " << name << endl
                  << "Last system error was: "
                  << svtksys::SystemTools::GetLastSystemError().c_str());
  }
}

//----------------------------------------------------------------------------
const char* svtkXMLCompositeDataWriter::GetDefaultFileExtension()
{
  return "vtm";
}

//----------------------------------------------------------------------------
const char* svtkXMLCompositeDataWriter::GetDataSetName()
{
  if (!this->InputInformation)
  {
    return "CompositeDataSet";
  }
  svtkDataObject* hdInput =
    svtkDataObject::SafeDownCast(this->InputInformation->Get(svtkDataObject::DATA_OBJECT()));
  if (!hdInput)
  {
    return nullptr;
  }
  return hdInput->GetClassName();
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::FillDataTypes(svtkCompositeDataSet* hdInput)
{
  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(hdInput->NewIterator());
  svtkDataObjectTreeIterator* treeIter = svtkDataObjectTreeIterator::SafeDownCast(iter);
  if (treeIter)
  {
    treeIter->VisitOnlyLeavesOn();
    treeIter->TraverseSubTreeOn();
  }
  iter->SkipEmptyNodesOff();

  this->Internal->DataTypes.clear();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataObject* dataObject = iter->GetCurrentDataObject();
    svtkDataSet* ds = svtkDataSet::SafeDownCast(dataObject);
    // BUG #0015942: Datasets with no cells or points are considered empty and
    // we'll skip then in our serialization code.
    if (ds && (ds->GetNumberOfPoints() > 0 || ds->GetNumberOfCells() > 0))
    {
      this->Internal->DataTypes.push_back(ds->GetDataObjectType());
    }
    else if (!ds && dataObject)
    {
      this->Internal->DataTypes.push_back(dataObject->GetDataObjectType());
    }
    else
    {
      this->Internal->DataTypes.push_back(-1);
    }
  }
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::CreateWriters(svtkCompositeDataSet* hdInput)
{
  this->Internal->Writers.clear();
  this->FillDataTypes(hdInput);

  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(hdInput->NewIterator());
  svtkDataObjectTreeIterator* treeIter = svtkDataObjectTreeIterator::SafeDownCast(iter);
  if (treeIter)
  {
    treeIter->VisitOnlyLeavesOn();
    treeIter->TraverseSubTreeOn();
  }
  iter->SkipEmptyNodesOff();

  size_t numDatasets = this->Internal->DataTypes.size();
  this->Internal->Writers.resize(numDatasets);

  int i = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), ++i)
  {
    svtkSmartPointer<svtkXMLWriter>& writer = this->Internal->Writers[i];
    svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    svtkTable* table = svtkTable::SafeDownCast(iter->GetCurrentDataObject());
    svtkHyperTreeGrid* htg = svtkHyperTreeGrid::SafeDownCast(iter->GetCurrentDataObject());
    if (ds == nullptr && table == nullptr && htg == nullptr)
    {
      writer = nullptr;
      continue;
    }

    // Create a writer based on the type of this input. We just instantiate
    // svtkXMLDataObjectWriter. That internally creates the write type of writer
    // based on the data type.
    writer.TakeReference(svtkXMLDataObjectWriter::NewWriter(this->Internal->DataTypes[i]));
    if (writer)
    {
      // Copy settings to the writer.
      writer->SetDebug(this->GetDebug());
      writer->SetByteOrder(this->GetByteOrder());
      writer->SetCompressor(this->GetCompressor());
      writer->SetBlockSize(this->GetBlockSize());
      writer->SetDataMode(this->GetDataMode());
      writer->SetEncodeAppendedData(this->GetEncodeAppendedData());
      writer->SetHeaderType(this->GetHeaderType());
      writer->SetIdType(this->GetIdType());

      // Pass input.
      writer->SetInputDataObject(iter->GetCurrentDataObject());
    }
  }
}

//----------------------------------------------------------------------------
svtkXMLWriter* svtkXMLCompositeDataWriter::GetWriter(int index)
{
  int size = static_cast<int>(this->Internal->Writers.size());
  if (index >= 0 && index < size)
  {
    return this->Internal->Writers[index];
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::SplitFileName()
{
  std::string fileName = this->FileName;
  std::string name;

  // Split the file name and extension from the path.
  std::string::size_type pos = fileName.find_last_of("/\\");
  if (pos != fileName.npos)
  {
    // Keep the slash in the file path.
    this->Internal->FilePath = fileName.substr(0, pos + 1);
    name = fileName.substr(pos + 1);
  }
  else
  {
    this->Internal->FilePath = "./";
    name = fileName;
  }

  // Split the extension from the file name.
  pos = name.find_last_of('.');
  if (pos != name.npos)
  {
    this->Internal->FilePrefix = name.substr(0, pos);
  }
  else
  {
    this->Internal->FilePrefix = name;

    // Since a subdirectory is used to store the files, we need to
    // change its name if there is no file extension.
    this->Internal->FilePrefix += "_data";
  }
}

//----------------------------------------------------------------------------
const char* svtkXMLCompositeDataWriter::GetFilePrefix()
{
  return this->Internal->FilePrefix.c_str();
}

//----------------------------------------------------------------------------
const char* svtkXMLCompositeDataWriter::GetFilePath()
{
  return this->Internal->FilePath.c_str();
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::ProgressCallbackFunction(
  svtkObject* caller, unsigned long, void* clientdata, void*)
{
  svtkAlgorithm* w = svtkAlgorithm::SafeDownCast(caller);
  if (w)
  {
    reinterpret_cast<svtkXMLCompositeDataWriter*>(clientdata)->ProgressCallback(w);
  }
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::ProgressCallback(svtkAlgorithm* w)
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
svtkStdString svtkXMLCompositeDataWriter::CreatePieceFileName(int piece)
{
  if (this->Internal->DataTypes[piece] < 0)
  {
    return "";
  }

  std::ostringstream stream;
  stream << this->Internal->FilePrefix.c_str() << "/" << this->Internal->FilePrefix.c_str() << "_"
         << piece << ".";
  const char* ext = this->GetDefaultFileExtensionForDataSet(this->Internal->DataTypes[piece]);
  stream << (ext ? ext : "");
  return stream.str();
}

//----------------------------------------------------------------------------
svtkExecutive* svtkXMLCompositeDataWriter::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataWriter::RemoveWrittenFiles(const char* SubDirectory)
{
  this->RemoveADirectory(SubDirectory);
  this->DeleteAFile();
  this->InputInformation = nullptr;
}
