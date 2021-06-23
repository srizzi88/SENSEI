//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_CanvasOSMesa_h
#define svtk_m_rendering_CanvasOSMesa_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/CanvasGL.h>

#include <memory>

namespace svtkm
{
namespace rendering
{

namespace detail
{

struct CanvasOSMesaInternals;

} // namespace detail

class SVTKM_RENDERING_EXPORT CanvasOSMesa : public CanvasGL
{
public:
  CanvasOSMesa(svtkm::Id width = 1024, svtkm::Id height = 1024);

  ~CanvasOSMesa();

  virtual void Initialize() override;

  virtual void RefreshColorBuffer() const override;

  virtual void Activate() override;

  virtual void Finish() override;

  svtkm::rendering::Canvas* NewCopy() const override;

private:
  std::shared_ptr<detail::CanvasOSMesaInternals> Internals;
};
}
} //namespace svtkm::rendering

#endif //svtk_m_rendering_CanvasOSMesa_h
