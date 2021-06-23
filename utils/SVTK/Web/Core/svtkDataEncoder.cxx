/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataEncoder.h"

#include "svtkBase64Utilities.h"
#include "svtkCommand.h"
#include "svtkConditionVariable.h"
#include "svtkImageData.h"
#include "svtkJPEGWriter.h"
#include "svtkMultiThreader.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPNGWriter.h"
#include "svtkUnsignedCharArray.h"

#include <cassert>
#include <cmath>
#include <map>
#include <vector>

#include <svtksys/SystemTools.hxx>

#define MAX_NUMBER_OF_THREADS_IN_POOL 32
//****************************************************************************
namespace
{
class svtkSharedData
{
public:
  struct OutputValueType
  {
  public:
    svtkTypeUInt32 TimeStamp;
    svtkSmartPointer<svtkUnsignedCharArray> Data;
    OutputValueType()
      : TimeStamp(0)
      , Data(nullptr)
    {
    }
  };

  enum
  {
    ENCODING_NONE = 0,
    ENCODING_BASE64 = 1
  };
  struct InputValueType
  {
  public:
    svtkTypeUInt32 OutputStamp;
    svtkSmartPointer<svtkImageData> Image;
    int Quality;
    int Encoding;
    InputValueType()
      : OutputStamp(0)
      , Image(nullptr)
      , Quality(100)
      , Encoding(ENCODING_BASE64)
    {
    }
  };

  typedef std::map<svtkTypeUInt32, InputValueType> InputMapType;
  typedef std::map<svtkTypeUInt32, OutputValueType> OutputMapType;

private:
  bool Done;
  svtkSimpleMutexLock DoneLock;
  svtkSimpleMutexLock OutputsLock;
  svtkSimpleConditionVariable OutputsAvailable;

  svtkSimpleMutexLock ThreadDoneLock;
  svtkSimpleConditionVariable ThreadDone;
  int ActiveThreadCount;

  //------------------------------------------------------------------------
  // OutputsLock must be held before accessing any of the following members.
  OutputMapType Outputs;

  //------------------------------------------------------------------------
  // Constructs used to synchronization.
  svtkSimpleMutexLock InputsLock;
  svtkSimpleConditionVariable InputsAvailable;

  //------------------------------------------------------------------------
  // InputsLock must be held before accessing any of the following members.
  InputMapType Inputs;

public:
  //------------------------------------------------------------------------
  svtkSharedData()
    : Done(false)
    , ActiveThreadCount(0)
  {
  }

  //------------------------------------------------------------------------
  // Each thread should call this method when it starts. It helps us clean up
  // threads when they are done.
  void BeginWorker()
  {
    this->ThreadDoneLock.Lock();
    this->ActiveThreadCount++;
    this->ThreadDoneLock.Unlock();
  }

  //------------------------------------------------------------------------
  // Each thread should call this method when it ends.
  void EndWorker()
  {
    this->ThreadDoneLock.Lock();
    this->ActiveThreadCount--;
    bool last_thread = (this->ActiveThreadCount == 0);
    this->ThreadDoneLock.Unlock();
    if (last_thread)
    {
      this->ThreadDone.Signal();
    }
  }

  //------------------------------------------------------------------------
  void RequestAndWaitForWorkersToEnd()
  {
    // Get the done lock so we other threads don't end up testing the Done
    // flag and quitting before this thread starts to wait for them to quit.
    this->DoneLock.Lock();
    this->Done = true;

    // Grab the ThreadDoneLock. so even if any thread ends up check this->Done
    // as soon as we release the lock, it won't get a chance to terminate.
    this->ThreadDoneLock.Lock();

    // release the done lock. Let threads test for this->Done flag.
    this->DoneLock.Unlock();

    // Tell all workers that inputs are available, so they will try to check
    // the input as well as the done flag.
    this->InputsAvailable.Broadcast();

    // Now wait for thread to terminate releasing this->ThreadDoneLock as soon
    // as we start waiting. Thus, no threads have got a chance to call
    // EndWorker() till the main thread starts waiting for them.
    this->ThreadDone.Wait(this->ThreadDoneLock);

    this->ThreadDoneLock.Unlock();

    // reset Done flag since all threads have died.
    this->Done = false;
  }

  //------------------------------------------------------------------------
  bool IsDone()
  {
    this->DoneLock.Lock();
    bool val = this->Done;
    this->DoneLock.Unlock();
    return val;
  }

