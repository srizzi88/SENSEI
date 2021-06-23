/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThreadedImageAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkThreadedImageAlgorithm.h"

#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiThreader.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <vector>

// If SMP backend is Sequential then fall back to svtkMultiThreader,
// else enable the newer svtkSMPTools code path by default.
#ifdef SVTK_SMP_Sequential
bool svtkThreadedImageAlgorithm::GlobalDefaultEnableSMP = false;
#else
bool svtkThreadedImageAlgorithm::GlobalDefaultEnableSMP = true;
#endif

//----------------------------------------------------------------------------
svtkThreadedImageAlgorithm::svtkThreadedImageAlgorithm()
{
  this->Threader = svtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();

  // SMP default settings
  this->EnableSMP = svtkThreadedImageAlgorithm::GlobalDefaultEnableSMP;

  // Splitting method
  this->SplitMode = SLAB;
  this->SplitPath[0] = 2;
  this->SplitPath[1] = 1;
  this->SplitPath[2] = 0;
  this->SplitPathLength = 3;

  // Minimum block size
  this->MinimumPieceSize[0] = 16;
  this->MinimumPieceSize[1] = 1;
  this->MinimumPieceSize[2] = 1;

  // The desired block size in bytes
  this->DesiredBytesPerPiece = 65536;
}

//----------------------------------------------------------------------------
svtkThreadedImageAlgorithm::~svtkThreadedImageAlgorithm()
{
  this->Threader->Delete();
}

//----------------------------------------------------------------------------
void svtkThreadedImageAlgorithm::SetGlobalDefaultEnableSMP(bool enable)
{
  if (enable != svtkThreadedImageAlgorithm::GlobalDefaultEnableSMP)
  {
    svtkThreadedImageAlgorithm::GlobalDefaultEnableSMP = enable;
  }
}

//----------------------------------------------------------------------------
bool svtkThreadedImageAlgorithm::GetGlobalDefaultEnableSMP()
{
  return svtkThreadedImageAlgorithm::GlobalDefaultEnableSMP;
}

//----------------------------------------------------------------------------
void svtkThreadedImageAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
  os << indent << "EnableSMP: " << (this->EnableSMP ? "On\n" : "Off\n");
  os << indent << "GlobalDefaultEnableSMP: "
     << (svtkThreadedImageAlgorithm::GlobalDefaultEnableSMP ? "On\n" : "Off\n");
  os << indent << "MinimumPieceSize: " << this->MinimumPieceSize[0] << " "
     << this->MinimumPieceSize[1] << " " << this->MinimumPieceSize[2] << "\n";
  os << indent << "DesiredBytesPerPiece: " << this->DesiredBytesPerPiece << "\n";
  os << indent << "SplitMode: "
     << (this->SplitMode == SLAB
            ? "Slab\n"
            : (this->SplitMode == BEAM ? "Beam\n"
                                       : (this->SplitMode == BLOCK ? "Block\n" : "Unknown\n")));
}

//----------------------------------------------------------------------------
struct svtkImageThreadStruct
{
  svtkThreadedImageAlgorithm* Filter;
  svtkInformation* Request;
  svtkInformationVector** InputsInfo;
  svtkInformationVector* OutputsInfo;
  svtkImageData*** Inputs;
  svtkImageData** Outputs;
  int* UpdateExtent;
};

