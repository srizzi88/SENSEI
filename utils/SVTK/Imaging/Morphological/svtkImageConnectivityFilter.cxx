/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================
  Copyright (c) 2014 David Gobbi
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of David Gobbi nor the names of any contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include "svtkImageConnectivityFilter.h"

#include "svtkDataSet.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageIterator.h"
#include "svtkImageStencilData.h"
#include "svtkImageStencilIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTemplateAliasMacro.h"
#include "svtkTypeTraits.h"
#include "svtkVersion.h"

#include <algorithm>
#include <stack>
#include <vector>

svtkStandardNewMacro(svtkImageConnectivityFilter);

//----------------------------------------------------------------------------
// Constructor sets default values
svtkImageConnectivityFilter::svtkImageConnectivityFilter()
{
  this->LabelMode = SeedScalar;
  this->ExtractionMode = SeededRegions;

  this->ScalarRange[0] = 0.5;
  this->ScalarRange[1] = SVTK_DOUBLE_MAX;

  this->SizeRange[0] = 1;
  this->SizeRange[1] = SVTK_ID_MAX;

  this->LabelConstantValue = 255;

  this->ActiveComponent = 0;

  this->LabelScalarType = SVTK_UNSIGNED_CHAR;

  this->GenerateRegionExtents = 0;

  this->ExtractedRegionLabels = svtkIdTypeArray::New();
  this->ExtractedRegionSizes = svtkIdTypeArray::New();
  this->ExtractedRegionSeedIds = svtkIdTypeArray::New();
  this->ExtractedRegionExtents = svtkIntArray::New();
  this->ExtractedRegionExtents->SetNumberOfComponents(6);

  this->SetNumberOfInputPorts(3);
}

//----------------------------------------------------------------------------
svtkImageConnectivityFilter::~svtkImageConnectivityFilter()
{
  if (this->ExtractedRegionSizes)
  {
    this->ExtractedRegionSizes->Delete();
  }
  if (this->ExtractedRegionLabels)
  {
    this->ExtractedRegionLabels->Delete();
  }
  if (this->ExtractedRegionSeedIds)
  {
    this->ExtractedRegionSeedIds->Delete();
  }
  if (this->ExtractedRegionExtents)
  {
    this->ExtractedRegionExtents->Delete();
  }
}

