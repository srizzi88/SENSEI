/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayCameraNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayCameraNode
 * @brief   links svtkCamera to OSPRay
 *
 * Translates svtkCamera state into OSPRay rendering calls
 */

#ifndef svtkOSPRayCameraNode_h
#define svtkOSPRayCameraNode_h

#include "svtkCameraNode.h"
#include "svtkRenderingRayTracingModule.h" // For export macro

class svtkInformationIntegerKey;
class svtkCamera;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayCameraNode : public svtkCameraNode
{
public:
  static svtkOSPRayCameraNode* New();
  svtkTypeMacro(svtkOSPRayCameraNode, svtkCameraNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  void Render(bool prepass) override;

protected:
  svtkOSPRayCameraNode();
  ~svtkOSPRayCameraNode() override;

private:
  svtkOSPRayCameraNode(const svtkOSPRayCameraNode&) = delete;
  void operator=(const svtkOSPRayCameraNode&) = delete;
};

#endif
