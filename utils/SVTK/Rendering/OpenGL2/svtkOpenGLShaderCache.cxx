/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLShaderCache.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLShaderCache.h"
#include "svtk_glew.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLHelper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkShader.h"
#include "svtkShaderProgram.h"

#include <cmath>
#include <sstream>

#include "svtksys/MD5.h"

class svtkOpenGLShaderCache::Private
{
public:
  svtksysMD5* md5;

  // map of hash to shader program structs
  std::map<std::string, svtkShaderProgram*> ShaderPrograms;

  Private() { md5 = svtksysMD5_New(); }

  ~Private() { svtksysMD5_Delete(this->md5); }

  //-----------------------------------------------------------------------------
  void ComputeMD5(
    const char* content, const char* content2, const char* content3, std::string& hash)
  {
    unsigned char digest[16];
    char md5Hash[33];
    md5Hash[32] = '\0';

    svtksysMD5_Initialize(this->md5);
    if (content)
    {
      svtksysMD5_Append(
        this->md5, reinterpret_cast<const unsigned char*>(content), (int)strlen(content));
    }
    if (content2)
    {
      svtksysMD5_Append(
        this->md5, reinterpret_cast<const unsigned char*>(content2), (int)strlen(content2));
    }
    if (content3)
    {
      svtksysMD5_Append(
        this->md5, reinterpret_cast<const unsigned char*>(content3), (int)strlen(content3));
    }
    svtksysMD5_Finalize(this->md5, digest);
    svtksysMD5_DigestToHex(digest, md5Hash);

    hash = md5Hash;
  }
};

// ----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOpenGLShaderCache);

// ----------------------------------------------------------------------------
svtkOpenGLShaderCache::svtkOpenGLShaderCache()
  : Internal(new Private)
{
  this->LastShaderBound = nullptr;
  this->OpenGLMajorVersion = 0;
  this->OpenGLMinorVersion = 0;
}

// ----------------------------------------------------------------------------
svtkOpenGLShaderCache::~svtkOpenGLShaderCache()
{
  typedef std::map<std::string, svtkShaderProgram*>::const_iterator SMapIter;
  SMapIter iter = this->Internal->ShaderPrograms.begin();
  for (; iter != this->Internal->ShaderPrograms.end(); ++iter)
  {
    iter->second->Delete();
  }

  delete this->Internal;
}

