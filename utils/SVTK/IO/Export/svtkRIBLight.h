/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRIBLight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRIBLight
 * @brief   RIP Light
 *
 * svtkRIBLight is a subclass of svtkLight that allows the user to
 * specify light source shaders and shadow casting lights for use with
 * RenderMan.
 *
 * @sa
 * svtkRIBExporter svtkRIBProperty
 */

#ifndef svtkRIBLight_h
#define svtkRIBLight_h

#include "svtkIOExportModule.h" // For export macro
#include "svtkLight.h"

class svtkRIBRenderer;

class SVTKIOEXPORT_EXPORT svtkRIBLight : public svtkLight
{
public:
  static svtkRIBLight* New();
  svtkTypeMacro(svtkRIBLight, svtkLight);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkBooleanMacro(Shadows, svtkTypeBool);
  svtkSetMacro(Shadows, svtkTypeBool);
  svtkGetMacro(Shadows, svtkTypeBool);

  void Render(svtkRenderer* ren, int index) override;

protected:
  svtkRIBLight();
  ~svtkRIBLight() override;

  svtkLight* Light;
  svtkTypeBool Shadows;

private:
  svtkRIBLight(const svtkRIBLight&) = delete;
  void operator=(const svtkRIBLight&) = delete;
};

#endif
