//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_rendering_Texture2D_h
#define svtk_m_rendering_Texture2D_h

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ExecutionObjectBase.h>

namespace svtkm
{
namespace rendering
{

enum class TextureFilterMode
{
  NearestNeighbour,
  Linear,
}; // enum TextureFilterMode

enum class TextureWrapMode
{
  Clamp,
  Repeat,
}; // enum TextureWrapMode

template <svtkm::IdComponent NumComponents>
class Texture2D
{
public:
  using TextureDataHandle = typename svtkm::cont::ArrayHandle<svtkm::UInt8>;
  using ColorType = svtkm::Vec<svtkm::Float32, NumComponents>;

  class Texture2DSampler;

#define UV_BOUNDS_CHECK(u, v, NoneType)                                                            \
  if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)                                                \
  {                                                                                                \
    return NoneType();                                                                             \
  }

  SVTKM_CONT
  Texture2D()
    : Width(0)
    , Height(0)
  {
  }

  SVTKM_CONT
  Texture2D(svtkm::Id width, svtkm::Id height, const TextureDataHandle& data)
    : Width(width)
    , Height(height)
    , FilterMode(TextureFilterMode::Linear)
    , WrapMode(TextureWrapMode::Clamp)
  {
    SVTKM_ASSERT(data.GetNumberOfValues() == (Width * Height * NumComponents));
    // We do not know the lifetime of the underlying data source of input `data`. Since it might
    // be from a shallow copy of the data source, we make a deep copy of the input data and keep
    // it's portal. The copy operation is very fast.
    svtkm::cont::Algorithm::Copy(data, Data);
  }

  SVTKM_CONT
  bool IsValid() const { return Width > 0 && Height > 0; }

  SVTKM_CONT
  TextureFilterMode GetFilterMode() const { return this->FilterMode; }

  SVTKM_CONT
  void SetFilterMode(TextureFilterMode filterMode) { this->FilterMode = filterMode; }

  SVTKM_CONT
  TextureWrapMode GetWrapMode() const { return this->WrapMode; }

  SVTKM_CONT
  void SetWrapMode(TextureWrapMode wrapMode) { this->WrapMode = wrapMode; }

  SVTKM_CONT Texture2DSampler GetExecObjectFactory() const
  {
    return Texture2DSampler(Width, Height, Data, FilterMode, WrapMode);
  }

  template <typename Device>
  class Texture2DSamplerExecutionObject
  {
  public:
    using TextureExecPortal =
      typename TextureDataHandle::template ExecutionTypes<Device>::PortalConst;

    SVTKM_CONT
    Texture2DSamplerExecutionObject()
      : Width(0)
      , Height(0)
    {
    }

    SVTKM_CONT
    Texture2DSamplerExecutionObject(svtkm::Id width,
                                    svtkm::Id height,
                                    const TextureDataHandle& data,
                                    TextureFilterMode filterMode,
                                    TextureWrapMode wrapMode)
      : Width(width)
      , Height(height)
      , Data(data.PrepareForInput(Device()))
      , FilterMode(filterMode)
      , WrapMode(wrapMode)
    {
    }

    SVTKM_EXEC
    inline ColorType GetColor(svtkm::Float32 u, svtkm::Float32 v) const
    {
      v = 1.0f - v;
      UV_BOUNDS_CHECK(u, v, ColorType);
      switch (FilterMode)
      {
        case TextureFilterMode::NearestNeighbour:
          return GetNearestNeighbourFilteredColor(u, v);

        case TextureFilterMode::Linear:
          return GetLinearFilteredColor(u, v);

        default:
          return ColorType();
      }
    }

  private:
    SVTKM_EXEC
    inline ColorType GetNearestNeighbourFilteredColor(svtkm::Float32 u, svtkm::Float32 v) const
    {
      svtkm::Id x = static_cast<svtkm::Id>(svtkm::Round(u * static_cast<svtkm::Float32>(Width - 1)));
      svtkm::Id y = static_cast<svtkm::Id>(svtkm::Round(v * static_cast<svtkm::Float32>(Height - 1)));
      return GetColorAtCoords(x, y);
    }

