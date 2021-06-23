/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMapperNode
 * @brief   svtkViewNode specialized for svtkMappers
 *
 * State storage and graph traversal for svtkMapper
 */

#ifndef svtkMapperNode_h
#define svtkMapperNode_h

#include "svtkRenderingSceneGraphModule.h" // For export macro
#include "svtkViewNode.h"

#include <vector> //for results

class svtkAbstractArray;
class svtkDataSet;
class svtkMapper;
class svtkPolyData;

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkMapperNode : public svtkViewNode
{
public:
  static svtkMapperNode* New();
  svtkTypeMacro(svtkMapperNode, svtkViewNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkMapperNode();
  ~svtkMapperNode() override;

  svtkAbstractArray* GetArrayToProcess(svtkDataSet* input, int& association);

private:
  svtkMapperNode(const svtkMapperNode&) = delete;
  void operator=(const svtkMapperNode&) = delete;
};

#endif
