/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWindowNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWindowNode
 * @brief   svtkViewNode specialized for svtkRenderWindows
 *
 * State storage and graph traversal for svtkRenderWindow
 */

#ifndef svtkWindowNode_h
#define svtkWindowNode_h

#include "svtkRenderingSceneGraphModule.h" // For export macro
#include "svtkViewNode.h"

class svtkUnsignedCharArray;
class svtkFloatArray;

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkWindowNode : public svtkViewNode
{
public:
  static svtkWindowNode* New();
  svtkTypeMacro(svtkWindowNode, svtkViewNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Build containers for our child nodes.
   */
  virtual void Build(bool prepass) override;

  /**
   * Get state of my renderable.
   */
  virtual void Synchronize(bool prepass) override;

  /**
   * Return the size of the last rendered image
   */
  virtual int* GetSize() { return this->Size; }

  /**
   * Get the most recent color buffer RGBA
   */
  virtual svtkUnsignedCharArray* GetColorBuffer() { return this->ColorBuffer; }

  /**
   * Get the most recent zbuffer buffer
   */
  virtual svtkFloatArray* GetZBuffer() { return this->ZBuffer; }

protected:
  svtkWindowNode();
  ~svtkWindowNode() override;

  // TODO: use a map with string keys being renderable's member name
  // state
  int Size[2];

  // stores the results of a render
  svtkUnsignedCharArray* ColorBuffer;
  svtkFloatArray* ZBuffer;

private:
  svtkWindowNode(const svtkWindowNode&) = delete;
  void operator=(const svtkWindowNode&) = delete;
};

#endif
