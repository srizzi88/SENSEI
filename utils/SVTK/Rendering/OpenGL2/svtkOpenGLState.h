/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLState.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLState
 * @brief   OpenGL state storage
 *
 * svtkOpenGLState is a class designed to keep track of the state of
 * an OpenGL context. Applications using SVTK have so much control
 * over the rendering process that is can be difficult in SVTK code
 * to know if the OpenGL state is correct for your code. The two
 * traditional solutions have been to set everything yourself
 * and to save and restore OpenGL state that you change. The former
 * makes your code work, the latter helps prevent your code from
 * breaking something else. The problem is that the former results
 * in tons of redundant OpenGL calls and the later is done by querying
 * the OpenGL state which can cause a pipeline sync/stall which is
 * very slow.
 *
 * To address these issues this class stores OpenGL state for commonly
 * used functions. Requests made to change state to the current
 * state become no-ops. Queries of state can be done by querying the
 * state stored in this class without impacting the OpenGL driver.
 *
 * This class is designed to hold all context related values and
 * could just as well be considered a representation of the OpenGL
 * context.
 *
 * To facilitate saving state and restoring it this class contains
 * a number of nested classes named Scoped<glFunction> that store
 * the state of that glFunction and when they go out of scope they restore
 * it. This is useful when you want to change the OpenGL state and then
 * automatically restore it when done. They can be used as follows
 *
 * {
 *   svtkOpenGLState *ostate = renWin->GetState();
 *   svtkOpenGLState::ScopedglDepthMask dmsaved(ostate);
 *   // the prior state is now saved
 *   ...
 *   ostate->glDepthMask(GL_TRUE);  // maybe change the state
 *   ... etc
 * } // prior state will be restored here as it goes out of scope
 *
 *
 * You must use this class to make state changing OpenGL class otherwise the
 * results will be undefined.
 *
 * For convenience some OpenGL calls that do not impact state are also
 * provided.
 */

#ifndef svtkOpenGLState_h
#define svtkOpenGLState_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include <array>                       // for ivar
#include <list>                        // for ivar
#include <map>                         // for ivar

class svtkOpenGLFramebufferObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLShaderCache;
class svtkOpenGLVertexBufferObjectCache;
class svtkTextureObject;
class svtkTextureUnitManager;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLState : public svtkObject
{
public:
  static svtkOpenGLState* New();
  svtkTypeMacro(svtkOpenGLState, svtkObject);

