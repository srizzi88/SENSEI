/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPOpenFOAMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This class was developed by Takuya Oshima at Niigata University,
// Japan (oshima@eng.niigata-u.ac.jp).

#include "svtkPOpenFOAMReader.h"

#include "svtkAppendCompositeDataLeaves.h"
#include "svtkCharArray.h"
#include "svtkCollection.h"
#include "svtkDataArraySelection.h"
#include "svtkDirectory.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkSortDataArray.h"
#include "svtkStdString.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStringArray.h"

svtkStandardNewMacro(svtkPOpenFOAMReader);
svtkCxxSetObjectMacro(svtkPOpenFOAMReader, Controller, svtkMultiProcessController);

//-----------------------------------------------------------------------------
svtkPOpenFOAMReader::svtkPOpenFOAMReader()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
  if (this->Controller == nullptr)
  {
    this->NumProcesses = 1;
    this->ProcessId = 0;
  }
  else
  {
    this->NumProcesses = this->Controller->GetNumberOfProcesses();
    this->ProcessId = this->Controller->GetLocalProcessId();
  }
  this->CaseType = RECONSTRUCTED_CASE;
  this->MTimeOld = 0;
}

//-----------------------------------------------------------------------------
svtkPOpenFOAMReader::~svtkPOpenFOAMReader()
{
  this->SetController(nullptr);
}

//-----------------------------------------------------------------------------
void svtkPOpenFOAMReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Case Type: " << this->CaseType << endl;
  os << indent << "MTimeOld: " << this->MTimeOld << endl;
  os << indent << "Number of Processes: " << this->NumProcesses << endl;
  os << indent << "Process Id: " << this->ProcessId << endl;
  os << indent << "Controller: " << this->Controller << endl;
}

