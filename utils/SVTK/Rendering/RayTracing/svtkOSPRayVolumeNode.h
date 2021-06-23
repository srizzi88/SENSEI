/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayVolumeNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayVolumeNode
 * @brief   links svtkVolume and svtkMapper to OSPRay
 *
 * Translates svtkVolume/Mapper state into OSPRay rendering calls
 */

#ifndef svtkOSPRayVolumeNode_h
#define svtkOSPRayVolumeNode_h

#include "svtkRenderingRayTracingModule.h" // For export macro
#include "svtkVolumeNode.h"

class svtkVolume;
class svtkCompositeDataDisplayAttributes;
class svtkDataArray;
class svtkInformationIntegerKey;
class svtkInformationObjectBaseKey;
class svtkInformationStringKey;
class svtkPiecewiseFunction;
class svtkPolyData;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayVolumeNode : public svtkVolumeNode
{
public:
  static svtkOSPRayVolumeNode* New();
  svtkTypeMacro(svtkOSPRayVolumeNode, svtkVolumeNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Overridden to take into account my renderables time, including
   * mapper and data into mapper inclusive of composite input
   */
  virtual svtkMTimeType GetMTime() override;

protected:
  svtkOSPRayVolumeNode();
  ~svtkOSPRayVolumeNode() override;

private:
  svtkOSPRayVolumeNode(const svtkOSPRayVolumeNode&) = delete;
  void operator=(const svtkOSPRayVolumeNode&) = delete;
};
#endif
