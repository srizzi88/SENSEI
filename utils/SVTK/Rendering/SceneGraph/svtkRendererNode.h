/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRendererNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRendererNode
 * @brief   svtkViewNode specialized for svtkRenderers
 *
 * State storage and graph traversal for svtkRenderer
 */

#ifndef svtkRendererNode_h
#define svtkRendererNode_h

#include "svtkRenderingSceneGraphModule.h" // For export macro
#include "svtkViewNode.h"

class svtkCollection;

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkRendererNode : public svtkViewNode
{
public:
  static svtkRendererNode* New();
  svtkTypeMacro(svtkRendererNode, svtkViewNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Build containers for our child nodes.
   */
  void Build(bool prepass) override;

  /**
   * Get/Set the framebuffer size
   */
  svtkGetVector2Macro(Size, int);
  svtkSetVector2Macro(Size, int);

  /**
   * Get/Set the window viewport
   */
  svtkGetVector4Macro(Viewport, double);
  svtkSetVector4Macro(Viewport, double);

  /**
   * Get/Set the window tile scale
   */
  svtkGetVector2Macro(Scale, int);
  svtkSetVector2Macro(Scale, int);

protected:
  svtkRendererNode();
  ~svtkRendererNode() override;

  int Size[2];
  double Viewport[4];
  int Scale[2];

private:
  svtkRendererNode(const svtkRendererNode&) = delete;
  void operator=(const svtkRendererNode&) = delete;
};

#endif