    SVTKM_EXEC
    inline ColorType GetLinearFilteredColor(svtkm::Float32 u, svtkm::Float32 v) const
    {
      u = u * static_cast<svtkm::Float32>(Width) - 0.5f;
      v = v * static_cast<svtkm::Float32>(Height) - 0.5f;
      svtkm::Id x = static_cast<svtkm::Id>(svtkm::Floor(u));
      svtkm::Id y = static_cast<svtkm::Id>(svtkm::Floor(v));
      svtkm::Float32 uRatio = u - static_cast<svtkm::Float32>(x);
      svtkm::Float32 vRatio = v - static_cast<svtkm::Float32>(y);
      svtkm::Float32 uOpposite = 1.0f - uRatio;
      svtkm::Float32 vOpposite = 1.0f - vRatio;
      svtkm::Id xn, yn;
      GetNextCoords(x, y, xn, yn);
      ColorType c1 = GetColorAtCoords(x, y);
      ColorType c2 = GetColorAtCoords(xn, y);
      ColorType c3 = GetColorAtCoords(x, yn);
      ColorType c4 = GetColorAtCoords(xn, yn);
      return (c1 * uOpposite + c2 * uRatio) * vOpposite + (c3 * uOpposite + c4 * uRatio) * vRatio;
    }

    SVTKM_EXEC
    inline ColorType GetColorAtCoords(svtkm::Id x, svtkm::Id y) const
    {
      svtkm::Id idx = (y * Width + x) * NumComponents;
      ColorType color;
      for (svtkm::IdComponent i = 0; i < NumComponents; ++i)
      {
        color[i] = Data.Get(idx + i) / 255.0f;
      }
      return color;
    }

    SVTKM_EXEC
    inline void GetNextCoords(svtkm::Id x, svtkm::Id y, svtkm::Id& xn, svtkm::Id& yn) const
    {
      switch (WrapMode)
      {
        case TextureWrapMode::Clamp:
          xn = (x + 1) < Width ? (x + 1) : x;
          yn = (y + 1) < Height ? (y + 1) : y;
          break;
        case TextureWrapMode::Repeat:
        default:
          xn = (x + 1) % Width;
          yn = (y + 1) % Height;
          break;
      }
    }

    svtkm::Id Width;
    svtkm::Id Height;
    TextureExecPortal Data;
    TextureFilterMode FilterMode;
    TextureWrapMode WrapMode;
  };

  class Texture2DSampler : public svtkm::cont::ExecutionObjectBase
  {
  public:
    SVTKM_CONT
    Texture2DSampler()
      : Width(0)
      , Height(0)
    {
    }

    SVTKM_CONT
    Texture2DSampler(svtkm::Id width,
                     svtkm::Id height,
                     const TextureDataHandle& data,
                     TextureFilterMode filterMode,
                     TextureWrapMode wrapMode)
      : Width(width)
      , Height(height)
      , Data(data)
      , FilterMode(filterMode)
      , WrapMode(wrapMode)
    {
    }

    template <typename Device>
    SVTKM_CONT Texture2DSamplerExecutionObject<Device> PrepareForExecution(Device) const
    {
      return Texture2DSamplerExecutionObject<Device>(
        this->Width, this->Height, this->Data, this->FilterMode, this->WrapMode);
    }

  private:
    svtkm::Id Width;
    svtkm::Id Height;
    TextureDataHandle Data;
    TextureFilterMode FilterMode;
    TextureWrapMode WrapMode;
  }; // class Texture2DSampler

private:
  svtkm::Id Width;
  svtkm::Id Height;
  TextureDataHandle Data;
  TextureFilterMode FilterMode;
  TextureWrapMode WrapMode;
}; // class Texture2D
}
} // namespace svtkm::rendering

#endif // svtk_m_rendering_Texture2D_h
