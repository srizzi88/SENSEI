/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayMoleculeMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayMoleculeMapperNode
 * @brief   links svtkMoleculeMapper to OSPRay
 *
 * Translates svtkMoleculeMapper state into OSPRay rendering calls
 */

#ifndef svtkOSPRayMoleculeMapperNode_h
#define svtkOSPRayMoleculeMapperNode_h

#include "svtkPolyDataMapperNode.h"
#include "svtkRenderingRayTracingModule.h" // For export macro

#include "RTWrapper/RTWrapper.h" // for handle types
#include <vector>

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayMoleculeMapperNode : public svtkPolyDataMapperNode
{
public:
  static svtkOSPRayMoleculeMapperNode* New();
  svtkTypeMacro(svtkOSPRayMoleculeMapperNode, svtkPolyDataMapperNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

protected:
  svtkOSPRayMoleculeMapperNode();
  ~svtkOSPRayMoleculeMapperNode() override;

  svtkTimeStamp BuildTime;
  std::vector<OSPGeometry> Geometries;

private:
  svtkOSPRayMoleculeMapperNode(const svtkOSPRayMoleculeMapperNode&) = delete;
  void operator=(const svtkOSPRayMoleculeMapperNode&) = delete;
};
#endif
