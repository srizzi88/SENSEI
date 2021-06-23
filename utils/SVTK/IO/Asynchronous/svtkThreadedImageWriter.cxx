/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThreadedImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkThreadedImageWriter.h"

#include "svtkBMPWriter.h"
#include "svtkCommand.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkJPEGWriter.h"
#include "svtkLogger.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPNGWriter.h"
#include "svtkPNMWriter.h"
#include "svtkPointData.h"
#include "svtkTIFFWriter.h"
#include "svtkThreadedTaskQueue.h"
#include "svtkUnsignedCharArray.h"
#include "svtkXMLImageDataWriter.h"
#include "svtkZLibDataCompressor.h"
#include "svtksys/FStream.hxx"

#include <cassert>
#include <cmath>
#include <iostream>

#include <svtksys/SystemTools.hxx>

#define MAX_NUMBER_OF_THREADS_IN_POOL 32
//****************************************************************************
namespace
{
static void EncodeAndWrite(const svtkSmartPointer<svtkImageData>& image, const std::string& fileName)
{
  svtkLogF(TRACE, "encoding: %s", fileName.c_str());
  assert(image != nullptr);

  std::size_t pos = fileName.rfind(".");
  std::string extension = fileName.substr(pos + 1);

  if (extension == "Z")
  {
    svtkNew<svtkZLibDataCompressor> zLib;
    float* zBuf = static_cast<svtkFloatArray*>(image->GetPointData()->GetScalars())->GetPointer(0);
    size_t bufSize = image->GetNumberOfPoints() * sizeof(float);
    unsigned char* cBuffer = new unsigned char[bufSize];
    size_t compressSize = zLib->Compress((unsigned char*)zBuf, bufSize, cBuffer, bufSize);
    svtksys::ofstream fileHandler(fileName.c_str(), ios::out | ios::binary);
    fileHandler.write((const char*)cBuffer, compressSize);
    delete[] cBuffer;
  }

  else if (extension == "png")
  {
    svtkNew<svtkPNGWriter> writer;
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(image);
    writer->Write();
  }

  else if (extension == "jpg" || extension == "jpeg")
  {
    svtkNew<svtkJPEGWriter> writer;
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(image);
    writer->Write();
  }

  else if (extension == "bmp")
  {
    svtkNew<svtkBMPWriter> writer;
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(image);
    writer->Write();
  }

  else if (extension == "ppm")
  {
    svtkNew<svtkPNMWriter> writer;
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(image);
    writer->Write();
  }

  else if (extension == "tif" || extension == "tiff")
  {
    svtkNew<svtkTIFFWriter> writer;
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(image);
    writer->Write();
  }

  else if (extension == "vti")
  {
    svtkNew<svtkXMLImageDataWriter> writer;
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(image);
    writer->Write();
  }

  else
  {
    svtkDataArray* scalars = image->GetPointData()->GetScalars();
    int scalarSize = scalars->GetDataTypeSize();
    const char* scalarPtr = static_cast<const char*>(scalars->GetVoidPointer(0));
    size_t numberOfScalars = image->GetNumberOfPoints();
    svtksys::ofstream fileHandler(fileName.c_str(), ios::out | ios::binary);
    fileHandler.write(scalarPtr, numberOfScalars * scalarSize);
  }
}
}

//****************************************************************************
class svtkThreadedImageWriter::svtkInternals
{
private:
  using TaskQueueType = svtkThreadedTaskQueue<void, svtkSmartPointer<svtkImageData>, std::string>;
  std::unique_ptr<TaskQueueType> Queue;

public:
  svtkInternals()
    : Queue(nullptr)
  {
  }

  ~svtkInternals() { this->TerminateAllWorkers(); }

  void TerminateAllWorkers()
  {
    if (this->Queue)
    {
      this->Queue->Flush();
    }
    this->Queue.reset(nullptr);
  }

  void SpawnWorkers(svtkTypeUInt32 numberOfThreads)
  {
    this->Queue.reset(new TaskQueueType(::EncodeAndWrite,
      /*strict_ordering=*/true,
      /*buffer_size=*/-1,
      /*max_concurrent_tasks=*/static_cast<int>(numberOfThreads)));
  }

  void PushImageToQueue(svtkSmartPointer<svtkImageData>&& data, std::string&& filename)
  {
    this->Queue->Push(std::move(data), std::move(filename));
  }
};

svtkStandardNewMacro(svtkThreadedImageWriter);
//----------------------------------------------------------------------------
svtkThreadedImageWriter::svtkThreadedImageWriter()
  : Internals(new svtkInternals())
{
  this->MaxThreads = MAX_NUMBER_OF_THREADS_IN_POOL;
}

//----------------------------------------------------------------------------
svtkThreadedImageWriter::~svtkThreadedImageWriter()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void svtkThreadedImageWriter::SetMaxThreads(svtkTypeUInt32 maxThreads)
{
  if (maxThreads < MAX_NUMBER_OF_THREADS_IN_POOL && maxThreads > 0)
  {
    this->MaxThreads = maxThreads;
  }
}

//----------------------------------------------------------------------------
void svtkThreadedImageWriter::Initialize()
{
  // Stop any started thread first
  this->Internals->TerminateAllWorkers();

  // Make sure we don't keep adding new threads
  // this->Internals->TerminateAllWorkers();
  // Register new worker threads
  this->Internals->SpawnWorkers(this->MaxThreads);
}

//----------------------------------------------------------------------------
void svtkThreadedImageWriter::EncodeAndWrite(svtkImageData* image, const char* fileName)
{
  // Error checking
  if (image == nullptr)
  {
    svtkErrorMacro(<< "Write:Please specify an input!");
    return;
  }

  // we make a shallow copy so that the caller doesn't have to take too much
  // care when modifying image besides the standard requirements for the case
  // where the image is propagated in the pipeline.
  svtkSmartPointer<svtkImageData> img;
  img.TakeReference(image->NewInstance());
  img->ShallowCopy(image);
  this->Internals->PushImageToQueue(std::move(img), std::string(fileName));
}

//----------------------------------------------------------------------------
void svtkThreadedImageWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkThreadedImageWriter::Finalize()
{
  this->Internals->TerminateAllWorkers();
}
