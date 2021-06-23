/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVolumeMapperNode
 * @brief   svtkViewNode specialized for svtkVolumeMappers
 *
 * State storage and graph traversal for svtkVolumeMapper/PolyDataMapper and Property
 * Made a choice to merge PolyDataMapper, PolyDataMapper and property together. If there
 * is a compelling reason to separate them we can.
 */

#ifndef svtkVolumeMapperNode_h
#define svtkVolumeMapperNode_h

#include "svtkMapperNode.h"
#include "svtkRenderingSceneGraphModule.h" // For export macro

#include <vector> //for results

class svtkActor;
class svtkVolumeMapper;
class svtkPolyData;

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkVolumeMapperNode : public svtkMapperNode
{
public:
  static svtkVolumeMapperNode* New();
  svtkTypeMacro(svtkVolumeMapperNode, svtkMapperNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkVolumeMapperNode();
  ~svtkVolumeMapperNode() override;

private:
  svtkVolumeMapperNode(const svtkVolumeMapperNode&) = delete;
  void operator=(const svtkVolumeMapperNode&) = delete;
};

#endif
