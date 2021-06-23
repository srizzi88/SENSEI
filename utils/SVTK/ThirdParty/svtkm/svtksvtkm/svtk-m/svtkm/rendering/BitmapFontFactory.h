//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_BitmapFontFactory_h
#define svtk_m_BitmapFontFactory_h

#include <svtkm/rendering/BitmapFont.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT BitmapFontFactory
{
public:
  static svtkm::rendering::BitmapFont CreateLiberation2Sans();
};
}
} //namespace svtkm::rendering

#endif
