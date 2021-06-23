/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMapper3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageMapper3D.h"

#include "svtkAbstractTransform.h"
#include "svtkDataArray.h"
#include "svtkExecutive.h"
#include "svtkGarbageCollector.h"
#include "svtkGraphicsFactory.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMatrix3x3.h"
#include "svtkMatrix4x4.h"
#include "svtkMultiThreader.h"
#include "svtkPlane.h"
#include "svtkRenderer.h"
#include "svtkScalarsToColors.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTemplateAliasMacro.h"

//----------------------------------------------------------------------------
svtkImageMapper3D::svtkImageMapper3D()
{
  // Default color conversion
  this->DefaultLookupTable = svtkScalarsToColors::New();
  this->DefaultLookupTable->SetVectorModeToRGBColors();

  this->Threader = svtkMultiThreader::New();
  this->NumberOfThreads = this->Threader->GetNumberOfThreads();
  this->Streaming = 0;

  this->Border = 0;
  this->Background = 0;

  this->SlicePlane = svtkPlane::New();
  this->SliceFacesCamera = 0;
  this->SliceAtFocalPoint = 0;

  this->DataToWorldMatrix = svtkMatrix4x4::New();
  this->CurrentProp = nullptr;
  this->CurrentRenderer = nullptr;

  this->MatteEnable = true;
  this->ColorEnable = true;
  this->DepthEnable = true;

  this->DataOrigin[0] = 0.0;
  this->DataOrigin[1] = 0.0;
  this->DataOrigin[2] = 0.0;

  this->DataSpacing[0] = 1.0;
  this->DataSpacing[1] = 1.0;
  this->DataSpacing[2] = 1.0;

  svtkMatrix3x3::Identity(this->DataDirection);

  this->DataWholeExtent[0] = 0;
  this->DataWholeExtent[1] = 0;
  this->DataWholeExtent[2] = 0;
  this->DataWholeExtent[3] = 0;
  this->DataWholeExtent[4] = 0;
  this->DataWholeExtent[5] = 0;
}

