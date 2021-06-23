/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRIBProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRIBProperty
 * @brief   RIP Property
 *
 * svtkRIBProperty is a subclass of svtkProperty that allows the user to
 * specify named shaders for use with RenderMan. Both surface
 * and displacement shaders can be specified. Parameters
 * for the shaders can be declared and set.
 *
 * @sa
 * svtkRIBExporter svtkRIBLight
 */

#ifndef svtkRIBProperty_h
#define svtkRIBProperty_h

#include "svtkIOExportModule.h" // For export macro
#include "svtkProperty.h"

class svtkRIBRenderer;

class SVTKIOEXPORT_EXPORT svtkRIBProperty : public svtkProperty
{
public:
  static svtkRIBProperty* New();
  svtkTypeMacro(svtkRIBProperty, svtkProperty);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If true (default) the surface shader uses the usual shader parameters:
   * Ka - Ambient amount
   * Kd - Diffuse amount
   * Ks - Specular amount
   * Roughness
   * SpecularColor
   * Additional surface shader parameters can be added with the
   * Set/AddSurfaceShaderParameter methods.
   * If false, all surface shader parameters must be specified
   */
  svtkSetMacro(SurfaceShaderUsesDefaultParameters, bool);
  svtkGetMacro(SurfaceShaderUsesDefaultParameters, bool);
  svtkBooleanMacro(SurfaceShaderUsesDefaultParameters, bool);
  //@}

  //@{
  /**
   * Specify the name of a surface shader.
   */
  svtkSetStringMacro(SurfaceShader);
  svtkGetStringMacro(SurfaceShader);
  //@}

  //@{
  /**
   * Specify the name of a displacement shader.
   */
  svtkSetStringMacro(DisplacementShader);
  svtkGetStringMacro(DisplacementShader);
  //@}

  //@{
  /**
   * Specify declarations for variables..
   */
  void SetVariable(const char* variable, const char* declaration);
  void AddVariable(const char* variable, const char* declaration);
  //@}

  /**
   * Get variable declarations
   */
  char* GetDeclarations();

  //@{
  /**
   * Specify parameter values for variables.
   * DEPRECATED: use (Set/Add)SurfaceShaderParameter instead.
   */
  void SetParameter(const char* parameter, const char* value);
  void AddParameter(const char* parameter, const char* value);
  //@}

  //@{
  /**
   * Specify parameter values for surface shader parameters
   */
  void SetSurfaceShaderParameter(const char* parameter, const char* value);
  void AddSurfaceShaderParameter(const char* parameter, const char* value);
  //@}

  //@{
  /**
   * Specify parameter values for displacement shader parameters
   */
  void SetDisplacementShaderParameter(const char* parameter, const char* value);
  void AddDisplacementShaderParameter(const char* parameter, const char* value);
  //@}

  //@{
  /**
   * Get parameters.
   */
  char* GetParameters(); // DEPRECATED: use GetSurfaceShaderParameters instead.
  char* GetSurfaceShaderParameters();
  char* GetDisplacementShaderParameters();
  //@}

protected:
  svtkRIBProperty();
  ~svtkRIBProperty() override;

  void Render(svtkActor* a, svtkRenderer* ren) override;
  svtkProperty* Property;
  char* SurfaceShader;
  char* DisplacementShader;
  char* Declarations;
  char* SurfaceShaderParameters;
  char* DisplacementShaderParameters;
  bool SurfaceShaderUsesDefaultParameters;

private:
  svtkRIBProperty(const svtkRIBProperty&) = delete;
  void operator=(const svtkRIBProperty&) = delete;
};

#endif
