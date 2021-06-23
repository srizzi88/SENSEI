//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_encode1_h
#define svtk_m_worklet_zfp_encode1_h

#include <svtkm/Types.h>
#include <svtkm/internal/ExportMacros.h>

#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/zfp/ZFPBlockWriter.h>
#include <svtkm/worklet/zfp/ZFPEncode.h>
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
SVTKM_EXEC inline void GatherPartial1(Scalar* q,
                                     const PortalType& scalars,
                                     svtkm::Id offset,
                                     int nx,
                                     int sx)
{
  svtkm::Id x;
  for (x = 0; x < nx; x++, offset += sx)
    q[x] = scalars.Get(offset);
  PadBlock(q, svtkm::UInt32(nx), 1);
}

template <typename Scalar, typename PortalType>
SVTKM_EXEC inline void Gather1(Scalar* fblock, const PortalType& scalars, svtkm::Id offset, int sx)
{
  svtkm::Id counter = 0;

  for (svtkm::Id x = 0; x < 4; x++, offset += sx)
  {
    fblock[counter] = scalars.Get(offset);
    counter++;
  }
}

struct Encode1 : public svtkm::worklet::WorkletMapField
{
protected:
  svtkm::Id Dims;        // field dims
  svtkm::Id PaddedDims;  // dims padded to a multiple of zfp block size
  svtkm::Id ZFPDims;     // zfp block dims
  svtkm::UInt32 MaxBits; // bits per zfp block

public:
  Encode1(const svtkm::Id dims, const svtkm::Id paddedDims, const svtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims = PaddedDims / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, AtomicArrayInOut bitstream);

  template <class InputScalarPortal, typename BitstreamPortal>
  SVTKM_EXEC void operator()(const svtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;

    svtkm::Id zfpBlock;
    zfpBlock = blockIdx % ZFPDims;
    svtkm::Id logicalStart = zfpBlock * svtkm::Id(4);

    constexpr svtkm::Int32 BlockSize = 4;
    Scalar fblock[BlockSize];

    bool partial = false;
    if (logicalStart + 4 > Dims)
      partial = true;

    if (partial)
    {
      const svtkm::Int32 nx =
        logicalStart + 4 > Dims ? svtkm::Int32(Dims - logicalStart) : svtkm::Int32(4);
      GatherPartial1(fblock, scalars, logicalStart, nx, 1);
    }
    else
    {
      Gather1(fblock, scalars, logicalStart, 1);
    }

    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;
    encoder.encode(fblock, static_cast<svtkm::Int32>(MaxBits), svtkm::UInt32(blockIdx), stream);
  }
};
}
}
}
#endif