// perform System and Output replacements
unsigned int svtkOpenGLShaderCache::ReplaceShaderValues(
  std::string& VSSource, std::string& FSSource, std::string& GSSource)
{
  // first handle renaming any Fragment shader inputs
  // if we have a geometry shader. By default fragment shaders
  // assume their inputs come from a Vertex Shader. When we
  // have a Geometry shader we rename the frament shader inputs
  // to come from the geometry shader
  if (!GSSource.empty())
  {
    svtkShaderProgram::Substitute(FSSource, "VSOut", "GSOut");
  }

#ifdef GL_ES_VERSION_3_0
  std::string version = "#version 300 es\n";
#else
  if (!this->OpenGLMajorVersion)
  {
    this->OpenGLMajorVersion = 3;
    this->OpenGLMinorVersion = 2;
    glGetIntegerv(GL_MAJOR_VERSION, &this->OpenGLMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &this->OpenGLMinorVersion);
  }

  std::string version = "#version 150\n";
  if (this->OpenGLMajorVersion == 3 && this->OpenGLMinorVersion == 1)
  {
    version = "#version 140\n";
  }
#endif

  svtkShaderProgram::Substitute(VSSource, "//SVTK::System::Dec",
    version +
      "#ifndef GL_ES\n"
      "#define highp\n"
      "#define mediump\n"
      "#define lowp\n"
      "#endif // GL_ES\n"
      "#define attribute in\n" // to be safe
      "#define varying out\n"  // to be safe
  );

  svtkShaderProgram::Substitute(FSSource, "//SVTK::System::Dec",
    version +
      "#ifdef GL_ES\n"
      "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
      "precision highp float;\n"
      "precision highp sampler2D;\n"
      "precision highp sampler3D;\n"
      "#else\n"
      "precision mediump float;\n"
      "precision mediump sampler2D;\n"
      "precision mediump sampler3D;\n"
      "#endif\n"
      "#define texelFetchBuffer texelFetch\n"
      "#define texture1D texture\n"
      "#define texture2D texture\n"
      "#define texture3D texture\n"
      "#else // GL_ES\n"
      "#define highp\n"
      "#define mediump\n"
      "#define lowp\n"
      "#if __VERSION__ == 150\n"
      "#define texelFetchBuffer texelFetch\n"
      "#define texture1D texture\n"
      "#define texture2D texture\n"
      "#define texture3D texture\n"
      "#endif\n"
      "#endif // GL_ES\n"
      "#define varying in\n" // to be safe
  );

  svtkShaderProgram::Substitute(GSSource, "//SVTK::System::Dec",
    version +
      "#ifdef GL_ES\n"
      "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
      "precision highp float;\n"
      "#else\n"
      "precision mediump float;\n"
      "#endif\n"
      "#else // GL_ES\n"
      "#define highp\n"
      "#define mediump\n"
      "#define lowp\n"
      "#endif // GL_ES\n");

  unsigned int count = 0;
  std::string fragDecls;
  bool done = false;
  while (!done)
  {
    std::ostringstream src;
    std::ostringstream dst;
    src << "gl_FragData[" << count << "]";
    // this naming has to match the bindings
    // in svtkOpenGLShaderProgram.cxx
    dst << "fragOutput" << count;
    done = !svtkShaderProgram::Substitute(FSSource, src.str(), dst.str());
    if (!done)
    {
#ifdef GL_ES_VERSION_3_0
      src.str("");
      src.clear();
      src << count;
      fragDecls += "layout(location = " + src.str() + ") ";
#endif
      fragDecls += "out vec4 " + dst.str() + ";\n";
      count++;
    }
  }
  svtkShaderProgram::Substitute(FSSource, "//SVTK::Output::Dec", fragDecls);
  return count;
}

svtkShaderProgram* svtkOpenGLShaderCache::ReadyShaderProgram(
  std::map<svtkShader::Type, svtkShader*> shaders, svtkTransformFeedback* cap)
{
  std::string VSSource = shaders[svtkShader::Vertex]->GetSource();
  std::string FSSource = shaders[svtkShader::Fragment]->GetSource();
  std::string GSSource = shaders[svtkShader::Geometry]->GetSource();

  unsigned int count = this->ReplaceShaderValues(VSSource, FSSource, GSSource);
  shaders[svtkShader::Vertex]->SetSource(VSSource);
  shaders[svtkShader::Fragment]->SetSource(FSSource);
  shaders[svtkShader::Geometry]->SetSource(GSSource);

  svtkShaderProgram* shader = this->GetShaderProgram(shaders);
  shader->SetNumberOfOutputs(count);

  return this->ReadyShaderProgram(shader, cap);
}

