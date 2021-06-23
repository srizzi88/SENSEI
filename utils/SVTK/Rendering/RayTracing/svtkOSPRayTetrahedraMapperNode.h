/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayTetrahedraMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayTetrahedraMapperNode
 * @brief   Unstructured grid volume renderer.
 *
 * svtkOSPRayTetrahedraMapperNode implements a volume rendering
 * that directly samples the AMR structure using OSPRay.
 *
 */

#ifndef svtkOSPRayTetrahedraMapperNode_h
#define svtkOSPRayTetrahedraMapperNode_h

#include "svtkOSPRayCache.h"               // For common cache infrastructure
#include "svtkRenderingRayTracingModule.h" // For export macro
#include "svtkVolumeMapperNode.h"

#include "RTWrapper/RTWrapper.h" // for handle types

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayTetrahedraMapperNode : public svtkVolumeMapperNode

{
public:
  svtkTypeMacro(svtkOSPRayTetrahedraMapperNode, svtkVolumeMapperNode);
  static svtkOSPRayTetrahedraMapperNode* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

protected:
  svtkOSPRayTetrahedraMapperNode();
  ~svtkOSPRayTetrahedraMapperNode() override;

  int NumColors;
  double SamplingRate;

  svtkTimeStamp BuildTime;
  svtkTimeStamp PropertyTime;

  OSPVolume OSPRayVolume;
  OSPTransferFunction TransferFunction;
  std::vector<float> TFVals;
  std::vector<float> TFOVals;

  std::vector<int> Cells;
  std::vector<osp::vec3f> Vertices;
  std::vector<float> Field;

  svtkOSPRayCache<svtkOSPRayCacheItemObject>* Cache;

private:
  svtkOSPRayTetrahedraMapperNode(const svtkOSPRayTetrahedraMapperNode&) = delete;
  void operator=(const svtkOSPRayTetrahedraMapperNode&) = delete;
};

#endif
