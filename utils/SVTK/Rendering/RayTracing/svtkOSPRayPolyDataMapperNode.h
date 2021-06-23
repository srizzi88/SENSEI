/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayPolyDataMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayPolyDataMapperNode
 * @brief   links svtkActor and svtkMapper to OSPRay
 *
 * Translates svtkActor/Mapper state into OSPRay rendering calls
 */

#ifndef svtkOSPRayPolyDataMapperNode_h
#define svtkOSPRayPolyDataMapperNode_h

#include "svtkOSPRayCache.h" // For common cache infrastructure
#include "svtkPolyDataMapperNode.h"
#include "svtkRenderingRayTracingModule.h" // For export macro

class svtkOSPRayActorNode;
class svtkPolyData;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayPolyDataMapperNode : public svtkPolyDataMapperNode
{
public:
  static svtkOSPRayPolyDataMapperNode* New();
  svtkTypeMacro(svtkOSPRayPolyDataMapperNode, svtkPolyDataMapperNode);
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
  svtkOSPRayPolyDataMapperNode();
  ~svtkOSPRayPolyDataMapperNode() override;

  void ORenderPoly(void* renderer, svtkOSPRayActorNode* aNode, svtkPolyData* poly,
    double* ambientColor, double* diffuseColor, double opacity, std::string material);

  class svtkOSPRayCacheItemGeometries
  {
  public:
    svtkOSPRayCacheItemGeometries() = default;
    svtkOSPRayCacheItemGeometries(const std::vector<OSPGeometry>& geometries_)
      : GeometriesAtTime(geometries_)
    {
    }

    ~svtkOSPRayCacheItemGeometries() = default;

    std::vector<OSPGeometry> GeometriesAtTime;
  };

  std::vector<OSPGeometry> Geometries;
  void ClearGeometries();

  svtkOSPRayCache<svtkOSPRayCacheItemGeometries>* GeometryCache{ nullptr };
  svtkOSPRayCache<svtkOSPRayCacheItemObject>* InstanceCache{ nullptr };

  /**
   * @brief adds geometries to ospray cache
   */
  void PopulateCache();

  /**
   * @brief add computed ospray geometries to renderer model.
   * Will grab from cache if cached.
   */
  void RenderGeometries();

  bool UseInstanceCache;
  bool UseGeometryCache;

private:
  svtkOSPRayPolyDataMapperNode(const svtkOSPRayPolyDataMapperNode&) = delete;
  void operator=(const svtkOSPRayPolyDataMapperNode&) = delete;
};

#endif