// return nullptr if there is an issue
svtkShaderProgram* svtkOpenGLShaderCache::ReadyShaderProgram(const char* vertexCode,
  const char* fragmentCode, const char* geometryCode, svtkTransformFeedback* cap)
{
  // perform system wide shader replacements
  // desktops to not use precision statements
  std::string VSSource = vertexCode;
  std::string FSSource = fragmentCode;
  std::string GSSource = geometryCode;

  unsigned int count = this->ReplaceShaderValues(VSSource, FSSource, GSSource);
  svtkShaderProgram* shader =
    this->GetShaderProgram(VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
  shader->SetNumberOfOutputs(count);

  return this->ReadyShaderProgram(shader, cap);
}

// return nullptr if there is an issue
svtkShaderProgram* svtkOpenGLShaderCache::ReadyShaderProgram(
  svtkShaderProgram* shader, svtkTransformFeedback* cap)
{
  if (!shader)
  {
    return nullptr;
  }

  if (shader->GetTransformFeedback() != cap)
  {
    this->ReleaseCurrentShader();
    shader->ReleaseGraphicsResources(nullptr);
    shader->SetTransformFeedback(cap);
  }

  // compile if needed
  if (!shader->GetCompiled() && !shader->CompileShader())
  {
    return nullptr;
  }

  // bind if needed
  if (!this->BindShader(shader))
  {
    return nullptr;
  }

  return shader;
}

svtkShaderProgram* svtkOpenGLShaderCache::GetShaderProgram(
  std::map<svtkShader::Type, svtkShader*> shaders)
{
  // compute the MD5 and the check the map
  std::string result;
  this->Internal->ComputeMD5(shaders[svtkShader::Vertex]->GetSource().c_str(),
    shaders[svtkShader::Fragment]->GetSource().c_str(),
    shaders[svtkShader::Geometry]->GetSource().c_str(), result);

  // does it already exist?
  typedef std::map<std::string, svtkShaderProgram*>::const_iterator SMapIter;
  SMapIter found = this->Internal->ShaderPrograms.find(result);
  if (found == this->Internal->ShaderPrograms.end())
  {
    // create one
    svtkShaderProgram* sps = svtkShaderProgram::New();
    sps->SetVertexShader(shaders[svtkShader::Vertex]);
    sps->SetFragmentShader(shaders[svtkShader::Fragment]);
    sps->SetGeometryShader(shaders[svtkShader::Geometry]);
    sps->SetMD5Hash(result); // needed?
    this->Internal->ShaderPrograms.insert(std::make_pair(result, sps));
    return sps;
  }
  else
  {
    return found->second;
  }
}

svtkShaderProgram* svtkOpenGLShaderCache::GetShaderProgram(
  const char* vertexCode, const char* fragmentCode, const char* geometryCode)
{
  // compute the MD5 and the check the map
  std::string result;
  this->Internal->ComputeMD5(vertexCode, fragmentCode, geometryCode, result);

  // does it already exist?
  typedef std::map<std::string, svtkShaderProgram*>::const_iterator SMapIter;
  SMapIter found = this->Internal->ShaderPrograms.find(result);
  if (found == this->Internal->ShaderPrograms.end())
  {
    // create one
    svtkShaderProgram* sps = svtkShaderProgram::New();
    sps->GetVertexShader()->SetSource(vertexCode);
    sps->GetFragmentShader()->SetSource(fragmentCode);
    if (geometryCode != nullptr)
    {
      sps->GetGeometryShader()->SetSource(geometryCode);
    }
    sps->SetMD5Hash(result); // needed?
    this->Internal->ShaderPrograms.insert(std::make_pair(result, sps));
    return sps;
  }
  else
  {
    return found->second;
  }
}

void svtkOpenGLShaderCache::ReleaseGraphicsResources(svtkWindow* win)
{
  // NOTE:
  // In the current implementation as of October 26th, if a shader
  // program is created by ShaderCache then it should make sure
  // that it releases the graphics resources used by these programs.
  // It is not wisely for callers to do that since then they would
  // have to loop over all the programs were in use and invoke
  // release graphics resources individually.

  this->ReleaseCurrentShader();

  typedef std::map<std::string, svtkShaderProgram*>::const_iterator SMapIter;
  SMapIter iter = this->Internal->ShaderPrograms.begin();
  for (; iter != this->Internal->ShaderPrograms.end(); ++iter)
  {
    iter->second->ReleaseGraphicsResources(win);
  }
  this->OpenGLMajorVersion = 0;
}

void svtkOpenGLShaderCache::ReleaseCurrentShader()
{
  // release prior shader
  if (this->LastShaderBound)
  {
    this->LastShaderBound->Release();
    this->LastShaderBound = nullptr;
  }
}

int svtkOpenGLShaderCache::BindShader(svtkShaderProgram* shader)
{
  if (this->LastShaderBound != shader)
  {
    // release prior shader
    if (this->LastShaderBound)
    {
      this->LastShaderBound->Release();
    }
    shader->Bind();
    this->LastShaderBound = shader;
  }

  if (shader->IsUniformUsed("svtkElapsedTime"))
  {
    shader->SetUniformf("svtkElapsedTime", this->ElapsedTime);
  }

  return 1;
}

// ----------------------------------------------------------------------------
void svtkOpenGLShaderCache::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
