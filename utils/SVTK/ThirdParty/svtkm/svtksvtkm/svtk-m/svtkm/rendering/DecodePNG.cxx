//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/DecodePNG.h>

#include <svtkm/cont/Logging.h>
#include <svtkm/internal/Configure.h>

SVTKM_THIRDPARTY_PRE_INCLUDE
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_DISK
#include <svtkm/thirdparty/lodepng/svtkmlodepng/lodepng.cpp>
SVTKM_THIRDPARTY_PRE_INCLUDE

namespace svtkm
{
namespace rendering
{

svtkm::UInt32 DecodePNG(std::vector<unsigned char>& out_image,
                       unsigned long& image_width,
                       unsigned long& image_height,
                       const unsigned char* in_png,
                       std::size_t in_size)
{
  using namespace svtkm::png;
  constexpr std::size_t bitdepth = 8;
  svtkm::UInt32 iw = 0;
  svtkm::UInt32 ih = 0;

  auto retcode = lodepng::decode(out_image, iw, ih, in_png, in_size, LCT_RGBA, bitdepth);
  image_width = iw;
  image_height = ih;
  return retcode;
}
}
} // namespace svtkm::rendering
