/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMRCReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkMRCReader.h"
#include "svtkObjectFactory.h"

#include "svtkByteSwap.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSetAttributes.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTypeInt16Array.h"
#include "svtkTypeInt8Array.h"
#include "svtkTypeUInt16Array.h"
#include "svtksys/FStream.hxx"

#include <cassert>

//#define SVTK_DEBUG_MRC_HEADER

namespace
{
//
// This struct is written based on the description found here:
// http://bio3d.colorado.edu/imod/doc/mrc_format.txt
//
struct mrc_file_header
{
  svtkTypeInt32 nx;
  svtkTypeInt32 ny;
  svtkTypeInt32 nz;
  svtkTypeInt32 mode;
  svtkTypeInt32 nxstart;
  svtkTypeInt32 nystart;
  svtkTypeInt32 nzstart;
  svtkTypeInt32 mx;
  svtkTypeInt32 my;
  svtkTypeInt32 mz;
  svtkTypeFloat32 xlen;
  svtkTypeFloat32 ylen;
  svtkTypeFloat32 zlen;
  svtkTypeFloat32 alpha;
  svtkTypeFloat32 beta;
  svtkTypeFloat32 gamma;
  svtkTypeInt32 mapc;
  svtkTypeInt32 mapr;
  svtkTypeInt32 maps;
  svtkTypeFloat32 amin;
  svtkTypeFloat32 amax;
  svtkTypeFloat32 amean;
  svtkTypeInt32 ispg;
  svtkTypeInt32 next;
  svtkTypeInt16 creatid;
  svtkTypeInt16 extra1[15];
  svtkTypeInt16 nint;
  svtkTypeInt16 nreal;
  svtkTypeInt32 extra2[5];
  svtkTypeInt32 imodStamp;
  svtkTypeInt32 imodFlags;
  svtkTypeInt16 idtype;
  svtkTypeInt16 lens;
  svtkTypeInt16 nd1;
  svtkTypeInt16 nd2;
  svtkTypeInt16 vd1;
  svtkTypeInt16 vd2;
  svtkTypeFloat32 tiltangles[6];
  svtkTypeFloat32 xorg;
  svtkTypeFloat32 yorg;
  svtkTypeFloat32 zorg;
  char cmap[4];
  char stamp[4];
  svtkTypeFloat32 rms;
  svtkTypeInt32 nlabl;
  char labl[10][80];
};

#ifdef SVTK_DEBUG_MRC_HEADER
std::ostream& operator<<(std::ostream& os, const mrc_file_header& hdr)
{
  os << "extents:" << hdr.nx << " " << hdr.ny << " " << hdr.nz << std::endl;
  os << "mode: " << hdr.mode << std::endl;
  os << "start: " << hdr.nxstart << " " << hdr.nystart << " " << hdr.nzstart << std::endl;
  os << "intervals: " << hdr.mx << " " << hdr.my << " " << hdr.mz << std::endl;
  os << "len: " << hdr.xlen << " " << hdr.ylen << " " << hdr.zlen << std::endl;
  os << "abg: " << hdr.alpha << " " << hdr.beta << " " << hdr.gamma << std::endl;
  os << "map: " << hdr.mapc << " " << hdr.mapr << " " << hdr.maps << std::endl;
  os << "min: " << hdr.amin << " max: " << hdr.amax << " mean: " << hdr.amean << std::endl;
  os << "ispg: " << hdr.ispg << " next: " << hdr.next << std::endl;
  // skipping extra for now
  os << "nint: " << hdr.nint << " nreal: " << hdr.nreal << std::endl;
  os << "imodStamp: " << hdr.imodStamp << " imodFlags: " << hdr.imodFlags << std::endl;
  os << "idtype: " << hdr.idtype << " lens: " << hdr.lens << std::endl;
  os << "nd1: " << hdr.nd1 << " nd2: " << hdr.nd2 << std::endl;
  os << "vd1: " << hdr.vd1 << " vd2: " << hdr.vd2 << std::endl;
  os << "tilt angles: " << hdr.tiltangles[0] << " " << hdr.tiltangles[1] << " " << hdr.tiltangles[2]
     << " " << hdr.tiltangles[3] << " " << hdr.tiltangles[4] << " " << hdr.tiltangles[5]
     << std::endl;
  os << "org: " << hdr.xorg << " " << hdr.yorg << " " << hdr.zorg << std::endl;
  os << "cmap: '" << hdr.cmap[0] << hdr.cmap[1] << hdr.cmap[2] << hdr.cmap[3] << "'";
  os << " stamp: '" << hdr.stamp[0] << hdr.stamp[1] << hdr.stamp[2] << hdr.stamp[3] << "'"
     << std::endl;
  os << "rms: " << hdr.rms << " nlabl: " << hdr.nlabl << std::endl;
  for (int i = 0; i < hdr.nlabl; ++i)
  {
    os.write(hdr.labl[i], 80);
    os << std::endl;
  }
}
#endif
}

