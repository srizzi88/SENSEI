/*=========================================================================

  Program:   ParaView
  Module:    svtkXMLCompositeDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLCompositeDataReader.h"

#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArraySelection.h"
#include "svtkDataSet.h"
#include "svtkEventForwarderCommand.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkUniformGrid.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLDataParser.h"
#include "svtkXMLHyperTreeGridReader.h"
#include "svtkXMLImageDataReader.h"
#include "svtkXMLPolyDataReader.h"
#include "svtkXMLRectilinearGridReader.h"
#include "svtkXMLStructuredGridReader.h"
#include "svtkXMLTableReader.h"
#include "svtkXMLUnstructuredGridReader.h"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <svtksys/SystemTools.hxx>

struct svtkXMLCompositeDataReaderEntry
{
  const char* extension;
  const char* name;
};

struct svtkXMLCompositeDataReaderInternals
{
  svtkXMLCompositeDataReaderInternals()
  {
    this->Piece = 0;
    this->NumPieces = 1;
    this->NumDataSets = 1;
    this->HasUpdateRestriction = false;
  }

  svtkSmartPointer<svtkXMLDataElement> Root;
  typedef std::map<std::string, svtkSmartPointer<svtkXMLReader> > ReadersType;
  ReadersType Readers;
  static const svtkXMLCompositeDataReaderEntry ReaderList[];
  unsigned int Piece;
  unsigned int NumPieces;
  unsigned int NumDataSets;
  std::set<int> UpdateIndices;
  bool HasUpdateRestriction;
};

//----------------------------------------------------------------------------
svtkXMLCompositeDataReader::svtkXMLCompositeDataReader()
  : PieceDistribution(Block)
{
  this->Internal = new svtkXMLCompositeDataReaderInternals;
}

//----------------------------------------------------------------------------
svtkXMLCompositeDataReader::~svtkXMLCompositeDataReader()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "PieceDistribution: ";
  switch (this->PieceDistribution)
  {
    case Block:
      os << "Block\n";
      break;

    case Interleave:
      os << "Interleave\n";
      break;

    default:
      os << "Invalid (!!)\n";
      break;
  }

  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* svtkXMLCompositeDataReader::GetDataSetName()
{
  return "svtkCompositeDataSet";
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataReader::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkXMLCompositeDataReader::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
std::string svtkXMLCompositeDataReader::GetFilePath()
{
  std::string filePath = this->FileName;
  std::string::size_type pos = filePath.find_last_of("/\\");
  if (pos != filePath.npos)
  {
    filePath = filePath.substr(0, pos);
  }
  else
  {
    filePath = "";
  }

  return filePath;
}

//----------------------------------------------------------------------------
svtkCompositeDataSet* svtkXMLCompositeDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkCompositeDataSet* svtkXMLCompositeDataReader::GetOutput(int port)
{
  svtkDataObject* output =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return svtkCompositeDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataReader::ReadPrimaryElement(svtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  svtkXMLDataElement* root = this->XMLParser->GetRootElement();
  int numNested = root->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    svtkXMLDataElement* eNested = root->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "FieldData") == 0)
    {
      this->FieldDataElement = eNested;
    }
  }

  // Simply save the XML tree. We'll iterate over it later.
  this->Internal->Root = ePrimary;
  return 1;
}

//----------------------------------------------------------------------------
svtkXMLDataElement* svtkXMLCompositeDataReader::GetPrimaryElement()
{
  return this->Internal->Root;
}

//----------------------------------------------------------------------------
std::string svtkXMLCompositeDataReader::GetFileNameFromXML(
  svtkXMLDataElement* xmlElem, const std::string& filePath)
{
  // Construct the name of the internal file.
  const char* file = xmlElem->GetAttribute("file");
  if (!file)
  {
    return std::string();
  }

  std::string fileName;
  if (!(file[0] == '/' || file[1] == ':'))
  {
    fileName = filePath;
    if (fileName.length())
    {
      fileName += "/";
    }
  }
  fileName += file;

  return fileName;
}

//----------------------------------------------------------------------------
svtkXMLReader* svtkXMLCompositeDataReader::GetReaderOfType(const char* type)
{
  if (!type)
  {
    return nullptr;
  }

  svtkXMLCompositeDataReaderInternals::ReadersType::iterator iter =
    this->Internal->Readers.find(type);
  if (iter != this->Internal->Readers.end())
  {
    return iter->second;
  }

  svtkXMLReader* reader = nullptr;
  if (strcmp(type, "svtkXMLImageDataReader") == 0)
  {
    reader = svtkXMLImageDataReader::New();
  }
  else if (strcmp(type, "svtkXMLUnstructuredGridReader") == 0)
  {
    reader = svtkXMLUnstructuredGridReader::New();
  }
  else if (strcmp(type, "svtkXMLPolyDataReader") == 0)
  {
    reader = svtkXMLPolyDataReader::New();
  }
  else if (strcmp(type, "svtkXMLRectilinearGridReader") == 0)
  {
    reader = svtkXMLRectilinearGridReader::New();
  }
  else if (strcmp(type, "svtkXMLStructuredGridReader") == 0)
  {
    reader = svtkXMLStructuredGridReader::New();
  }
  else if (strcmp(type, "svtkXMLTableReader") == 0)
  {
    reader = svtkXMLTableReader::New();
  }
  else if (strcmp(type, "svtkXMLHyperTreeGridReader") == 0)
  {
    reader = svtkXMLHyperTreeGridReader::New();
  }
  if (reader)
  {
    if (this->GetParserErrorObserver())
    {
      reader->SetParserErrorObserver(this->GetParserErrorObserver());
    }
    if (this->HasObserver("ErrorEvent"))
    {
      svtkNew<svtkEventForwarderCommand> fwd;
      fwd->SetTarget(this);
      reader->AddObserver("ErrorEvent", fwd);
    }
    this->Internal->Readers[type] = reader;
    reader->Delete();
  }
  return reader;
}

//----------------------------------------------------------------------------
svtkXMLReader* svtkXMLCompositeDataReader::GetReaderForFile(const std::string& fileName)
{
  // Get the file extension.
  std::string ext = svtksys::SystemTools::GetFilenameLastExtension(fileName);
  if (!ext.empty())
  {
    // remove "." from the extension.
    ext.erase(0, 1);
  }

  // Search for the reader matching this extension.
  const char* rname = nullptr;
  for (const svtkXMLCompositeDataReaderEntry* readerEntry = this->Internal->ReaderList;
       !rname && readerEntry->extension; ++readerEntry)
  {
    if (ext == readerEntry->extension)
    {
      rname = readerEntry->name;
    }
  }

  return this->GetReaderOfType(rname);
}

//----------------------------------------------------------------------------
unsigned int svtkXMLCompositeDataReader::CountLeaves(svtkXMLDataElement* elem)
{
  unsigned int count = 0;
  if (elem)
  {
    unsigned int max = elem->GetNumberOfNestedElements();
    for (unsigned int cc = 0; cc < max; ++cc)
    {
      svtkXMLDataElement* child = elem->GetNestedElement(cc);
      if (child && child->GetName())
      {
        if (strcmp(child->GetName(), "DataSet") == 0)
        {
          count++;
        }
        else
        {
          count += this->CountLeaves(child);
        }
      }
    }
  }
  return count;
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataReader::ReadXMLData()
{
  svtkInformation* info = this->GetCurrentOutputInformation();

  this->Internal->Piece =
    static_cast<unsigned int>(info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  this->Internal->NumPieces = static_cast<unsigned int>(
    info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  this->Internal->NumDataSets = this->CountLeaves(this->GetPrimaryElement());

  svtkDataObject* doOutput = info->Get(svtkDataObject::DATA_OBJECT());
  svtkCompositeDataSet* composite = svtkCompositeDataSet::SafeDownCast(doOutput);
  if (!composite)
  {
    return;
  }

  this->ReadFieldData();

  // Find the path to this file in case the internal files are
  // specified as relative paths.
  std::string filePath = this->GetFilePath();

  svtkInformation* outInfo = this->GetCurrentOutputInformation();
  if (outInfo->Has(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES()))
  {
    this->Internal->HasUpdateRestriction = true;
    this->Internal->UpdateIndices = std::set<int>();
    int length = outInfo->Length(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
    if (length > 0)
    {
      int* idx = outInfo->Get(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES());
      this->Internal->UpdateIndices = std::set<int>(idx, idx + length);

      // Change the total number of datasets so that we'll properly load
      // balance across the valid datasets.
      this->Internal->NumDataSets = length;
    }
  }
  else
  {
    this->Internal->HasUpdateRestriction = false;
  }

  // All processes create the entire tree structure however, but each one only
  // reads the datasets assigned to it.
  unsigned int dataSetIndex = 0;
  this->ReadComposite(this->GetPrimaryElement(), composite, filePath.c_str(), dataSetIndex);
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataReader::ShouldReadDataSet(unsigned int idx)
{
  // Apply the update restriction:
  if (this->Internal->HasUpdateRestriction)
  {
    auto iter = this->Internal->UpdateIndices.find(idx);
    if (iter == this->Internal->UpdateIndices.end())
    {
      return 0;
    }

    // Update the dataset index to its position in the update indices:
    idx = std::distance(this->Internal->UpdateIndices.begin(), iter);
  }

  int result = 0;

  switch (this->PieceDistribution)
  {
    case svtkXMLCompositeDataReader::Block:
      result = this->DataSetIsValidForBlockStrategy(idx) ? 1 : 0;
      break;

    case svtkXMLCompositeDataReader::Interleave:
      result = this->DataSetIsValidForInterleaveStrategy(idx) ? 1 : 0;
      break;

    default:
      svtkErrorMacro("Invalid PieceDistribution setting: " << this->PieceDistribution);
      break;
  }

  return result;
}

//------------------------------------------------------------------------------
bool svtkXMLCompositeDataReader::DataSetIsValidForBlockStrategy(unsigned int idx)
{
  // Minimum number of datasets per block:
  unsigned int blockSize = 1;

  // Number of blocks with an extra dataset due to overflow:
  unsigned int overflowBlocks = 0;

  // Adjust values if overflow is detected:
  if (this->Internal->NumPieces < this->Internal->NumDataSets)
  {
    blockSize = this->Internal->NumDataSets / this->Internal->NumPieces;
    overflowBlocks = this->Internal->NumDataSets % this->Internal->NumPieces;
  }

  // Size of an overflow block:
  const unsigned int blockSizeOverflow = blockSize + 1;

  unsigned int minDS; // Minimum valid dataset index
  unsigned int maxDS; // Maximum valid dataset index
  if (this->Internal->Piece < overflowBlocks)
  {
    minDS = blockSizeOverflow * this->Internal->Piece;
    maxDS = minDS + blockSizeOverflow;
  }
  else
  {
    // Account for earlier blocks that have an overflowed dataset:
    const unsigned int overflowOffset = blockSizeOverflow * overflowBlocks;
    // Number of preceding blocks that don't overflow:
    const unsigned int regularBlocks = this->Internal->Piece - overflowBlocks;
    // Offset due to regular blocks:
    const unsigned int regularOffset = blockSize * regularBlocks;

    minDS = overflowOffset + regularOffset;
    maxDS = minDS + blockSize;
  }

  return idx >= minDS && idx < maxDS;
}

//------------------------------------------------------------------------------
bool svtkXMLCompositeDataReader::DataSetIsValidForInterleaveStrategy(unsigned int idx)
{
  // Use signed integers for the modulus -- otherwise weird things like
  // (-1 % 3) == 0 will happen!
  int i = static_cast<int>(idx);
  int p = static_cast<int>(this->Internal->Piece);
  int n = static_cast<int>(this->Internal->NumPieces);

  return ((i - p) % n) == 0;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkXMLCompositeDataReader::ReadDataObject(
  svtkXMLDataElement* xmlElem, const char* filePath)
{
  // Get the reader for this file
  std::string fileName = this->GetFileNameFromXML(xmlElem, filePath);
  if (fileName.empty())
  { // No filename in XML element. Not necessarily an error.
    return nullptr;
  }
  svtkXMLReader* reader = this->GetReaderForFile(fileName);
  if (!reader)
  {
    svtkErrorMacro("Could not create reader for " << fileName);
    return nullptr;
  }
  reader->SetFileName(fileName.c_str());
  reader->GetPointDataArraySelection()->CopySelections(this->PointDataArraySelection);
  reader->GetCellDataArraySelection()->CopySelections(this->CellDataArraySelection);
  reader->GetColumnArraySelection()->CopySelections(this->ColumnArraySelection);
  reader->Update();
  svtkDataObject* output = reader->GetOutputDataObject(0);
  if (!output)
  {
    return nullptr;
  }

  svtkDataObject* outputCopy = output->NewInstance();
  outputCopy->ShallowCopy(output);
  return outputCopy;
}

//----------------------------------------------------------------------------
svtkDataSet* svtkXMLCompositeDataReader::ReadDataset(svtkXMLDataElement* xmlElem, const char* filePath)
{
  return svtkDataSet::SafeDownCast(ReadDataObject(xmlElem, filePath));
}

//----------------------------------------------------------------------------
int svtkXMLCompositeDataReader::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Superclass::RequestInformation(request, inputVector, outputVector);

  svtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//----------------------------------------------------------------------------
void svtkXMLCompositeDataReader::SyncDataArraySelections(
  svtkXMLReader* accum, svtkXMLDataElement* xmlElem, const std::string& filePath)
{
  // Get the reader for this file
  std::string fileName = this->GetFileNameFromXML(xmlElem, filePath);
  if (fileName.empty())
  { // No filename in XML element. Not necessarily an error.
    return;
  }
  svtkXMLReader* reader = this->GetReaderForFile(fileName);
  if (!reader)
  {
    svtkErrorMacro("Could not create reader for " << fileName);
    return;
  }
  reader->SetFileName(fileName.c_str());
  // initialize array selection so we don't have any residual array selections
  // from previous use of the reader.
  reader->GetPointDataArraySelection()->RemoveAllArrays();
  reader->GetCellDataArraySelection()->RemoveAllArrays();
  reader->GetColumnArraySelection()->RemoveAllArrays();
  reader->UpdateInformation();

  // Merge the arrays:
  accum->GetPointDataArraySelection()->Union(reader->GetPointDataArraySelection());
  accum->GetCellDataArraySelection()->Union(reader->GetCellDataArraySelection());
  accum->GetColumnArraySelection()->Union(reader->GetColumnArraySelection());
}

//----------------------------------------------------------------------------
const svtkXMLCompositeDataReaderEntry svtkXMLCompositeDataReaderInternals::ReaderList[] = {
  { "vtp", "svtkXMLPolyDataReader" }, { "vtu", "svtkXMLUnstructuredGridReader" },
  { "vti", "svtkXMLImageDataReader" }, { "vtr", "svtkXMLRectilinearGridReader" },
  { "vts", "svtkXMLStructuredGridReader" }, { "vtt", "svtkXMLTableReader" },
  { "htg", "svtkXMLHyperTreeGridReader" }, { nullptr, nullptr }
};