  //------------------------------------------------------------------------
  void PushAndTakeReference(
    svtkTypeUInt32 key, svtkImageData*& data, svtkTypeUInt64 stamp, int quality, int encoding)
  {
    this->InputsLock.Lock();
    {
      svtkSharedData::InputValueType& value = this->Inputs[key];
      value.Image.TakeReference(data);
      value.OutputStamp = stamp;
      value.Quality = quality;
      value.Encoding = encoding;
      data = nullptr;
    }
    this->InputsLock.Unlock();
    this->InputsAvailable.Signal();
  }

  //------------------------------------------------------------------------
  svtkTypeUInt64 GetExpectedOutputStamp(svtkTypeUInt32 key)
  {
    svtkTypeUInt64 stamp = 0;
    this->InputsLock.Lock();
    svtkSharedData::InputMapType::iterator iter = this->Inputs.find(key);
    if (iter != this->Inputs.end())
    {
      stamp = iter->second.OutputStamp;
    }
    this->InputsLock.Unlock();
    return stamp;
  }

  //------------------------------------------------------------------------
  // NOTE: This method may suspend the calling thread until inputs become
  // available.
  svtkTypeUInt64 GetNextInputToProcess(
    svtkTypeUInt32& key, svtkSmartPointer<svtkImageData>& image, int& quality, int& encoding)
  {
    svtkTypeUInt32 stamp = 0;

    this->InputsLock.Lock();
    do
    {
      // Check if we have an input available, if so, return it.
      InputMapType::iterator iter;
      for (iter = this->Inputs.begin(); iter != this->Inputs.end(); ++iter)
      {
        if (iter->second.Image != nullptr)
        {
          key = iter->first;
          image = iter->second.Image;
          iter->second.Image = nullptr;
          stamp = iter->second.OutputStamp;
          quality = iter->second.Quality;
          encoding = iter->second.Encoding;
          break;
        }
      }
      if (image == nullptr && !this->IsDone())
      {
        // No data is available, let's wait till it becomes available.
        this->InputsAvailable.Wait(this->InputsLock);
      }

    } while (image == nullptr && !this->IsDone());

    this->InputsLock.Unlock();
    return stamp;
  }

  //------------------------------------------------------------------------
  void SetOutputReference(
    const svtkTypeUInt32& key, svtkTypeUInt64 timestamp, svtkUnsignedCharArray*& dataRef)
  {
    this->OutputsLock.Lock();
    assert(dataRef->GetReferenceCount() == 1);
    OutputMapType::iterator iter = this->Outputs.find(key);
    if (iter == this->Outputs.end() || iter->second.Data == nullptr ||
      iter->second.TimeStamp < timestamp)
    {
      // cout << "Done: " <<
      //  svtkMultiThreader::GetCurrentThreadID() << " "
      //  << key << ", " << timestamp << endl;
      this->Outputs[key].TimeStamp = timestamp;
      this->Outputs[key].Data.TakeReference(dataRef);
      dataRef = nullptr;
    }
    else
    {
      dataRef->Delete();
      dataRef = nullptr;
    }
    this->OutputsLock.Unlock();
    this->OutputsAvailable.Broadcast();
  }

  //------------------------------------------------------------------------
  bool CopyLatestOutputIfDifferent(svtkTypeUInt32 key, svtkUnsignedCharArray* data)
  {
    svtkTypeUInt64 dataTimeStamp = 0;
    this->OutputsLock.Lock();
    {
      const svtkSharedData::OutputValueType& output = this->Outputs[key];
      if (output.Data != nullptr &&
        (output.Data->GetMTime() > data->GetMTime() ||
          output.Data->GetNumberOfTuples() != data->GetNumberOfTuples()))
      {
        data->DeepCopy(output.Data);
        data->Modified();
      }
      dataTimeStamp = output.TimeStamp;
    }
    this->OutputsLock.Unlock();

    svtkTypeUInt64 outputTS = 0;

    this->InputsLock.Lock();
    svtkSharedData::InputMapType::iterator iter = this->Inputs.find(key);
    if (iter != this->Inputs.end())
    {
      outputTS = iter->second.OutputStamp;
    }
    this->InputsLock.Unlock();

    return (dataTimeStamp >= outputTS);
  }

