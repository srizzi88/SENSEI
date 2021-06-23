/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkShaderProgram
 * @brief   a glsl shader program
 *
 * This class contains the vertex, fragment, geometry shaders that combine to make a shader program
 */

#ifndef svtkShaderProgram_h
#define svtkShaderProgram_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // for export macro

#include <map>    // For member variables.
#include <string> // For member variables.

class svtkMatrix3x3;
class svtkMatrix4x4;
class svtkTransformFeedback;
class svtkShader;
class VertexArrayObject;
class svtkWindow;

/**
 * @brief The ShaderProgram uses one or more Shader objects.
 *
 * This class creates a Vertex or Fragment shader, that can be attached to a
 * ShaderProgram in order to render geometry etc.
 */

class SVTKRENDERINGOPENGL2_EXPORT svtkShaderProgram : public svtkObject
{
public:
  static svtkShaderProgram* New();
  svtkTypeMacro(svtkShaderProgram, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the vertex shader for this program
   */
  svtkGetObjectMacro(VertexShader, svtkShader);
  void SetVertexShader(svtkShader*);
  //@}

  //@{
  /**
   * Get the fragment shader for this program
   */
  svtkGetObjectMacro(FragmentShader, svtkShader);
  void SetFragmentShader(svtkShader*);
  //@}

  //@{
  /**
   * Get the geometry shader for this program
   */
  svtkGetObjectMacro(GeometryShader, svtkShader);
  void SetGeometryShader(svtkShader*);
  //@}

  //@{
  /**
   * Get/Set a TransformFeedbackCapture object on this shader program.
   */
  svtkGetObjectMacro(TransformFeedback, svtkTransformFeedback);
  void SetTransformFeedback(svtkTransformFeedback* tfc);
  //@}

  //@{
  /**
   * Set/Get flag for if this program is compiled
   */
  svtkGetMacro(Compiled, bool);
  svtkSetMacro(Compiled, bool);
  svtkBooleanMacro(Compiled, bool);
  //@}

  /**
   * Set/Get the md5 hash of this program
   */
  std::string GetMD5Hash() const { return this->MD5Hash; }
  void SetMD5Hash(const std::string& hash) { this->MD5Hash = hash; }

  /** Options for attribute normalization. */
  enum NormalizeOption
  {
    /// The values range across the limits of the numeric type.
    /// This option instructs the rendering engine to normalize them to
    /// the range [0.0, 1.0] for unsigned types, and [-1.0, 1.0] for signed
    /// types.
    /// For example, unsigned char values will be mapped so that 0 = 0.0,
    /// and 255 = 1.0.
    /// The resulting floating point numbers will be passed into
    /// the shader program.
    Normalize,
    /// The values should be used as-is. Do not perform any normalization.
    NoNormalize
  };

  /**
   * Check if the program is currently bound, or not.
   * @return True if the program is bound, false otherwise.
   */
  bool isBound() const { return this->Bound; }

  /**
   * release any graphics resources this class is using.
   */
  void ReleaseGraphicsResources(svtkWindow* win);

  /** Get the handle of the shader program. */
  int GetHandle() const { return Handle; }

  /** Get the error message (empty if none) for the shader program. */
  std::string GetError() const { return Error; }

  /**
   * Enable the named attribute array. Return false if the attribute array is
   * not contained in the linked shader program.
   */
  bool EnableAttributeArray(const char* name);

  /**
   * Disable the named attribute array. Return false if the attribute array is
   * not contained in the linked shader program.
   */
  bool DisableAttributeArray(const char* name);

  /**
   * Use the named attribute array with the bound BufferObject.
   * @param name of the attribute (as seen in the shader program).
   * @param offset into the bound BufferObject.
   * @param stride The stride of the element access (i.e. the size of each
   * element in the currently bound BufferObject). 0 may be used to indicate
   * tightly packed data.
   * @param elementType Tag identifying the memory representation of the
   * element.
   * @param elementTupleSize The number of elements per vertex (e.g. a 3D
   * position attribute would be 3).
   * @param normalize Indicates the range used by the attribute data.
   * See NormalizeOption for more information.
   * @return false if the attribute array does not exist.
   */
  bool UseAttributeArray(const char* name, int offset, size_t stride, int elementType,
    int elementTupleSize, NormalizeOption normalize);

  /**
   * Upload the supplied array of tightly packed values to the named attribute.
   * BufferObject attributes should be preferred and this may be removed in
   * future.
   *
   * @param name Attribute name
   * @param array Container of data. See note.
   * @param tupleSize The number of elements per vertex, e.g. a 3D coordinate
   * array will have a tuple size of 3.
   * @param  normalize Indicates the range used by the attribute data.
   * See NormalizeOption for more information.
   *
   * @note The T type must have tightly packed values of
   * T::value_type accessible by reference via T::operator[].
   * Additionally, the standard size() and empty() methods must be implemented.
   * The std::vector classes is an example of such a container.
   */
  template <class T>
  bool SetAttributeArray(
    const char* name, const T& array, int tupleSize, NormalizeOption normalize);

  /** Set the @p name uniform value to int @p v. */
  bool SetUniformi(const char* name, int v);
  bool SetUniformf(const char* name, float v);
  bool SetUniform2i(const char* name, const int v[2]);
  bool SetUniform2f(const char* name, const float v[2]);
  bool SetUniform3f(const char* name, const float v[3]);
  bool SetUniform3f(const char* name, const double v[3]);
  bool SetUniform4f(const char* name, const float v[4]);
  bool SetUniform3uc(const char* name, const unsigned char v[3]); // maybe remove
  bool SetUniform4uc(const char* name, const unsigned char v[4]); // maybe remove
  bool SetUniformMatrix(const char* name, svtkMatrix3x3* v);
  bool SetUniformMatrix(const char* name, svtkMatrix4x4* v);
  bool SetUniformMatrix3x3(const char* name, float* v);
  bool SetUniformMatrix4x4(const char* name, float* v);

  /** Set the @p name uniform array to @p f with @p count elements */
  bool SetUniform1iv(const char* name, const int count, const int* f);
  bool SetUniform1fv(const char* name, const int count, const float* f);
  bool SetUniform2fv(const char* name, const int count, const float* f);
  bool SetUniform2fv(const char* name, const int count, const float (*f)[2]);
  bool SetUniform3fv(const char* name, const int count, const float* f);
  bool SetUniform3fv(const char* name, const int count, const float (*f)[3]);
  bool SetUniform4fv(const char* name, const int count, const float* f);
  bool SetUniform4fv(const char* name, const int count, const float (*f)[4]);
  bool SetUniformMatrix4x4v(const char* name, const int count, float* v);

  // How many outputs does this program produce
  // only valid for OpenGL 3.2 or later
  svtkSetMacro(NumberOfOutputs, unsigned int);

  /**
   * perform in place string substitutions, indicate if a substitution was done
   * this is useful for building up shader strings which typically involve
   * lots of string substitutions.
   *
   * \param[in] shader  The source shader object to perform substitutions on
   * \param[in] search  The string to search for
   * \param[in] replace The string replacement
   * \param[in] all     Whether to replace all matches or just the first one
   * \return    A boolean indicating whether the replacement was successful
   */
  static bool Substitute(
    std::string& source, const std::string& search, const std::string& replace, bool all = true);

  /**
   * Perform in-place string substitutions on the shader source string and
   * indicate if one or all substitutions were done. This is useful for building
   * up shader strings which typically involve a lot of string substitutions.
   *
   * \param[in] shader  The source shader object to perform substitutions on
   * \param[in] search  The string to search for
   * \param[in] replace The string replacement
   * \param[in] all     Whether to replace all matches or just the first one
   * \return    A boolean indicating whether the replacement was successful
   */
  static bool Substitute(
    svtkShader* shader, const std::string& search, const std::string& replace, bool all = true);

  /**
   * methods to inquire as to what uniforms/attributes are used by
   * this shader.  This can save some compute time if the uniforms
   * or attributes are expensive to compute
   */
  bool IsUniformUsed(const char*);

  /**
   * Return true if the compiled and linked shader has an attribute matching @a
   * name.
   */
  bool IsAttributeUsed(const char* name);

  // maps of std::string are super slow when calling find
  // with a string literal or const char * as find
  // forces construction/copy/destruction of a
  // std::string copy of the const char *
  // In spite of the doubters this can really be a
  // huge CPU hog.
  struct cmp_str
  {
    bool operator()(const char* a, const char* b) const { return strcmp(a, b) < 0; }
  };

  //@{
  /**
   * When developing shaders, it's often convenient to tweak the shader and
   * re-render incrementally. This provides a mechanism to do the same. To debug
   * any shader program, set `FileNamePrefixForDebugging` to a file path e.g.
   * `/tmp/myshaders`. Subsequently, when `Bind()` is called on the shader
   * program, it will check for files named `<FileNamePrefixForDebugging>VS.glsl`,
   * `<FileNamePrefixForDebugging>GS.glsl` and `<FileNamePrefixForDebugging>FS.glsl` for
   * vertex shader, geometry shader and fragment shader codes respectively. If
   * a file doesn't exist, then it dumps out the current code to that file.
   * If the file exists, then the shader is recompiled to use the contents of that file.
   * Thus, after the files have been dumped in the first render, you can open the files
   * in a text editor and update as needed. On following render, the modified
   * contexts from the file will be used.
   *
   * This is only intended for debugging during development and should not be
   * used in production.
   */
  svtkSetStringMacro(FileNamePrefixForDebugging);
  svtkGetStringMacro(FileNamePrefixForDebugging);
  //@}

  //@{
  /**
   * Set/Get times that can be used to track when a set of
   * uniforms was last updated. This can be used to reduce
   * redundant SetUniformCalls
   */
  enum UniformGroups
  {
    CameraGroup,
    LightingGroup,
    UserGroup, // always will be last
  };
  void SetUniformGroupUpdateTime(int, svtkMTimeType tm);
  svtkMTimeType GetUniformGroupUpdateTime(int);
  //@}

  // returns the location for a uniform or attribute in
  // this program. Is cached for performance.
  int FindUniform(const char* name);
  int FindAttributeArray(const char* name);

protected:
  svtkShaderProgram();
  ~svtkShaderProgram() override;

  /***************************************************************
   * The following functions are only for use by the shader cache
   * which is why they are protected and that class is a friend
   * you need to use the shader cache to compile/link/bind your shader
   * do not try to do it yourself as it will screw up the cache
   ***************************************************************/
  friend class svtkOpenGLShaderCache;

  /**
   * Attach the supplied shader to this program.
   * @note A maximum of one Vertex shader and one Fragment shader can be
   * attached to a shader program.
   * @return true on success.
   */
  bool AttachShader(const svtkShader* shader);

  /** Detach the supplied shader from this program.
   * @note A maximum of one Vertex shader and one Fragment shader can be
   * attached to a shader program.
   * @return true on success.
   */
  bool DetachShader(const svtkShader* shader);

  /**
   * Compile this shader program and attached shaders
   */
  virtual int CompileShader();

  /**
   * Attempt to link the shader program.
   * @return false on failure. Query error to get the reason.
   * @note The shaders attached to the program must have been compiled.
   */
  bool Link();

  /**
   * Bind the program in order to use it. If the program has not been linked
   * then link() will be called.
   */
  bool Bind();

  /** Releases the shader program from the current context. */
  void Release();

  /************* end **************************************/

  svtkShader* VertexShader;
  svtkShader* FragmentShader;
  svtkShader* GeometryShader;
  svtkTransformFeedback* TransformFeedback;

  // hash of the shader program
  std::string MD5Hash;

  bool SetAttributeArrayInternal(
    const char* name, void* buffer, int type, int tupleSize, NormalizeOption normalize);
  int Handle;
  int VertexShaderHandle;
  int FragmentShaderHandle;
  int GeometryShaderHandle;

  bool Linked;
  bool Bound;
  bool Compiled;

  // for glsl 1.5 or later, how many outputs
  // does this shader create
  // they will be bound in order to
  // fragOutput0 fragOutput1 etc...
  unsigned int NumberOfOutputs;

  std::string Error;

  // since we are using const char * arrays we have to
  // free our memory :-)
  void ClearMaps();
  std::map<const char*, int, cmp_str> AttributeLocs;
  std::map<const char*, int, cmp_str> UniformLocs;

  std::map<int, svtkMTimeType> UniformGroupMTimes;

  friend class VertexArrayObject;

private:
  svtkShaderProgram(const svtkShaderProgram&) = delete;
  void operator=(const svtkShaderProgram&) = delete;

  char* FileNamePrefixForDebugging;
};

#endif
