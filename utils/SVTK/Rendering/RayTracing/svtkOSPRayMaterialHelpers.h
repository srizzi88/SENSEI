/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayMaterial.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayMaterialHelpers
 * @brief   convert svtk appearance controls to ospray materials
 *
 * Routines that convert svtk's appearance controlling state into ospray
 * specific calls to create materials. The key piece of information is the
 * svtkProperty::MaterialName, the rest is looked up from the
 * svtkOSPRayMaterialLibrary singleton.
 * The routines here are used by svtkOSPRayPolyDataMapperNode at render time.
 *
 * The contents here are private implementation details, and not meant to
 * be part of SVTK's public API.
 *
 * @sa svtkOSPRayMaterialLibrary
 */

#ifndef svtkOSPRayMaterialHelpers_h
#define svtkOSPRayMaterialHelpers_h

#include <map>
#include <string>

#include "RTWrapper/RTWrapper.h" // for handle types

class svtkImageData;
class svtkOSPRayRendererNode;

namespace svtkOSPRayMaterialHelpers
{

/**
 * Helper function to make a 2d OSPRay Texture.
 * Was promoted from OSPRay because of deprecation there.
 */
OSPTexture NewTexture2D(RTW::Backend* backend, const osp::vec2i& size, const OSPTextureFormat type,
  void* data, const uint32_t _flags, size_t sizeOf);

/**
 * Manufacture an ospray texture from a 2d svtkImageData
 */
OSPTexture SVTKToOSPTexture(RTW::Backend* backend, svtkImageData* vColorTextureMap);

/**
 * Construct a set of ospray materials for all of the material names.
 */
void MakeMaterials(
  svtkOSPRayRendererNode* orn, OSPRenderer oRenderer, std::map<std::string, OSPMaterial>& mats);

/**
 * Construct one ospray material within the given renderer that
 * corresponds to the visual characteristics set out in the named
 * material in the material library.
 */
OSPMaterial MakeMaterial(svtkOSPRayRendererNode* orn, OSPRenderer oRenderer, std::string nickname);

/**
 * Wraps ospNewMaterial
 */
OSPMaterial NewMaterial(svtkOSPRayRendererNode* orn, OSPRenderer oRenderer, std::string ospMatName);

}
#endif
// SVTK-HeaderTest-Exclude: svtkOSPRayMaterialHelpers.h