//-----------------------------------------------------------------------------
void svtkPOpenFOAMReader::SetCaseType(const int t)
{
  if (this->CaseType != t)
  {
    this->CaseType = static_cast<caseType>(t);
    this->Refresh = true;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int svtkPOpenFOAMReader::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->CaseType == RECONSTRUCTED_CASE)
  {
    int ret = 1;
    if (this->ProcessId == 0)
    {
      ret = this->Superclass::RequestInformation(request, inputVector, outputVector);
    }
    if (this->NumProcesses > 1)
    {
      // if there was an error in process 0 abort all processes
      this->BroadcastStatus(ret);
      if (ret == 0)
      {
        svtkErrorMacro(<< "The master process returned an error.");
        return 0;
      }

      svtkDoubleArray* timeValues;
      if (this->ProcessId == 0)
      {
        timeValues = this->Superclass::GetTimeValues();
      }
      else
      {
        timeValues = svtkDoubleArray::New();
      }
      this->Controller->Broadcast(timeValues, 0);
      if (this->ProcessId != 0)
      {
        this->Superclass::SetTimeInformation(outputVector, timeValues);
        timeValues->Delete();
        this->Superclass::Refresh = false;
      }
      this->GatherMetaData(); // pvserver deadlocks without this
    }

    return ret;
  }

  if (!this->Superclass::FileName || strlen(this->Superclass::FileName) == 0)
  {
    svtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  if (*this->Superclass::FileNameOld != this->Superclass::FileName ||
    this->Superclass::ListTimeStepsByControlDict !=
      this->Superclass::ListTimeStepsByControlDictOld ||
    this->Superclass::SkipZeroTime != this->Superclass::SkipZeroTimeOld ||
    this->Superclass::Refresh)
  {
    // retain selection status when just refreshing a case
    if (!this->Superclass::FileNameOld->empty() &&
      *this->Superclass::FileNameOld != this->Superclass::FileName)
    {
      // clear selections
      this->Superclass::CellDataArraySelection->RemoveAllArrays();
      this->Superclass::PointDataArraySelection->RemoveAllArrays();
      this->Superclass::LagrangianDataArraySelection->RemoveAllArrays();
      this->Superclass::PatchDataArraySelection->RemoveAllArrays();
    }

    *this->Superclass::FileNameOld = svtkStdString(this->FileName);
    this->Superclass::Readers->RemoveAllItems();
    this->Superclass::NumberOfReaders = 0;

    svtkStringArray* procNames = svtkStringArray::New();
    svtkDoubleArray* timeValues;

    // recreate case information
    svtkStdString masterCasePath, controlDictPath;
    this->Superclass::CreateCasePath(masterCasePath, controlDictPath);

    this->Superclass::CreateCharArrayFromString(
      this->Superclass::CasePath, "CasePath", masterCasePath);

    int ret = 1;
    if (this->ProcessId == 0)
    {
      // search and list processor subdirectories
      svtkDirectory* dir = svtkDirectory::New();
      if (!dir->Open(masterCasePath.c_str()))
      {
        svtkErrorMacro(<< "Can't open " << masterCasePath.c_str());
        dir->Delete();
        this->BroadcastStatus(ret = 0);
        return 0;
      }
      svtkIntArray* procNos = svtkIntArray::New();
      for (int fileI = 0; fileI < dir->GetNumberOfFiles(); fileI++)
      {
        const svtkStdString subDir(dir->GetFile(fileI));
        if (subDir.substr(0, 9) == "processor")
        {
          const svtkStdString procNoStr(subDir.substr(9, svtkStdString::npos));
          char* conversionEnd;
          const int procNo = strtol(procNoStr.c_str(), &conversionEnd, 10);
          if (procNoStr.c_str() + procNoStr.length() == conversionEnd && procNo >= 0)
          {
            procNos->InsertNextValue(procNo);
            procNames->InsertNextValue(subDir);
          }
        }
      }
      procNos->Squeeze();
      procNames->Squeeze();
      dir->Delete();

      // sort processor subdirectories by processor numbers
      svtkSortDataArray::Sort(procNos, procNames);
      procNos->Delete();

      // get time directories from the first processor subdirectory
      if (procNames->GetNumberOfTuples() > 0)
      {
        svtkOpenFOAMReader* masterReader = svtkOpenFOAMReader::New();
        masterReader->SetFileName(this->FileName);
        masterReader->SetParent(this);
        masterReader->SetSkipZeroTime(this->SkipZeroTime);
        masterReader->SetUse64BitLabels(this->Use64BitLabels);
        masterReader->SetUse64BitFloats(this->Use64BitFloats);
        if (!masterReader->MakeInformationVector(outputVector, procNames->GetValue(0)) ||
          !masterReader->MakeMetaDataAtTimeStep(true))
        {
          procNames->Delete();
          masterReader->Delete();
          this->BroadcastStatus(ret = 0);
          return 0;
        }
        this->Superclass::Readers->AddItem(masterReader);
        timeValues = masterReader->GetTimeValues();
        masterReader->Delete();
      }
      else
      {
        timeValues = svtkDoubleArray::New();
        this->Superclass::SetTimeInformation(outputVector, timeValues);
      }
    }
    else
    {
      timeValues = svtkDoubleArray::New();
    }

    if (this->NumProcesses > 1)
    {
      // if there was an error in process 0 abort all processes
      this->BroadcastStatus(ret);
      if (ret == 0)
      {
        svtkErrorMacro(<< "The master process returned an error.");
        timeValues->Delete(); // don't have to care about process 0
        return 0;
      }

      this->Broadcast(procNames);
      this->Controller->Broadcast(timeValues, 0);
      if (this->ProcessId != 0)
      {
        this->Superclass::SetTimeInformation(outputVector, timeValues);
        timeValues->Delete();
      }
    }

    if (this->ProcessId == 0 && procNames->GetNumberOfTuples() == 0)
    {
      timeValues->Delete();
    }

    // create reader instances for other processor subdirectories
    // skip processor0 since it's already created
    for (int procI = (this->ProcessId ? this->ProcessId : this->NumProcesses);
         procI < procNames->GetNumberOfTuples(); procI += this->NumProcesses)
    {
      svtkOpenFOAMReader* subReader = svtkOpenFOAMReader::New();
      subReader->SetFileName(this->FileName);
      subReader->SetParent(this);
      subReader->SetUse64BitLabels(this->Use64BitLabels);
      subReader->SetUse64BitFloats(this->Use64BitFloats);
      // if getting metadata failed simply delete the reader instance
      if (subReader->MakeInformationVector(nullptr, procNames->GetValue(procI)) &&
        subReader->MakeMetaDataAtTimeStep(true))
      {
        this->Superclass::Readers->AddItem(subReader);
      }
      else
      {
        svtkWarningMacro(<< "Removing reader for processor subdirectory "
                        << procNames->GetValue(procI).c_str());
      }
      subReader->Delete();
    }

    procNames->Delete();

    this->GatherMetaData();
    this->Superclass::Refresh = false;
  }

  outputVector->GetInformationObject(0)->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//-----------------------------------------------------------------------------
int svtkPOpenFOAMReader::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->CaseType == RECONSTRUCTED_CASE)
  {
    int ret = 1;
    if (this->ProcessId == 0)
    {
      ret = this->Superclass::RequestData(request, inputVector, outputVector);
    }
    this->BroadcastStatus(ret);
    this->GatherMetaData();
    return ret;
  }

  svtkSmartPointer<svtkMultiProcessController> splitController;
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int ret = 1;
  if (this->Superclass::Readers->GetNumberOfItems() > 0)
  {
    int nSteps = 0;
    double requestedTimeValue(0);
    if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
      requestedTimeValue = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
      nSteps = outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
      if (nSteps > 0)
      {
        outInfo->Set(svtkDataObject::DATA_TIME_STEP(), requestedTimeValue);
      }
    }

    svtkAppendCompositeDataLeaves* append = svtkAppendCompositeDataLeaves::New();
    // append->AppendFieldDataOn();

    svtkOpenFOAMReader* reader;
    this->Superclass::CurrentReaderIndex = 0;
    this->Superclass::Readers->InitTraversal();
    while ((reader = svtkOpenFOAMReader::SafeDownCast(
              this->Superclass::Readers->GetNextItemAsObject())) != nullptr)
    {
      // even if the child readers themselves are not modified, mark
      // them as modified if "this" has been modified, since they
      // refer to the property of "this"
      if ((nSteps > 0 && reader->SetTimeValue(requestedTimeValue)) ||
        this->MTimeOld != this->GetMTime())
      {
        reader->Modified();
      }
      if (reader->MakeMetaDataAtTimeStep(false))
      {
        append->AddInputConnection(reader->GetOutputPort());
      }
    }

    this->GatherMetaData();

    if (append->GetNumberOfInputConnections(0) == 0)
    {
      output->Initialize();
      ret = 0;
    }
    else
    {
      // reader->RequestInformation() and RequestData() are called
      // for all reader instances without setting UPDATE_TIME_STEPS
      append->Update();
      output->ShallowCopy(append->GetOutput());
    }
    append->Delete();

    // known issue: output for process without sub-reader will not have CasePath
    output->GetFieldData()->AddArray(this->Superclass::CasePath);

    // Processor 0 needs to broadcast the structure of the multiblock
    // to the processors that didn't have the chance to load something
    // To do so, we split the controller to broadcast only to the interested
    // processors (else case below) and avoid useless communication.
    splitController.TakeReference(
      this->Controller->PartitionController(this->ProcessId == 0, this->ProcessId));
    if (this->ProcessId == 0)
    {
      svtkNew<svtkMultiBlockDataSet> mb;
      mb->CopyStructure(output);
      splitController->Broadcast(mb, 0);
    }
  }
  else
  {
    this->GatherMetaData();

    // This rank did not receive anything so data structure is void.
    // Let's receive the empty but structured multiblock from processor 0
    splitController.TakeReference(this->Controller->PartitionController(true, this->ProcessId));
    svtkNew<svtkMultiBlockDataSet> mb;
    splitController->Broadcast(mb, 0);
    output->CopyStructure(mb);
  }

  this->Superclass::UpdateStatus();
  this->MTimeOld = this->GetMTime();

  return ret;
}

