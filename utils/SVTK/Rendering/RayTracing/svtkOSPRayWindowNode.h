/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayWindowNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayWindowNode
 * @brief   links svtkRendererWindows to OSPRay
 *
 * Translates svtkRenderWindow state into OSPRay rendering calls
 */

#ifndef svtkOSPRayWindowNode_h
#define svtkOSPRayWindowNode_h

#include "svtkRenderingRayTracingModule.h" // For export macro
#include "svtkWindowNode.h"

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayWindowNode : public svtkWindowNode
{
public:
  static svtkOSPRayWindowNode* New();
  svtkTypeMacro(svtkOSPRayWindowNode, svtkWindowNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

protected:
  svtkOSPRayWindowNode();
  ~svtkOSPRayWindowNode() override;

private:
  svtkOSPRayWindowNode(const svtkOSPRayWindowNode&) = delete;
  void operator=(const svtkOSPRayWindowNode&) = delete;
};

#endif
