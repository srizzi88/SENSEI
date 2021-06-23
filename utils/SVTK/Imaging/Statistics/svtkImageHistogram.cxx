/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageHistogram.h"

#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageStencilData.h"
#include "svtkImageStencilIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiThreader.h"
#include "svtkObjectFactory.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTemplateAliasMacro.h"

#include <cmath>

// turn off 64-bit ints when templating over all types
#undef SVTK_USE_INT64
#define SVTK_USE_INT64 0
#undef SVTK_USE_UINT64
#define SVTK_USE_UINT64 0

svtkStandardNewMacro(svtkImageHistogram);

//----------------------------------------------------------------------------
// Data needed for each thread.
class svtkImageHistogramThreadData
{
public:
  svtkImageHistogramThreadData()
    : Data(nullptr)
  {
  }

  svtkIdType* Data;
  int Range[2];
};

// Holds thread-local data for SMP implementation.
class svtkImageHistogramSMPThreadLocal : public svtkSMPThreadLocal<svtkImageHistogramThreadData>
{
public:
  typedef svtkSMPThreadLocal<svtkImageHistogramThreadData>::iterator iterator;
};

//----------------------------------------------------------------------------
// Constructor sets default values
svtkImageHistogram::svtkImageHistogram()
{
  this->ActiveComponent = -1;
  this->AutomaticBinning = false;
  this->MaximumNumberOfBins = 65536;
  this->NumberOfBins = 256;
  this->BinOrigin = 0.0;
  this->BinSpacing = 1.0;

  this->GenerateHistogramImage = true;
  this->HistogramImageSize[0] = 256;
  this->HistogramImageSize[1] = 256;
  this->HistogramImageScale = svtkImageHistogram::Linear;

  this->Histogram = svtkIdTypeArray::New();
  this->Total = 0;

  this->ThreadData = nullptr;
  this->SMPThreadData = nullptr;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkImageHistogram::~svtkImageHistogram()
{
  if (this->Histogram)
  {
    this->Histogram->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkImageHistogram::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Stencil: " << this->GetStencil() << "\n";

  os << indent << "ActiveComponent: " << this->ActiveComponent << "\n";

  os << indent << "AutomaticBinning: " << (this->AutomaticBinning ? "On\n" : "Off\n") << "\n";
  os << indent << "MaximumNumberOfBins: " << this->MaximumNumberOfBins << "\n";
  os << indent << "NumberOfBins: " << this->NumberOfBins << "\n";
  os << indent << "BinOrigin: " << this->BinOrigin << "\n";
  os << indent << "BinSpacing: " << this->BinSpacing << "\n";

  os << indent << "GenerateHistogramImage: " << (this->GenerateHistogramImage ? "On\n" : "Off\n")
     << "\n";
  os << indent << "HistogramImageSize: " << this->HistogramImageSize[0] << " "
     << this->HistogramImageSize[1] << "\n";
  os << indent << "HistogramImageScale: " << this->GetHistogramImageScaleAsString() << "\n";

  os << indent << "Total: " << this->Total << "\n";
  os << indent << "Histogram: " << this->Histogram << "\n";
}

//----------------------------------------------------------------------------
const char* svtkImageHistogram::GetHistogramImageScaleAsString()
{
  const char* s = "Unknown";

  switch (this->HistogramImageScale)
  {
    case svtkImageHistogram::Log:
      s = "Log";
      break;
    case svtkImageHistogram::Sqrt:
      s = "Sqrt";
      break;
    case svtkImageHistogram::Linear:
      s = "Linear";
      break;
  }

  return s;
}

//----------------------------------------------------------------------------
svtkIdTypeArray* svtkImageHistogram::GetHistogram()
{
  return this->Histogram;
}

//----------------------------------------------------------------------------
void svtkImageHistogram::SetStencilData(svtkImageStencilData* stencil)
{
  this->SetInputData(1, stencil);
}

//----------------------------------------------------------------------------
void svtkImageHistogram::SetStencilConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
svtkImageStencilData* svtkImageHistogram::GetStencil()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkImageStencilData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int svtkImageHistogram::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageStencilData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageHistogram::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageHistogram::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  int outWholeExt[6];
  double outOrigin[3];
  double outSpacing[3];

  outWholeExt[0] = 0;
  outWholeExt[1] = this->HistogramImageSize[0] - 1;
  outWholeExt[2] = 0;
  outWholeExt[3] = this->HistogramImageSize[1] - 1;
  outWholeExt[4] = 0;
  outWholeExt[5] = 0;

  outOrigin[0] = 0.0;
  outOrigin[1] = 0.0;
  outOrigin[2] = 0.0;

  outSpacing[0] = 1.0;
  outSpacing[1] = 1.0;
  outSpacing[2] = 1.0;

  if (!this->GenerateHistogramImage)
  {
    outWholeExt[1] = -1;
    outWholeExt[3] = -1;
    outWholeExt[5] = -1;
  }

  if (this->GetNumberOfOutputPorts() > 0)
  {
    svtkInformation* outInfo = outputVector->GetInformationObject(0);

    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outWholeExt, 6);

    outInfo->Set(svtkDataObject::ORIGIN(), outOrigin, 3);
    outInfo->Set(svtkDataObject::SPACING(), outSpacing, 3);

    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_UNSIGNED_CHAR, 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageHistogram::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  int inExt[6];
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // need to set the stencil update extent to the input extent
  if (this->GetNumberOfInputConnections(1) > 0)
  {
    svtkInformation* stencilInfo = inputVector[1]->GetInformationObject(0);
    stencilInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
// anonymous namespace for internal classes and functions
namespace
{

struct svtkImageHistogramThreadStruct
{
  svtkImageHistogram* Algorithm;
  svtkInformation* Request;
  svtkInformationVector** InputsInfo;
  svtkInformationVector* OutputsInfo;
  int* UpdateExtent;
};

//----------------------------------------------------------------------------
// override from svtkThreadedImageAlgorithm to split input extent, instead
// of splitting the output extent
SVTK_THREAD_RETURN_TYPE svtkImageHistogramThreadedExecute(void* arg)
{
  svtkMultiThreader::ThreadInfo* ti = static_cast<svtkMultiThreader::ThreadInfo*>(arg);
  svtkImageHistogramThreadStruct* ts = static_cast<svtkImageHistogramThreadStruct*>(ti->UserData);

  // execute the actual method with appropriate extent
  // first find out how many pieces extent can be split into.
  int splitExt[6];
  int total =
    ts->Algorithm->SplitExtent(splitExt, ts->UpdateExtent, ti->ThreadID, ti->NumberOfThreads);

  if (ti->ThreadID < total && splitExt[1] >= splitExt[0] && splitExt[3] >= splitExt[2] &&
    splitExt[5] >= splitExt[4])
  {
    ts->Algorithm->ThreadedRequestData(
      ts->Request, ts->InputsInfo, ts->OutputsInfo, nullptr, nullptr, splitExt, ti->ThreadID);
  }

  return SVTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
template <class T>
void svtkImageHistogramExecuteRange(svtkImageData* inData, svtkImageStencilData* stencil, T* inPtr,
  int extent[6], double range[2], int component)
{
  svtkImageStencilIterator<T> inIter(inData, stencil, extent, nullptr);

  T xmin = svtkTypeTraits<T>::Max();
  T xmax = svtkTypeTraits<T>::Min();

  // set up components
  int nc = inData->GetNumberOfScalarComponents();
  int c = component;
  if (c < 0)
  {
    nc = 1;
    c = 0;
  }

  // iterate over all spans in the stencil
  while (!inIter.IsAtEnd())
  {
    if (inIter.IsInStencil())
    {
      inPtr = inIter.BeginSpan();
      T* inPtrEnd = inIter.EndSpan();
      if (inPtr != inPtrEnd)
      {
        int n = static_cast<int>((inPtrEnd - inPtr) / nc);
        inPtr += c;
        do
        {
          T x = *inPtr;

          xmin = (xmin < x ? xmin : x);
          xmax = (xmax > x ? xmax : x);

          inPtr += nc;
        } while (--n);
      }
    }
    inIter.NextSpan();
  }

  range[0] = xmin;
  range[1] = xmax;
}

//----------------------------------------------------------------------------
template <class T>
void svtkImageHistogramExecute(svtkImageHistogram* self, svtkImageData* inData,
  svtkImageStencilData* stencil, T* inPtr, int extent[6], svtkIdType* outPtr, int binRange[2],
  double o, double s, int component, int threadId)
{
  svtkImageStencilIterator<T> inIter(inData, stencil, extent, ((threadId == 0) ? self : nullptr));

  // set up components
  int nc = inData->GetNumberOfScalarComponents();
  int c = component;
  if (c < 0)
  {
    nc = 1;
    c = 0;
  }

  // compute shift/scale values for fast bin computation
  double xmin = binRange[0];
  double xmax = binRange[1];
  double xshift = -o;
  double xscale = 1.0 / s;

  // iterate over all spans in the stencil
  while (!inIter.IsAtEnd())
  {
    if (inIter.IsInStencil())
    {
      inPtr = inIter.BeginSpan();
      T* inPtrEnd = inIter.EndSpan();

      // iterate over all voxels in the span
      if (inPtr != inPtrEnd)
      {
        int n = static_cast<int>((inPtrEnd - inPtr) / nc);
        inPtr += c;
        do
        {
          double x = *inPtr;

          x += xshift;
          x *= xscale;

          x = (x > xmin ? x : xmin);
          x = (x < xmax ? x : xmax);

          int xi = static_cast<int>(x + 0.5);

          outPtr[xi]++;

          inPtr += nc;
        } while (--n);
      }
    }
    inIter.NextSpan();
  }
}

//----------------------------------------------------------------------------
template <class T>
void svtkImageHistogramExecuteInt(svtkImageHistogram* self, svtkImageData* inData,
  svtkImageStencilData* stencil, T* inPtr, int extent[6], svtkIdType* outPtr, int component,
  int threadId)
{
  svtkImageStencilIterator<T> inIter(inData, stencil, extent, ((threadId == 0) ? self : nullptr));

  // set up components
  int nc = inData->GetNumberOfScalarComponents();
  int c = component;
  if (c < 0)
  {
    nc = 1;
    c = 0;
  }

  // iterate over all spans in the stencil
  while (!inIter.IsAtEnd())
  {
    if (inIter.IsInStencil())
    {
      inPtr = inIter.BeginSpan();
      T* inPtrEnd = inIter.EndSpan();

      // iterate over all voxels in the span
      if (inPtr != inPtrEnd)
      {
        int n = static_cast<int>((inPtrEnd - inPtr) / nc);
        inPtr += c;
        do
        {
          outPtr[*inPtr]++;
          inPtr += nc;
        } while (--n);
      }
    }
    inIter.NextSpan();
  }
}

// no-op version for float
void svtkImageHistogramExecuteInt(
  svtkImageHistogram*, svtkImageData*, svtkImageStencilData*, float*, int[6], svtkIdType*, int, int)
{
}

// no-op version for double
void svtkImageHistogramExecuteInt(
  svtkImageHistogram*, svtkImageData*, svtkImageStencilData*, double*, int[6], svtkIdType*, int, int)
{
}

//----------------------------------------------------------------------------
void svtkImageHistogramGenerateImage(
  svtkIdType* histogram, int nx, unsigned char* outPtr, int scale, int size[2], int extent[6])
{
  svtkIdType incX = 1;
  svtkIdType incY = (extent[1] - extent[0] + 1);

  // find tallest peak in histogram
  svtkIdType peak = 0;
  int ix;
  for (ix = 0; ix < nx; ++ix)
  {
    svtkIdType c = histogram[ix];
    peak = (peak >= c ? peak : c);
  }

  // compute vertical scale factor
  double b = 0.0;
  if (peak > 0)
  {
    double sum = peak;
    switch (scale)
    {
      case svtkImageHistogram::Log:
        sum = log(sum) + 1.0;
        break;
      case svtkImageHistogram::Sqrt:
        sum = sqrt(sum);
        break;
      case svtkImageHistogram::Linear:
        break;
    }
    b = (size[1] - 1) / sum;
  }

  // compute horizontal scale factor
  double a = 0.0;
  if (size[0] > 0)
  {
    a = nx * 1.0 / size[0];
  }

  double x = extent[0] * a;
  ix = static_cast<int>(x);
  for (int i = extent[0]; i <= extent[1]; i++)
  {
    // use max of the original bins to compute new bin height
    double sum = histogram[ix];
    x = (i + 1) * a;
    int ix1 = static_cast<int>(x);
    for (; ix < ix1; ix++)
    {
      double v = histogram[ix];
      sum = (sum > v ? sum : v);
    }
    // scale the bin height
    if (sum > 0)
    {
      switch (scale)
      {
        case svtkImageHistogram::Log:
          sum = log(sum) + 1;
          break;
        case svtkImageHistogram::Sqrt:
          sum = sqrt(sum);
          break;
        case svtkImageHistogram::Linear:
          break;
      }
    }
    int height = static_cast<int>(sum * b);
    height = (height < extent[3] ? height : extent[3]);
    // draw the bin
    unsigned char* outPtr1 = outPtr;
    int j = extent[2];
    for (; j <= height; j++)
    {
      *outPtr1 = 255;
      outPtr1 += incY;
    }
    for (; j <= extent[3]; j++)
    {
      *outPtr1 = 0;
      outPtr1 += incY;
    }
    outPtr += incX;
  }
}

} // end anonymous namespace

// Functor for svtkSMPTools execution
class svtkImageHistogramFunctor
{
public:
  // Create the functor, provide all info it needs to execute.
  svtkImageHistogramFunctor(svtkImageHistogramThreadStruct* pipelineInfo,
    svtkImageHistogramSMPThreadLocal* threadLocal, svtkIdType pieces, svtkIdTypeArray* histogram,
    svtkIdType* total)
    : PipelineInfo(pipelineInfo)
    , ThreadLocal(threadLocal)
    , NumberOfPieces(pieces)
    , Histogram(histogram)
    , Total(total)
  {
  }

  void Initialize() {}
  void operator()(svtkIdType begin, svtkIdType end);
  void Reduce();

private:
  svtkImageHistogramThreadStruct* PipelineInfo;
  svtkImageHistogramSMPThreadLocal* ThreadLocal;
  svtkIdType NumberOfPieces;
  svtkIdTypeArray* Histogram;
  svtkIdType* Total;
};

// Called by svtkSMPTools to execute the algorithm over specific pieces.
void svtkImageHistogramFunctor::operator()(svtkIdType begin, svtkIdType end)
{
  svtkImageHistogramThreadStruct* ts = this->PipelineInfo;

  ts->Algorithm->SMPRequestData(ts->Request, ts->InputsInfo, ts->OutputsInfo, nullptr, nullptr,
    begin, end, this->NumberOfPieces, ts->UpdateExtent);
}

// Called by svtkSMPTools once the multi-threading has finished.
void svtkImageHistogramFunctor::Reduce()
{
  svtkIdType* histogram = this->Histogram->GetPointer(0);
  svtkIdType total = 0;

  int numberOfBins =
    static_cast<svtkImageHistogram*>(this->PipelineInfo->Algorithm)->GetNumberOfBins();

  // clear histogram to zero
  for (int i = 0; i < numberOfBins; i++)
  {
    histogram[i] = 0;
  }

  // sum the histograms created by each thread
  for (svtkImageHistogramSMPThreadLocal::iterator iter = this->ThreadLocal->begin();
       iter != this->ThreadLocal->end(); ++iter)
  {
    svtkIdType* data = iter->Data;
    if (data)
    {
      int minbin = iter->Range[0];
      int maxbin = iter->Range[1];
      for (int j = minbin; j <= maxbin; j++)
      {
        svtkIdType f = data[j];
        histogram[j] += f;
        total += f;
      }
      delete[] data;
    }
  }

  (*this->Total) = total;
}

//----------------------------------------------------------------------------
// override from svtkThreadedImageAlgorithm to customize the multithreading
int svtkImageHistogram::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* info = inputVector[0]->GetInformationObject(0);
  svtkImageData* image = svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  int scalarType = image->GetScalarType();
  double scalarRange[2];

  // handle automatic binning
  if (this->AutomaticBinning)
  {
    switch (scalarType)
    {
      case SVTK_CHAR:
      case SVTK_UNSIGNED_CHAR:
      case SVTK_SIGNED_CHAR:
      {
        svtkDataArray::GetDataTypeRange(scalarType, scalarRange);
        this->NumberOfBins = 256;
        this->BinSpacing = 1.0;
        this->BinOrigin = scalarRange[0];
      }
      break;
      case SVTK_SHORT:
      case SVTK_UNSIGNED_SHORT:
      case SVTK_INT:
      case SVTK_UNSIGNED_INT:
      case SVTK_LONG:
      case SVTK_UNSIGNED_LONG:
      {
        this->ComputeImageScalarRange(image, scalarRange);
        if (scalarRange[0] > 0)
        {
          scalarRange[0] = 0;
        }
        if (scalarRange[1] < 0)
        {
          scalarRange[1] = 0;
        }
        unsigned long binMaxId = static_cast<unsigned long>(scalarRange[1] - scalarRange[0]);
        this->BinOrigin = scalarRange[0];
        this->BinSpacing = 1.0;
        if (binMaxId < 255)
        {
          binMaxId = 255;
        }
        if (binMaxId > static_cast<unsigned long>(this->MaximumNumberOfBins - 1))
        {
          binMaxId = static_cast<unsigned long>(this->MaximumNumberOfBins - 1);
          if (binMaxId > 0)
          {
            this->BinSpacing = (scalarRange[1] - scalarRange[0]) / binMaxId;
          }
        }
        this->NumberOfBins = static_cast<int>(binMaxId + 1);
      }
      break;
      default:
      {
        this->NumberOfBins = this->MaximumNumberOfBins;
        this->ComputeImageScalarRange(image, scalarRange);
        if (scalarRange[0] > 0)
        {
          scalarRange[0] = 0;
        }
        if (scalarRange[1] < 0)
        {
          scalarRange[1] = 0;
        }
        this->BinOrigin = scalarRange[0];
        this->BinSpacing = 1.0;
        if (scalarRange[1] > scalarRange[0])
        {
          if (this->NumberOfBins > 1)
          {
            this->BinSpacing = (scalarRange[1] - scalarRange[0]) / (this->NumberOfBins - 1);
          }
        }
      }
      break;
    }
  }

  // get the input extent
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  int extent[6];
  inData->GetExtent(extent);

  // setup the threads structure
  svtkImageHistogramThreadStruct ts;
  ts.Algorithm = this;
  ts.Request = request;
  ts.InputsInfo = inputVector;
  ts.OutputsInfo = outputVector;
  ts.UpdateExtent = extent;

  // allocate the output data
  this->PrepareImageData(inputVector, outputVector);

  // create the histogram array
  this->Histogram->SetNumberOfComponents(1);
  this->Histogram->SetNumberOfTuples(this->NumberOfBins);
  svtkIdType* histogram = this->Histogram->GetPointer(0);

  // clear histogram to zero
  int nx = this->NumberOfBins;
  int ix;
  for (ix = 0; ix < nx; ++ix)
  {
    histogram[ix] = 0;
  }

  if (this->EnableSMP)
  {
    // code for svtkSMPTools
    svtkIdType n = svtkSMPTools::GetEstimatedNumberOfThreads();

    // do a dummy execution of SplitExtent to compute the number of pieces
    svtkIdType pieces = this->SplitExtent(nullptr, extent, 0, n);

    // create the thread-local object and the functor
    svtkImageHistogramSMPThreadLocal tlocal;
    svtkImageHistogramFunctor functor(&ts, &tlocal, pieces, this->Histogram, &this->Total);
    this->SMPThreadData = &tlocal;
    bool debug = this->Debug;
    this->Debug = false;
    svtkSMPTools::For(0, pieces, functor);
    this->Debug = debug;
    this->SMPThreadData = nullptr;
  }
  else
  {
    // code for svtkMultiThreader
    svtkIdType n = this->NumberOfThreads;

    // do a dummy execution of SplitExtent to compute the number of pieces
    n = this->SplitExtent(nullptr, extent, 0, n);
    this->ThreadData = new svtkImageHistogramThreadData[n];
    this->Threader->SetNumberOfThreads(n);
    this->Threader->SetSingleMethod(svtkImageHistogramThreadedExecute, &ts);

    // always shut off debugging to avoid threading problems with GetMacros
    bool debug = this->Debug;
    this->Debug = false;
    this->Threader->SingleMethodExecute();
    this->Debug = debug;

    // piece together the histogram results from each thread
    svtkIdType total = 0;
    for (int j = 0; j < n; j++)
    {
      svtkIdType* outPtr2 = this->ThreadData[j].Data;
      if (outPtr2)
      {
        int xmin = this->ThreadData[j].Range[0];
        int xmax = this->ThreadData[j].Range[1];
        for (ix = xmin; ix <= xmax; ++ix)
        {
          svtkIdType c = *outPtr2++;
          histogram[ix] += c;
          total += c;
        }
      }
    }

    // set the total
    this->Total = total;

    // delete the temporary memory
    for (int j = 0; j < n; j++)
    {
      delete[] this->ThreadData[j].Data;
    }
    delete[] this->ThreadData;
  }

  // generate the output image
  if (this->GetNumberOfOutputPorts() > 0 && this->GenerateHistogramImage)
  {
    info = outputVector->GetInformationObject(0);
    image = svtkImageData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
    int* outExt = image->GetExtent();
    svtkImageHistogramGenerateImage(this->Histogram->GetPointer(0), this->NumberOfBins,
      static_cast<unsigned char*>(image->GetScalarPointerForExtent(outExt)),
      this->HistogramImageScale, this->HistogramImageSize, outExt);
  }

  return 1;
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void svtkImageHistogram::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** svtkNotUsed(inData), svtkImageData** svtkNotUsed(outData), int extent[6],
  int threadId)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  void* inPtr = inData->GetScalarPointerForExtent(extent);

  svtkImageStencilData* stencil = this->GetStencil();

  double binOrigin = this->BinOrigin;
  double binSpacing = this->BinSpacing;
  int scalarType = inData->GetScalarType();
  int component = this->ActiveComponent;

  // can use faster binning method for int data
  bool useFastExecute = (binSpacing == 1.0 && scalarType != SVTK_FLOAT && scalarType != SVTK_DOUBLE);

  double scalarRange[2];

  // compute the scalar range of the data unless it is byte data,
  // this allows us to allocate less memory for the histogram
  if (scalarType == SVTK_CHAR || scalarType == SVTK_UNSIGNED_CHAR || scalarType == SVTK_SIGNED_CHAR)
  {
    svtkDataArray::GetDataTypeRange(scalarType, scalarRange);
  }
  else
  {
    switch (scalarType)
    {
      svtkTemplateAliasMacro(svtkImageHistogramExecuteRange(
        inData, stencil, static_cast<SVTK_TT*>(inPtr), extent, scalarRange, component));
      default:
        svtkErrorMacro(<< "Execute: Unknown ScalarType");
    }

    // if no voxels (e.g. due to stencil) then return
    if (scalarRange[0] > scalarRange[1])
    {
      return;
    }
  }

  // convert to bin numbers
  int maxBin = this->NumberOfBins - 1;
  double scale = 1.0 / binSpacing;
  double minBinRange = (scalarRange[0] - binOrigin) * scale;
  double maxBinRange = (scalarRange[1] - binOrigin) * scale;
  if (minBinRange < 0)
  {
    minBinRange = 0;
    useFastExecute = false;
  }
  if (maxBinRange > maxBin)
  {
    maxBinRange = maxBin;
    useFastExecute = false;
  }

  svtkIdType* histogram;
  int* binRange;

  if (this->EnableSMP)
  {
    // code for svtkSMPTools
    svtkImageHistogramThreadData* threadLocal = &this->SMPThreadData->Local();
    binRange = threadLocal->Range;

    int a = svtkMath::Floor(minBinRange + 0.5);
    int b = svtkMath::Floor(maxBinRange + 0.5);
    if (threadLocal->Data == nullptr)
    {
      // allocate the histogram
      histogram = new svtkIdType[this->NumberOfBins];
      for (int i = a; i <= b; i++)
      {
        histogram[i] = 0;
      }
      threadLocal->Data = histogram;
      binRange[0] = a;
      binRange[1] = b;
    }
    else
    {
      // expand the range, if necessary
      histogram = threadLocal->Data;
      if (a < binRange[0])
      {
        for (int i = a; i < binRange[0]; i++)
        {
          histogram[i] = 0;
        }
        binRange[0] = a;
      }
      if (b > binRange[1])
      {
        for (int i = binRange[1] + 1; i <= b; i++)
        {
          histogram[i] = 0;
        }
        binRange[1] = b;
      }
    }
  }
  else
  {
    // code for svtkMultiThreader
    svtkImageHistogramThreadData* threadLocal = &this->ThreadData[threadId];
    binRange = threadLocal->Range;
    binRange[0] = svtkMath::Floor(minBinRange + 0.5);
    binRange[1] = svtkMath::Floor(maxBinRange + 0.5);
    // allocate the histogram
    int n = binRange[1] - binRange[0] + 1;
    histogram = new svtkIdType[n];
    threadLocal->Data = histogram;
    svtkIdType* tmpPtr = histogram;
    do
    {
      *tmpPtr++ = 0;
    } while (--n);
    // adjust the pointer to allow direct indexing
    histogram -= binRange[0];
  }

  // generate the histogram
  if (useFastExecute)
  {
    // adjust the pointer to allow direct indexing
    histogram -= svtkMath::Floor(binOrigin + 0.5);

    // fast path for integer data
    switch (scalarType)
    {
      svtkTemplateAliasMacro(svtkImageHistogramExecuteInt(this, inData, stencil,
        static_cast<SVTK_TT*>(inPtr), extent, histogram, component, threadId));
      default:
        svtkErrorMacro(<< "Execute: Unknown ScalarType");
    }
  }
  else
  {
    // bin via floating point shift/scale
    switch (scalarType)
    {
      svtkTemplateAliasMacro(
        svtkImageHistogramExecute(this, inData, stencil, static_cast<SVTK_TT*>(inPtr), extent,
          histogram, binRange, binOrigin, binSpacing, component, threadId));
      default:
        svtkErrorMacro(<< "Execute: Unknown ScalarType");
    }
  }
}

//----------------------------------------------------------------------------
void svtkImageHistogram::ComputeImageScalarRange(svtkImageData* data, double range[2])
{
  if (data->GetNumberOfScalarComponents() == 1)
  {
    data->GetScalarRange(range);
    return;
  }

  int* extent = data->GetExtent();
  void* inPtr = data->GetScalarPointerForExtent(extent);
  int component = this->ActiveComponent;

  switch (data->GetScalarType())
  {
    svtkTemplateAliasMacro(svtkImageHistogramExecuteRange(
      data, nullptr, static_cast<SVTK_TT*>(inPtr), extent, range, component));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
  }
}
