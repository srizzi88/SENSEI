/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShaderProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLShaderProperty
 * @brief   represent GPU shader properties
 *
 * svtkOpenGLShaderProperty is used to hold user-defined modifications of a
 * GPU shader program used in a mapper.
 *
 * @sa
 * svtkShaderProperty svtkUniforms svtkOpenGLUniform
 *
 * @par Thanks:
 * Developed by Simon Drouin (sdrouin2@bwh.harvard.edu) at Brigham and Women's Hospital.
 *
 */

#ifndef svtkOpenGLShaderProperty_h
#define svtkOpenGLShaderProperty_h

#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkShader.h"                 // For methods (shader types)
#include "svtkShaderProperty.h"
#include <map> // used for ivar

class svtkOpenGLUniforms;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLShaderProperty : public svtkShaderProperty
{
public:
  svtkTypeMacro(svtkOpenGLShaderProperty, svtkShaderProperty);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with no shader replacements
   */
  static svtkOpenGLShaderProperty* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(svtkOpenGLShaderProperty* p);

  virtual void AddVertexShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;
  virtual void AddFragmentShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;
  virtual void AddGeometryShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) override;

  int GetNumberOfShaderReplacements() override;
  std::string GetNthShaderReplacementTypeAsString(svtkIdType index) override;
  void GetNthShaderReplacement(svtkIdType index, std::string& name, bool& replaceFirst,
    std::string& replacementValue, bool& replaceAll) override;

  void ClearVertexShaderReplacement(const std::string& originalValue, bool replaceFirst) override;
  void ClearFragmentShaderReplacement(const std::string& originalValue, bool replaceFirst) override;
  void ClearGeometryShaderReplacement(const std::string& originalValue, bool replaceFirst) override;
  void ClearAllVertexShaderReplacements() override;
  void ClearAllFragmentShaderReplacements() override;
  void ClearAllGeometryShaderReplacements() override;
  void ClearAllShaderReplacements() override;

  //@{
  /**
   * This function enables you to apply your own substitutions
   * to the shader creation process. The shader code in this class
   * is created by applying a bunch of string replacements to a
   * shader template. Using this function you can apply your
   * own string replacements to add features you desire.
   */
  void AddShaderReplacement(svtkShader::Type shaderType, // vertex, fragment, etc
    const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll);
  void ClearShaderReplacement(svtkShader::Type shaderType, // vertex, fragment, etc
    const std::string& originalValue, bool replaceFirst);
  void ClearAllShaderReplacements(svtkShader::Type shaderType);
  //@}

  /**
   * @brief GetAllShaderReplacements returns all user-specified shader
   * replacements. It is provided for iteration purpuses only (const)
   * and is mainly used by mappers when building the shaders.
   * @return const reference to internal map holding all replacements
   */
  typedef std::map<svtkShader::ReplacementSpec, svtkShader::ReplacementValue> ReplacementMap;
  const ReplacementMap& GetAllShaderReplacements() { return this->UserShaderReplacements; }

protected:
  svtkOpenGLShaderProperty();
  ~svtkOpenGLShaderProperty() override;

  ReplacementMap UserShaderReplacements;

private:
  svtkOpenGLShaderProperty(const svtkOpenGLShaderProperty&) = delete;
  void operator=(const svtkOpenGLShaderProperty&) = delete;
};

#endif
