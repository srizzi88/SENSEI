/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmfReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXdmfReader.h"
#include "svtkXdmfHeavyData.h"
#include "svtkXdmfReaderInternal.h"

#include "svtkCharArray.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataObjectTypes.h"
#include "svtkDataSet.h"
#include "svtkExtentTranslator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkXMLParser.h"
#include "svtksys/FStream.hxx"

svtkCxxSetObjectMacro(svtkXdmfReader, InputArray, svtkCharArray);

//============================================================================
class svtkXdmfReaderTester : public svtkXMLParser
{
public:
  svtkTypeMacro(svtkXdmfReaderTester, svtkXMLParser);
  static svtkXdmfReaderTester* New();
  int TestReadFile()
  {
    this->Valid = 0;
    if (!this->FileName)
    {
      return 0;
    }

    svtksys::ifstream inFile(this->FileName);
    if (!inFile)
    {
      return 0;
    }

    this->SetStream(&inFile);
    this->Done = 0;

    this->Parse();

    if (this->Done && this->Valid)
    {
      return 1;
    }
    return 0;
  }
  void StartElement(const char* name, const char**) override
  {
    this->Done = 1;
    if (strcmp(name, "Xdmf") == 0)
    {
      this->Valid = 1;
    }
  }

protected:
  svtkXdmfReaderTester()
  {
    this->Valid = 0;
    this->Done = 0;
  }

private:
  void ReportStrayAttribute(const char*, const char*, const char*) override {}
  void ReportMissingAttribute(const char*, const char*) override {}
  void ReportBadAttribute(const char*, const char*, const char*) override {}
  void ReportUnknownElement(const char*) override {}
  void ReportXmlParseError() override {}

  int ParsingComplete() override { return this->Done; }
  int Valid;
  int Done;
  svtkXdmfReaderTester(const svtkXdmfReaderTester&) = delete;
  void operator=(const svtkXdmfReaderTester&) = delete;
};
svtkStandardNewMacro(svtkXdmfReaderTester);