  //------------------------------------------------------------------------
  void Flush(svtkTypeUInt32 key, svtkTypeUInt64 timestamp)
  {
    this->OutputsLock.Lock();
    while (this->Outputs[key].TimeStamp < timestamp)
    {
      // output is not yet ready, we have to wait.
      this->OutputsAvailable.Wait(this->OutputsLock);
    }

    this->OutputsLock.Unlock();
  }
};

SVTK_THREAD_RETURN_TYPE Worker(void* calldata)
{
  // cout << "Start Thread: " << svtkMultiThreader::GetCurrentThreadID() << endl;
  svtkMultiThreader::ThreadInfo* info = reinterpret_cast<svtkMultiThreader::ThreadInfo*>(calldata);
  svtkSharedData* sharedData = reinterpret_cast<svtkSharedData*>(info->UserData);

  sharedData->BeginWorker();

  while (true)
  {
    svtkTypeUInt32 key = 0;
    svtkSmartPointer<svtkImageData> image;
    svtkTypeUInt64 timestamp = 0;
    // these are defaults, reset by GetNextInputToProcess
    int quality = 100;
    int encoding = 1;

    timestamp = sharedData->GetNextInputToProcess(key, image, quality, encoding);

    if (timestamp == 0 || image == nullptr)
    {
      // end thread.
      break;
    }

    // cout << "Working Thread: " << svtkMultiThreader::GetCurrentThreadID() << endl;

    // Do the encoding.
    svtkNew<svtkJPEGWriter> writer;
    writer->WriteToMemoryOn();
    writer->SetInputData(image);
    writer->SetQuality(quality);
    writer->Write();
    svtkUnsignedCharArray* data = writer->GetResult();

    svtkUnsignedCharArray* result = svtkUnsignedCharArray::New();
    if (encoding)
    {
      result->SetNumberOfComponents(1);
      result->SetNumberOfTuples(std::ceil(1.5 * data->GetNumberOfTuples()));
      unsigned long size = svtkBase64Utilities::Encode(
        data->GetPointer(0), data->GetNumberOfTuples(), result->GetPointer(0), /*mark_end=*/0);
      result->SetNumberOfTuples(static_cast<svtkIdType>(size) + 1);
      result->SetValue(size, 0);
    }
    else
    {
      result->ShallowCopy(data);
    }
    // Pass over the "result" reference.
    sharedData->SetOutputReference(key, timestamp, result);
    assert(result == nullptr);
  }

  // cout << "Closing Thread: " << svtkMultiThreader::GetCurrentThreadID() << endl;
  sharedData->EndWorker();
  return SVTK_THREAD_RETURN_VALUE;
}
}

//****************************************************************************
class svtkDataEncoder::svtkInternals
{
private:
  std::map<svtkTypeUInt32, svtkSmartPointer<svtkUnsignedCharArray> > ClonedOutputs;
  std::vector<int> RunningThreadIds;

public:
  svtkNew<svtkMultiThreader> Threader;
  svtkSharedData SharedData;
  svtkTypeUInt64 Counter;

  svtkSmartPointer<svtkUnsignedCharArray> lastBase64Image;

  svtkInternals()
    : Counter(0)
  {
    lastBase64Image = svtkSmartPointer<svtkUnsignedCharArray>::New();
  }

  void TerminateAllWorkers()
  {
    // request and wait for all threads to close.
    if (!this->RunningThreadIds.empty())
    {
      this->SharedData.RequestAndWaitForWorkersToEnd();
    }

    // Stop threads
    while (!this->RunningThreadIds.empty())
    {
      this->Threader->TerminateThread(this->RunningThreadIds.back());
      this->RunningThreadIds.pop_back();
    }
  }

  void SpawnWorkers(svtkTypeUInt32 numberOfThreads)
  {
    for (svtkTypeUInt32 cc = 0; cc < numberOfThreads; cc++)
    {
      this->RunningThreadIds.push_back(this->Threader->SpawnThread(&Worker, &this->SharedData));
    }
  }

  // Since changes to svtkObjectBase::ReferenceCount are not thread safe, we have
  // this level of indirection between the outputs stored in SharedData and
  // passed back to the user/main thread.
  bool GetLatestOutput(svtkTypeUInt32 key, svtkSmartPointer<svtkUnsignedCharArray>& data)
  {
    svtkSmartPointer<svtkUnsignedCharArray>& output = this->ClonedOutputs[key];
    if (!output)
    {
      output = svtkSmartPointer<svtkUnsignedCharArray>::New();
    }
    data = output;
    return this->SharedData.CopyLatestOutputIfDifferent(key, data);
  }

