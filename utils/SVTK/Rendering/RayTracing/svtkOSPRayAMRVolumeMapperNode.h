/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayAMRVolumeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkOSPRayAMRVolumeMapperNode
 * @brief links svtkVolumeMapper  to OSPRay
 *
 * Translates svtkAMRVolumeMapper state into OSPRay rendering calls
 * Directly samples the svtkAMR data structure without resampling
 * Data is expected to be overlapping, only floats and doubles are now
 * supported.
 */

#ifndef svtkOSPRayAMRVolumeMapperNode_h
#define svtkOSPRayAMRVolumeMapperNode_h

#include "svtkOSPRayVolumeMapperNode.h"
#include "svtkRenderingRayTracingModule.h" // For export macro

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayAMRVolumeMapperNode : public svtkOSPRayVolumeMapperNode
{
public:
  static svtkOSPRayAMRVolumeMapperNode* New();
  svtkTypeMacro(svtkOSPRayAMRVolumeMapperNode, svtkOSPRayVolumeMapperNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Traverse graph in ospray's preferred order and render
   */
  virtual void Render(bool prepass) override;

protected:
  svtkOSPRayAMRVolumeMapperNode();
  ~svtkOSPRayAMRVolumeMapperNode() = default;

private:
  svtkOSPRayAMRVolumeMapperNode(const svtkOSPRayAMRVolumeMapperNode&) = delete;
  void operator=(const svtkOSPRayAMRVolumeMapperNode&) = delete;

  float OldSamplingRate;
};
#endif