//-----------------------------------------------------------------------------
void svtkPOpenFOAMReader::BroadcastStatus(int& status)
{
  if (this->NumProcesses > 1)
  {
    this->Controller->Broadcast(&status, 1, 0);
  }
}

//-----------------------------------------------------------------------------
void svtkPOpenFOAMReader::GatherMetaData()
{
  if (this->NumProcesses > 1)
  {
    this->AllGather(this->Superclass::PatchDataArraySelection);
    this->AllGather(this->Superclass::CellDataArraySelection);
    this->AllGather(this->Superclass::PointDataArraySelection);
    this->AllGather(this->Superclass::LagrangianDataArraySelection);
    // omit removing duplicated entries of LagrangianPaths as well
    // when the number of processes is 1 assuming there's no duplicate
    // entry within a process
    this->AllGather(this->Superclass::LagrangianPaths);
  }
}

//-----------------------------------------------------------------------------
// Broadcast a svtkStringArray in process 0 to all processes
void svtkPOpenFOAMReader::Broadcast(svtkStringArray* sa)
{
  svtkIdType lengths[2];
  if (this->ProcessId == 0)
  {
    lengths[0] = sa->GetNumberOfTuples();
    lengths[1] = 0;
    for (int strI = 0; strI < sa->GetNumberOfTuples(); strI++)
    {
      lengths[1] += static_cast<svtkIdType>(sa->GetValue(strI).length()) + 1;
    }
  }
  this->Controller->Broadcast(lengths, 2, 0);
  char* contents = new char[lengths[1]];
  if (this->ProcessId == 0)
  {
    for (int strI = 0, idx = 0; strI < sa->GetNumberOfTuples(); strI++)
    {
      const int len = static_cast<int>(sa->GetValue(strI).length()) + 1;
      memmove(contents + idx, sa->GetValue(strI).c_str(), len);
      idx += len;
    }
  }
  this->Controller->Broadcast(contents, lengths[1], 0);
  if (this->ProcessId != 0)
  {
    sa->Initialize();
    sa->SetNumberOfTuples(lengths[0]);
    for (int strI = 0, idx = 0; strI < lengths[0]; strI++)
    {
      sa->SetValue(strI, contents + idx);
      idx += static_cast<int>(sa->GetValue(strI).length()) + 1;
    }
  }
  delete[] contents;
}

