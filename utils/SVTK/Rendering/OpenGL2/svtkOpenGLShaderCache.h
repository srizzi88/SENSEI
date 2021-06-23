/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLShaderCache
 * @brief   manage Shader Programs within a context
 *
 * svtkOpenGLShaderCache manages shader program compilation and binding
 */

#ifndef svtkOpenGLShaderCache_h
#define svtkOpenGLShaderCache_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkShader.h"                 // for svtkShader::Type
#include <map>                         // for methods

class svtkTransformFeedback;
class svtkShaderProgram;
class svtkWindow;
class svtkOpenGLRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLShaderCache : public svtkObject
{
public:
  static svtkOpenGLShaderCache* New();
  svtkTypeMacro(svtkOpenGLShaderCache, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // make sure the specified shaders are compiled, linked, and bound
  virtual svtkShaderProgram* ReadyShaderProgram(const char* vertexCode, const char* fragmentCode,
    const char* geometryCode, svtkTransformFeedback* cap = nullptr);

  // make sure the specified shaders are compiled, linked, and bound
  // will increment the reference count on the shaders if it
  // needs to keep them around
  virtual svtkShaderProgram* ReadyShaderProgram(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkTransformFeedback* cap = nullptr);

  // make sure the specified shaders are compiled, linked, and bound
  virtual svtkShaderProgram* ReadyShaderProgram(
    svtkShaderProgram* shader, svtkTransformFeedback* cap = nullptr);

  /**
   * Release the current shader.  Basically go back to
   * having no shaders loaded.  This is useful for old
   * legacy code that relies on no shaders being loaded.
   */
  void ReleaseCurrentShader();

  /**
   * Free up any resources being used by the provided shader
   */
  virtual void ReleaseGraphicsResources(svtkWindow* win);

  /**
   * Get/Clear the last Shader bound, called by shaders as they release
   * their graphics resources
   */
  virtual void ClearLastShaderBound() { this->LastShaderBound = nullptr; }
  svtkGetObjectMacro(LastShaderBound, svtkShaderProgram);

  // Set the time in seconds elapsed since the first render
  void SetElapsedTime(float val) { this->ElapsedTime = val; }

protected:
  svtkOpenGLShaderCache();
  ~svtkOpenGLShaderCache() override;

  // perform System and Output replacements in place. Returns
  // the number of outputs
  virtual unsigned int ReplaceShaderValues(
    std::string& VSSource, std::string& FSSource, std::string& GSSource);

  virtual svtkShaderProgram* GetShaderProgram(
    const char* vertexCode, const char* fragmentCode, const char* geometryCode);
  virtual svtkShaderProgram* GetShaderProgram(std::map<svtkShader::Type, svtkShader*> shaders);
  virtual int BindShader(svtkShaderProgram* shader);

  class Private;
  Private* Internal;
  svtkShaderProgram* LastShaderBound;

  int OpenGLMajorVersion;
  int OpenGLMinorVersion;

  float ElapsedTime;

private:
  svtkOpenGLShaderCache(const svtkOpenGLShaderCache&) = delete;
  void operator=(const svtkOpenGLShaderCache&) = delete;
};

#endif
