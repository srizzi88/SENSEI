//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_zfp_encode_h
#define svtk_m_worklet_zfp_encode_h

#include <svtkm/Types.h>
#include <svtkm/internal/ExportMacros.h>
#include <svtkm/worklet/zfp/ZFPBlockWriter.h>
#include <svtkm/worklet/zfp/ZFPCodec.h>
#include <svtkm/worklet/zfp/ZFPTypeInfo.h>

namespace svtkm
{
namespace worklet
{
namespace zfp
{

template <typename Scalar>
SVTKM_EXEC void PadBlock(Scalar* p, svtkm::UInt32 n, svtkm::UInt32 s)
{
  switch (n)
  {
    case 0:
      p[0 * s] = 0;
    /* FALLTHROUGH */
    case 1:
      p[1 * s] = p[0 * s];
    /* FALLTHROUGH */
    case 2:
      p[2 * s] = p[1 * s];
    /* FALLTHROUGH */
    case 3:
      p[3 * s] = p[0 * s];
    /* FALLTHROUGH */
    default:
      break;
  }
}

template <svtkm::Int32 N, typename FloatType>
inline SVTKM_EXEC svtkm::Int32 MaxExponent(const FloatType* vals)
{
  FloatType maxVal = 0;
  for (svtkm::Int32 i = 0; i < N; ++i)
  {
    maxVal = svtkm::Max(maxVal, svtkm::Abs(vals[i]));
  }

  if (maxVal > 0)
  {
    svtkm::Int32 exponent;
    svtkm::Frexp(maxVal, &exponent);
    /* clamp exponent in case x is denormal */
    return svtkm::Max(exponent, 1 - get_ebias<FloatType>());
  }
  return -get_ebias<FloatType>();
}

// maximum number of bit planes to encode
inline SVTKM_EXEC svtkm::Int32 precision(svtkm::Int32 maxexp, svtkm::Int32 maxprec, svtkm::Int32 minexp)
{
  return svtkm::Min(maxprec, svtkm::Max(0, maxexp - minexp + 8));
}

template <typename Scalar>
inline SVTKM_EXEC Scalar quantize(Scalar x, svtkm::Int32 e)
{
  return svtkm::Ldexp(x, (CHAR_BIT * (svtkm::Int32)sizeof(Scalar) - 2) - e);
}

template <typename Int, typename Scalar, svtkm::Int32 BlockSize>
inline SVTKM_EXEC void fwd_cast(Int* iblock, const Scalar* fblock, svtkm::Int32 emax)
{
  Scalar s = quantize<Scalar>(1, emax);
  for (svtkm::Int32 i = 0; i < BlockSize; ++i)
  {
    iblock[i] = static_cast<Int>(s * fblock[i]);
  }
}

template <typename Int, svtkm::Int32 S>
inline SVTKM_EXEC void fwd_lift(Int* p)
{
  Int x, y, z, w;
  x = *p;
  p += S;
  y = *p;
  p += S;
  z = *p;
  p += S;
  w = *p;
  p += S;

  /*
  ** non-orthogonal transform
  **        ( 4  4  4  4) (x)
  ** 1/16 * ( 5  1 -1 -5) (y)
  **        (-4  4  4 -4) (z)
  **        (-2  6 -6  2) (w)
  */
  x += w;
  x >>= 1;
  w -= x;
  z += y;
  z >>= 1;
  y -= z;
  x += z;
  x >>= 1;
  z -= x;
  w += y;
  w >>= 1;
  y -= w;
  w += y >> 1;
  y -= w >> 1;

  p -= S;
  *p = w;
  p -= S;
  *p = z;
  p -= S;
  *p = y;
  p -= S;
  *p = x;
}

template <typename Int, typename UInt>
inline SVTKM_EXEC UInt int2uint(const Int x);

template <>
inline SVTKM_EXEC svtkm::UInt64 int2uint<svtkm::Int64, svtkm::UInt64>(const svtkm::Int64 x)
{
  return (static_cast<svtkm::UInt64>(x) + (svtkm::UInt64)0xaaaaaaaaaaaaaaaaull) ^
    (svtkm::UInt64)0xaaaaaaaaaaaaaaaaull;
}

template <>
inline SVTKM_EXEC svtkm::UInt32 int2uint<svtkm::Int32, svtkm::UInt32>(const svtkm::Int32 x)
{
  return (static_cast<svtkm::UInt32>(x) + (svtkm::UInt32)0xaaaaaaaau) ^ (svtkm::UInt32)0xaaaaaaaau;
}



template <typename UInt, typename Int, svtkm::Int32 BlockSize>
inline SVTKM_EXEC void fwd_order(UInt* ublock, const Int* iblock)
{
  const zfp::ZFPCodec<BlockSize> codec;
  for (svtkm::Int32 i = 0; i < BlockSize; ++i)
  {
    svtkm::UInt8 idx = codec.CodecLookup(i);
    ublock[i] = int2uint<Int, UInt>(iblock[idx]);
  }
}

template <typename Int, svtkm::Int32 BlockSize>
inline SVTKM_EXEC void fwd_xform(Int* p);

template <>
inline SVTKM_EXEC void fwd_xform<svtkm::Int64, 64>(svtkm::Int64* p)
{
  svtkm::UInt32 x, y, z;
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      fwd_lift<svtkm::Int64, 1>(p + 4 * y + 16 * z);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      fwd_lift<svtkm::Int64, 4>(p + 16 * z + 1 * x);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      fwd_lift<svtkm::Int64, 16>(p + 1 * x + 4 * y);
}

template <>
inline SVTKM_EXEC void fwd_xform<svtkm::Int32, 64>(svtkm::Int32* p)
{
  svtkm::UInt32 x, y, z;
  /* transform along x */
  for (z = 0; z < 4; z++)
    for (y = 0; y < 4; y++)
      fwd_lift<svtkm::Int32, 1>(p + 4 * y + 16 * z);
  /* transform along y */
  for (x = 0; x < 4; x++)
    for (z = 0; z < 4; z++)
      fwd_lift<svtkm::Int32, 4>(p + 16 * z + 1 * x);
  /* transform along z */
  for (y = 0; y < 4; y++)
    for (x = 0; x < 4; x++)
      fwd_lift<svtkm::Int32, 16>(p + 1 * x + 4 * y);
}

template <>
inline SVTKM_EXEC void fwd_xform<svtkm::Int64, 16>(svtkm::Int64* p)
{
  svtkm::UInt32 x, y;
  /* transform along x */
  for (y = 0; y < 4; y++)
    fwd_lift<svtkm::Int64, 1>(p + 4 * y);
  /* transform along y */
  for (x = 0; x < 4; x++)
    fwd_lift<svtkm::Int64, 4>(p + 1 * x);
}

template <>
inline SVTKM_EXEC void fwd_xform<svtkm::Int32, 16>(svtkm::Int32* p)
{
  svtkm::UInt32 x, y;
  /* transform along x */
  for (y = 0; y < 4; y++)
    fwd_lift<svtkm::Int32, 1>(p + 4 * y);
  /* transform along y */
  for (x = 0; x < 4; x++)
    fwd_lift<svtkm::Int32, 4>(p + 1 * x);
}

template <>
inline SVTKM_EXEC void fwd_xform<svtkm::Int64, 4>(svtkm::Int64* p)
{
  /* transform along x */
  fwd_lift<svtkm::Int64, 1>(p);
}

template <>
inline SVTKM_EXEC void fwd_xform<svtkm::Int32, 4>(svtkm::Int32* p)
{
  /* transform along x */
  fwd_lift<svtkm::Int32, 1>(p);
}

template <svtkm::Int32 BlockSize, typename PortalType, typename Int>
SVTKM_EXEC void encode_block(BlockWriter<BlockSize, PortalType>& stream,
                            svtkm::Int32 maxbits,
                            svtkm::Int32 maxprec,
                            Int* iblock)
{
  using UInt = typename zfp_traits<Int>::UInt;

  fwd_xform<Int, BlockSize>(iblock);

  UInt ublock[BlockSize];
  fwd_order<UInt, Int, BlockSize>(ublock, iblock);

  svtkm::UInt32 intprec = CHAR_BIT * (svtkm::UInt32)sizeof(UInt);
  svtkm::UInt32 kmin =
    intprec > (svtkm::UInt32)maxprec ? intprec - static_cast<svtkm::UInt32>(maxprec) : 0;
  svtkm::UInt32 bits = static_cast<svtkm::UInt32>(maxbits);
  svtkm::UInt32 i, m;
  svtkm::UInt32 n = 0;
  svtkm::UInt64 x;
  /* encode one bit plane at a time from MSB to LSB */
  for (svtkm::UInt32 k = intprec; bits && k-- > kmin;)
  {
    /* step 1: extract bit plane #k to x */
    x = 0;
    for (i = 0; i < BlockSize; i++)
    {
      x += (svtkm::UInt64)((ublock[i] >> k) & 1u) << i;
    }
    /* step 2: encode first n bits of bit plane */
    m = svtkm::Min(n, bits);
    bits -= m;
    x = stream.write_bits(x, m);
    /* step 3: unary run-length encode remainder of bit plane */
    for (; n < BlockSize && bits && (bits--, stream.write_bit(!!x)); x >>= 1, n++)
    {
      for (; n < BlockSize - 1 && bits && (bits--, !stream.write_bit(x & 1u)); x >>= 1, n++)
      {
      }
    }
  }
}


template <svtkm::Int32 BlockSize, typename Scalar, typename PortalType>
inline SVTKM_EXEC void zfp_encodef(Scalar* fblock,
                                  svtkm::Int32 maxbits,
                                  svtkm::UInt32 blockIdx,
                                  PortalType& stream)
{
  using Int = typename zfp::zfp_traits<Scalar>::Int;
  zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, svtkm::Id(blockIdx));
  svtkm::Int32 emax = zfp::MaxExponent<BlockSize, Scalar>(fblock);
  //  std::cout<<"EMAX "<<emax<<"\n";
  svtkm::Int32 maxprec =
    zfp::precision(emax, zfp::get_precision<Scalar>(), zfp::get_min_exp<Scalar>());
  svtkm::UInt32 e = svtkm::UInt32(maxprec ? emax + zfp::get_ebias<Scalar>() : 0);
  /* encode block only if biased exponent is nonzero */
  if (e)
  {

    const svtkm::UInt32 ebits = svtkm::UInt32(zfp::get_ebits<Scalar>()) + 1;
    blockWriter.write_bits(2 * e + 1, ebits);

    Int iblock[BlockSize];
    zfp::fwd_cast<Int, Scalar, BlockSize>(iblock, fblock, emax);

    encode_block<BlockSize>(blockWriter, maxbits - svtkm::Int32(ebits), maxprec, iblock);
  }
}

// helpers so we can do partial template instantiation since
// the portal type could be on any backend
template <svtkm::Int32 BlockSize, typename Scalar, typename PortalType>
struct ZFPBlockEncoder
{
};

template <svtkm::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, svtkm::Float32, PortalType>
{
  SVTKM_EXEC void encode(svtkm::Float32* fblock,
                        svtkm::Int32 maxbits,
                        svtkm::UInt32 blockIdx,
                        PortalType& stream)
  {
    zfp_encodef<BlockSize>(fblock, maxbits, blockIdx, stream);
  }
};

template <svtkm::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, svtkm::Float64, PortalType>
{
  SVTKM_EXEC void encode(svtkm::Float64* fblock,
                        svtkm::Int32 maxbits,
                        svtkm::UInt32 blockIdx,
                        PortalType& stream)
  {
    zfp_encodef<BlockSize>(fblock, maxbits, blockIdx, stream);
  }
};

template <svtkm::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, svtkm::Int32, PortalType>
{
  SVTKM_EXEC void encode(svtkm::Int32* fblock,
                        svtkm::Int32 maxbits,
                        svtkm::UInt32 blockIdx,
                        PortalType& stream)
  {
    using Int = typename zfp::zfp_traits<svtkm::Int32>::Int;
    zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, svtkm::Id(blockIdx));
    encode_block<BlockSize>(blockWriter, maxbits, get_precision<svtkm::Int32>(), (Int*)fblock);
  }
};

template <svtkm::Int32 BlockSize, typename PortalType>
struct ZFPBlockEncoder<BlockSize, svtkm::Int64, PortalType>
{
  SVTKM_EXEC void encode(svtkm::Int64* fblock,
                        svtkm::Int32 maxbits,
                        svtkm::UInt32 blockIdx,
                        PortalType& stream)
  {
    using Int = typename zfp::zfp_traits<svtkm::Int64>::Int;
    zfp::BlockWriter<BlockSize, PortalType> blockWriter(stream, maxbits, svtkm::Id(blockIdx));
    encode_block<BlockSize>(blockWriter, maxbits, get_precision<svtkm::Int64>(), (Int*)fblock);
  }
};
}
}
} // namespace svtkm::worklet::zfp
#endif
