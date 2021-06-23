//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_decode3_h
#define svtk_m_worklet_zfp_decode3_h

#include <svtkm/Types.h>
#include <svtkm/internal/ExportMacros.h>

#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/zfp/ZFPBlockWriter.h>
#include <svtkm/worklet/zfp/ZFPDecode.h>
#include <svtkm/worklet/zfp/ZFPFunctions.h>
#include <svtkm/worklet/zfp/ZFPStructs.h>
#include <svtkm/worklet/zfp/ZFPTypeInfo.h>

namespace svtkm
{
namespace worklet
{
namespace zfp
{

template <typename Scalar, typename PortalType>
SVTKM_EXEC inline void ScatterPartial3(const Scalar* q,
                                      PortalType& scalars,
                                      const svtkm::Id3 dims,
                                      svtkm::Id offset,
                                      svtkm::Int32 nx,
                                      svtkm::Int32 ny,
                                      svtkm::Int32 nz)
{
  svtkm::Id x, y, z;
  for (z = 0; z < nz; z++, offset += dims[0] * dims[1] - ny * dims[0], q += 4 * (4 - ny))
  {
    for (y = 0; y < ny; y++, offset += dims[0] - nx, q += 4 - nx)
    {
      for (x = 0; x < nx; x++, offset++, q++)
      {
        scalars.Set(offset, *q);
      }
    }
  }
}

template <typename Scalar, typename PortalType>
SVTKM_EXEC inline void Scatter3(const Scalar* q,
                               PortalType& scalars,
                               const svtkm::Id3 dims,
                               svtkm::Id offset)
{
  for (svtkm::Id z = 0; z < 4; z++, offset += dims[0] * dims[1] - 4 * dims[0])
  {
    for (svtkm::Id y = 0; y < 4; y++, offset += dims[0] - 4)
    {
      for (svtkm::Id x = 0; x < 4; x++, ++offset)
      {
        scalars.Set(offset, *q++);
      } // x
    }   // y
  }     // z
}

struct Decode3 : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Id3 Dims;       // field dims
  svtkm::Id3 PaddedDims; // dims padded to a multiple of zfp block size
  svtkm::Id3 ZFPDims;    // zfp block dims
  svtkm::UInt32 MaxBits; // bits per zfp block
public:
  Decode3(const svtkm::Id3 dims, const svtkm::Id3 paddedDims, const svtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims[0] = PaddedDims[0] / 4;
    ZFPDims[1] = PaddedDims[1] / 4;
    ZFPDims[2] = PaddedDims[2] / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayIn bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  SVTKM_EXEC void operator()(const svtkm::Id blockIdx,
                            InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr svtkm::Int32 BlockSize = 64;
    Scalar fblock[BlockSize];
    // clear
    for (svtkm::Int32 i = 0; i < BlockSize; ++i)
    {
      fblock[i] = static_cast<Scalar>(0);
    }


    zfp::zfp_decode<BlockSize>(
      fblock, svtkm::Int32(MaxBits), static_cast<svtkm::UInt32>(blockIdx), stream);

    svtkm::Id3 zfpBlock;
    zfpBlock[0] = blockIdx % ZFPDims[0];
    zfpBlock[1] = (blockIdx / ZFPDims[0]) % ZFPDims[1];
    zfpBlock[2] = blockIdx / (ZFPDims[0] * ZFPDims[1]);
    svtkm::Id3 logicalStart = zfpBlock * svtkm::Id(4);


    svtkm::Id offset = (logicalStart[2] * Dims[1] + logicalStart[1]) * Dims[0] + logicalStart[0];
    bool partial = false;
    if (logicalStart[0] + 4 > Dims[0])
      partial = true;
    if (logicalStart[1] + 4 > Dims[1])
      partial = true;
    if (logicalStart[2] + 4 > Dims[2])
      partial = true;
    if (partial)
    {
      const svtkm::Int32 nx =
        logicalStart[0] + 4 > Dims[0] ? svtkm::Int32(Dims[0] - logicalStart[0]) : svtkm::Int32(4);
      const svtkm::Int32 ny =
        logicalStart[1] + 4 > Dims[1] ? svtkm::Int32(Dims[1] - logicalStart[1]) : svtkm::Int32(4);
      const svtkm::Int32 nz =
        logicalStart[2] + 4 > Dims[2] ? svtkm::Int32(Dims[2] - logicalStart[2]) : svtkm::Int32(4);
      ScatterPartial3(fblock, scalars, Dims, offset, nx, ny, nz);
    }
    else
    {
      Scatter3(fblock, scalars, Dims, offset);
    }
  }
};
}
}
} // namespace svtkm::worklet::zfp
#endif
