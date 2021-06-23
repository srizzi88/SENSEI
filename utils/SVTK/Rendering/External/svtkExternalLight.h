/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExternalLight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExternalLight
 * @brief   a virtual light object for tweaking existing lights
 * in an external 3D rendering context
 *
 *
 * svtkExternalLight is a virtual light object for tweaking existing lights in
 * an external 3D rendering context. It provides a mechanism to adjust and
 * control parameters of existing lights in an external OpenGL context.
 *
 * It provides methods to locate and point the light,
 * and set its brightness and color. In addition to the
 * basic infinite distance point light source attributes, you can also specify
 * the light attenuation values and cone angle. These attributes are only used
 * if the light is a positional light.
 *
 * By default, svtkExternalLight overrides specific light parameters as set by
 * the user. Setting the #ReplaceMode to ALL_PARAMS, will set all
 * the light parameter values to the ones set in svtkExternalLight.
 *
 * @warning
 * Use the svtkExternalLight object to tweak parameters of lights created in the
 * external context. This class does NOT create new lights in the scene.
 *
 * @par Example:
 * Usage example for svtkExternalLight in conjunction with
 * svtkExternalOpenGLRenderer and \ref ExternalSVTKWidget
 * \code{.cpp}
 *    svtkNew<svtkExternalLight> exLight;
 *    exLight->SetLightIndex(GL_LIGHT0); // GL_LIGHT0 identifies the external light
 *    exLight->SetDiffuseColor(1.0, 0.0, 0.0); // Changing diffuse color
 *    svtkNew<ExternalSVTKWidget> exWidget;
 *    svtkExternalOpenGLRenderer* ren =
 * svtkExternalOpenGLRenderer::SafeDownCast(exWidget->AddRenderer());
 *    ren->AddExternalLight(exLight.GetPointer());
 * \endcode
 *
 * @sa
 * svtkExternalOpenGLRenderer \ref ExternalSVTKWidget
 */

#ifndef svtkExternalLight_h
#define svtkExternalLight_h

#include "svtkLight.h"
#include "svtkRenderingExternalModule.h" // For export macro

class SVTKRENDERINGEXTERNAL_EXPORT svtkExternalLight : public svtkLight
{
public:
  svtkTypeMacro(svtkExternalLight, svtkLight);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create an external light object with the focal point at the origin and its
   * position set to (0,0,1). The light is a Headlight, its color is white
   * (black ambient, white diffuse, white specular),
   * intensity=1, the light is turned on, positional lighting is off,
   * ConeAngle=30, AttenuationValues=(1,0,0), Exponent=1 and the
   * TransformMatrix is NULL. The light index is GL_LIGHT0, which means the
   * existing light with index GL_LIGHT0 will be affected by this light.
   */
  static svtkExternalLight* New();

  enum ReplaceModes
  {
    INDIVIDUAL_PARAMS = 0, // default
    ALL_PARAMS = 1
  };

  //@{
  /**
   * Set/Get light index
   * This should be the OpenGL light identifier. (e.g.: GL_LIGHT0)
   * (Default: GL_LIGHT0)
   */
  svtkSetMacro(LightIndex, int);
  svtkGetMacro(LightIndex, int);
  //@}

  //@{
  /**
   * Set/Get replace mode
   * This determines how this ExternalLight will be used to tweak parameters on
   * an existing light in the rendering context.
   * (Default: INDIVIDUAL_PARAMS)

   * \li \c svtkExternalLight::INDIVIDUAL_PARAMS : Replace parameters
   * specifically set by the user by calling the parameter
   * Set method. (e.g. SetDiffuseColor())

   * \li \c svtkExternalLight::ALL_PARAMS : Replace all
   * parameters of the light with the parameters in svtkExternalLight object.
   */
  svtkSetMacro(ReplaceMode, int);
  svtkGetMacro(ReplaceMode, int);
  //@}

  /**
   * Override Set method to keep a record of changed value
   */
  void SetPosition(double, double, double) override;
  using Superclass::SetPosition;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetFocalPoint(double, double, double) override;
  using Superclass::SetFocalPoint;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetAmbientColor(double, double, double) override;
  using Superclass::SetAmbientColor;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetDiffuseColor(double, double, double) override;
  using Superclass::SetDiffuseColor;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetSpecularColor(double, double, double) override;
  using Superclass::SetSpecularColor;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetIntensity(double) override;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetConeAngle(double) override;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetAttenuationValues(double, double, double) override;
  using Superclass::SetAttenuationValues;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetExponent(double) override;

  /**
   * Override Set method to keep a record of changed value
   */
  void SetPositional(svtkTypeBool) override;

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(PositionSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(FocalPointSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(AmbientColorSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(DiffuseColorSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(SpecularColorSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(IntensitySet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(ConeAngleSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(AttenuationValuesSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(ExponentSet, bool);
  //@}

  //@{
  /**
   * Check whether value set by user
   */
  svtkGetMacro(PositionalSet, bool);
  //@}

protected:
  svtkExternalLight();
  ~svtkExternalLight() override;

  int LightIndex;
  int ReplaceMode;

  bool PositionSet;
  bool FocalPointSet;
  bool AmbientColorSet;
  bool DiffuseColorSet;
  bool SpecularColorSet;
  bool IntensitySet;
  bool ConeAngleSet;
  bool AttenuationValuesSet;
  bool ExponentSet;
  bool PositionalSet;

private:
  svtkExternalLight(const svtkExternalLight&) = delete;
  void operator=(const svtkExternalLight&) = delete;
};

#endif // svtkExternalLight_h
