/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSkybox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkSkybox
 * @brief Renders a skybox environment
 *
 * You must provide a texture cube map using the SetTexture method.
 */

#ifndef svtkSkybox_h
#define svtkSkybox_h

#include "svtkActor.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkSkybox : public svtkActor
{
public:
  static svtkSkybox* New();
  svtkTypeMacro(svtkSkybox, svtkActor);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
   * method GetBounds(double bounds[6]) is available from the superclass.)
   */
  using Superclass::GetBounds;
  double* GetBounds() override;

  //@{
  /**
   * Set/Get the projection to be used
   */
  enum Projection
  {
    Cube,
    Sphere,
    Floor,
    StereoSphere
  };
  svtkGetMacro(Projection, int);
  svtkSetMacro(Projection, int);
  void SetProjectionToCube() { this->SetProjection(svtkSkybox::Cube); }
  void SetProjectionToSphere() { this->SetProjection(svtkSkybox::Sphere); }
  void SetProjectionToStereoSphere() { this->SetProjection(svtkSkybox::StereoSphere); }
  void SetProjectionToFloor() { this->SetProjection(svtkSkybox::Floor); }
  //@}

  //@{
  /**
   * Set/Get the plane equation for the floor.
   */
  svtkSetVector4Macro(FloorPlane, float);
  svtkGetVector4Macro(FloorPlane, float);
  svtkSetVector3Macro(FloorRight, float);
  svtkGetVector3Macro(FloorRight, float);
  //@}

protected:
  svtkSkybox();
  ~svtkSkybox() override;

  int Projection;
  float FloorPlane[4];
  float FloorRight[3];

private:
  svtkSkybox(const svtkSkybox&) = delete;
  void operator=(const svtkSkybox&) = delete;
};

#endif // svtkSkybox_h