//----------------------------------------------------------------------------
// For streaming and threads.  Splits output update extent into num pieces.
// This method needs to be called num times.  Results must not overlap for
// consistent starting extent.  Subclass can override this method.
// This method returns the number of pieces resulting from a successful split.
// This can be from 1 to "total".
// If 1 is returned, the extent cannot be split.
int svtkThreadedImageAlgorithm::SplitExtent(int splitExt[6], int startExt[6], int num, int total)
{
  // split path (the order in which to split the axes)
  int pathlen = this->SplitPathLength;
  int mode = this->SplitMode;
  int axis0 = this->SplitPath[0];
  int axis1 = this->SplitPath[1];
  int axis2 = this->SplitPath[2];
  int path[3] = { axis0, axis1, axis2 };

  // divisions
  int divs[3] = { 1, 1, 1 };

  // this needs 64 bits to avoid overflow in the math below
  const svtkTypeInt64 size[3] = { startExt[1] - startExt[0] + 1, startExt[3] - startExt[2] + 1,
    startExt[5] - startExt[4] + 1 };

  // check for valid extent
  if (size[0] <= 0 || size[1] <= 0 || size[2] <= 0)
  {
    return 0;
  }

  // divide out the minimum block size
  int maxdivs[3] = { 1, 1, 1 };
  for (int i = 0; i < 3; i++)
  {
    if (size[i] > this->MinimumPieceSize[i] && this->MinimumPieceSize[i] > 0)
    {
      maxdivs[i] = size[i] / this->MinimumPieceSize[i];
    }
  }

  // make sure total is not greater than max number of pieces
  svtkTypeInt64 maxPieces = maxdivs[axis0];
  svtkTypeInt64 maxPieces2D = maxPieces;
  if (pathlen > 1)
  {
    maxPieces *= maxdivs[axis1];
    maxPieces2D = maxPieces;
    if (pathlen > 2)
    {
      maxPieces *= maxdivs[axis2];
    }
  }
  if (total > maxPieces)
  {
    total = maxPieces;
  }

  if (mode == SLAB || pathlen < 2)
  {
    // split the axes in the given order
    divs[axis0] = maxdivs[axis0];
    if (total < maxdivs[axis0])
    {
      divs[axis0] = total;
    }
    else if (pathlen > 1)
    {
      divs[axis1] = maxdivs[axis1];
      int q = total / divs[axis0];
      if (q < maxdivs[axis1])
      {
        divs[axis1] = q;
      }
      else if (pathlen > 2)
      {
        divs[axis2] = q / divs[axis1];
      }
    }
  }
  else if (mode == BEAM || pathlen < 3)
  {
    // split two of the axes first, leave third axis for last
    if (total < maxPieces2D)
    {
      // split until we get the desired number of pieces
      while (divs[axis0] * divs[axis1] < total)
      {
        axis0 = path[0];
        axis1 = path[1];

        // if necessary, swap axes to keep a good aspect ratio
        if (size[axis0] * divs[axis1] < size[axis1] * divs[axis0])
        {
          axis0 = path[1];
          axis1 = path[0];
        }

        // compute the new split for this axis
        divs[axis0] = divs[axis1] * size[axis0] / size[axis1] + 1;
      }

      // compute final division
      divs[axis0] = total / divs[axis1];
      if (divs[axis0] > maxdivs[axis0])
      {
        divs[axis0] = maxdivs[axis0];
      }
      divs[axis1] = total / divs[axis0];
      if (divs[axis1] > maxdivs[axis1])
      {
        divs[axis1] = maxdivs[axis1];
        divs[axis0] = total / divs[axis1];
      }
    }
    else
    {
      // maximum split for first two axes
      divs[axis0] = maxdivs[axis0];
      divs[axis1] = maxdivs[axis1];
      if (pathlen > 2)
      {
        // split the third axis
        divs[axis2] = total / (divs[axis0] * divs[axis1]);
      }
    }
  }
  else // block mode: keep blocks roughly cube shaped
  {
    // split until we get the desired number of pieces
    while (divs[0] * divs[1] * divs[2] < total)
    {
      axis0 = path[0];
      axis1 = path[1];
      axis2 = path[2];

      // check whether z or y is best candidate for splitting
      if (size[axis0] * divs[axis1] < size[axis1] * divs[axis0])
      {
        axis1 = axis0;
        axis0 = path[1];
      }

      if (pathlen > 2)
      {
        // check if x is the best candidate for splitting
        if (size[axis0] * divs[path[2]] < size[path[2]] * divs[axis0])
        {
          axis2 = axis1;
          axis1 = axis0;
          axis0 = path[2];
        }
        // now find the second best candidate
        if (size[axis1] * divs[axis2] < size[axis2] * divs[axis1])
        {
          int tmp = axis2;
          axis2 = axis1;
          axis1 = tmp;
        }
      }

      // compute the new split for this axis
      divs[axis0] = divs[axis1] * size[axis0] / size[axis1] + 1;

      // if axis0 reached maxdivs, remove it from the split path
      if (divs[axis0] >= maxdivs[axis0])
      {
        divs[axis0] = maxdivs[axis0];
        if (--pathlen == 1)
        {
          break;
        }
        if (axis0 != path[2])
        {
          if (axis0 != path[1])
          {
            path[0] = path[1];
          }
          path[1] = path[2];
          path[2] = axis0;
        }
      }
    }

    // compute the final division
    divs[axis0] = total / (divs[axis1] * divs[axis2]);
    if (divs[axis0] > maxdivs[axis0])
    {
      divs[axis0] = maxdivs[axis0];
    }
    divs[axis1] = total / (divs[axis0] * divs[axis2]);
    if (divs[axis1] > maxdivs[axis1])
    {
      divs[axis1] = maxdivs[axis1];
    }
    divs[axis2] = total / (divs[axis0] * divs[axis1]);
    if (divs[axis2] > maxdivs[axis2])
    {
      divs[axis2] = maxdivs[axis2];
    }
  }

  // compute new total from the chosen divisions
  total = divs[0] * divs[1] * divs[2];

  if (splitExt)
  {
    // compute increments
    int a = divs[0];
    int b = a * divs[1];

    // compute 3D block index
    int i = num;
    int index[3];
    index[2] = i / b;
    i -= index[2] * b;
    index[1] = i / a;
    i -= index[1] * a;
    index[0] = i;

    // compute the extent for the resulting block
    for (int j = 0; j < 3; j++)
    {
      splitExt[2 * j] = index[j] * size[j] / divs[j];
      splitExt[2 * j + 1] = (index[j] + 1) * size[j] / divs[j] - 1;
      splitExt[2 * j] += startExt[2 * j];
      splitExt[2 * j + 1] += startExt[2 * j];
    }
  }

  // return the number of blocks (may be fewer than requested)
  return total;
}