//----------------------------------------------------------------------------
svtkImageMapper3D::~svtkImageMapper3D()
{
  if (this->DefaultLookupTable)
  {
    this->DefaultLookupTable->Delete();
  }
  if (this->Threader)
  {
    this->Threader->Delete();
  }
  if (this->SlicePlane)
  {
    this->SlicePlane->Delete();
  }
  if (this->DataToWorldMatrix)
  {
    this->DataToWorldMatrix->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkImageMapper3D::SetInputData(svtkImageData* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageMapper3D::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkImageData::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
void svtkImageMapper3D::ReleaseGraphicsResources(svtkWindow*)
{
  // see subclass for implementation
}

//----------------------------------------------------------------------------
void svtkImageMapper3D::Render(svtkRenderer*, svtkImageSlice*)
{
  // see subclass for implementation
}

//----------------------------------------------------------------------------
svtkTypeBool svtkImageMapper3D::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DataWholeExtent);
    inInfo->Get(svtkDataObject::SPACING(), this->DataSpacing);
    inInfo->Get(svtkDataObject::ORIGIN(), this->DataOrigin);
    if (inInfo->Has(svtkDataObject::DIRECTION()))
    {
      inInfo->Get(svtkDataObject::DIRECTION(), this->DataDirection);
    }
    else
    {
      svtkMatrix3x3::Identity(this->DataDirection);
    }

    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void svtkImageMapper3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SlicePlane: " << this->SlicePlane << "\n";
  os << indent << "SliceAtFocalPoint: " << (this->SliceAtFocalPoint ? "On\n" : "Off\n");
  os << indent << "SliceFacesCamera: " << (this->SliceFacesCamera ? "On\n" : "Off\n");
  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "Background: " << (this->Background ? "On\n" : "Off\n");
  os << indent << "NumberOfThreads: " << this->NumberOfThreads << "\n";
  os << indent << "Streaming: " << (this->Streaming ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
svtkDataObject* svtkImageMapper3D::GetDataObjectInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return this->GetInputDataObject(0, 0);
}

//----------------------------------------------------------------------------
svtkDataSet* svtkImageMapper3D::GetDataSetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
int svtkImageMapper3D::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageMapper3D::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
static svtkRenderer* svtkImageMapper3DFindRenderer(svtkProp* prop, int& count)
{
  svtkRenderer* ren = nullptr;

  int n = prop->GetNumberOfConsumers();
  for (int i = 0; i < n; i++)
  {
    svtkObjectBase* o = prop->GetConsumer(i);
    svtkProp3D* a = nullptr;
    if ((ren = svtkRenderer::SafeDownCast(o)))
    {
      count++;
    }
    else if ((a = svtkProp3D::SafeDownCast(o)))
    {
      ren = svtkImageMapper3DFindRenderer(a, count);
    }
  }

  return ren;
}

//----------------------------------------------------------------------------
static void svtkImageMapper3DComputeMatrix(svtkProp* prop, double mat[16])
{
  svtkMatrix4x4* propmat = prop->GetMatrix();
  svtkMatrix4x4::DeepCopy(mat, propmat);

  int n = prop->GetNumberOfConsumers();
  for (int i = 0; i < n; i++)
  {
    svtkObjectBase* o = prop->GetConsumer(i);
    svtkProp3D* a = nullptr;
    if ((a = svtkProp3D::SafeDownCast(o)))
    {
      svtkImageMapper3DComputeMatrix(a, mat);
      if (a->IsA("svtkAssembly") || a->IsA("svtkImageStack"))
      {
        svtkMatrix4x4::Multiply4x4(mat, *propmat->Element, mat);
      }
    }
  }
}

//----------------------------------------------------------------------------
svtkRenderer* svtkImageMapper3D::GetCurrentRenderer()
{
  svtkImageSlice* prop = this->CurrentProp;
  svtkRenderer* ren = this->CurrentRenderer;
  int count = 0;

  if (ren)
  {
    return ren;
  }

  if (!prop)
  {
    return nullptr;
  }

  ren = svtkImageMapper3DFindRenderer(prop, count);

  if (count > 1)
  {
    svtkErrorMacro("Cannot follow camera, mapper is associated with "
                  "multiple renderers");
    ren = nullptr;
  }

  return ren;
}

//----------------------------------------------------------------------------
svtkMatrix4x4* svtkImageMapper3D::GetDataToWorldMatrix()
{
  svtkProp3D* prop = this->CurrentProp;

  if (prop)
  {
    if (this->CurrentRenderer)
    {
      this->DataToWorldMatrix->DeepCopy(prop->GetMatrix());
    }
    else
    {
      double mat[16];
      svtkImageMapper3DComputeMatrix(prop, mat);
      this->DataToWorldMatrix->DeepCopy(mat);
    }
  }

  return this->DataToWorldMatrix;
}

//----------------------------------------------------------------------------
// Convert char data without changing format
static void svtkImageMapperCopy(const unsigned char* inPtr, unsigned char* outPtr, int ncols,
  int nrows, int numComp, svtkIdType inIncX, svtkIdType inIncY, svtkIdType outIncY)
{
  // loop through the data and copy it for the texture
  if (numComp == 1)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        *outPtr = *inPtr;
        outPtr++;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else if (numComp == 2)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        outPtr[0] = inPtr[0];
        outPtr[1] = inPtr[1];
        outPtr += 2;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else if (numComp == 3)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        outPtr[0] = inPtr[0];
        outPtr[1] = inPtr[1];
        outPtr[2] = inPtr[2];
        outPtr += 3;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else // if (numComp == 4)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        outPtr[0] = inPtr[0];
        outPtr[1] = inPtr[1];
        outPtr[2] = inPtr[2];
        outPtr[3] = inPtr[3];
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
}

//----------------------------------------------------------------------------
// Convert char data to RGBA
static void svtkImageMapperConvertToRGBA(const unsigned char* inPtr, unsigned char* outPtr,
  int ncols, int nrows, int numComp, svtkIdType inIncX, svtkIdType inIncY, svtkIdType outIncY)
{
  unsigned char alpha = 255;

  // loop through the data and copy it for the texture
  if (numComp == 1)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        unsigned char val = inPtr[0];
        outPtr[0] = val;
        outPtr[1] = val;
        outPtr[2] = val;
        outPtr[3] = alpha;
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else if (numComp == 2)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        unsigned char val = inPtr[0];
        unsigned char a = inPtr[1];
        outPtr[0] = val;
        outPtr[1] = val;
        outPtr[2] = val;
        outPtr[3] = a;
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else if (numComp == 3)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        outPtr[0] = inPtr[0];
        outPtr[1] = inPtr[1];
        outPtr[2] = inPtr[2];
        outPtr[3] = alpha;
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else // if (numComp == 4)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        outPtr[0] = inPtr[0];
        outPtr[1] = inPtr[1];
        outPtr[2] = inPtr[2];
        outPtr[3] = inPtr[3];
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
}

//----------------------------------------------------------------------------
// Convert data to unsigned char

template <class F>
inline F svtkImageMapperClamp(F x, F xmin, F xmax)
{
  // do not change this code: it compiles into min/max opcodes
  x = (x > xmin ? x : xmin);
  x = (x < xmax ? x : xmax);
  return x;
}

template <class F, class T>
void svtkImageMapperShiftScale(const T* inPtr, unsigned char* outPtr, int ncols, int nrows,
  int numComp, svtkIdType inIncX, svtkIdType inIncY, svtkIdType outIncY, F shift, F scale)
{
  const F vmin = static_cast<F>(0);
  const F vmax = static_cast<F>(255);
  unsigned char alpha = 255;

  // loop through the data and copy it for the texture
  if (numComp == 1)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        // Pixel operation
        F val = (inPtr[0] + shift) * scale;
        val = svtkImageMapperClamp(val, vmin, vmax);
        unsigned char cval = static_cast<unsigned char>(val + 0.5);
        outPtr[0] = cval;
        outPtr[1] = cval;
        outPtr[2] = cval;
        outPtr[3] = alpha;
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else if (numComp == 2)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        // Pixel operation
        F val = (inPtr[0] + shift) * scale;
        val = svtkImageMapperClamp(val, vmin, vmax);
        unsigned char cval = static_cast<unsigned char>(val + 0.5);
        val = (inPtr[1] + shift) * scale;
        val = svtkImageMapperClamp(val, vmin, vmax);
        unsigned char aval = static_cast<unsigned char>(val + 0.5);
        outPtr[0] = cval;
        outPtr[1] = cval;
        outPtr[2] = cval;
        outPtr[3] = aval;
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else if (numComp == 3)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        // Pixel operation
        F r = (inPtr[0] + shift) * scale;
        F g = (inPtr[1] + shift) * scale;
        F b = (inPtr[2] + shift) * scale;
        r = svtkImageMapperClamp(r, vmin, vmax);
        g = svtkImageMapperClamp(g, vmin, vmax);
        b = svtkImageMapperClamp(b, vmin, vmax);
        outPtr[0] = static_cast<unsigned char>(r + 0.5);
        outPtr[1] = static_cast<unsigned char>(g + 0.5);
        outPtr[2] = static_cast<unsigned char>(b + 0.5);
        outPtr[3] = alpha;
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
  else // if (numComp == 4)
  {
    for (int idy = 0; idy < nrows; idy++)
    {
      for (int idx = 0; idx < ncols; idx++)
      {
        // Pixel operation
        F r = (inPtr[0] + shift) * scale;
        F g = (inPtr[1] + shift) * scale;
        F b = (inPtr[2] + shift) * scale;
        F a = (inPtr[3] + shift) * scale;
        r = svtkImageMapperClamp(r, vmin, vmax);
        g = svtkImageMapperClamp(g, vmin, vmax);
        b = svtkImageMapperClamp(b, vmin, vmax);
        a = svtkImageMapperClamp(a, vmin, vmax);
        outPtr[0] = static_cast<unsigned char>(r + 0.5);
        outPtr[1] = static_cast<unsigned char>(g + 0.5);
        outPtr[2] = static_cast<unsigned char>(b + 0.5);
        outPtr[3] = static_cast<unsigned char>(a + 0.5);
        outPtr += 4;
        inPtr += inIncX;
      }
      outPtr += outIncY;
      inPtr += inIncY;
    }
  }
}

//----------------------------------------------------------------------------
static void svtkImageMapperConvertImageScalarsToRGBA(void* inPtr, unsigned char* outPtr, int ncols,
  int nrows, int numComp, svtkIdType inIncX, svtkIdType inIncY, svtkIdType outIncY, int scalarType,
  double scalarRange[2])
{
  double shift = -scalarRange[0];
  double scale = 255.0;

  if (scalarRange[0] < scalarRange[1])
  {
    scale /= (scalarRange[1] - scalarRange[0]);
  }
  else
  {
    scale = 1e+32;
  }

  // Check if the data can be simply copied
  if (scalarType == SVTK_UNSIGNED_CHAR && static_cast<int>(shift * scale) == 0 &&
    static_cast<int>((255 + shift) * scale) == 255)
  {
    svtkImageMapperConvertToRGBA(
      static_cast<unsigned char*>(inPtr), outPtr, ncols, nrows, numComp, inIncX, inIncY, outIncY);
  }
  else
  {
    switch (scalarType)
    {
      svtkTemplateAliasMacro(svtkImageMapperShiftScale(static_cast<SVTK_TT*>(inPtr), outPtr, ncols,
        nrows, numComp, inIncX, inIncY, outIncY, shift, scale));
      default:
        svtkGenericWarningMacro("ConvertImageScalarsToRGBA: Unknown input ScalarType");
    }
  }
}

//----------------------------------------------------------------------------
template <class T>
void svtkImageMapperMakeContiguous(
  const T* inPtr, T* outPtr, int ncols, int numComp, svtkIdType inIncX)
{
  if (numComp == 1)
  {
    for (int idx = 0; idx < ncols; idx++)
    {
      *outPtr = *inPtr;
      outPtr++;
      inPtr += inIncX;
    }
  }
  else
  {
    inIncX -= numComp;
    for (int idx = 0; idx < ncols; idx++)
    {
      int idc = numComp;
      do
      {
        *outPtr++ = *inPtr++;
      } while (--idc);
      inPtr += inIncX;
    }
  }
}

static void svtkImageMapperApplyLookupTableToImageScalars(void* inPtr, unsigned char* outPtr,
  int ncols, int nrows, int numComp, svtkIdType inIncX, svtkIdType inIncY, svtkIdType outIncY,
  int scalarType, svtkScalarsToColors* lookupTable)
{
  // number of values per row of input image
  int scalarSize = svtkDataArray::GetDataTypeSize(scalarType);

  // convert incs from continuous increments to regular increment
  outIncY += 4 * ncols;
  inIncY += inIncX * ncols;
  inIncY *= scalarSize;

  // if data not contiguous, make a temporary array
  void* newPtr = nullptr;
  if (inIncX > static_cast<svtkIdType>(numComp))
  {
    newPtr = malloc(scalarSize * numComp * ncols);
  }

  // loop through the data and copy it for the texture
  for (int idy = 0; idy < nrows; idy++)
  {
    void* tmpPtr = inPtr;

    // make contiguous if necessary
    if (inIncX > static_cast<svtkIdType>(numComp))
    {
      tmpPtr = newPtr;

      if (scalarSize == 1)
      {
        svtkImageMapperMakeContiguous(
          static_cast<char*>(inPtr), static_cast<char*>(tmpPtr), ncols, numComp, inIncX);
      }
      else if (scalarSize == 2)
      {
        svtkImageMapperMakeContiguous(
          static_cast<short*>(inPtr), static_cast<short*>(tmpPtr), ncols, numComp, inIncX);
      }
      else if (scalarSize == 4)
      {
        svtkImageMapperMakeContiguous(
          static_cast<float*>(inPtr), static_cast<float*>(tmpPtr), ncols, numComp, inIncX);
      }
      else
      {
        svtkImageMapperMakeContiguous(static_cast<double*>(inPtr), static_cast<double*>(tmpPtr),
          ncols, numComp * (scalarSize >> 3), inIncX * (scalarSize >> 3));
      }
    }

    // pass the data through the lookup table
    if (numComp == 1)
    {
      lookupTable->MapScalarsThroughTable(tmpPtr, outPtr, scalarType, ncols, numComp, SVTK_RGBA);
    }
    else
    {
      lookupTable->MapVectorsThroughTable(tmpPtr, outPtr, scalarType, ncols, numComp, SVTK_RGBA);
    }

    outPtr += outIncY;
    inPtr = static_cast<void*>(static_cast<char*>(inPtr) + inIncY);
  }

  if (newPtr)
  {
    free(newPtr);
  }
}

//----------------------------------------------------------------------------
struct svtkImageMapperThreadStruct
{
  void* InputPtr;
  unsigned char* OutputPtr;
  int ImageSize[2];
  int ScalarType;
  int NumComp;
  svtkIdType InIncX;
  svtkIdType InIncY;
  svtkIdType OutIncX;
  svtkIdType OutIncY;
  double Range[2];
  svtkScalarsToColors* LookupTable;
};

static SVTK_THREAD_RETURN_TYPE svtkImageMapperMapColors(void* arg)
{
  svtkMultiThreader::ThreadInfo* mtti = static_cast<svtkMultiThreader::ThreadInfo*>(arg);
  int threadId = mtti->ThreadID;
  int threadCount = mtti->NumberOfThreads;

  svtkImageMapperThreadStruct* imts = static_cast<svtkImageMapperThreadStruct*>(mtti->UserData);

  int ncols = imts->ImageSize[0];
  int nrows = imts->ImageSize[1];
  int scalarSize = svtkDataArray::GetDataTypeSize(imts->ScalarType);

  // only split in vertical direction
  if (threadCount > nrows)
  {
    threadCount = nrows;
    if (threadId >= threadCount)
    {
      return SVTK_THREAD_RETURN_VALUE;
    }
  }

  // adjust pointers
  int firstRow = threadId * nrows / threadCount;
  int lastRow = (threadId + 1) * nrows / threadCount;
  void* inputPtr = static_cast<char*>(imts->InputPtr) +
    (imts->InIncX * ncols + imts->InIncY) * firstRow * scalarSize;
  unsigned char* outputPtr = imts->OutputPtr + (imts->OutIncX * ncols + imts->OutIncY) * firstRow;
  nrows = lastRow - firstRow;

  // reformat the data for use as a texture
  if (imts->LookupTable)
  {
    // apply a lookup table
    svtkImageMapperApplyLookupTableToImageScalars(inputPtr, outputPtr, ncols, nrows, imts->NumComp,
      imts->InIncX, imts->InIncY, imts->OutIncY, imts->ScalarType, imts->LookupTable);
  }
  else
  {
    // no lookup table, do a shift/scale calculation
    svtkImageMapperConvertImageScalarsToRGBA(inputPtr, outputPtr, ncols, nrows, imts->NumComp,
      imts->InIncX, imts->InIncY, imts->OutIncY, imts->ScalarType, imts->Range);
  }

  return SVTK_THREAD_RETURN_VALUE;
}

//----------------------------------------------------------------------------
// Given an image and an extent that describes a single slice, this method
// will return a contiguous block of unsigned char data that can be loaded
// into a texture.
// The values of xsize, ysize, bytesPerPixel, and reuseTexture must be
// pre-loaded with the current texture size and depth, with subTexture
// set to 1 if only a subTexture is to be generated.
// When the method returns, these values will be set to the dimensions
// of the data that was produced, and subTexture will remain set to 1
// if xsize,ysize describe a subtexture size.
// If subTexture is not set to one upon return, then xsize,ysize will
// describe the full texture size, with the assumption that the full
// texture must be reloaded.
// If reuseData is false upon return, then the returned array must be
// freed after use with delete [].
unsigned char* svtkImageMapper3D::MakeTextureData(svtkImageProperty* property, svtkImageData* input,
  int extent[6], int& xsize, int& ysize, int& bytesPerPixel, bool& reuseTexture, bool& reuseData)
{
  int xdim, ydim;
  int imageSize[2];
  int textureSize[2];

  // compute image size and texture size from extent
  this->ComputeTextureSize(extent, xdim, ydim, imageSize, textureSize);

  // number of components
  int numComp = input->GetNumberOfScalarComponents();
  int scalarType = input->GetScalarType();
  int textureBytesPerPixel = 4;

  // lookup table and window/level
  double colorWindow = 255.0;
  double colorLevel = 127.5;
  svtkScalarsToColors* lookupTable = nullptr;

  if (property)
  {
    colorWindow = property->GetColorWindow();
    colorLevel = property->GetColorLevel();
    lookupTable = property->GetLookupTable();
  }

  // check if the input is pre-formatted as colors
  int inputIsColors = false;
  if (lookupTable == nullptr && scalarType == SVTK_UNSIGNED_CHAR && colorLevel == 127.5 &&
    colorWindow == 255.0)
  {
    inputIsColors = true;
    if (reuseData && numComp < 4)
    {
      textureBytesPerPixel = numComp;
    }
  }

  // reuse texture if texture size has not changed
  if (xsize == textureSize[0] && ysize == textureSize[1] && bytesPerPixel == textureBytesPerPixel &&
    reuseTexture)
  {
    // if texture is reused, only reload the image portion
    xsize = imageSize[0];
    ysize = imageSize[1];
  }
  else
  {
    xsize = textureSize[0];
    ysize = textureSize[1];
    bytesPerPixel = textureBytesPerPixel;
    reuseTexture = false;
  }

  // if the image is already of the desired size and type
  if (xsize == imageSize[0] && ysize == imageSize[1])
  {
    // Check if the data needed for the texture is a contiguous region
    // of the input data: this requires that xdim = 0 and ydim = 1
    // OR xextent = 1 pixel and xdim = 1 and ydim = 2
    // OR xdim = 0 and ydim = 2 and yextent = 1 pixel.
    // In addition the corresponding x display extents must match the
    // extent of the data
    int* dataExtent = input->GetExtent();

    if ((xdim == 0 && ydim == 1 && extent[0] == dataExtent[0] && extent[1] == dataExtent[1]) ||
      (xdim == 1 && ydim == 2 && dataExtent[0] == dataExtent[1] && extent[2] == dataExtent[2] &&
        extent[3] == dataExtent[3]) ||
      (xdim == 0 && ydim == 2 && dataExtent[2] == dataExtent[3] && extent[0] == dataExtent[0] &&
        extent[1] == dataExtent[1]))
    {
      // if contiguous and correct data type, use data as-is
      if (inputIsColors && reuseData)
      {
        return static_cast<unsigned char*>(input->GetScalarPointerForExtent(extent));
      }
    }
  }

  // could not directly use input data, so allocate a new array
  reuseData = false;

  unsigned char* outPtr = new unsigned char[ysize * xsize * bytesPerPixel];

  // output increments
  svtkIdType outIncY = bytesPerPixel * (xsize - imageSize[0]);

  // input pointer and increments
  svtkIdType inInc[3];
  void* inPtr = input->GetScalarPointerForExtent(extent);
  input->GetIncrements(inInc);
  svtkIdType inIncX = inInc[xdim];
  svtkIdType inIncY = inInc[ydim] - inInc[xdim] * imageSize[0];

  // convert Window/Level to a scalar range
  double range[2];
  range[0] = colorLevel - 0.5 * colorWindow;
  range[1] = colorLevel + 0.5 * colorWindow;

  if (lookupTable)
  {
    if (property && !property->GetUseLookupTableScalarRange())
    {
      // no way to do this without modifying the table
      lookupTable->SetRange(range);
    }
    // make sure table is up to date
    lookupTable->Build();
  }

  if (inputIsColors && !lookupTable)
  {
    // just copy the data
    svtkImageMapperCopy(static_cast<unsigned char*>(inPtr), outPtr, imageSize[0], imageSize[1],
      numComp, inIncX, inIncY, outIncY);
  }
  else
  {
    // do a multi-threaded conversion
    svtkImageMapperThreadStruct imts;
    imts.InputPtr = inPtr;
    imts.OutputPtr = outPtr;
    imts.ImageSize[0] = imageSize[0];
    imts.ImageSize[1] = imageSize[1];
    imts.ScalarType = scalarType;
    imts.NumComp = numComp;
    imts.InIncX = inIncX;
    imts.InIncY = inIncY;
    imts.OutIncX = 4;
    imts.OutIncY = outIncY;
    imts.Range[0] = range[0];
    imts.Range[1] = range[1];
    imts.LookupTable = lookupTable;

    int numThreads = this->NumberOfThreads;
    numThreads = ((numThreads <= imageSize[1]) ? numThreads : imageSize[1]);

    this->Threader->SetNumberOfThreads(numThreads);
    this->Threader->SetSingleMethod(&svtkImageMapperMapColors, &imts);
    this->Threader->SingleMethodExecute();
  }

  return outPtr;
}

//----------------------------------------------------------------------------
// Compute the coords and tcoords for the image
void svtkImageMapper3D::MakeTextureGeometry(
  const int extent[6], double coords[12], double tcoords[8])
{
  int xdim, ydim;
  int imageSize[2];
  int textureSize[2];

  // compute image size and texture size from extent
  this->ComputeTextureSize(extent, xdim, ydim, imageSize, textureSize);

  // get spacing/origin for the quad coordinates
  double* spacing = this->DataSpacing;
  double* origin = this->DataOrigin;
  double* direction = this->DataDirection;

  // stretch the geometry one half-pixel
  double dext[6]{ static_cast<double>(extent[0]), static_cast<double>(extent[1]),
    static_cast<double>(extent[2]), static_cast<double>(extent[3]), static_cast<double>(extent[4]),
    static_cast<double>(extent[5]) };
  if (this->Border)
  {
    dext[xdim * 2] -= 0.5;
    dext[xdim * 2 + 1] += 0.5;
    dext[ydim * 2] -= 0.5;
    dext[ydim * 2 + 1] += 0.5;
  }

  // compute the world coordinates of the quad
  svtkImageData::TransformContinuousIndexToPhysicalPoint(
    dext[0], dext[2], dext[4], origin, spacing, direction, coords);
  svtkImageData::TransformContinuousIndexToPhysicalPoint(
    dext[1], dext[2 + (xdim == 1)], dext[4], origin, spacing, direction, coords + 3);
  svtkImageData::TransformContinuousIndexToPhysicalPoint(
    dext[1], dext[3], dext[5], origin, spacing, direction, coords + 6);
  svtkImageData::TransformContinuousIndexToPhysicalPoint(
    dext[0], dext[2 + (ydim == 1)], dext[5], origin, spacing, direction, coords + 9);

  if (tcoords)
  {
    // compute the tcoords
    double textureBorder = 0.5 * (this->Border == 0);

    tcoords[0] = textureBorder / textureSize[0];
    tcoords[1] = textureBorder / textureSize[1];

    tcoords[2] = (imageSize[0] - textureBorder) / textureSize[0];
    tcoords[3] = tcoords[1];

    tcoords[4] = tcoords[2];
    tcoords[5] = (imageSize[1] - textureBorder) / textureSize[1];

    tcoords[6] = tcoords[0];
    tcoords[7] = tcoords[5];
  }
}

//----------------------------------------------------------------------------
void svtkImageMapper3D::ComputeTextureSize(
  const int extent[6], int& xdim, int& ydim, int imageSize[2], int textureSize[2])
{
  // find dimension indices that will correspond to the
  // columns and rows of the 2D texture
  xdim = 1;
  ydim = 2;
  if (extent[0] != extent[1])
  {
    xdim = 0;
    if (extent[2] != extent[3])
    {
      ydim = 1;
    }
  }

  // compute the image dimensions
  imageSize[0] = (extent[xdim * 2 + 1] - extent[xdim * 2] + 1);
  imageSize[1] = (extent[ydim * 2 + 1] - extent[ydim * 2] + 1);

  textureSize[0] = imageSize[0];
  textureSize[1] = imageSize[1];
}

//----------------------------------------------------------------------------
void svtkImageMapper3D::GetSlicePlaneInDataCoords(svtkMatrix4x4* propMatrix, double normal[4])
{
  double point[3];
  this->SlicePlane->GetNormal(normal);
  this->SlicePlane->GetOrigin(point);

  // The plane has a transform, though most people forget
  svtkAbstractTransform* planeTransform = this->SlicePlane->GetTransform();
  if (planeTransform)
  {
    planeTransform->TransformNormalAtPoint(point, normal, normal);
    planeTransform->TransformPoint(point, point);
  }

  // Convert to a homogeneous normal in data coords
  normal[3] = -svtkMath::Dot(point, normal);

  // Transform to data coordinates
  if (propMatrix)
  {
    double mat[16];
    svtkMatrix4x4::Transpose(*propMatrix->Element, mat);
    svtkMatrix4x4::MultiplyPoint(mat, normal, normal);
  }

  // Normalize the "normal" part for good measure
  double l = svtkMath::Norm(normal);
  normal[0] /= l;
  normal[1] /= l;
  normal[2] /= l;
  normal[3] /= l;
}

//----------------------------------------------------------------------------
void svtkImageMapper3D::GetBackgroundColor(svtkImageProperty* property, double color[4])
{
  color[0] = 0.0;
  color[1] = 0.0;
  color[2] = 0.0;
  color[3] = 1.0;

  if (property)
  {
    svtkScalarsToColors* table = property->GetLookupTable();
    if (table)
    {
      double v = property->GetColorLevel() - 0.5 * property->GetColorWindow();
      if (property->GetUseLookupTableScalarRange())
      {
        v = table->GetRange()[0];
      }
      table->GetColor(v, color);
      color[3] = table->GetOpacity(v);
    }
  }
}

//----------------------------------------------------------------------------
void svtkImageMapper3D::CheckerboardRGBA(unsigned char* data, int xsize, int ysize, double originx,
  double originy, double spacingx, double spacingy)
{
  static double tol = 7.62939453125e-06;
  static double maxval = 2147483647;
  static double minval = -2147483647;

  originx += 1.0 + tol;
  originy += 1.0 + tol;

  originx = (originx > minval ? originx : minval);
  originx = (originx < maxval ? originx : maxval);
  originy = (originy > minval ? originy : minval);
  originy = (originy < maxval ? originy : maxval);

  spacingx = fabs(spacingx);
  spacingy = fabs(spacingy);

  spacingx = (spacingx < maxval ? spacingx : maxval);
  spacingy = (spacingy < maxval ? spacingy : maxval);
  spacingx = (spacingx != 0 ? spacingx : maxval);
  spacingy = (spacingy != 0 ? spacingy : maxval);

  int xn = static_cast<int>(spacingx + tol);
  int yn = static_cast<int>(spacingy + tol);
  double fx = spacingx - xn;
  double fy = spacingy - yn;

  int state = 0;
  int tmpstate = ~state;
  double spacing2x = 2 * spacingx;
  double spacing2y = 2 * spacingy;
  originx -= ceil(originx / spacing2x) * spacing2x;
  while (originx < 0)
  {
    originx += spacing2x;
  }
  originy -= ceil(originy / spacing2y) * spacing2y;
  while (originy < 0)
  {
    originy += spacing2y;
  }
  double tmporiginx = originx - spacingx;
  originx = (tmporiginx < 0 ? originx : tmporiginx);
  state = (tmporiginx < 0 ? state : tmpstate);
  tmpstate = ~state;
  double tmporiginy = originy - spacingy;
  originy = (tmporiginy < 0 ? originy : tmporiginy);
  state = (tmporiginy < 0 ? state : tmpstate);

  int xm = static_cast<int>(originx);
  int savexm = xm;
  int ym = static_cast<int>(originy);
  double gx = originx - xm;
  double savegx = gx;
  double gy = originy - ym;

  int inc = 4;
  data += (inc - 1);
  for (int j = 0; j < ysize;)
  {
    double tmpy = gy - 1.0;
    gy = (tmpy < 0 ? gy : tmpy);
    int yextra = (tmpy >= 0);
    ym += yextra;
    int ry = ysize - j;
    ym = (ym < ry ? ym : ry);
    j += ym;

    for (; ym; --ym)
    {
      tmpstate = state;
      xm = savexm;
      gx = savegx;

      for (int i = 0; i < xsize;)
      {
        double tmpx = gx - 1.0;
        gx = (tmpx < 0 ? gx : tmpx);
        int xextra = (tmpx >= 0);
        xm += xextra;
        int rx = xsize - i;
        xm = (xm < rx ? xm : rx);
        i += xm;
        if ((tmpstate & xm))
        {
          do
          {
            *data = 0;
            data += inc;
          } while (--xm);
        }
        data += inc * xm;
        xm = xn;
        tmpstate = ~tmpstate;
        gx += fx;
      }
    }

    ym = yn;
    state = ~state;
    gy += fy;
  }
}
