//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_structs_h
#define svtk_m_worklet_zfp_structs_h

#define ZFP_MIN_BITS 0    /* minimum number of bits per block */
#define ZFP_MAX_BITS 4171 /* maximum number of bits per block */
#define ZFP_MAX_PREC 64   /* maximum precision supported */
#define ZFP_MIN_EXP -1074 /* minimum floating-point base-2 exponent */

#include <svtkm/worklet/zfp/ZFPFunctions.h>
#include <svtkm/worklet/zfp/ZFPTypeInfo.h>

namespace svtkm
{
namespace worklet
{
namespace zfp
{

struct ZFPStream
{
  svtkm::UInt32 minbits;
  svtkm::UInt32 maxbits;
  svtkm::UInt32 maxprec;
  svtkm::Int32 minexp;

  template <typename T>
  svtkm::Float64 SetRate(const svtkm::Float64 rate, const svtkm::Int32 dims, T svtkmNotUsed(valueType))
  {
    svtkm::UInt32 n = 1u << (2 * dims);
    svtkm::UInt32 bits = (unsigned int)floor(n * rate + 0.5);
    bits = zfp::MinBits<T>(bits);
    //if (wra) {
    //  /* for write random access, round up to next multiple of stream word size */
    //  bits += (uint)stream_word_bits - 1;
    //  bits &= ~(stream_word_bits - 1);
    //}
    minbits = bits;
    maxbits = bits;
    maxprec = ZFP_MAX_PREC;
    minexp = ZFP_MIN_EXP;
    return (double)bits / n;
  }
};
}
}
} // namespace svtkm::worklet::zfp
#endif
