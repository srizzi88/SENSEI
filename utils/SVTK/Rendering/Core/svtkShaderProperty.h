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
 * @class   svtkShaderProperty
 * @brief   represent GPU shader properties
 *
 * svtkShaderProperty is used to hold user-defined modifications of a
 * GPU shader program used in a mapper.
 *
 * @sa
 * svtkVolume svtkOpenGLUniform
 *
 * @par Thanks:
 * Developed by Simon Drouin (sdrouin2@bwh.harvard.edu) at Brigham and Women's Hospital.
 *
 */

#ifndef svtkShaderProperty_h
#define svtkShaderProperty_h

#include "svtkNew.h" // For iVars
#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkUniforms;

class SVTKRENDERINGCORE_EXPORT svtkShaderProperty : public svtkObject
{
public:
  svtkTypeMacro(svtkShaderProperty, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with no shader replacements
   */
  static svtkShaderProperty* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(svtkShaderProperty* p);

  /**
   * @brief GetShaderMTime returns the last time a modification
   * was made that affected the code of the shader (either code
   * replacement was changed or one or more uniform variables were
   * added or removed. This timestamp can be used by mappers to
   * determine if the shader must be recompiled. Simply changing
   * the value of an existing uniform variable doesn't affect this
   * timestamp as it doesn't change the shader code.
   * @return timestamp of the last modification
   */
  svtkMTimeType GetShaderMTime();

  //@{
  /**
   * Allow the program to set the shader codes used directly
   * instead of using the built in templates. Be aware, if
   * set, this template will be used for all cases,
   * primitive types, picking etc.
   */
  bool HasVertexShaderCode();
  bool HasFragmentShaderCode();
  bool HasGeometryShaderCode();
  svtkSetStringMacro(VertexShaderCode);
  svtkGetStringMacro(VertexShaderCode);
  svtkSetStringMacro(FragmentShaderCode);
  svtkGetStringMacro(FragmentShaderCode);
  svtkSetStringMacro(GeometryShaderCode);
  svtkGetStringMacro(GeometryShaderCode);
  //@}

  //@{
  /**
   * The Uniforms object allows to set custom uniform variables
   * that are used in replacement shader code.
   */
  svtkGetObjectMacro(FragmentCustomUniforms, svtkUniforms);
  svtkGetObjectMacro(VertexCustomUniforms, svtkUniforms);
  svtkGetObjectMacro(GeometryCustomUniforms, svtkUniforms);
  //@}

  //@{
  /**
   * This function enables you to apply your own substitutions
   * to the shader creation process. The shader code in this class
   * is created by applying a bunch of string replacements to a
   * shader template. Using this function you can apply your
   * own string replacements to add features you desire.
   */
  virtual void AddVertexShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) = 0;
  virtual void AddFragmentShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) = 0;
  virtual void AddGeometryShaderReplacement(const std::string& originalValue,
    bool replaceFirst, // do this replacement before the default
    const std::string& replacementValue, bool replaceAll) = 0;
  virtual int GetNumberOfShaderReplacements() = 0;
  virtual std::string GetNthShaderReplacementTypeAsString(svtkIdType index) = 0;
  virtual void GetNthShaderReplacement(svtkIdType index, std::string& name, bool& replaceFirst,
    std::string& replacementValue, bool& replaceAll) = 0;
  virtual void ClearVertexShaderReplacement(
    const std::string& originalValue, bool replaceFirst) = 0;
  virtual void ClearFragmentShaderReplacement(
    const std::string& originalValue, bool replaceFirst) = 0;
  virtual void ClearGeometryShaderReplacement(
    const std::string& originalValue, bool replaceFirst) = 0;
  virtual void ClearAllVertexShaderReplacements() = 0;
  virtual void ClearAllFragmentShaderReplacements() = 0;
  virtual void ClearAllGeometryShaderReplacements() = 0;
  virtual void ClearAllShaderReplacements() = 0;
  //@}

protected:
  svtkShaderProperty();
  ~svtkShaderProperty() override;

  char* VertexShaderCode;
  char* FragmentShaderCode;
  char* GeometryShaderCode;

  svtkNew<svtkUniforms> FragmentCustomUniforms;
  svtkNew<svtkUniforms> VertexCustomUniforms;
  svtkNew<svtkUniforms> GeometryCustomUniforms;

private:
  svtkShaderProperty(const svtkShaderProperty&) = delete;
  void operator=(const svtkShaderProperty&) = delete;
};

#endif
