/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayVolumeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayVolumeMapperNode
 * @brief   links svtkVolumeMapper  to OSPRay
 *
 * Translates svtkVolumeMapper state into OSPRay rendering calls
 */

#ifndef svtkOSPRayVolumeMapperNode_h
#define svtkOSPRayVolumeMapperNode_h

#include "svtkOSPRayCache.h"               // For common cache infrastructure
#include "svtkRenderingRayTracingModule.h" // For export macro
#include "svtkVolumeMapperNode.h"

#include "RTWrapper/RTWrapper.h" // for handle types

class svtkAbstractArray;
class svtkDataSet;
class svtkVolume;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayVolumeMapperNode : public svtkVolumeMapperNode
{
public:
  static svtkOSPRayVolumeMapperNode* New();
  svtkTypeMacro(svtkOSPRayVolumeMapperNode, svtkVolumeMapperNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

  /**
   * TODO: fix me
   * should be controlled by SVTK SampleDistance, otherwise
   * should use macros and modify self.
   */
  void SetSamplingRate(double rate) { this->SamplingRate = rate; }
  double GetSamplingRate() { return this->SamplingRate; }

protected:
  svtkOSPRayVolumeMapperNode();
  ~svtkOSPRayVolumeMapperNode();

  /**
   * updates internal OSPRay transfer function for volume
   */
  void UpdateTransferFunction(RTW::Backend* be, svtkVolume* vol, double* dataRange = nullptr);

  // TODO: SetAndGetters?
  int NumColors;
  double SamplingRate;
  double SamplingStep; // base sampling step of each voxel
  bool UseSharedBuffers;
  bool Shade; // volume shading set through volProperty
  OSPData SharedData;

  svtkTimeStamp BuildTime;
  svtkTimeStamp PropertyTime;

  OSPGeometry OSPRayIsosurface;
  OSPVolume OSPRayVolume;
  OSPTransferFunction TransferFunction;
  std::vector<float> TFVals;
  std::vector<float> TFOVals;

  svtkOSPRayCache<svtkOSPRayCacheItemObject>* Cache;

private:
  svtkOSPRayVolumeMapperNode(const svtkOSPRayVolumeMapperNode&) = delete;
  void operator=(const svtkOSPRayVolumeMapperNode&) = delete;
};
#endif