  //@{
  // cached OpenGL methods. By calling these the context will check
  // the current value prior to making the OpenGL call. This can reduce
  // the burden on the driver.
  //
  void svtkglClearColor(float red, float green, float blue, float alpha);
  void svtkglClearDepth(double depth);
  void svtkglDepthFunc(unsigned int val);
  void svtkglDepthMask(unsigned char flag);
  void svtkglColorMask(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void svtkglViewport(int x, int y, int width, int height);
  void svtkglScissor(int x, int y, int width, int height);
  void svtkglEnable(unsigned int cap);
  void svtkglDisable(unsigned int cap);
  void svtkglBlendFunc(unsigned int sfactor, unsigned int dfactor)
  {
    this->svtkglBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
  }
  void svtkglBlendFuncSeparate(unsigned int sfactorRGB, unsigned int dfactorRGB,
    unsigned int sfactorAlpha, unsigned int dfactorAlpha);
  void svtkglBlendEquation(unsigned int val);
  void svtkglBlendEquationSeparate(unsigned int col, unsigned int alpha);
  void svtkglCullFace(unsigned int val);
  void svtkglActiveTexture(unsigned int);

  void svtkglBindFramebuffer(unsigned int target, unsigned int fb);
  void svtkglDrawBuffer(unsigned int);
  void svtkglDrawBuffers(unsigned int n, unsigned int*);
  void svtkglReadBuffer(unsigned int);

  void svtkBindFramebuffer(unsigned int target, svtkOpenGLFramebufferObject* fo);
  void svtkDrawBuffers(unsigned int n, unsigned int*, svtkOpenGLFramebufferObject*);
  void svtkReadBuffer(unsigned int, svtkOpenGLFramebufferObject*);
  //@}

  //@{
  // Methods to reset the state to the current OpenGL context value.
  // These methods are useful when interfacing with third party code
  // that may have changed the opengl state.
  //
  void ResetGLClearColorState();
  void ResetGLClearDepthState();
  void ResetGLDepthFuncState();
  void ResetGLDepthMaskState();
  void ResetGLColorMaskState();
  void ResetGLViewportState();
  void ResetGLScissorState();
  void ResetGLBlendFuncState();
  void ResetGLBlendEquationState();
  void ResetGLCullFaceState();
  void ResetGLActiveTexture();
  //@}

  //@{
  // OpenGL functions that we provide an API for even though they may
  // not hold any state.
  void svtkglClear(unsigned int mask);
  //@}

  //@{
  // Get methods that can be used to query state if the state is not cached
  // they fall through and call the underlying opengl functions
  void svtkglGetBooleanv(unsigned int pname, unsigned char* params);
  void svtkglGetIntegerv(unsigned int pname, int* params);
  void svtkglGetDoublev(unsigned int pname, double* params);
  void svtkglGetFloatv(unsigned int pname, float* params);
  //@}

  // convenience to get all 4 values at once
  void GetBlendFuncState(int*);

  // convenience to return a bool
  // as opposed to a unsigned char
  bool GetEnumState(unsigned int name);

  // convenience method to set a enum (glEnable/glDisable)
  void SetEnumState(unsigned int name, bool value);

  /**
   * convenience method to reset an enum state from current openGL context
   */
  void ResetEnumState(unsigned int name);

  // superclass for Scoped subclasses
  template <typename T>
  class SVTKRENDERINGOPENGL2_EXPORT ScopedValue
  {
  public:
    ~ScopedValue() // restore value
    {
      ((*this->State).*(this->Method))(this->Value);
    }

  protected:
    svtkOpenGLState* State;
    T Value;
    void (svtkOpenGLState::*Method)(T);
  };

  /**
   * Activate a texture unit for this texture
   */
  void ActivateTexture(svtkTextureObject*);

  /**
   * Deactivate a previously activated texture
   */
  void DeactivateTexture(svtkTextureObject*);

  /**
   * Get the texture unit for a given texture object
   */
  int GetTextureUnitForTexture(svtkTextureObject*);

  /**
   * Check to make sure no textures have been left active
   */
  void VerifyNoActiveTextures();

  //@{
  /**
   * Store/Restore the current framebuffer bindings and buffers.
   */
  void PushFramebufferBindings()
  {
    this->PushDrawFramebufferBinding();
    this->PushReadFramebufferBinding();
  }
  void PushDrawFramebufferBinding();
  void PushReadFramebufferBinding();

  void PopFramebufferBindings()
  {
    this->PopReadFramebufferBinding();
    this->PopDrawFramebufferBinding();
  }
  void PopDrawFramebufferBinding();
  void PopReadFramebufferBinding();

  void ResetFramebufferBindings();
  //@}

  // Scoped classes you can use to save state
  class SVTKRENDERINGOPENGL2_EXPORT ScopedglDepthMask : public ScopedValue<unsigned char>
  {
  public:
    ScopedglDepthMask(svtkOpenGLState* state);
  };
  class SVTKRENDERINGOPENGL2_EXPORT ScopedglClearColor : public ScopedValue<std::array<float, 4> >
  {
  public:
    ScopedglClearColor(svtkOpenGLState* state);
  };
  class SVTKRENDERINGOPENGL2_EXPORT ScopedglColorMask
    : public ScopedValue<std::array<unsigned char, 4> >
  {
  public:
    ScopedglColorMask(svtkOpenGLState* state);
  };
  class SVTKRENDERINGOPENGL2_EXPORT ScopedglScissor : public ScopedValue<std::array<int, 4> >
  {
  public:
    ScopedglScissor(svtkOpenGLState* state);
  };
  class SVTKRENDERINGOPENGL2_EXPORT ScopedglViewport : public ScopedValue<std::array<int, 4> >
  {
  public:
    ScopedglViewport(svtkOpenGLState* state);
  };
  class SVTKRENDERINGOPENGL2_EXPORT ScopedglBlendFuncSeparate
    : public ScopedValue<std::array<unsigned int, 4> >
  {
  public:
    ScopedglBlendFuncSeparate(svtkOpenGLState* state);
  };
  class SVTKRENDERINGOPENGL2_EXPORT ScopedglDepthFunc : public ScopedValue<unsigned int>
  {
  public:
    ScopedglDepthFunc(svtkOpenGLState* state);
  };
  class SVTKRENDERINGOPENGL2_EXPORT ScopedglActiveTexture : public ScopedValue<unsigned int>
  {
  public:
    ScopedglActiveTexture(svtkOpenGLState* state);
  };

  class ScopedglEnableDisable
  {
  public:
    ScopedglEnableDisable(svtkOpenGLState* state, unsigned int name)
    {
      this->State = state;
      this->Name = name;
      unsigned char val;
      this->State->svtkglGetBooleanv(name, &val);
      this->Value = val == 1;
    }
    ~ScopedglEnableDisable() // restore value
    {
      this->State->SetEnumState(this->Name, this->Value);
    }

  protected:
    svtkOpenGLState* State;
    unsigned int Name;
    bool Value;
  };

  /**
   * Initialize OpenGL context using current state
   */
  void Initialize(svtkOpenGLRenderWindow*);

  /**
   * Set the texture unit manager.
   */
  void SetTextureUnitManager(svtkTextureUnitManager* textureUnitManager);

  /**
   * Returns its texture unit manager object. A new one will be created if one
   * hasn't already been set up.
   */
  svtkTextureUnitManager* GetTextureUnitManager();

  // get the shader program cache for this context
  svtkGetObjectMacro(ShaderCache, svtkOpenGLShaderCache);

  // get the vbo buffer cache for this context
  svtkGetObjectMacro(VBOCache, svtkOpenGLVertexBufferObjectCache);

  // set the VBO Cache to use for this state
  // this allows two contexts to share VBOs
  // basically this is OPenGL's support for shared
  // lists
  void SetVBOCache(svtkOpenGLVertexBufferObjectCache* val);

  /**
   * Get a mapping of svtk data types to native texture formats for this window
   * we put this on the RenderWindow so that every texture does not have to
   * build these structures themselves
   */
  int GetDefaultTextureInternalFormat(
    int svtktype, int numComponents, bool needInteger, bool needFloat, bool needSRGB);

protected:
  svtkOpenGLState(); // set initial values
  ~svtkOpenGLState() override;

  void BlendFuncSeparate(std::array<unsigned int, 4> val);
  void ClearColor(std::array<float, 4> val);
  void ColorMask(std::array<unsigned char, 4> val);
  void Scissor(std::array<int, 4> val);
  void Viewport(std::array<int, 4> val);

  int TextureInternalFormats[SVTK_UNICODE_STRING][3][5];
  void InitializeTextureInternalFormats();

  svtkTextureUnitManager* TextureUnitManager;
  std::map<const svtkTextureObject*, int> TextureResourceIds;

  /**
   * Check that this OpenGL state has consistent values
   * with the current OpenGL context
   */
  void CheckState();

  // framebuffers hold state themselves
  // specifically they hold their draw and read buffers
  // and when bound they reinstate those buffers
  class SVTKRENDERINGOPENGL2_EXPORT BufferBindingState
  {
  public:
    BufferBindingState();
    // bool operator==(const BufferBindingState& a, const BufferBindingState& b);
    // either this holds a svtkOpenGLFramebufferObject
    svtkOpenGLFramebufferObject* Framebuffer;
    // or the handle to an unknown OpenGL FO
    unsigned int Binding;
    unsigned int ReadBuffer;
    unsigned int DrawBuffers[10];
    unsigned int GetBinding();
    unsigned int GetDrawBuffer(unsigned int);
    unsigned int GetReadBuffer();
  };
  std::list<BufferBindingState> DrawBindings;
  std::list<BufferBindingState> ReadBindings;

  class SVTKRENDERINGOPENGL2_EXPORT GLState
  {
  public:
    double ClearDepth;
    unsigned char DepthMask;
    unsigned int DepthFunc;
    unsigned int BlendEquationValue1;
    unsigned int BlendEquationValue2;
    unsigned int CullFaceMode;
    unsigned int ActiveTexture;
    std::array<float, 4> ClearColor;
    std::array<unsigned char, 4> ColorMask;
    std::array<int, 4> Viewport;
    std::array<int, 4> Scissor;
    std::array<unsigned int, 4> BlendFunc;
    bool DepthTest;
    bool CullFace;
    bool ScissorTest;
    bool StencilTest;
    bool Blend;
    bool MultiSample;
    int MaxTextureSize;
    int MajorVersion;
    int MinorVersion;
    BufferBindingState DrawBinding;
    BufferBindingState ReadBinding;
    GLState() {}
  };

  GLState CurrentState;

  svtkOpenGLVertexBufferObjectCache* VBOCache;
  svtkOpenGLShaderCache* ShaderCache;

private:
  svtkOpenGLState(const svtkOpenGLState&) = delete;
  void operator=(const svtkOpenGLState&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkOpenGLState.h