  // Once an imagedata has been written to memory as a jpg or png, this
  // convenience function can encode that image as a Base64 string.
  const char* GetBase64EncodedImage(svtkUnsignedCharArray* encodedInputImage)
  {
    this->lastBase64Image->SetNumberOfComponents(1);
    this->lastBase64Image->SetNumberOfTuples(
      std::ceil(1.5 * encodedInputImage->GetNumberOfTuples()));
    unsigned long size = svtkBase64Utilities::Encode(encodedInputImage->GetPointer(0),
      encodedInputImage->GetNumberOfTuples(), this->lastBase64Image->GetPointer(0), /*mark_end=*/0);

    this->lastBase64Image->SetNumberOfTuples(static_cast<svtkIdType>(size) + 1);
    this->lastBase64Image->SetValue(size, 0);

    return reinterpret_cast<char*>(this->lastBase64Image->GetPointer(0));
  }
};

svtkStandardNewMacro(svtkDataEncoder);
//----------------------------------------------------------------------------
svtkDataEncoder::svtkDataEncoder()
  : Internals(new svtkInternals())
{
  this->MaxThreads = 3;
  this->Initialize();
}

//----------------------------------------------------------------------------
svtkDataEncoder::~svtkDataEncoder()
{
  this->Internals->TerminateAllWorkers();
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void svtkDataEncoder::SetMaxThreads(svtkTypeUInt32 maxThreads)
{
  if (maxThreads < MAX_NUMBER_OF_THREADS_IN_POOL && maxThreads > 0)
  {
    this->MaxThreads = maxThreads;
  }
}

//----------------------------------------------------------------------------
void svtkDataEncoder::Initialize()
{
  this->Internals->TerminateAllWorkers();
  this->Internals->SpawnWorkers(this->MaxThreads);
}

//----------------------------------------------------------------------------
void svtkDataEncoder::PushAndTakeReference(
  svtkTypeUInt32 key, svtkImageData*& data, int quality, int encoding)
{
  // if data->ReferenceCount != 1, it means the caller thread is keep an extra
  // reference and that's bad.
  assert(data->GetReferenceCount() == 1);

  this->Internals->SharedData.PushAndTakeReference(
    key, data, ++this->Internals->Counter, quality, encoding);
  assert(data == nullptr);
}

//----------------------------------------------------------------------------
bool svtkDataEncoder::GetLatestOutput(svtkTypeUInt32 key, svtkSmartPointer<svtkUnsignedCharArray>& data)
{
  return this->Internals->GetLatestOutput(key, data);
}

//----------------------------------------------------------------------------
const char* svtkDataEncoder::EncodeAsBase64Png(svtkImageData* img, int compressionLevel)
{
  // Perform in-memory write of image as png
  svtkNew<svtkPNGWriter> writer;
  writer->WriteToMemoryOn();
  writer->SetInputData(img);
  writer->SetCompressionLevel(compressionLevel);
  writer->Write();

  // Return Base64-encoded string
  return this->Internals->GetBase64EncodedImage(writer->GetResult());
}

//----------------------------------------------------------------------------
const char* svtkDataEncoder::EncodeAsBase64Jpg(svtkImageData* img, int quality)
{
  // Perform in-memory write of image as jpg
  svtkNew<svtkJPEGWriter> writer;
  writer->WriteToMemoryOn();
  writer->SetInputData(img);
  writer->SetQuality(quality);
  writer->Write();

  // Return Base64-encoded string
  return this->Internals->GetBase64EncodedImage(writer->GetResult());
}

//----------------------------------------------------------------------------
void svtkDataEncoder::Flush(svtkTypeUInt32 key)
{
  svtkTypeUInt64 outputTS = this->Internals->SharedData.GetExpectedOutputStamp(key);
  if (outputTS != 0)
  {
    // Now wait till we see the outputTS in the output for key.
    // cout << "Wait till : " << outputTS << endl;
    this->Internals->SharedData.Flush(key, outputTS);
  }
}

//----------------------------------------------------------------------------
void svtkDataEncoder::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkDataEncoder::Finalize()
{
  this->Internals->TerminateAllWorkers();
}
