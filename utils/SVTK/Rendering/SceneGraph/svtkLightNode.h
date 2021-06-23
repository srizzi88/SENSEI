/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLightNode
 * @brief   svtkViewNode specialized for svtkLights
 *
 * State storage and graph traversal for svtkLight
 */

#ifndef svtkLightNode_h
#define svtkLightNode_h

#include "svtkRenderingSceneGraphModule.h" // For export macro
#include "svtkViewNode.h"

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkLightNode : public svtkViewNode
{
public:
  static svtkLightNode* New();
  svtkTypeMacro(svtkLightNode, svtkViewNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkLightNode();
  ~svtkLightNode() override;

private:
  svtkLightNode(const svtkLightNode&) = delete;
  void operator=(const svtkLightNode&) = delete;
};

#endif
