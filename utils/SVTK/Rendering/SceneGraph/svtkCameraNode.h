/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCameraNode
 * @brief   svtkViewNode specialized for svtkCameras
 *
 * State storage and graph traversal for svtkCamera
 */

#ifndef svtkCameraNode_h
#define svtkCameraNode_h

#include "svtkRenderingSceneGraphModule.h" // For export macro
#include "svtkViewNode.h"

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkCameraNode : public svtkViewNode
{
public:
  static svtkCameraNode* New();
  svtkTypeMacro(svtkCameraNode, svtkViewNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkCameraNode();
  ~svtkCameraNode() override;

private:
  svtkCameraNode(const svtkCameraNode&) = delete;
  void operator=(const svtkCameraNode&) = delete;
};

#endif
