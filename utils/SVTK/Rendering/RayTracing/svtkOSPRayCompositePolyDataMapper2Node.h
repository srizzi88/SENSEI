/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayCompositePolyDataMapper2Node.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayCompositePolyDataMapper2Node
 * @brief   links svtkActor and svtkMapper to OSPRay
 *
 * Translates svtkActor/Mapper state into OSPRay rendering calls
 */

#ifndef svtkOSPRayCompositePolyDataMapper2Node_h
#define svtkOSPRayCompositePolyDataMapper2Node_h

#include "svtkColor.h" // used for ivars
#include "svtkOSPRayPolyDataMapperNode.h"
#include "svtkRenderingRayTracingModule.h" // For export macro
#include <stack>                          // used for ivars

class svtkDataObject;
class svtkCompositePolyDataMapper2;
class svtkOSPRayRendererNode;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayCompositePolyDataMapper2Node
  : public svtkOSPRayPolyDataMapperNode
{
public:
  static svtkOSPRayCompositePolyDataMapper2Node* New();
  svtkTypeMacro(svtkOSPRayCompositePolyDataMapper2Node, svtkOSPRayPolyDataMapperNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

  /**
   * Invalidates cached rendering data.
   */
  virtual void Invalidate(bool prepass) override;

protected:
  svtkOSPRayCompositePolyDataMapper2Node();
  ~svtkOSPRayCompositePolyDataMapper2Node();

  class RenderBlockState
  {
  public:
    std::stack<bool> Visibility;
    std::stack<double> Opacity;
    std::stack<svtkColor3d> AmbientColor;
    std::stack<svtkColor3d> DiffuseColor;
    std::stack<svtkColor3d> SpecularColor;
    std::stack<std::string> Material;
  };

  RenderBlockState BlockState;
  void RenderBlock(svtkOSPRayRendererNode* orn, svtkCompositePolyDataMapper2* cpdm, svtkActor* actor,
    svtkDataObject* dobj, unsigned int& flat_index);

private:
  svtkOSPRayCompositePolyDataMapper2Node(const svtkOSPRayCompositePolyDataMapper2Node&) = delete;
  void operator=(const svtkOSPRayCompositePolyDataMapper2Node&) = delete;
};
#endif
