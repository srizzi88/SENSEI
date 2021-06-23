/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPImageWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPImageWriter.h"

#include "svtkAlgorithmOutput.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPipelineSize.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtksys/FStream.hxx"

#define svtkPIWCloseFile                                                                            \
  if (file && fileOpenedHere)                                                                      \
  {                                                                                                \
    this->WriteFileTrailer(file, cache);                                                           \
    svtksys::ofstream* ofile = dynamic_cast<svtksys::ofstream*>(file);                               \
    if (ofile)                                                                                     \
    {                                                                                              \
      ofile->close();                                                                              \
    }                                                                                              \
    delete file;                                                                                   \
  }

svtkStandardNewMacro(svtkPImageWriter);

#ifdef write
#undef write
#endif

#ifdef close
#undef close
#endif

//----------------------------------------------------------------------------
svtkPImageWriter::svtkPImageWriter()
{
  // Set a default memory limit of a gibibyte
  this->MemoryLimit = 1 * 1024 * 1024;

  this->SizeEstimator = svtkPipelineSize::New();
}

//----------------------------------------------------------------------------
svtkPImageWriter::~svtkPImageWriter()
{
  if (this->SizeEstimator)
  {
    this->SizeEstimator->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkPImageWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "MemoryLimit (in kibibytes): " << this->MemoryLimit << "\n";
}

//----------------------------------------------------------------------------
// Breaks region into pieces with correct dimensionality.
void svtkPImageWriter::RecursiveWrite(
  int axis, svtkImageData* cache, svtkInformation* inInfo, ostream* file)
{
  int min, max, mid;
  svtkImageData* data;
  int fileOpenedHere = 0;
  unsigned long inputMemorySize;

  // if we need to open another slice, do it
  if (!file && (axis + 1) == this->FileDimensionality)
  {
    // determine the name
    if (this->FileName)
    {
      snprintf(this->InternalFileName, this->InternalFileNameSize, "%s", this->FileName);
    }
    else
    {
      if (this->FilePrefix)
      {
        snprintf(this->InternalFileName, this->InternalFileNameSize, this->FilePattern,
          this->FilePrefix, this->FileNumber);
      }
      else
      {
        snprintf(
          this->InternalFileName, this->InternalFileNameSize, this->FilePattern, this->FileNumber);
      }
    }
    // Open the file
#ifdef _WIN32
    file = new svtksys::ofstream(this->InternalFileName, ios::out | ios::binary);
#else
    file = new svtksys::ofstream(this->InternalFileName, ios::out);
#endif
    fileOpenedHere = 1;
    if (file->fail())
    {
      svtkErrorMacro("RecursiveWrite: Could not open file " << this->InternalFileName);
      delete file;
      return;
    }

    // Subclasses can write a header with this method call.
    this->WriteFileHeader(
      file, cache, inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
    ++this->FileNumber;
  }

  // Get the pipeline information for the input
  svtkAlgorithm* inAlg = this->GetInputAlgorithm();

  // Set a hint not to combine with previous requests
  inInfo->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), SVTK_UPDATE_EXTENT_REPLACE);

  // Propagate the update extent so we can determine pipeline size
  inAlg->PropagateUpdateExtent();

  // Go back to the previous behaviour
  inInfo->Set(
    svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), SVTK_UPDATE_EXTENT_COMBINE);

  // Now we can ask how big the pipeline will be
  inputMemorySize = this->SizeEstimator->GetEstimatedSize(this, 0, 0);

  // will the current request fit into memory
  // if so the just get the data and write it out
  if (inputMemorySize < this->MemoryLimit)
  {
#ifndef NDEBUG
    int* ext = inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
#endif
    svtkDebugMacro("Getting input extent: " << ext[0] << ", " << ext[1] << ", " << ext[2] << ", "
                                           << ext[3] << ", " << ext[4] << ", " << ext[5] << endl);
    this->GetInputAlgorithm()->Update();
    data = cache;
    this->RecursiveWrite(axis, cache, data, inInfo, file);
    svtkPIWCloseFile;
    return;
  }

  // if the current request did not fit into memory
  // the we will split the current axis
  int* updateExtent = inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  this->GetInput()->GetAxisUpdateExtent(axis, min, max, updateExtent);

  svtkDebugMacro("Axes: " << axis << "(" << min << ", " << max << "), UpdateMemory: "
                         << inputMemorySize << ", Limit: " << this->MemoryLimit << endl);

  if (min == max)
  {
    if (axis > 0)
    {
      this->RecursiveWrite(axis - 1, cache, inInfo, file);
    }
    else
    {
      svtkWarningMacro("MemoryLimit too small for one pixel of information!!");
    }
    svtkPIWCloseFile;
    return;
  }

  mid = (min + max) / 2;

  int axisUpdateExtent[6];

  // if it is the y axis then flip by default
  if (axis == 1 && !this->FileLowerLeft)
  {
    // first half
    cache->SetAxisUpdateExtent(axis, mid + 1, max, updateExtent, axisUpdateExtent);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), axisUpdateExtent, 6);
    this->RecursiveWrite(axis, cache, inInfo, file);

    // second half
    cache->SetAxisUpdateExtent(axis, min, mid, updateExtent, axisUpdateExtent);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), axisUpdateExtent, 6);
    this->RecursiveWrite(axis, cache, inInfo, file);
  }
  else
  {
    // first half
    cache->SetAxisUpdateExtent(axis, min, mid, updateExtent, axisUpdateExtent);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), axisUpdateExtent, 6);
    this->RecursiveWrite(axis, cache, inInfo, file);

    // second half
    cache->SetAxisUpdateExtent(axis, mid + 1, max, updateExtent, axisUpdateExtent);
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), axisUpdateExtent, 6);
    this->RecursiveWrite(axis, cache, inInfo, file);
  }

  // restore original extent
  cache->SetAxisUpdateExtent(axis, min, max, updateExtent, axisUpdateExtent);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), axisUpdateExtent, 6);

  // if we opened the file here, then we need to close it up
  svtkPIWCloseFile;
}