//-----------------------------------------------------------------------------
// AllGather svtkStringArray from and to all processes
void svtkPOpenFOAMReader::AllGather(svtkStringArray* s)
{
  svtkIdType length = 0;
  for (int strI = 0; strI < s->GetNumberOfTuples(); strI++)
  {
    length += static_cast<svtkIdType>(s->GetValue(strI).length()) + 1;
  }
  svtkIdType* lengths = new svtkIdType[this->NumProcesses];
  this->Controller->AllGather(&length, lengths, 1);
  svtkIdType totalLength = 0;
  svtkIdType* offsets = new svtkIdType[this->NumProcesses];
  for (int procI = 0; procI < this->NumProcesses; procI++)
  {
    offsets[procI] = totalLength;
    totalLength += lengths[procI];
  }
  char *allContents = new char[totalLength], *contents = new char[length];
  for (int strI = 0, idx = 0; strI < s->GetNumberOfTuples(); strI++)
  {
    const int len = static_cast<int>(s->GetValue(strI).length()) + 1;
    memmove(contents + idx, s->GetValue(strI).c_str(), len);
    idx += len;
  }
  this->Controller->AllGatherV(contents, allContents, length, lengths, offsets);
  delete[] contents;
  delete[] lengths;
  delete[] offsets;
  s->Initialize();
  for (int idx = 0; idx < totalLength; idx += static_cast<int>(strlen(allContents + idx)) + 1)
  {
    const char* str = allContents + idx;
    // insert only when the same string is not found
    if (s->LookupValue(str) == -1)
    {
      s->InsertNextValue(str);
    }
  }
  s->Squeeze();
  delete[] allContents;
}

//-----------------------------------------------------------------------------
// AllGather svtkDataArraySelections from and to all processes
void svtkPOpenFOAMReader::AllGather(svtkDataArraySelection* s)
{
  svtkIdType length = 0;
  for (int strI = 0; strI < s->GetNumberOfArrays(); strI++)
  {
    length += static_cast<svtkIdType>(strlen(s->GetArrayName(strI))) + 2;
  }
  svtkIdType* lengths = new svtkIdType[this->NumProcesses];
  this->Controller->AllGather(&length, lengths, 1);
  svtkIdType totalLength = 0;
  svtkIdType* offsets = new svtkIdType[this->NumProcesses];
  for (int procI = 0; procI < this->NumProcesses; procI++)
  {
    offsets[procI] = totalLength;
    totalLength += lengths[procI];
  }
  char *allContents = new char[totalLength], *contents = new char[length];
  for (int strI = 0, idx = 0; strI < s->GetNumberOfArrays(); strI++)
  {
    const char* arrayName = s->GetArrayName(strI);
    contents[idx] = s->ArrayIsEnabled(arrayName);
    const int len = static_cast<int>(strlen(arrayName)) + 1;
    memmove(contents + idx + 1, arrayName, len);
    idx += len + 1;
  }
  this->Controller->AllGatherV(contents, allContents, length, lengths, offsets);
  delete[] contents;
  delete[] lengths;
  delete[] offsets;
  // do not RemoveAllArray so that the previous arrays are preserved
  // s->RemoveAllArrays();
  for (int idx = 0; idx < totalLength; idx += static_cast<int>(strlen(allContents + idx + 1)) + 2)
  {
    const char* arrayName = allContents + idx + 1;
    s->AddArray(arrayName);
    if (allContents[idx] == 0)
    {
      s->DisableArray(arrayName);
    }
    else
    {
      s->EnableArray(arrayName);
    }
  }
  delete[] allContents;
}