class svtkMRCReader::svtkInternal
{
public:
  svtksys::ifstream* stream;
  mrc_file_header header;

  svtkInternal()
    : stream(nullptr)
    , header()
  {
  }

  ~svtkInternal() { delete stream; }

  void openFile(const char* file)
  {
    delete stream;
    stream = new svtksys::ifstream(file, std::ios::binary);
  }
};

svtkStandardNewMacro(svtkMRCReader);

svtkMRCReader::svtkMRCReader()
{
  this->FileName = nullptr;
  this->Internals = new svtkInternal;
  this->SetNumberOfInputPorts(0);
}

svtkMRCReader::~svtkMRCReader()
{
  this->SetFileName(nullptr);
  delete this->Internals;
}

void svtkMRCReader::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "nullptr") << ", "
     << std::endl;
}

namespace
{
int getFileDataType(int mode)
{
  switch (mode)
  {
    case 0:
    case 16:
      return SVTK_TYPE_UINT8;
    case 2:
    case 4:
      return SVTK_FLOAT;
    case 1:
    case 3:
      return SVTK_TYPE_INT16;
    case 6:
      return SVTK_TYPE_UINT16;
    default:
      return -1;
  }
}

int getFileDataNumComponents(int mode)
{
  switch (mode)
  {
    case 0:
    case 1:
    case 2:
      return 1;
    case 3:
    case 4:
    case 6:
      return 2;
    case 16:
      return 3;
    default:
      return -1;
  }
}
}

int svtkMRCReader::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // If this fails then the packing is wrong and the file's header will not
  // be read in correctly
  assert(sizeof(mrc_file_header) == 1024);
  if (this->FileName)
  {
    this->Internals->openFile(this->FileName);
    if (!this->Internals->stream)
    {
      svtkErrorMacro("Error opening input file");
      return 0;
    }
    this->Internals->stream->read((char*)&this->Internals->header, sizeof(mrc_file_header));
    if (this->Internals->header.stamp[0] != ((char)17))
    // This is what the big-endian MRC files are supposed to look like.  I don't have one to
    // test with though.  However, if it does not look like that, assume it is little endian.
    // There are some non-conformant programs that don't correctly fill in this field, and
    // assuming little endian is safer.
    {
      svtkByteSwap::Swap4LERange(&this->Internals->header, 24);
      svtkByteSwap::Swap2LERange(&this->Internals->header.creatid, 1);
      svtkByteSwap::Swap2LERange(&this->Internals->header.nint, 2);
      svtkByteSwap::Swap4LERange(&this->Internals->header.imodStamp, 2);
      svtkByteSwap::Swap2LERange(&this->Internals->header.idtype, 6);
      svtkByteSwap::Swap4LERange(&this->Internals->header.tiltangles, 9);
      svtkByteSwap::Swap4LERange(&this->Internals->header.rms, 2);
    }
    else
    {
      svtkByteSwap::Swap4BERange(&this->Internals->header, 24);
      svtkByteSwap::Swap2BERange(&this->Internals->header.creatid, 1);
      svtkByteSwap::Swap2BERange(&this->Internals->header.nint, 2);
      svtkByteSwap::Swap4BERange(&this->Internals->header.imodStamp, 2);
      svtkByteSwap::Swap2BERange(&this->Internals->header.idtype, 6);
      svtkByteSwap::Swap4BERange(&this->Internals->header.tiltangles, 9);
      svtkByteSwap::Swap4BERange(&this->Internals->header.rms, 2);
    }
#ifdef SVTK_DEBUG_MRC_HEADER
    std::cout << this->Internals->header;
#endif
    int extent[6];
    extent[0] = this->Internals->header.nxstart;
    extent[1] = extent[0] + this->Internals->header.nx - 1;
    extent[2] = this->Internals->header.nystart;
    extent[3] = extent[2] + this->Internals->header.ny - 1;
    extent[4] = this->Internals->header.nzstart;
    extent[5] = extent[4] + this->Internals->header.nz - 1;
    double dataSpacing[3];
    dataSpacing[0] = this->Internals->header.xlen / this->Internals->header.mx;
    dataSpacing[1] = this->Internals->header.ylen / this->Internals->header.my;
    dataSpacing[2] = this->Internals->header.zlen / this->Internals->header.mz;
    double dataOrigin[3];
    dataOrigin[0] = this->Internals->header.xorg;
    dataOrigin[1] = this->Internals->header.yorg;
    dataOrigin[2] = this->Internals->header.zorg;
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
    outInfo->Set(svtkDataObject::SPACING(), dataSpacing, 3);
    outInfo->Set(svtkDataObject::ORIGIN(), dataOrigin, 3);

    svtkDataObject::SetPointDataActiveScalarInfo(outInfo,
      getFileDataType(this->Internals->header.mode),
      getFileDataNumComponents(this->Internals->header.mode));

    outInfo->Set(svtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);
    return 1;
  }
  else
  {
    svtkErrorMacro("No input file set");
    return 0;
  }
}

