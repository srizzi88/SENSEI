/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVolumeNode
 * @brief   svtkViewNode specialized for svtkActors
 *
 * State storage and graph traversal for svtkActor/Mapper and Property
 * Made a choice to merge actor, mapper and property together. If there
 * is a compelling reason to separate them we can.
 */

#ifndef svtkVolumeNode_h
#define svtkVolumeNode_h

#include "svtkRenderingSceneGraphModule.h" // For export macro
#include "svtkViewNode.h"

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkVolumeNode : public svtkViewNode
{
public:
  static svtkVolumeNode* New();
  svtkTypeMacro(svtkVolumeNode, svtkViewNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Build containers for our child nodes.
   */
  virtual void Build(bool prepass) override;

protected:
  svtkVolumeNode();
  ~svtkVolumeNode() override;

private:
  svtkVolumeNode(const svtkVolumeNode&) = delete;
  void operator=(const svtkVolumeNode&) = delete;
};

#endif
