//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_TextureGL_h
#define svtk_m_TextureGL_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/Types.h>

#include <memory>
#include <vector>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT TextureGL
{
public:
  TextureGL();

  ~TextureGL();

  bool Valid() const;

  void Enable() const;

  void Disable() const;

  void CreateAlphaFromRGBA(svtkm::Id width, svtkm::Id height, const std::vector<unsigned char>& rgba);

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_TextureGL_h