namespace
{

typedef void (*ByteSwapFunction)(void*, size_t);

ByteSwapFunction getByteSwapFunction(int svtkType, bool isLittleEndian)
{
  int size = 0;
  switch (svtkType)
  {
    svtkTemplateMacro(size = sizeof(SVTK_TT));
  }
  if (size == 2)
  {
    return isLittleEndian ? &svtkByteSwap::Swap2LERange : &svtkByteSwap::Swap2BERange;
  }
  else if (size == 4)
  {
    return isLittleEndian ? &svtkByteSwap::Swap4LERange : &svtkByteSwap::Swap4BERange;
  }
  else if (size == 8)
  {
    return isLittleEndian ? &svtkByteSwap::Swap8LERange : &svtkByteSwap::Swap8BERange;
  }
  return nullptr;
}

template <typename T>
void readData(int numComponents, int* outExt, svtkIdType* outInc, svtkIdType* inOffsets,
  T* const outPtr, svtksys::ifstream& stream, svtkIdType dataStartPos,
  ByteSwapFunction byteSwapFunction)
{
  svtkIdType lineSize = (outExt[1] - outExt[0] + 1) * numComponents;
  T* ptr = outPtr;
  for (svtkIdType z = outExt[4]; z <= outExt[5]; ++z)
  {
    for (svtkIdType y = outExt[2]; y <= outExt[3]; ++y)
    {
      assert(z >= 0 && y >= 0 && outExt[0] >= 0);
      svtkIdType offset = z * inOffsets[2] + y * inOffsets[1] + outExt[0] * inOffsets[0];
      offset = dataStartPos + offset * sizeof(T);

      stream.seekg(offset, svtksys::ifstream::beg);
      // read the line
      stream.read((char*)ptr, lineSize * sizeof(T));
      if (byteSwapFunction)
      {
        byteSwapFunction((void*)outPtr, lineSize);
      }
      // update the data pointer
      ptr += (lineSize + outInc[1]);
    }
    ptr += outInc[2];
  }
}

}

void svtkMRCReader::ExecuteDataWithInformation(
  svtkDataObject* svtkNotUsed(output), svtkInformation* outInfo)
{
  svtkIdType outInc[3];
  svtkIdType inOffsets[3];
  int* outExt;
  int modifiedOutExt[6];
  int* execExt = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  svtkImageData* data = svtkImageData::GetData(outInfo);
  this->AllocateOutputData(data, outInfo, execExt);

  if (data->GetNumberOfPoints() <= 0)
  {
    return;
  }
  outExt = data->GetExtent();
  // this should result in the bottom corner of the image having extent
  // 0,0,0 which makes the 'where in the file is this extent' math easier
  modifiedOutExt[0] = outExt[0] - this->Internals->header.nxstart;
  modifiedOutExt[1] = outExt[1] - this->Internals->header.nxstart;
  modifiedOutExt[2] = outExt[2] - this->Internals->header.nystart;
  modifiedOutExt[3] = outExt[3] - this->Internals->header.nystart;
  modifiedOutExt[4] = outExt[4] - this->Internals->header.nzstart;
  modifiedOutExt[5] = outExt[5] - this->Internals->header.nzstart;
  data->GetContinuousIncrements(outExt, outInc[0], outInc[1], outInc[2]);
  void* outPtr = data->GetScalarPointer(outExt[0], outExt[2], outExt[4]);

  if (!this->Internals->stream)
  {
    return;
  }
  // data start position is 1024 (the header size) plus the extended header size
  svtkIdType dataStartPos = 1024 + this->Internals->header.next;
  this->Internals->stream->seekg(dataStartPos, svtksys::ifstream::beg);

  int svtkType = getFileDataType(this->Internals->header.mode);
  int numComponents = getFileDataNumComponents(this->Internals->header.mode);
  inOffsets[0] = numComponents;
  inOffsets[1] = this->Internals->header.nx * numComponents;
  inOffsets[2] = this->Internals->header.ny * this->Internals->header.nx * numComponents;

  // This is what the big-endian MRC files are supposed to look like.  I don't have one to
  // test with though.  However, if it does not look like that, assume it is little endian.
  // There are some non-conformant programs that don't correctly fill in this field, and
  // assuming little endian is safer.
  bool fileIsLittleEndian = (this->Internals->header.stamp[0] != ((char)17));

  ByteSwapFunction byteSwapFunction = getByteSwapFunction(svtkType, fileIsLittleEndian);
  switch (svtkType)
  {
    svtkTemplateMacro(readData<SVTK_TT>(numComponents, modifiedOutExt, outInc, inOffsets,
      static_cast<SVTK_TT*>(outPtr), *this->Internals->stream, dataStartPos, byteSwapFunction));
    default:
      svtkErrorMacro("Unknown data type");
  }
}