//----------------------------------------------------------------------------
int svtkImageConnectivityFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 2)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageStencilData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageConnectivityFilter::SetStencilConnection(svtkAlgorithmOutput* stencil)
{
  this->SetInputConnection(1, stencil);
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkImageConnectivityFilter::GetStencilConnection()
{
  return this->GetInputConnection(1, 0);
}

//----------------------------------------------------------------------------
void svtkImageConnectivityFilter::SetStencilData(svtkImageStencilData* stencil)
{
  this->SetInputData(1, stencil);
}

//----------------------------------------------------------------------------
void svtkImageConnectivityFilter::SetSeedConnection(svtkAlgorithmOutput* seeds)
{
  this->SetInputConnection(2, seeds);
}

//----------------------------------------------------------------------------
svtkAlgorithmOutput* svtkImageConnectivityFilter::GetSeedConnection()
{
  return this->GetInputConnection(2, 0);
}

//----------------------------------------------------------------------------
void svtkImageConnectivityFilter::SetSeedData(svtkDataSet* seeds)
{
  this->SetInputData(2, seeds);
}

//----------------------------------------------------------------------------
const char* svtkImageConnectivityFilter::GetLabelScalarTypeAsString()
{
  const char* result = "Unknown";
  switch (this->LabelScalarType)
  {
    case SVTK_UNSIGNED_CHAR:
      result = "UnsignedChar";
      break;
    case SVTK_SHORT:
      result = "Short";
      break;
    case SVTK_UNSIGNED_SHORT:
      result = "UnsignedShort";
      break;
    case SVTK_INT:
      result = "Int";
      break;
  }
  return result;
}

//----------------------------------------------------------------------------
const char* svtkImageConnectivityFilter::GetLabelModeAsString()
{
  const char* result = "Unknown";
  switch (this->LabelMode)
  {
    case SeedScalar:
      result = "SeedScalar";
      break;
    case ConstantValue:
      result = "ConstantValue";
      break;
    case SizeRank:
      result = "SizeRank";
      break;
  }
  return result;
}

//----------------------------------------------------------------------------
const char* svtkImageConnectivityFilter::GetExtractionModeAsString()
{
  const char* result = "Unknown";
  switch (this->ExtractionMode)
  {
    case SeededRegions:
      result = "SeededRegions";
      break;
    case AllRegions:
      result = "AllRegions";
      break;
    case LargestRegion:
      result = "LargestRegion";
      break;
  }
  return result;
}

//----------------------------------------------------------------------------
svtkIdType svtkImageConnectivityFilter::GetNumberOfExtractedRegions()
{
  return this->ExtractedRegionLabels->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
namespace
{

// Methods for the connectivity algorithm
class svtkICF
{
public:
  // Simple struct that holds information about a region.
  struct Region;

  // A class that is a vector of regions.
  class RegionVector;

protected:
  // Simple class that holds a seed location and a scalar value.
  class Seed;

  // A functor to assist in comparing region sizes.
  struct CompareSize;

  // Remove all but the largest region from the output image.
  template <class OT>
  static void PruneAllButLargest(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
    int extent[6], const OT& value, svtkICF::RegionVector& regionInfo);

  // Remove the smallest region from the output image.
  // This is called when there are no labels left, i.e. when the label
  // value reaches the maximum allowed by the output data type.
  template <class OT>
  static void PruneSmallestRegion(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
    int extent[6], svtkICF::RegionVector& regionInfo);

  // Remove all islands that aren't in the given range of sizes
  template <class OT>
  static void PruneBySize(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
    int extent[6], svtkIdType sizeRange[2], svtkICF::RegionVector& regionInfo);

  // This is the function that grows a region from a seed.
  template <class OT>
  static svtkIdType Fill(OT* outPtr, svtkIdType outInc[3], int outLimits[6], unsigned char* maskPtr,
    int maxIdx[3], int fillExtent[6], std::stack<svtkICF::Seed>& seedStack);

  // Add a region to the list of regions.
  template <class OT>
  static void AddRegion(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
    int extent[6], svtkIdType sizeRange[2], svtkICF::RegionVector& regionInfo, svtkIdType voxelCount,
    svtkIdType regionId, int regionExtent[6], int extractionMode);

  // Fill the ExtractedRegionSizes and ExtractedRegionLabels arrays.
  static void GenerateRegionArrays(svtkImageConnectivityFilter* self,
    svtkICF::RegionVector& regionInfo, svtkDataArray* seedScalars, int extent[6], int minLabel,
    int maxLabel);

  // Relabel the image, usually the last method called.
  template <class OT>
  static void Relabel(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
    int extent[6], svtkIdTypeArray* labelMap);

  // Sort the ExtractedRegionLabels array and the other arrays.
  static void SortRegionArrays(svtkImageConnectivityFilter* self);

  // Finalize the output
  template <class OT>
  static void Finish(svtkImageConnectivityFilter* self, svtkImageData* outData, OT* outPtr,
    svtkImageStencilData* stencil, int extent[6], svtkDataArray* seedScalars,
    svtkICF::RegionVector& regionInfo);

  // Subtract the lower extent limit from "limits", and return the
  // extent size subtract 1 in maxIdx.
  static int* ZeroBaseExtent(const int wholeExtent[6], int extent[6], int maxIdx[3]);

  // Execute method for when point seeds are provided.
  template <class OT>
  static void SeededExecute(svtkImageConnectivityFilter* self, svtkImageData* outData,
    svtkDataSet* seedData, svtkImageStencilData* stencil, OT* outPtr, unsigned char* maskPtr,
    int extent[6], svtkICF::RegionVector& regionInfo);

  // Execute method for when no seeds are provided.
  template <class OT>
  static void SeedlessExecute(svtkImageConnectivityFilter* self, svtkImageData* outData,
    svtkImageStencilData* stencil, OT* outPtr, unsigned char* maskPtr, int extent[6],
    svtkICF::RegionVector& regionInfo);

public:
  // Create a bit mask from the input
  template <class IT>
  static void ExecuteInput(svtkImageConnectivityFilter* self, svtkImageData* inData, IT* inPtr,
    unsigned char* maskPtr, svtkImageStencilData* stencil, int extent[6]);

  // Generate the output
  template <class OT>
  static void ExecuteOutput(svtkImageConnectivityFilter* self, svtkImageData* outData,
    svtkDataSet* seedData, svtkImageStencilData* stencil, OT* outPtr, unsigned char* maskPtr,
    int extent[6]);

  // Utility method to find the intersection of two extents.
  // Returns false if the extents do not intersect.
  static bool IntersectExtents(const int extent1[6], const int extent2[6], int output[6]);
};

//----------------------------------------------------------------------------
// seed struct: structured coordinates and a value,
// the coords can be accessed with [] and the value with *
class svtkICF::Seed
{
public:
  Seed(int i, int j, int k, int v)
  {
    pos[0] = i;
    pos[1] = j;
    pos[2] = k;
    value = v;
  }

  Seed(const svtkICF::Seed& seed)
  {
    pos[0] = seed.pos[0];
    pos[1] = seed.pos[1];
    pos[2] = seed.pos[2];
    value = seed.value;
  }

  int& operator[](int i) { return pos[i]; }

  int& operator*() { return value; }

  svtkICF::Seed& operator=(const svtkICF::Seed& seed)
  {
    pos[0] = seed.pos[0];
    pos[1] = seed.pos[1];
    pos[2] = seed.pos[2];
    value = seed.value;
    return *this;
  }

private:
  int pos[3];
  int value;
};

//----------------------------------------------------------------------------
// region struct: size and id
struct svtkICF::Region
{
  Region(svtkIdType s, svtkIdType i, const int e[6])
    : size(s)
    , id(i)
  {
    extent[0] = e[0];
    extent[1] = e[1];
    extent[2] = e[2];
    extent[3] = e[3];
    extent[4] = e[4];
    extent[5] = e[5];
  }
  Region()
    : size(0)
    , id(0)
  {
    extent[0] = extent[1] = extent[2] = 0;
    extent[3] = extent[4] = extent[5] = 0;
  }

  svtkIdType size;
  svtkIdType id;
  int extent[6];
};

//----------------------------------------------------------------------------
class svtkICF::RegionVector : public std::vector<svtkICF::Region>
{
public:
  typedef std::vector<svtkICF::Region>::iterator iterator;

  // get the smallest of the regions in the vector
  iterator smallest()
  {
    // start at 1, because 0 is the background
    iterator small = begin() + 1;
    if (small != end())
    {
      for (iterator i = small + 1; i != end(); ++i)
      {
        if (i->size <= small->size)
        {
          small = i;
        }
      }
    }
    return small;
  }

  // get the largest of the regions in the vector
  iterator largest()
  {
    iterator large = begin() + 1;
    if (large != end())
    {
      // start at 1, because 0 is the background
      for (iterator i = large + 1; i != end(); ++i)
      {
        if (i->size > large->size)
        {
          large = i;
        }
      }
    }
    return large;
  }
};

//----------------------------------------------------------------------------
bool svtkICF::IntersectExtents(const int extent1[6], const int extent2[6], int output[6])
{
  bool rval = true;

  int i = 0;
  while (i < 6)
  {
    output[i] = (extent1[i] > extent2[i] ? extent1[i] : extent2[i]);
    i++;
    output[i] = (extent1[i] < extent2[i] ? extent1[i] : extent2[i]);
    rval &= (output[i - 1] <= output[i]);
    i++;
  }

  return rval;
}

//----------------------------------------------------------------------------
template <class IT>
void svtkICF::ExecuteInput(svtkImageConnectivityFilter* self, svtkImageData* inData, IT*,
  unsigned char* maskPtr, svtkImageStencilData* stencil, int extent[6])
{
  // Get active component (only one component is thresholded)
  int nComponents = inData->GetNumberOfScalarComponents();
  int activeComponent = self->GetActiveComponent();
  if (activeComponent < 0 || activeComponent > nComponents)
  {
    activeComponent = 0;
  }

  // Get the scalar range clamped to the input type range
  double drange[2];
  self->GetScalarRange(drange);
  IT srange[2];
  srange[0] = svtkTypeTraits<IT>::Min();
  srange[1] = svtkTypeTraits<IT>::Max();
  if (drange[0] > static_cast<double>(srange[1]))
  {
    srange[0] = srange[1];
  }
  else if (drange[0] > static_cast<double>(srange[0]))
  {
    srange[0] = static_cast<IT>(drange[0]);
  }
  if (drange[1] < static_cast<double>(srange[0]))
  {
    srange[1] = srange[0];
  }
  else if (drange[1] < static_cast<double>(srange[1]))
  {
    srange[1] = static_cast<IT>(drange[1]);
  }

  // offset into the mask
  unsigned char* maskPtr1 = maskPtr;
  unsigned char bit = 1;
  unsigned char bits = 0;

  svtkImageStencilIterator<IT> iter(inData, stencil, extent);
  for (; !iter.IsAtEnd(); iter.NextSpan())
  {
    IT* inPtr = iter.BeginSpan();
    IT* inPtrEnd = iter.EndSpan();
    if (iter.IsInStencil())
    {
      while (inPtr != inPtrEnd)
      {
        IT val = inPtr[activeComponent];
        if (val < srange[0] || val > srange[1])
        {
          bits ^= bit;
        }
        bit <<= 1;
        if (bit == 0)
        {
          *maskPtr1++ = bits;
          bits = 0;
          bit = 1;
        }
        inPtr += nComponents;
      }
    }
    else
    {
      // set all bits that are outside the stencil region
      while (inPtr != inPtrEnd)
      {
        bits ^= bit;
        bit <<= 1;
        if (bit == 0)
        {
          *maskPtr1++ = bits;
          bits = 0;
          bit = 1;
        }
        inPtr += nComponents;
      }
    }
  }

  // write the last byte to the bitmask
  if (bit != 1)
  {
    *maskPtr1++ = bits;
  }
}

//----------------------------------------------------------------------------
template <class OT>
void svtkICF::PruneAllButLargest(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
  int extent[6], const OT& value, svtkICF::RegionVector& regionInfo)
{
  // clip the extent with the output extent
  int outExt[6];
  outData->GetExtent(outExt);
  if (!svtkICF::IntersectExtents(outExt, extent, outExt))
  {
    return;
  }

  // find the largest region
  svtkICF::RegionVector::iterator largest = regionInfo.largest();
  if (largest != regionInfo.end())
  {
    // get its position, remove all other regions from the list
    OT t = std::distance(regionInfo.begin(), largest);
    regionInfo[1] = *largest;
    regionInfo.erase(regionInfo.begin() + 2, regionInfo.end());

    // remove all other regions from the output
    svtkImageStencilIterator<OT> iter(outData, stencil, outExt);
    for (; !iter.IsAtEnd(); iter.NextSpan())
    {
      if (iter.IsInStencil())
      {
        outPtr = iter.BeginSpan();
        OT* endPtr = iter.EndSpan();
        for (; outPtr != endPtr; ++outPtr)
        {
          OT v = *outPtr;
          if (v == t)
          {
            *outPtr = value;
          }
          else if (v != 0)
          {
            *outPtr = 0;
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
template <class OT>
void svtkICF::PruneSmallestRegion(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
  int extent[6], svtkICF::RegionVector& regionInfo)
{
  // clip the extent with the output extent
  int outExt[6];
  outData->GetExtent(outExt);
  if (!svtkICF::IntersectExtents(outExt, extent, outExt))
  {
    return;
  }

  // find the smallest region
  svtkICF::RegionVector::iterator smallest = regionInfo.smallest();
  if (smallest != regionInfo.end())
  {
    // get the index to the smallest value and remove it
    OT t = std::distance(regionInfo.begin(), smallest);
    regionInfo.erase(smallest);

    // remove the corresponding region from the output
    svtkImageStencilIterator<OT> iter(outData, stencil, outExt);
    for (; !iter.IsAtEnd(); iter.NextSpan())
    {
      if (iter.IsInStencil())
      {
        outPtr = iter.BeginSpan();
        OT* endPtr = iter.EndSpan();
        for (; outPtr != endPtr; ++outPtr)
        {
          OT v = *outPtr;
          if (v == t)
          {
            *outPtr = 0;
          }
          else if (v > t)
          {
            *outPtr = v - 1;
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
template <class OT>
void svtkICF::PruneBySize(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
  int extent[6], svtkIdType sizeRange[2], svtkICF::RegionVector& regionInfo)
{
  // find all the regions in the allowed size range
  size_t n = regionInfo.size();
  std::vector<OT> newlabels(n);
  newlabels[0] = 0;
  size_t m = 1;
  for (size_t i = 1; i < n; i++)
  {
    size_t l = 0;
    svtkIdType s = regionInfo[i].size;
    if (s >= sizeRange[0] && s <= sizeRange[1])
    {
      l = m++;
      if (i != l)
      {
        regionInfo[l] = regionInfo[i];
      }
    }
    newlabels[i] = static_cast<OT>(l);
  }

  // were any regions outside of the range?
  if (m < n)
  {
    // resize regionInfo
    regionInfo.resize(m);

    // clip the extent with the output extent
    int outExt[6];
    outData->GetExtent(outExt);
    if (!svtkICF::IntersectExtents(outExt, extent, outExt))
    {
      return;
    }

    // remove the corresponding regions from the output
    svtkImageStencilIterator<OT> iter(outData, stencil, outExt);
    for (; !iter.IsAtEnd(); iter.NextSpan())
    {
      if (iter.IsInStencil())
      {
        outPtr = iter.BeginSpan();
        OT* endPtr = iter.EndSpan();
        for (; outPtr != endPtr; ++outPtr)
        {
          OT v = *outPtr;
          if (v != 0)
          {
            *outPtr = newlabels[v];
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// Perform a flood fill for each given seed.
template <class OT>
svtkIdType svtkICF::Fill(OT* outPtr, svtkIdType outInc[3], int outLimits[6], unsigned char* maskPtr,
  int maxIdx[3], int fillExtent[6], std::stack<svtkICF::Seed>& seedStack)
{
  svtkIdType counter = 0;

  while (!seedStack.empty())
  {
    // get the seed at the top of the stack
    svtkICF::Seed seed = seedStack.top();
    seedStack.pop();

    // get the offset into the bitmask
    svtkIdType bitOffset = seed[2];
    bitOffset = bitOffset * (maxIdx[1] + 1) + seed[1];
    bitOffset = bitOffset * (maxIdx[0] + 1) + seed[0];
    unsigned char bit = 1 << static_cast<unsigned char>(bitOffset & 0x7);
    unsigned char* maskPtr1 = maskPtr + (bitOffset >> 3);

    // if already colored, skip
    if ((*maskPtr1 & bit) != 0)
    {
      continue;
    }

    // paint the mask and count the voxel
    *maskPtr1 ^= bit;
    counter++;

    if (fillExtent != nullptr)
    {
      if (seed[0] < fillExtent[0])
      {
        fillExtent[0] = seed[0];
      }
      if (seed[0] > fillExtent[1])
      {
        fillExtent[1] = seed[0];
      }
      if (seed[1] < fillExtent[2])
      {
        fillExtent[2] = seed[1];
      }
      if (seed[1] > fillExtent[3])
      {
        fillExtent[3] = seed[1];
      }
      if (seed[2] < fillExtent[4])
      {
        fillExtent[4] = seed[2];
      }
      if (seed[2] > fillExtent[5])
      {
        fillExtent[5] = seed[2];
      }
    }

    if (outLimits == nullptr)
    {
      // get the pointer to the seed position in the mask
      svtkIdType outOffset = (seed[0] * outInc[0] + seed[1] * outInc[1] + seed[2] * outInc[2]);

      // set the output
      outPtr[outOffset] = static_cast<OT>(*seed);
    }
    else if (seed[0] >= outLimits[0] && seed[0] <= outLimits[1] && seed[1] >= outLimits[2] &&
      seed[1] <= outLimits[3] && seed[2] >= outLimits[4] && seed[2] <= outLimits[5])
    {
      // get the pointer to the seed position in the mask
      svtkIdType outOffset = ((seed[0] - outLimits[0]) * outInc[0] +
        (seed[1] - outLimits[2]) * outInc[1] + (seed[2] - outLimits[4]) * outInc[2]);

      // set the output if within output extent
      outPtr[outOffset] = static_cast<OT>(*seed);
    }

    // push the new seeds for the six neighbors, make sure offsets in X are
    // pushed last so that they will be popped first (we want to raster X,
    // Y, and Z in that order).
    int i = 3;
    do
    {
      i--;
      if (seed[i] > 0)
      {
        seed[i]--;
        seedStack.push(seed);
        seed[i]++;
      }
      if (seed[i] < maxIdx[i])
      {
        seed[i]++;
        seedStack.push(seed);
        seed[i]--;
      }
    } while (i != 0);
  }

  return counter;
}

//----------------------------------------------------------------------------
template <class OT>
void svtkICF::AddRegion(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil,
  int extent[6], svtkIdType sizeRange[2], svtkICF::RegionVector& regionInfo, svtkIdType voxelCount,
  svtkIdType regionId, int regionExtent[6], int extractionMode)
{
  regionInfo.push_back(svtkICF::Region(voxelCount, regionId, regionExtent));
  // check if the label value has reached its maximum, and if so,
  // remove some of the regions
  if (regionInfo.size() > static_cast<size_t>(svtkTypeTraits<OT>::Max()))
  {
    svtkICF::PruneBySize(outData, outPtr, stencil, extent, sizeRange, regionInfo);

    // if that didn't remove anything, try these:
    if (regionInfo.size() > static_cast<size_t>(svtkTypeTraits<OT>::Max()))
    {
      if (extractionMode == svtkImageConnectivityFilter::LargestRegion)
      {
        OT label = 1;
        svtkICF::PruneAllButLargest(outData, outPtr, stencil, extent, label, regionInfo);
      }
      else
      {
        svtkICF::PruneSmallestRegion(outData, outPtr, stencil, extent, regionInfo);
      }
    }
  }
}

//----------------------------------------------------------------------------
// a functor to sort region indices by region size
struct svtkICF::CompareSize
{
  CompareSize(svtkICF::RegionVector& r)
    : Regions(&r)
  {
  }

  svtkICF::RegionVector* Regions;

  bool operator()(svtkIdType x, svtkIdType y) { return ((*Regions)[x].size > (*Regions)[y].size); }
};

//----------------------------------------------------------------------------
void svtkICF::GenerateRegionArrays(svtkImageConnectivityFilter* self,
  svtkICF::RegionVector& regionInfo, svtkDataArray* seedScalars, int extent[6], int minLabel,
  int maxLabel)
{
  // clamp the default label value to the range of the output data type
  int constantLabel = self->GetLabelConstantValue();
  constantLabel = (constantLabel > minLabel ? constantLabel : minLabel);
  constantLabel = (constantLabel < maxLabel ? constantLabel : maxLabel);

  // build the arrays
  svtkIdTypeArray* sizes = self->GetExtractedRegionSizes();
  svtkIdTypeArray* ids = self->GetExtractedRegionSeedIds();
  svtkIdTypeArray* labels = self->GetExtractedRegionLabels();
  svtkIntArray* extents = self->GetExtractedRegionExtents();

  if (regionInfo.size() == 1)
  {
    // only background is present, there are no connected regions
    sizes->Reset();
    ids->Reset();
    labels->Reset();
    extents->Reset();
  }
  else if (self->GetExtractionMode() == svtkImageConnectivityFilter::LargestRegion)
  {
    // only one region (the largest) will be output
    sizes->SetNumberOfValues(1);
    ids->SetNumberOfValues(1);
    labels->SetNumberOfValues(1);
    extents->SetNumberOfTuples(1);

    // get info for the largest region
    svtkICF::RegionVector::iterator largest = regionInfo.largest();

    // default label value is 1
    int label = 1;

    // check which label mode was selected
    switch (self->GetLabelMode())
    {
      // use the scalars of the seed points as labels
      case svtkImageConnectivityFilter::SeedScalar:
        if (seedScalars)
        {
          label = constantLabel;
          // get the label from the seed scalars
          if (largest->id >= 0)
          {
            double s = seedScalars->GetTuple1(largest->id);
            s = (s > minLabel ? s : minLabel);
            s = (s < maxLabel ? s : maxLabel);
            label = svtkMath::Floor(s + 0.5);
          }
        }
        break;
      // use the specified ConstantValue for all regions
      case svtkImageConnectivityFilter::ConstantValue:
        label = constantLabel;
        break;
    }

    // create the arrays for the single region present in the output
    sizes->SetValue(0, largest->size);
    ids->SetValue(0, largest->id);
    labels->SetValue(0, label);
    const int* regionExt = largest->extent;
    int* extPtr = extents->GetPointer(0);
    // rebase the region extent starting at "extent" lower bound
    for (int k = 0; k < 3; k++)
    {
      extPtr[2 * k] = regionExt[2 * k] + extent[2 * k];
      extPtr[2 * k + 1] = regionExt[2 * k + 1] + extent[2 * k];
    }
  }
  else
  {
    // multiple regions might be present in the output (we subtract
    // one because the background doesn't count as a region)
    svtkIdType n = static_cast<svtkIdType>(regionInfo.size()) - 1;
    sizes->SetNumberOfValues(n);
    ids->SetNumberOfValues(n);
    labels->SetNumberOfValues(n);
    extents->SetNumberOfTuples(n);

    // build the arrays (this part is easy!)
    for (svtkIdType i = 0; i < n; i++)
    {
      sizes->SetValue(i, regionInfo[i + 1].size);
      ids->SetValue(i, regionInfo[i + 1].id);
      labels->SetValue(i, i + 1);
      const int* regionExt = regionInfo[i + 1].extent;
      int* extPtr = extents->GetPointer(6 * i);
      // rebase the region extent starting at "extent" lower bound
      for (int k = 0; k < 3; k++)
      {
        extPtr[2 * k] = regionExt[2 * k] + extent[2 * k];
        extPtr[2 * k + 1] = regionExt[2 * k + 1] + extent[2 * k];
      }
    }

    // some label modes require additional actions to be done
    switch (self->GetLabelMode())
    {
      // change the labels to match the scalars for the seed points
      case svtkImageConnectivityFilter::SeedScalar:
        if (seedScalars)
        {
          for (svtkIdType i = 0; i < n; i++)
          {
            int label = constantLabel;
            svtkIdType j = regionInfo[i + 1].id;
            if (j >= 0)
            {
              double s = seedScalars->GetTuple1(j);
              s = (s > minLabel ? s : minLabel);
              s = (s < maxLabel ? s : maxLabel);
              label = svtkMath::Floor(s + 0.5);
            }
            labels->SetValue(i, label);
          }
        }
        break;
      // order the labels by the size rank of the regions
      case svtkImageConnectivityFilter::SizeRank:
      {
        svtkICF::CompareSize cmpfunc(regionInfo);
        std::vector<svtkIdType> tmpi(static_cast<size_t>(n));
        for (svtkIdType i = 0; i < n; i++)
        {
          tmpi[i] = i + 1;
        }
        std::stable_sort(tmpi.begin(), tmpi.end(), cmpfunc);
        for (svtkIdType i = 0; i < n; i++)
        {
          labels->SetValue(tmpi[i] - 1, i + 1);
        }
      }
      break;
      // set all labels to the same value
      case svtkImageConnectivityFilter::ConstantValue:
        for (svtkIdType i = 0; i < n; i++)
        {
          labels->SetValue(i, constantLabel);
        }
        break;
    }
  }
}

//----------------------------------------------------------------------------
// generate the output image
template <class OT>
void svtkICF::Relabel(svtkImageData* outData, OT* outPtr, svtkImageStencilData* stencil, int extent[6],
  svtkIdTypeArray* labelMap)
{
  // clip the extent with the output extent
  int outExt[6];
  outData->GetExtent(outExt);
  if (!svtkICF::IntersectExtents(outExt, extent, outExt))
  {
    return;
  }

  svtkImageStencilIterator<OT> iter(outData, stencil, outExt);

  // loop through the output voxels and change the "region id" value
  // stored in the voxel into a "region label" value.
  for (; !iter.IsAtEnd(); iter.NextSpan())
  {
    outPtr = iter.BeginSpan();
    OT* outEnd = iter.EndSpan();

    if (iter.IsInStencil())
    {
      for (; outPtr != outEnd; outPtr++)
      {
        OT v = *outPtr;
        if (v > 0)
        {
          *outPtr = labelMap->GetValue(v - 1);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkICF::SortRegionArrays(svtkImageConnectivityFilter* self)
{
  svtkIdTypeArray* sizes = self->GetExtractedRegionSizes();
  svtkIdTypeArray* ids = self->GetExtractedRegionSeedIds();
  svtkIdTypeArray* labels = self->GetExtractedRegionLabels();
  svtkIntArray* extents = self->GetExtractedRegionExtents();

  svtkIdType* sizePtr = sizes->GetPointer(0);
  svtkIdType* idPtr = ids->GetPointer(0);
  svtkIdType* labelPtr = labels->GetPointer(0);
  int* extentPtr = extents->GetPointer(0);

  svtkIdType n = labels->GetNumberOfTuples();

  // only SizeRank needs re-sorting
  if (self->GetLabelMode() == svtkImageConnectivityFilter::SizeRank)
  {
    std::vector<svtkIdType> sizeVector(sizePtr, sizePtr + n);
    std::vector<svtkIdType> idVector(idPtr, idPtr + n);
    std::vector<int> extentVector(extentPtr, extentPtr + 6 * n);
    for (svtkIdType i = 0; i < n; i++)
    {
      int j = labelPtr[i] - 1;
      labelPtr[i] = static_cast<int>(i + 1);
      sizePtr[j] = sizeVector[i];
      idPtr[j] = idVector[i];
      for (int k = 0; k < 6; k++)
      {
        extentPtr[6 * j + k] = extentVector[6 * i + k];
      }
    }
  }
}

//----------------------------------------------------------------------------
template <class OT>
void svtkICF::Finish(svtkImageConnectivityFilter* self, svtkImageData* outData, OT* outPtr,
  svtkImageStencilData* stencil, int extent[6], svtkDataArray* seedScalars,
  svtkICF::RegionVector& regionInfo)
{
  // Get the execution parameters
  int labelMode = self->GetLabelMode();
  int extractionMode = self->GetExtractionMode();
  svtkIdType sizeRange[2];
  self->GetSizeRange(sizeRange);

  // get only the regions in the requested range of sizes
  svtkICF::PruneBySize(outData, outPtr, stencil, extent, sizeRange, regionInfo);

  // create the three region info arrays
  svtkICF::GenerateRegionArrays(
    self, regionInfo, seedScalars, extent, svtkTypeTraits<OT>::Min(), svtkTypeTraits<OT>::Max());

  svtkIdTypeArray* labelArray = self->GetExtractedRegionLabels();
  if (labelArray->GetNumberOfTuples() > 0)
  {
    // do the extraction and final labeling
    if (extractionMode == svtkImageConnectivityFilter::LargestRegion)
    {
      OT label = static_cast<OT>(labelArray->GetValue(0));
      svtkICF::PruneAllButLargest(outData, outPtr, stencil, extent, label, regionInfo);
    }
    else if (labelMode != svtkImageConnectivityFilter::SeedScalar || seedScalars != nullptr)
    {
      // this is done unless labelMode == SeedScalar and seedScalars == 0
      svtkICF::Relabel(outData, outPtr, stencil, extent, labelArray);
    }

    // sort the three region info arrays (must be done after Relabel)
    svtkICF::SortRegionArrays(self);
  }
}

//----------------------------------------------------------------------------
int* svtkICF::ZeroBaseExtent(const int wholeExtent[6], int extent[6], int maxIdx[3])
{
  // Indexing goes from 0 to maxIdX
  maxIdx[0] = wholeExtent[1] - wholeExtent[0];
  maxIdx[1] = wholeExtent[3] - wholeExtent[2];
  maxIdx[2] = wholeExtent[5] - wholeExtent[4];

  // Get the limits for the output data
  bool useLimits = false;
  for (int k = 0; k < 3; k++)
  {
    extent[2 * k] -= wholeExtent[2 * k];
    useLimits |= (extent[2 * k] != 0);
    extent[2 * k + 1] -= wholeExtent[2 * k];
    useLimits |= (extent[2 * k + 1] != maxIdx[k]);
  }

  // return limits as nullptr if we don't have to use them
  return (useLimits ? extent : nullptr);
}

//----------------------------------------------------------------------------
template <class OT>
void svtkICF::SeededExecute(svtkImageConnectivityFilter* self, svtkImageData* outData,
  svtkDataSet* seedData, svtkImageStencilData* stencil, OT* outPtr, unsigned char* maskPtr,
  int extent[6], svtkICF::RegionVector& regionInfo)
{
  // Get execution parameters
  int extractionMode = self->GetExtractionMode();
  svtkIdType sizeRange[2];
  self->GetSizeRange(sizeRange);

  svtkIdType outInc[3];
  outData->GetIncrements(outInc);

  double spacing[3];
  double origin[3];
  outData->GetOrigin(origin);
  outData->GetSpacing(spacing);

  int outExt[6];
  outData->GetExtent(outExt);

  // Indexing will go from 0 to maxIdX, and the lower limit if "extent" will
  // be subtracted from outExt.  If outExt was the same as extent, then nullptr
  // is returned, else outExt is returned.
  int maxIdx[3];
  int* outLimits = svtkICF::ZeroBaseExtent(extent, outExt, maxIdx);

  // for measuring extent of fill
  int seedExtent[6];
  // only set to non-null if extent generation was requested
  int* fillExtent = (self->GetGenerateRegionExtents() ? seedExtent : nullptr);

  // label consecutively, starting at 1
  OT label = 1;

  std::stack<svtkICF::Seed> seedStack;

  svtkIdType nPoints = seedData->GetNumberOfPoints();
  svtkDataArray* scalars = seedData->GetPointData()->GetScalars();

  for (svtkIdType i = 0; i < nPoints; i++)
  {
    if (scalars && scalars->GetComponent(i, 0) == 0)
    {
      continue;
    }

    double point[3];
    seedData->GetPoint(i, point);
    int idx[3];
    bool outOfBounds = false;

    // convert point from data coords to image index
    for (int j = 0; j < 3; j++)
    {
      idx[j] = svtkMath::Floor((point[j] - origin[j]) / spacing[j] + 0.5);
      idx[j] -= extent[2 * j];
      outOfBounds |= (idx[j] < 0 || idx[j] > maxIdx[j]);
    }

    if (outOfBounds)
    {
      continue;
    }

    // initialize the region extent from the seed position
    seedExtent[0] = seedExtent[1] = idx[0];
    seedExtent[2] = seedExtent[3] = idx[1];
    seedExtent[4] = seedExtent[5] = idx[2];

    seedStack.push(svtkICF::Seed(idx[0], idx[1], idx[2], label));

    // find all voxels that are connected to the seed
    svtkIdType voxelCount =
      svtkICF::Fill(outPtr, outInc, outLimits, maskPtr, maxIdx, fillExtent, seedStack);

    if (voxelCount != 0)
    {
      svtkICF::AddRegion(outData, outPtr, stencil, extent, sizeRange, regionInfo, voxelCount, i,
        seedExtent, extractionMode);
      label = static_cast<OT>(regionInfo.size());
    }
  }
}

//----------------------------------------------------------------------------
template <class OT>
void svtkICF::SeedlessExecute(svtkImageConnectivityFilter* self, svtkImageData* outData,
  svtkImageStencilData* stencil, OT* outPtr, unsigned char* maskPtr, int extent[6],
  svtkICF::RegionVector& regionInfo)
{
  // Get execution parameters
  int extractionMode = self->GetExtractionMode();
  svtkIdType sizeRange[2];
  self->GetSizeRange(sizeRange);

  svtkIdType outInc[3];
  outData->GetIncrements(outInc);

  int outExt[6];
  outData->GetExtent(outExt);

  // Indexing will go from 0 to maxIdX, and the lower limit if "extent" will
  // be subtracted from outExt.  If outExt was the same as extent, then nullptr
  // is returned, else outExt is returned.
  int maxIdx[3];
  int* outLimits = svtkICF::ZeroBaseExtent(extent, outExt, maxIdx);

  // for measuring extent of fill
  int seedExtent[6];
  // only set to non-null if extent generation was requested
  int* fillExtent = (self->GetGenerateRegionExtents() ? seedExtent : nullptr);

  // keep track of position in bitmask
  unsigned char* maskPtr1 = maskPtr;
  unsigned char bit = 1;

  std::stack<svtkICF::Seed> seedStack;

  for (int zIdx = 0; zIdx <= maxIdx[2]; zIdx++)
  {
    for (int yIdx = 0; yIdx <= maxIdx[1]; yIdx++)
    {
      for (int xIdx = 0; xIdx <= maxIdx[0]; xIdx++)
      {
        // check the bitmask to see if voxel is already colored
        unsigned char bitSet = *maskPtr1 & bit;
        bit <<= 1;
        if (bit == 0)
        {
          maskPtr1++;
          bit = 1;
        }

        // if already colored, skip
        if (bitSet != 0)
        {
          continue;
        }

        // initialize the region extent from the seed position
        seedExtent[0] = seedExtent[1] = xIdx;
        seedExtent[2] = seedExtent[3] = yIdx;
        seedExtent[4] = seedExtent[5] = zIdx;

        OT label = static_cast<OT>(regionInfo.size());
        seedStack.push(svtkICF::Seed(xIdx, yIdx, zIdx, label));

        // find all voxels that are connected to the seed
        svtkIdType voxelCount =
          svtkICF::Fill(outPtr, outInc, outLimits, maskPtr, maxIdx, fillExtent, seedStack);

        if (voxelCount != 0)
        {
          if (voxelCount == 1 && static_cast<OT>(regionInfo.size()) == svtkTypeTraits<OT>::Max())
          {
            // smallest region is definitely the one we just added
            svtkIdType outOffset = (xIdx * outInc[0] + yIdx * outInc[1] + zIdx * outInc[2]);
            outPtr[outOffset] = 0;
          }
          else
          {
            svtkICF::AddRegion(outData, outPtr, stencil, extent, sizeRange, regionInfo, voxelCount,
              -1, seedExtent, extractionMode);
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class OT>
void svtkICF::ExecuteOutput(svtkImageConnectivityFilter* self, svtkImageData* outData,
  svtkDataSet* seedData, svtkImageStencilData* stencil, OT* outPtr, unsigned char* maskPtr,
  int extent[6])
{
  // push the "background" onto the region vector
  svtkICF::RegionVector regionInfo;
  regionInfo.push_back(svtkICF::Region(0, 0, extent));

  // execution depends on how regions are seeded
  svtkDataArray* seedScalars = nullptr;
  if (seedData)
  {
    seedScalars = seedData->GetPointData()->GetScalars();
    svtkICF::SeededExecute(self, outData, seedData, stencil, outPtr, maskPtr, extent, regionInfo);
  }

  // if no seeds, or if AllRegions selected, search for all regions
  int extractionMode = self->GetExtractionMode();
  if (!seedData || extractionMode == svtkImageConnectivityFilter::AllRegions)
  {
    svtkICF::SeedlessExecute(self, outData, stencil, outPtr, maskPtr, extent, regionInfo);
  }

  // do final relabelling and other bookkeeping
  svtkICF::Finish(self, outData, outPtr, stencil, extent, seedScalars, regionInfo);
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int svtkImageConnectivityFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->LabelScalarType, 1);

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageConnectivityFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* stencilInfo = inputVector[1]->GetInformationObject(0);

  int extent[6];
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  if (stencilInfo)
  {
    stencilInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageConnectivityFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* stencilInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* seedInfo = inputVector[2]->GetInformationObject(0);

  svtkImageData* outData = static_cast<svtkImageData*>(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* inData = static_cast<svtkImageData*>(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataSet* seedData = nullptr;
  if (seedInfo)
  {
    seedData = static_cast<svtkDataSet*>(seedInfo->Get(svtkDataObject::DATA_OBJECT()));
  }

  svtkImageStencilData* stencil = nullptr;
  if (stencilInfo)
  {
    stencil = static_cast<svtkImageStencilData*>(stencilInfo->Get(svtkDataObject::DATA_OBJECT()));
  }

  int outExt[6];
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);
  this->AllocateOutputData(outData, outInfo, outExt);

  outData->GetPointData()->GetScalars()->SetName("RegionId");
  void* outPtr = outData->GetScalarPointerForExtent(outExt);

  // clear the output
  size_t size = (outExt[1] - outExt[0] + 1);
  size *= (outExt[3] - outExt[2] + 1);
  size *= (outExt[5] - outExt[4] + 1);
  size_t byteSize = size * outData->GetScalarSize();
  memset(outPtr, 0, byteSize);

  // we need all the voxels that might be connected to the seed
  int extent[6];
  inData->GetExtent(extent);

  // voxels outside the stencil extent can be excluded
  if (stencil)
  {
    int stencilExtent[6];
    stencil->GetExtent(stencilExtent);
    if (!svtkICF::IntersectExtents(extent, stencilExtent, extent))
    {
      // if stencil doesn't overlap the input, return
      return 1;
    }
  }

  int outScalarType = outData->GetScalarType();
  if (outScalarType != SVTK_UNSIGNED_CHAR && outScalarType != SVTK_SHORT &&
    outScalarType != SVTK_UNSIGNED_SHORT && outScalarType != SVTK_INT)
  {
    svtkErrorMacro("Execute: Output ScalarType is " << outData->GetScalarType()
                                                   << ", but it must be one of SVTK_UNSIGNED_CHAR, "
                                                      "SVTK_SHORT, SVTK_UNSIGNED_SHORT, or SVTK_INT");
    return 0;
  }

  // create and clear the image bitmask (each bit is a voxel)
  size = (extent[1] - extent[0] + 1);
  size *= (extent[3] - extent[2] + 1);
  size *= (extent[5] - extent[4] + 1);
  byteSize = (size + 7) / 8;
  unsigned char* mask = new unsigned char[byteSize];

  // get scalar pointers
  void* inPtr = inData->GetScalarPointerForExtent(extent);

  int rval = 1;

  switch (inData->GetScalarType())
  {
    svtkTemplateAliasMacro(
      svtkICF::ExecuteInput(this, inData, static_cast<SVTK_TT*>(inPtr), mask, stencil, extent));

    default:
      svtkErrorMacro(<< "Execute: Unknown input ScalarType");
      rval = 0;
  }

  switch (outData->GetScalarType())
  {
    case SVTK_UNSIGNED_CHAR:
      svtkICF::ExecuteOutput(
        this, outData, seedData, stencil, static_cast<unsigned char*>(outPtr), mask, extent);
      break;

    case SVTK_SHORT:
      svtkICF::ExecuteOutput(
        this, outData, seedData, stencil, static_cast<short*>(outPtr), mask, extent);
      break;

    case SVTK_UNSIGNED_SHORT:
      svtkICF::ExecuteOutput(
        this, outData, seedData, stencil, static_cast<unsigned short*>(outPtr), mask, extent);
      break;

    case SVTK_INT:
      svtkICF::ExecuteOutput(
        this, outData, seedData, stencil, static_cast<int*>(outPtr), mask, extent);
      break;
  }

  delete[] mask;

  return rval;
}

//----------------------------------------------------------------------------
void svtkImageConnectivityFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LabelScalarType: " << this->GetLabelScalarTypeAsString() << "\n";

  os << indent << "LabelMode: " << this->GetLabelModeAsString() << "\n";

  os << indent << "ExtractionMode: " << this->GetExtractionModeAsString() << "\n";

  os << indent << "LabelConstantValue: " << this->LabelConstantValue << "\n";

  os << indent << "NumberOfExtractedRegions: " << this->GetNumberOfExtractedRegions() << "\n";

  os << indent << "ExtractedRegionLabels: " << this->ExtractedRegionLabels << "\n";

  os << indent << "ExtractedRegionSizes: " << this->ExtractedRegionSizes << "\n";

  os << indent << "ExtractedRegionSeedIds: " << this->ExtractedRegionSeedIds << "\n";

  os << indent << "ExtractedRegionExtents: " << this->ExtractedRegionExtents << "\n";

  os << indent << "ScalarRange: " << this->ScalarRange[0] << " " << this->ScalarRange[1] << "\n";

  os << indent << "SizeRange: " << this->SizeRange[0] << " " << this->SizeRange[1] << "\n";

  os << indent << "ActiveComponent: " << this->ActiveComponent << "\n";

  os << indent << "GenerateRegionExtents: " << (this->GenerateRegionExtents ? "On\n" : "Off\n");

  os << indent << "SeedConnection: " << this->GetSeedConnection() << "\n";

  os << indent << "StencilConnection: " << this->GetStencilConnection() << "\n";
}