svtkStandardNewMacro(svtkXdmfReader);
//----------------------------------------------------------------------------
svtkXdmfReader::svtkXdmfReader()
{
  this->DomainName = 0;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->XdmfDocument = new svtkXdmfDocument();
  this->LastTimeIndex = 0;
  this->SILUpdateStamp = 0;

  this->PointArraysCache = new svtkXdmfArraySelection;
  this->CellArraysCache = new svtkXdmfArraySelection;
  this->GridsCache = new svtkXdmfArraySelection;
  this->SetsCache = new svtkXdmfArraySelection;

  this->FileName = nullptr;
  this->ReadFromInputString = false;
  this->InputString = nullptr;
  this->InputStringLength = 0;
  this->InputStringPos = 0;
  this->InputArray = nullptr;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkXdmfReader::~svtkXdmfReader()
{
  this->SetDomainName(0);
  delete this->XdmfDocument;
  this->XdmfDocument = 0;

  delete this->PointArraysCache;
  delete this->CellArraysCache;
  delete this->GridsCache;
  delete this->SetsCache;

  this->ClearDataSetCache();

  this->SetFileName(nullptr);
  delete[] this->InputString;
}

//----------------------------------------------------------------------------
void svtkXdmfReader::SetInputString(const char* in)
{
  int len = 0;
  if (in != nullptr)
  {
    len = static_cast<int>(strlen(in));
  }
  this->SetInputString(in, len);
}

//----------------------------------------------------------------------------
void svtkXdmfReader::SetBinaryInputString(const char* in, int len)
{
  this->SetInputString(in, len);
}

//----------------------------------------------------------------------------
void svtkXdmfReader::SetInputString(const char* in, int len)
{
  if (this->Debug)
  {
    svtkDebugMacro(<< "SetInputString len: " << len << " in: " << (in ? in : "(null)"));
  }

  if (this->InputString && in && strncmp(in, this->InputString, len) == 0)
  {
    return;
  }

  delete[] this->InputString;

  if (in && len > 0)
  {
    // Add a nullptr terminator so that GetInputString
    // callers (from wrapped languages) get a valid
    // C string in *ALL* cases...
    //
    this->InputString = new char[len + 1];
    memcpy(this->InputString, in, len);
    this->InputString[len] = 0;
    this->InputStringLength = len;
  }
  else
  {
    this->InputString = nullptr;
    this->InputStringLength = 0;
  }

  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmfReader::CanReadFile(const char* filename)
{
  svtkXdmfReaderTester* tester = svtkXdmfReaderTester::New();
  tester->SetFileName(filename);
  int res = tester->TestReadFile();
  tester->Delete();
  return res;
}

//----------------------------------------------------------------------------
int svtkXdmfReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkXdmfReader::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObjectInternal(outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool svtkXdmfReader::PrepareDocument()
{
  // Calling this method repeatedly is okay. It does work only when something
  // has changed.
  if (this->GetReadFromInputString())
  {
    const char* data = 0;
    unsigned int data_length = 0;
    if (this->InputArray)
    {
      data = this->InputArray->GetPointer(0);
      data_length = static_cast<unsigned int>(
        this->InputArray->GetNumberOfTuples() * this->InputArray->GetNumberOfComponents());
    }
    else if (this->InputString)
    {
      data = this->InputString;
      data_length = this->InputStringLength;
    }
    else
    {
      svtkErrorMacro("No input string specified");
      return false;
    }
    if (!this->XdmfDocument->ParseString(data, data_length))
    {
      svtkErrorMacro("Failed to parse xmf.");
      return false;
    }
  }
  else
  {
    // Parse the file...
    if (!this->FileName)
    {
      svtkErrorMacro("File name not set");
      return false;
    }

    // First make sure the file exists.  This prevents an empty file
    // from being created on older compilers.
    if (!svtksys::SystemTools::FileExists(this->FileName))
    {
      svtkErrorMacro("Error opening file " << this->FileName);
      return false;
    }

    if (!this->XdmfDocument->Parse(this->FileName))
    {
      svtkErrorMacro("Failed to parse xmf file: " << this->FileName);
      return false;
    }
  }

  if (this->DomainName)
  {
    if (!this->XdmfDocument->SetActiveDomain(this->DomainName))
    {
      svtkErrorMacro("Invalid domain: " << this->DomainName);
      return false;
    }
  }
  else
  {
    this->XdmfDocument->SetActiveDomain(static_cast<int>(0));
  }

  if (this->XdmfDocument->GetActiveDomain() &&
    this->XdmfDocument->GetActiveDomain()->GetSIL()->GetMTime() > this->GetMTime())
  {
    this->SILUpdateStamp++;
  }

  this->LastTimeIndex = 0; // reset time index when the file changes.
  return (this->XdmfDocument->GetActiveDomain() != 0);
}

//----------------------------------------------------------------------------
int svtkXdmfReader::RequestDataObjectInternal(svtkInformationVector* outputVector)
{
  if (!this->PrepareDocument())
  {
    return 0;
  }

  int svtk_type = this->XdmfDocument->GetActiveDomain()->GetSVTKDataType();
  if (this->XdmfDocument->GetActiveDomain()->GetSetsSelection()->GetNumberOfArrays() > 0)
  {
    // if the data has any sets, then we are forced to using multiblock.
    svtk_type = SVTK_MULTIBLOCK_DATA_SET;
  }

  svtkDataObject* output = svtkDataObject::GetData(outputVector, 0);
  if (!output || output->GetDataObjectType() != svtk_type)
  {
    output = svtkDataObjectTypes::NewDataObject(svtk_type);
    outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), output);
    this->GetOutputPortInformation(0)->Set(
      svtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    output->Delete();
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmfReader::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  if (!this->PrepareDocument())
  {
    return 0;
  }

  // Pass any cached user-selections to the active domain.
  this->PassCachedSelections();

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkXdmfDomain* domain = this->XdmfDocument->GetActiveDomain();

  // * Publish the fact that this reader can satisfy any piece request.
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  this->LastTimeIndex = this->ChooseTimeStep(outInfo);

  // Set the requested time-step on the domain. Thus, now when we go to get
  // information, we can (ideally) get information about that time-step.
  // this->XdmfDocument->GetActiveDomain()->SetTimeStep(this->LastTimeIndex);

  // * If producing structured dataset put information about whole extents etc.
  if (domain->GetNumberOfGrids() == 1 && domain->IsStructured(domain->GetGrid(0)) &&
    domain->GetSetsSelection()->GetNumberOfArrays() == 0)
  {
    xdmf2::XdmfGrid* xmfGrid = domain->GetGrid(0);
    // just in the case the top-level grid is a temporal collection, then pick
    // the sub-grid to fetch the extents etc.
    xmfGrid = domain->GetGrid(xmfGrid, domain->GetTimeForIndex(this->LastTimeIndex));
    int whole_extent[6];
    if (domain->GetWholeExtent(xmfGrid, whole_extent))
    {
      // re-scale the whole_extent using the stride.
      whole_extent[1] /= this->Stride[0];
      whole_extent[3] /= this->Stride[1];
      whole_extent[5] /= this->Stride[2];

      outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whole_extent, 6);
    }
    double origin[3];
    double spacing[3];
    if (domain->GetOriginAndSpacing(xmfGrid, origin, spacing))
    {
      spacing[0] *= this->Stride[0];
      spacing[1] *= this->Stride[1];
      spacing[2] *= this->Stride[2];
      outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
      outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
    }
  }

  // * Publish the SIL which provides information about the grid hierarchy.
  outInfo->Set(svtkDataObject::SIL(), domain->GetSIL());

  // * Publish time information.
  const std::map<int, XdmfFloat64>& ts = domain->GetTimeStepsRev();
  std::vector<double> time_steps(ts.size());
  std::map<int, XdmfFloat64>::const_iterator it = ts.begin();
  for (int i = 0; it != ts.end(); i++, ++it)
  {
    time_steps[i] = it->second;
  }

  if (time_steps.size() > 0)
  {
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &time_steps[0],
      static_cast<int>(time_steps.size()));
    double timeRange[2];
    timeRange[0] = time_steps.front();
    timeRange[1] = time_steps.back();
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmfReader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  if (!this->PrepareDocument())
  {
    return 0;
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // * Collect information about what part of the data is requested.
  unsigned int updatePiece = 0;
  unsigned int updateNumPieces = 1;
  int ghost_levels = 0;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
    outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
  {
    updatePiece = static_cast<unsigned int>(
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    updateNumPieces = static_cast<unsigned int>(
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  }
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
  {
    ghost_levels = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  }

  // will be set for structured datasets only.
  int update_extent[6] = { 0, -1, 0, -1, 0, -1 };
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), update_extent);
    if (outInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int wholeExtent[6];
      outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
      svtkNew<svtkExtentTranslator> et;
      et->SetWholeExtent(wholeExtent);
      et->SetPiece(updatePiece);
      et->SetNumberOfPieces(updateNumPieces);
      et->SetGhostLevel(ghost_levels);
      et->PieceToExtent();
      et->GetExtent(update_extent);
    }
  }

  this->LastTimeIndex = this->ChooseTimeStep(outInfo);
  if (this->LastTimeIndex == 0)
  {
    this->ClearDataSetCache();
  }

  svtkXdmfHeavyData dataReader(this->XdmfDocument->GetActiveDomain(), this);
  dataReader.Piece = updatePiece;
  dataReader.NumberOfPieces = updateNumPieces;
  dataReader.GhostLevels = ghost_levels;
  dataReader.Extents[0] = update_extent[0] * this->Stride[0];
  dataReader.Extents[1] = update_extent[1] * this->Stride[0];
  dataReader.Extents[2] = update_extent[2] * this->Stride[1];
  dataReader.Extents[3] = update_extent[3] * this->Stride[1];
  dataReader.Extents[4] = update_extent[4] * this->Stride[2];
  dataReader.Extents[5] = update_extent[5] * this->Stride[2];
  dataReader.Stride[0] = this->Stride[0];
  dataReader.Stride[1] = this->Stride[1];
  dataReader.Stride[2] = this->Stride[2];
  dataReader.Time = this->XdmfDocument->GetActiveDomain()->GetTimeForIndex(this->LastTimeIndex);

  svtkDataObject* data = dataReader.ReadData();
  if (!data)
  {
    svtkErrorMacro("Failed to read data.");
    return 0;
  }

  svtkDataObject* output = svtkDataObject::GetData(outInfo);

  if (!output->IsA(data->GetClassName()))
  {
    // BUG #0013766: Just in case the data type expected doesn't match the
    // produced data type, we should print a warning.
    svtkWarningMacro("Data type generated (" << data->GetClassName()
                                            << ") "
                                               "does not match data type expected ("
                                            << output->GetClassName()
                                            << "). "
                                               "Reader may not produce valid data.");
  }
  output->ShallowCopy(data);
  data->Delete();

  if (this->LastTimeIndex < this->XdmfDocument->GetActiveDomain()->GetTimeSteps().size())
  {
    double time = this->XdmfDocument->GetActiveDomain()->GetTimeForIndex(this->LastTimeIndex);
    output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), time);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmfReader::ChooseTimeStep(svtkInformation* outInfo)
{
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // we do not support multiple timestep requests.
    double time = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    return this->XdmfDocument->GetActiveDomain()->GetIndexForTime(time);
  }

  // if no timestep was requested, just return what we read last.
  return this->LastTimeIndex;
}

//----------------------------------------------------------------------------
svtkXdmfArraySelection* svtkXdmfReader::GetPointArraySelection()
{
  return this->XdmfDocument->GetActiveDomain()
    ? this->XdmfDocument->GetActiveDomain()->GetPointArraySelection()
    : this->PointArraysCache;
}

//----------------------------------------------------------------------------
svtkXdmfArraySelection* svtkXdmfReader::GetCellArraySelection()
{
  return this->XdmfDocument->GetActiveDomain()
    ? this->XdmfDocument->GetActiveDomain()->GetCellArraySelection()
    : this->CellArraysCache;
}

//----------------------------------------------------------------------------
svtkXdmfArraySelection* svtkXdmfReader::GetGridSelection()
{
  return this->XdmfDocument->GetActiveDomain()
    ? this->XdmfDocument->GetActiveDomain()->GetGridSelection()
    : this->GridsCache;
}

//----------------------------------------------------------------------------
svtkXdmfArraySelection* svtkXdmfReader::GetSetsSelection()
{
  return this->XdmfDocument->GetActiveDomain()
    ? this->XdmfDocument->GetActiveDomain()->GetSetsSelection()
    : this->SetsCache;
}

//----------------------------------------------------------------------------
int svtkXdmfReader::GetNumberOfGrids()
{
  return this->GetGridSelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmfReader::SetGridStatus(const char* gridname, int status)
{
  this->GetGridSelection()->SetArrayStatus(gridname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmfReader::GetGridStatus(const char* arrayname)
{
  return this->GetGridSelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmfReader::GetGridName(int index)
{
  return this->GetGridSelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
int svtkXdmfReader::GetNumberOfPointArrays()
{
  return this->GetPointArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmfReader::SetPointArrayStatus(const char* arrayname, int status)
{
  this->GetPointArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmfReader::GetPointArrayStatus(const char* arrayname)
{
  return this->GetPointArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmfReader::GetPointArrayName(int index)
{
  return this->GetPointArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
int svtkXdmfReader::GetNumberOfCellArrays()
{
  return this->GetCellArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmfReader::SetCellArrayStatus(const char* arrayname, int status)
{
  this->GetCellArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmfReader::GetCellArrayStatus(const char* arrayname)
{
  return this->GetCellArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmfReader::GetCellArrayName(int index)
{
  return this->GetCellArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
int svtkXdmfReader::GetNumberOfSets()
{
  return this->GetSetsSelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmfReader::SetSetStatus(const char* arrayname, int status)
{
  this->GetSetsSelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmfReader::GetSetStatus(const char* arrayname)
{
  return this->GetSetsSelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmfReader::GetSetName(int index)
{
  return this->GetSetsSelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
void svtkXdmfReader::PassCachedSelections()
{
  if (!this->XdmfDocument->GetActiveDomain())
  {
    return;
  }

  this->GetPointArraySelection()->Merge(*this->PointArraysCache);
  this->GetCellArraySelection()->Merge(*this->CellArraysCache);
  this->GetGridSelection()->Merge(*this->GridsCache);
  this->GetSetsSelection()->Merge(*this->SetsCache);

  // Clear the cache.
  this->PointArraysCache->clear();
  this->CellArraysCache->clear();
  this->GridsCache->clear();
  this->SetsCache->clear();
}

//----------------------------------------------------------------------------
void svtkXdmfReader::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "ReadFromInputString: " << (this->ReadFromInputString ? "On\n" : "Off\n");

  if (this->InputArray)
  {
    os << indent << "Input Array: "
       << "\n";
    this->InputArray->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Input String: (None)\n";
  }

  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
svtkGraph* svtkXdmfReader::GetSIL()
{
  if (svtkXdmfDomain* domain = this->XdmfDocument->GetActiveDomain())
  {
    return domain->GetSIL();
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkXdmfReader::ClearDataSetCache()
{
  XdmfReaderCachedData::iterator it = this->DataSetCache.begin();
  while (it != this->DataSetCache.end())
  {
    if (it->second.dataset != nullptr)
    {
      it->second.dataset->Delete();
    }
    ++it;
  }
  this->DataSetCache.clear();
}

//----------------------------------------------------------------------------
svtkXdmfReader::XdmfReaderCachedData& svtkXdmfReader::GetDataSetCache()
{
  return this->DataSetCache;
}