//----------------------------------------------------------------------------
// The old way to thread an image filter, before svtkSMPTools existed:
// this mess is really a simple function. All it does is call
// the ThreadedExecute method after setting the correct
// extent for this thread. It's just a pain to calculate
// the correct extent.
static SVTK_THREAD_RETURN_TYPE svtkThreadedImageAlgorithmThreadedExecute(void* arg)
{
  svtkImageThreadStruct* str;
  int splitExt[6], total;
  int threadId, threadCount;

  threadId = static_cast<svtkMultiThreader::ThreadInfo*>(arg)->ThreadID;
  threadCount = static_cast<svtkMultiThreader::ThreadInfo*>(arg)->NumberOfThreads;

  str =
    static_cast<svtkImageThreadStruct*>(static_cast<svtkMultiThreader::ThreadInfo*>(arg)->UserData);

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  total = str->Filter->SplitExtent(splitExt, str->UpdateExtent, threadId, threadCount);

  if (threadId < total)
  {
    // return if nothing to do
    if (splitExt[1] < splitExt[0] || splitExt[3] < splitExt[2] || splitExt[5] < splitExt[4])
    {
      return SVTK_THREAD_RETURN_VALUE;
    }
    str->Filter->ThreadedRequestData(str->Request, str->InputsInfo, str->OutputsInfo, str->Inputs,
      str->Outputs, splitExt, threadId);
  }

  return SVTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
// This functor is used with svtkSMPTools to execute the algorithm in pieces
// split over the extent of the data.
class svtkThreadedImageAlgorithmFunctor
{
public:
  // Create the functor by providing all of the information that will be
  // needed by the ThreadedRequestData method that the functor will call.
  svtkThreadedImageAlgorithmFunctor(svtkThreadedImageAlgorithm* algo, svtkInformation* request,
    svtkInformationVector** inputsInfo, svtkInformationVector* outputsInfo, svtkImageData*** inputs,
    svtkImageData** outputs, const int extent[6], svtkIdType pieces)
    : Algorithm(algo)
    , Request(request)
    , InputsInfo(inputsInfo)
    , OutputsInfo(outputsInfo)
    , Inputs(inputs)
    , Outputs(outputs)
    , NumberOfPieces(pieces)
  {
    for (int i = 0; i < 6; i++)
    {
      this->Extent[i] = extent[i];
    }
  }

  // Called by svtkSMPTools to execute the algorithm over specific pieces.
  void operator()(svtkIdType begin, svtkIdType end)
  {
    this->Algorithm->SMPRequestData(this->Request, this->InputsInfo, this->OutputsInfo,
      this->Inputs, this->Outputs, begin, end, this->NumberOfPieces, this->Extent);
  }

private:
  svtkThreadedImageAlgorithmFunctor() = delete;

  svtkThreadedImageAlgorithm* Algorithm;
  svtkInformation* Request;
  svtkInformationVector** InputsInfo;
  svtkInformationVector* OutputsInfo;
  svtkImageData*** Inputs;
  svtkImageData** Outputs;
  int Extent[6];
  svtkIdType NumberOfPieces;
};

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void svtkThreadedImageAlgorithm::SMPRequestData(svtkInformation* request,
  svtkInformationVector** inputVector, svtkInformationVector* outputVector, svtkImageData*** inData,
  svtkImageData** outData, svtkIdType begin, svtkIdType end, svtkIdType numPieces, int extent[6])
{
  for (svtkIdType piece = begin; piece < end; piece++)
  {
    int splitExt[6] = { 0, -1, 0, -1, 0, -1 };

    svtkIdType total = this->SplitExtent(splitExt, extent, piece, numPieces);

    // check for valid piece and extent
    if (piece < total && splitExt[0] <= splitExt[1] && splitExt[2] <= splitExt[3] &&
      splitExt[4] <= splitExt[5])
    {
      this->ThreadedRequestData(
        request, inputVector, outputVector, inData, outData, splitExt, piece);
    }
  }
}

//----------------------------------------------------------------------------
void svtkThreadedImageAlgorithm::PrepareImageData(svtkInformationVector** inputVector,
  svtkInformationVector* outputVector, svtkImageData*** inDataObjects, svtkImageData** outDataObjects)
{
  svtkImageData* firstInput = nullptr;
  svtkImageData* firstOutput = nullptr;

  // now we must create the output array
  int numOutputPorts = this->GetNumberOfOutputPorts();
  for (int i = 0; i < numOutputPorts; i++)
  {
    svtkInformation* info = outputVector->GetInformationObject(i);
    svtkImageData* outData = svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
    if (i == 0)
    {
      firstOutput = outData;
    }
    if (outDataObjects)
    {
      outDataObjects[i] = outData;
    }
    if (outData)
    {
      int updateExtent[6];
      info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);

      // unlike geometry filters, for image filters data is pre-allocated
      // in the superclass (which means, in this class)
      this->AllocateOutputData(outData, info, updateExtent);
    }
  }

  // now create the inputs array
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i = 0; i < numInputPorts; i++)
  {
    svtkInformationVector* portInfo = inputVector[i];
    int numConnections = portInfo->GetNumberOfInformationObjects();
    for (int j = 0; j < numConnections; j++)
    {
      svtkInformation* info = portInfo->GetInformationObject(j);
      svtkImageData* inData = svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
      if (i == 0 && j == 0)
      {
        firstInput = inData;
      }
      if (inDataObjects && inDataObjects[i])
      {
        inDataObjects[i][j] = inData;
      }
    }
  }

  // copy other arrays
  if (firstInput && firstOutput)
  {
    this->CopyAttributeData(firstInput, firstOutput, inputVector);
  }
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int svtkThreadedImageAlgorithm::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // count the total number of inputs, outputs
  int numInputPorts = this->GetNumberOfInputPorts();
  int numOutputPorts = this->GetNumberOfOutputPorts();
  int numDataObjects = numOutputPorts;
  for (int i = 0; i < numInputPorts; i++)
  {
    numDataObjects += inputVector[i]->GetNumberOfInformationObjects();
  }

  // ThreadedRequestData() needs to be given the inputs and outputs
  // as raw pointers, but we use std::vector for memory allocation
  svtkImageData*** inputs = nullptr;
  svtkImageData** outputs = nullptr;
  std::vector<svtkImageData*> connections(numDataObjects);
  std::vector<svtkImageData**> ports(numInputPorts);
  size_t offset = 0;

  // set pointers to the lists of data objects and input ports
  if (numInputPorts)
  {
    inputs = &ports[0];
    for (int i = 0; i < numInputPorts; i++)
    {
      inputs[i] = &connections[offset];
      offset += inputVector[i]->GetNumberOfInformationObjects();
    }
  }

  // set pointer to the list of output data objects
  if (numOutputPorts)
  {
    outputs = &connections[offset];
  }

  // allocate the output data and call CopyAttributeData
  this->PrepareImageData(inputVector, outputVector, inputs, outputs);

  // need bytes per voxel to compute block size
  int bytesPerVoxel = 1;

  // get the update extent from the output, if there is an output
  int updateExtent[6] = { 0, -1, 0, -1, 0, -1 };
  if (numOutputPorts)
  {
    svtkImageData* outData = outputs[0];
    if (outData)
    {
      bytesPerVoxel = (outData->GetScalarSize() * outData->GetNumberOfScalarComponents());
      outData->GetExtent(updateExtent);
    }
  }
  else
  {
    // if no output, get update extent from the first input
    for (int inPort = 0; inPort < numInputPorts; inPort++)
    {
      if (this->GetNumberOfInputConnections(inPort))
      {
        svtkImageData* inData = inputs[inPort][0];
        if (inData)
        {
          bytesPerVoxel = (inData->GetScalarSize() * inData->GetNumberOfScalarComponents());
          inData->GetExtent(updateExtent);
          break;
        }
      }
    }
  }

  // verify that there is an extent for execution
  if (updateExtent[0] <= updateExtent[1] && updateExtent[2] <= updateExtent[3] &&
    updateExtent[4] <= updateExtent[5])
  {
    if (this->EnableSMP)
    {
      // SMP is enabled, use svtkSMPTools to thread the filter
      svtkIdType pieces = svtkSMPTools::GetEstimatedNumberOfThreads();

      // compute a reasonable number of pieces, this will be a multiple of
      // the number of available threads and relative to the data size
      svtkTypeInt64 bytesize = (static_cast<svtkTypeInt64>(updateExtent[1] - updateExtent[0] + 1) *
        static_cast<svtkTypeInt64>(updateExtent[3] - updateExtent[2] + 1) *
        static_cast<svtkTypeInt64>(updateExtent[5] - updateExtent[4] + 1) * bytesPerVoxel);
      svtkTypeInt64 bytesPerPiece = this->DesiredBytesPerPiece;

      if (bytesPerPiece > 0 && bytesPerPiece < bytesize)
      {
        svtkTypeInt64 b = pieces * bytesPerPiece;
        pieces *= (bytesize + b - 1) / b;
      }
      // do a dummy execution of SplitExtent to compute the number of pieces
      int subExtent[6];
      pieces = this->SplitExtent(subExtent, updateExtent, 0, pieces);

      // always shut off debugging to avoid threading problems with GetMacros
      bool debug = this->Debug;
      this->Debug = false;

      svtkThreadedImageAlgorithmFunctor functor(
        this, request, inputVector, outputVector, inputs, outputs, updateExtent, pieces);

      svtkSMPTools::For(0, pieces, functor);

      this->Debug = debug;
    }
    else
    {
      // if SMP is not enabled, use the svtkMultiThreader
      svtkImageThreadStruct str;
      str.Filter = this;
      str.Request = request;
      str.InputsInfo = inputVector;
      str.OutputsInfo = outputVector;
      str.Inputs = inputs;
      str.Outputs = outputs;
      str.UpdateExtent = updateExtent;

      // do a dummy execution of SplitExtent to compute the number of pieces
      int subExtent[6];
      svtkIdType pieces = this->SplitExtent(subExtent, updateExtent, 0, this->NumberOfThreads);
      this->Threader->SetNumberOfThreads(pieces);
      this->Threader->SetSingleMethod(svtkThreadedImageAlgorithmThreadedExecute, &str);
      // always shut off debugging to avoid threading problems with GetMacros
      bool debug = this->Debug;
      this->Debug = false;
      this->Threader->SingleMethodExecute();
      this->Debug = debug;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void svtkThreadedImageAlgorithm::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int extent[6], int threadId)
{
  this->ThreadedExecute(inData[0][0], outData[0], extent, threadId);
}

//----------------------------------------------------------------------------
// The execute method created by the subclass.
void svtkThreadedImageAlgorithm::ThreadedExecute(
  svtkImageData* inData, svtkImageData* outData, int extent[6], int threadId)
{
  (void)inData;
  (void)outData;
  (void)extent;
  (void)threadId;
  svtkErrorMacro("Subclass should override this method!!!");
}
