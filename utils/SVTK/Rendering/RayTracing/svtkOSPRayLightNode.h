/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayLightNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayLightNode
 * @brief   links svtkLights to OSPRay
 *
 * Translates svtkLight state into OSPRay rendering calls
 */

#ifndef svtkOSPRayLightNode_h
#define svtkOSPRayLightNode_h

#include "svtkLightNode.h"
#include "svtkRenderingRayTracingModule.h" // For export macro

#include "RTWrapper/RTWrapper.h" // for handle types

#include <string> // for std::string

class svtkInformationDoubleKey;
class svtkInformationIntegerKey;
class svtkLight;
class svtkOSPRayRendererNode;

class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayLightNode : public svtkLightNode
{
public:
  static svtkOSPRayLightNode* New();
  svtkTypeMacro(svtkOSPRayLightNode, svtkLightNode);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Make ospray calls to render me.
   */
  virtual void Render(bool prepass) override;

  //@{
  /**
   * A global multiplier to all ospray lights.
   * default is 1.0
   */
  static void SetLightScale(double s);
  static double GetLightScale();
  //@}

  // state beyond rendering core...

  /**
   * When present on light, the light acts as an ambient source.
   * An AmbientLight is one that has no specific position in space and for
   * which only the ambient color term affects the result.
   */
  static svtkInformationIntegerKey* IS_AMBIENT();

  //@{
  /**
   * Convenience method to set/get IS_AMBIENT on a svtkLight.
   */
  static void SetIsAmbient(int, svtkLight*);
  static int GetIsAmbient(svtkLight*);
  //@}

  /**
   * The radius setting, when > 0.0, produces soft shadows in the
   * path tracer.
   */
  static svtkInformationDoubleKey* RADIUS();

  //@{
  /**
   * Convenience method to set/get RADIUS on a svtkLight.
   */
  static void SetRadius(double, svtkLight*);
  static double GetRadius(svtkLight*);
  //@}

protected:
  svtkOSPRayLightNode();
  ~svtkOSPRayLightNode() override;

private:
  svtkOSPRayLightNode(const svtkOSPRayLightNode&) = delete;
  void operator=(const svtkOSPRayLightNode&) = delete;

  static double LightScale;
  void* OLight;
};

#endif
