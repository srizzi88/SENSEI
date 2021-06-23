/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextureObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextureObject
 * @brief   abstracts an OpenGL texture object.
 *
 * svtkTextureObject represents an OpenGL texture object. It provides API to
 * create textures using data already loaded into pixel buffer objects. It can
 * also be used to create textures without uploading any data.
 */

#ifndef svtkTextureObject_h
#define svtkTextureObject_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkWeakPointer.h"            // for render context

class svtkOpenGLBufferObject;
class svtkOpenGLHelper;
class svtkOpenGLRenderWindow;
class svtkOpenGLVertexArrayObject;
class svtkPixelBufferObject;
class svtkShaderProgram;
class svtkWindow;
class svtkGenericOpenGLResourceFreeCallback;

class SVTKRENDERINGOPENGL2_EXPORT svtkTextureObject : public svtkObject
{
public:
  // DepthTextureCompareFunction values.
  enum
  {
    Lequal = 0, // r=R<=Dt ? 1.0 : 0.0
    Gequal,     // r=R>=Dt ? 1.0 : 0.0
    Less,       // r=R<D_t ? 1.0 : 0.0
    Greater,    // r=R>Dt ? 1.0 : 0.0
    Equal,      // r=R==Dt ? 1.0 : 0.0
    NotEqual,   // r=R!=Dt ? 1.0 : 0.0
    AlwaysTrue, //  r=1.0 // WARNING "Always" is macro defined in X11/X.h...
    Never,      // r=0.0
    NumberOfDepthTextureCompareFunctions
  };

// ClampToBorder is not supported in ES 2.0
// Wrap values.
#ifndef GL_ES_VERSION_3_0
  enum { ClampToEdge = 0, Repeat, MirroredRepeat, ClampToBorder, NumberOfWrapModes };
#else
  enum
  {
    ClampToEdge = 0,
    Repeat,
    MirroredRepeat,
    NumberOfWrapModes
  };
#endif

  // MinificationFilter values.
  enum
  {
    Nearest = 0,
    Linear,
    NearestMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapNearest,
    LinearMipmapLinear,
    NumberOfMinificationModes
  };

  // depth/color format
  enum
  {
    Native = 0, // will try to match with the depth buffer format.
    Fixed8,
    Fixed16,
    Fixed24,
    Fixed32,
    Float16,
    Float32,
    NumberOfDepthFormats
  };

  static svtkTextureObject* New();
  svtkTypeMacro(svtkTextureObject, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the context. This does not increase the reference count of the
   * context to avoid reference loops.

   * {
   * this->TextureObject = svtkTextureObject::New();
   * }SetContext() may raise an error is the OpenGL context does not support the
   * required OpenGL extensions.
   */
  void SetContext(svtkOpenGLRenderWindow*);
  svtkOpenGLRenderWindow* GetContext();
  //@}

  //@{
  /**
   * Get the texture dimensions.
   * These are the properties of the OpenGL texture this instance represents.
   */
  svtkGetMacro(Width, unsigned int);
  svtkGetMacro(Height, unsigned int);
  svtkGetMacro(Depth, unsigned int);
  svtkGetMacro(Samples, unsigned int);
  svtkGetMacro(Components, int);
  unsigned int GetTuples() { return this->Width * this->Height * this->Depth; }
  //@}

  svtkGetMacro(NumberOfDimensions, int);

  // for MSAA textures set the number of samples
  svtkSetMacro(Samples, unsigned int);

  //@{
  /**
   * Returns OpenGL texture target to which the texture is/can be bound.
   */
  svtkGetMacro(Target, unsigned int);
  //@}

  //@{
  /**
   * Returns the OpenGL handle.
   */
  svtkGetMacro(Handle, unsigned int);
  //@}

  /**
   * Return the texture unit used for this texture
   */
  int GetTextureUnit();

  //@{
  /**
   * Bind the texture, must have been created using Create().
   * A side affect is that tex parameters are sent.
   * RenderWindow must be set before calling this.
   */
  void Bind();
  //@}

  /**
   * Activate and Bind the texture
   */
  virtual void Activate();

  /**
   * Deactivate and UnBind the texture
   */
  void Deactivate();

  /**
   * Deactivate and UnBind the texture
   */
  virtual void ReleaseGraphicsResources(svtkWindow* win);

  /**
   * Tells if the texture object is bound to the active texture image unit.
   * (a texture object can be bound to multiple texture image unit).
   */
  bool IsBound();

  /**
   * Send all the texture object parameters to the hardware if not done yet.
   * Parameters are automatically sent as a side affect of Bind. Disable
   * this by setting AutoParameters 0.
   * \pre is_bound: IsBound()
   */
  void SendParameters();

  //@{
  /**
   * Get/Set AutoParameters flag.
   * When enabled, SendParameters method is called automatically when the texture is bound.
   */
  svtkSetMacro(AutoParameters, int);
  svtkGetMacro(AutoParameters, int);
  svtkBooleanMacro(AutoParameters, int);
  //@}

  /**
   * Create a 2D texture from client memory
   * numComps must be in [1-4].
   */
  bool Create2DFromRaw(
    unsigned int width, unsigned int height, int numComps, int dataType, void* data);

  /**
   * Create a 2D depth texture using a raw pointer.
   * This is a blocking call. If you can, use PBO instead.
   * raw can be null in order to allocate texture without initialization.
   */
  bool CreateDepthFromRaw(
    unsigned int width, unsigned int height, int internalFormat, int rawType, void* raw);

  /**
   * Create a texture buffer basically a 1D texture that can be
   * very large for passing data into the fragment shader
   */
  bool CreateTextureBuffer(
    unsigned int numValues, int numComps, int dataType, svtkOpenGLBufferObject* bo);

  /**
   * Create a cube texture from 6 buffers from client memory.
   * Image data must be provided in the following order: +X -X +Y -Y +Z -Z.
   * numComps must be in [1-4].
   */
  bool CreateCubeFromRaw(
    unsigned int width, unsigned int height, int numComps, int dataType, void* data[6]);

// 1D  textures are not supported in ES 2.0 or 3.0
#ifndef GL_ES_VERSION_3_0

  /**
   * Create a 1D texture using the PBO.
   * Eventually we may start supporting creating a texture from subset of data
   * in the PBO, but for simplicity we'll begin with entire PBO data.
   * numComps must be in [1-4].
   * shaderSupportsTextureInt is true if the shader has an alternate
   * implementation supporting sampler with integer values.
   * Even if the card supports texture int, it does not mean that
   * the implementor of the shader made a version that supports texture int.
   */
  bool Create1D(int numComps, svtkPixelBufferObject* pbo, bool shaderSupportsTextureInt);

  /**
   * Create 1D texture from client memory
   */
  bool Create1DFromRaw(unsigned int width, int numComps, int dataType, void* data);
#endif

  /**
   * Create a 2D texture using the PBO.
   * Eventually we may start supporting creating a texture from subset of data
   * in the PBO, but for simplicity we'll begin with entire PBO data.
   * numComps must be in [1-4].
   */
  bool Create2D(unsigned int width, unsigned int height, int numComps, svtkPixelBufferObject* pbo,
    bool shaderSupportsTextureInt);

  /**
   * Create a 3D texture using the PBO.
   * Eventually we may start supporting creating a texture from subset of data
   * in the PBO, but for simplicity we'll begin with entire PBO data.
   * numComps must be in [1-4].
   */
  bool Create3D(unsigned int width, unsigned int height, unsigned int depth, int numComps,
    svtkPixelBufferObject* pbo, bool shaderSupportsTextureInt);

  /**
   * Create a 3D texture from client memory
   * numComps must be in [1-4].
   */
  bool Create3DFromRaw(unsigned int width, unsigned int height, unsigned int depth, int numComps,
    int dataType, void* data);

  /**
   * Create a 3D texture using the GL_PROXY_TEXTURE_3D target.  This serves
   * as a pre-allocation step which assists in verifying that the size
   * of the texture to be created is supported by the implementation and that
   * there is sufficient texture memory available for it.
   */
  bool AllocateProxyTexture3D(unsigned int const width, unsigned int const height,
    unsigned int const depth, int const numComps, int const dataType);

  /**
   * This is used to download raw data from the texture into a pixel buffer. The
   * pixel buffer API can then be used to download the pixel buffer data to CPU
   * arrays. The caller takes on the responsibility of deleting the returns
   * svtkPixelBufferObject once it done with it.
   */
  svtkPixelBufferObject* Download();
  svtkPixelBufferObject* Download(unsigned int target, unsigned int level);

  /**
   * Create a 2D depth texture using a PBO.
   * \pre: valid_internalFormat: internalFormat>=0 && internalFormat<NumberOfDepthFormats
   */
  bool CreateDepth(
    unsigned int width, unsigned int height, int internalFormat, svtkPixelBufferObject* pbo);

  /**
   * Create a 2D depth texture but does not initialize its values.
   */
  bool AllocateDepth(unsigned int width, unsigned int height, int internalFormat);

  /**
   * Create a 2D septh stencil texture but does not initialize its values.
   */
  bool AllocateDepthStencil(unsigned int width, unsigned int height);

  /**
   * Create a 1D color texture but does not initialize its values.
   * Internal format is deduced from numComps and svtkType.
   */
  bool Allocate1D(unsigned int width, int numComps, int svtkType);

  /**
   * Create a 2D color texture but does not initialize its values.
   * Internal format is deduced from numComps and svtkType.
   */
  bool Allocate2D(
    unsigned int width, unsigned int height, int numComps, int svtkType, int level = 0);

  /**
   * Create a 3D color texture but does not initialize its values.
   * Internal format is deduced from numComps and svtkType.
   */
  bool Allocate3D(
    unsigned int width, unsigned int height, unsigned int depth, int numComps, int svtkType);

  //@{
  /**
   * Create texture without uploading any data.
   */
  bool Create2D(unsigned int width, unsigned int height, int numComps, int svtktype, bool)
  {
    return this->Allocate2D(width, height, numComps, svtktype);
  }
  bool Create3D(
    unsigned int width, unsigned int height, unsigned int depth, int numComps, int svtktype, bool)
  {
    return this->Allocate3D(width, height, depth, numComps, svtktype);
  }
  //@}

  /**
   * Get the data type for the texture as a svtk type int i.e. SVTK_INT etc.
   */
  int GetSVTKDataType();

  //@{
  /**
   * Get the data type for the texture as GLenum type.
   */
  int GetDataType(int svtk_scalar_type);
  void SetDataType(unsigned int glType);
  int GetDefaultDataType(int svtk_scalar_type);
  //@}

  //@{
  /**
   * Get/Set internal format (OpenGL internal format) that should
   * be used.
   * (https://www.opengl.org/sdk/docs/man2/xhtml/glTexImage2D.xml)
   */
  unsigned int GetInternalFormat(int svtktype, int numComps, bool shaderSupportsTextureInt);
  void SetInternalFormat(unsigned int glInternalFormat);
  unsigned int GetDefaultInternalFormat(int svtktype, int numComps, bool shaderSupportsTextureInt);
  //@}

  //@{
  /**
   * Get/Set format (OpenGL internal format) that should
   * be used.
   * (https://www.opengl.org/sdk/docs/man2/xhtml/glTexImage2D.xml)
   */
  unsigned int GetFormat(int svtktype, int numComps, bool shaderSupportsTextureInt);
  void SetFormat(unsigned int glFormat);
  unsigned int GetDefaultFormat(int svtktype, int numComps, bool shaderSupportsTextureInt);
  //@}

  /**
   * Reset format, internal format, and type of the texture.

   * This method is useful when a texture is reused in a
   * context same as the previous render call. In such
   * cases, texture destruction does not happen and therefore
   * previous set values are used.
   */
  void ResetFormatAndType();

  unsigned int GetMinificationFilterMode(int svtktype);
  unsigned int GetMagnificationFilterMode(int svtktype);
  unsigned int GetWrapSMode(int svtktype);
  unsigned int GetWrapTMode(int svtktype);
  unsigned int GetWrapRMode(int svtktype);

  //@{
  /**
   * Optional, require support for floating point depth buffer
   * formats. If supported extensions will be loaded, however
   * loading will fail if the extension is required but not
   * available.
   */
  svtkSetMacro(RequireDepthBufferFloat, bool);
  svtkGetMacro(RequireDepthBufferFloat, bool);
  svtkGetMacro(SupportsDepthBufferFloat, bool);
  //@}

  //@{
  /**
   * Optional, require support for floating point texture
   * formats. If supported extensions will be loaded, however
   * loading will fail if the extension is required but not
   * available.
   */
  svtkSetMacro(RequireTextureFloat, bool);
  svtkGetMacro(RequireTextureFloat, bool);
  svtkGetMacro(SupportsTextureFloat, bool);
  //@}

  //@{
  /**
   * Optional, require support for integer texture
   * formats. If supported extensions will be loaded, however
   * loading will fail if the extension is required but not
   * available.
   */
  svtkSetMacro(RequireTextureInteger, bool);
  svtkGetMacro(RequireTextureInteger, bool);
  svtkGetMacro(SupportsTextureInteger, bool);
  //@}

  //@{
  /**
   * Wrap mode for the first texture coordinate "s"
   * Valid values are:
   * - Clamp
   * - ClampToEdge
   * - Repeat
   * - ClampToBorder
   * - MirroredRepeat
   * Initial value is Repeat (as in OpenGL spec)
   */
  svtkGetMacro(WrapS, int);
  svtkSetMacro(WrapS, int);
  //@}

  //@{
  /**
   * Wrap mode for the first texture coordinate "t"
   * Valid values are:
   * - Clamp
   * - ClampToEdge
   * - Repeat
   * - ClampToBorder
   * - MirroredRepeat
   * Initial value is Repeat (as in OpenGL spec)
   */
  svtkGetMacro(WrapT, int);
  svtkSetMacro(WrapT, int);
  //@}

  //@{
  /**
   * Wrap mode for the first texture coordinate "r"
   * Valid values are:
   * - Clamp
   * - ClampToEdge
   * - Repeat
   * - ClampToBorder
   * - MirroredRepeat
   * Initial value is Repeat (as in OpenGL spec)
   */
  svtkGetMacro(WrapR, int);
  svtkSetMacro(WrapR, int);
  //@}

  //@{
  /**
   * Minification filter mode.
   * Valid values are:
   * - Nearest
   * - Linear
   * - NearestMipmapNearest
   * - NearestMipmapLinear
   * - LinearMipmapNearest
   * - LinearMipmapLinear
   * Initial value is Nearest (note initial value in OpenGL spec
   * is NearestMipMapLinear but this is error-prone because it makes the
   * texture object incomplete. ).
   */
  svtkGetMacro(MinificationFilter, int);
  svtkSetMacro(MinificationFilter, int);
  //@}

  //@{
  /**
   * Magnification filter mode.
   * Valid values are:
   * - Nearest
   * - Linear
   * Initial value is Nearest
   */
  svtkGetMacro(MagnificationFilter, int);
  svtkSetMacro(MagnificationFilter, int);
  //@}

  /**
   * Tells if the magnification mode is linear (true) or nearest (false).
   * Initial value is false (initial value in OpenGL spec is true).
   */
  void SetLinearMagnification(bool val) { this->SetMagnificationFilter(val ? Linear : Nearest); }

  bool GetLinearMagnification() { return this->MagnificationFilter == Linear; }

  //@{
  /**
   * Border Color (RGBA). The values can be any valid float value,
   * if the gpu supports it. Initial value is (0.0f, 0.0f, 0.0f, 0.0f),
   * as in the OpenGL spec.
   */
  svtkSetVector4Macro(BorderColor, float);
  svtkGetVector4Macro(BorderColor, float);
  //@}

  //@{
  /**
   * Lower-clamp the computed LOD against this value. Any float value is valid.
   * Initial value is -1000.0f, as in OpenGL spec.
   */
  svtkSetMacro(MinLOD, float);
  svtkGetMacro(MinLOD, float);
  //@}

  //@{
  /**
   * Upper-clamp the computed LOD against this value. Any float value is valid.
   * Initial value is 1000.0f, as in OpenGL spec.
   */
  svtkSetMacro(MaxLOD, float);
  svtkGetMacro(MaxLOD, float);
  //@}

  //@{
  /**
   * Level of detail of the first texture image. A texture object is a list of
   * texture images. It is a non-negative integer value.
   * Initial value is 0, as in OpenGL spec.
   */
  svtkSetMacro(BaseLevel, int);
  svtkGetMacro(BaseLevel, int);
  //@}

  //@{
  /**
   * Level of detail of the first texture image. A texture object is a list of
   * texture images. It is a non-negative integer value.
   * Initial value is 1000, as in OpenGL spec.
   */
  svtkSetMacro(MaxLevel, int);
  svtkGetMacro(MaxLevel, int);
  //@}

  //@{
  /**
   * Tells if the output of a texture unit with a depth texture uses
   * comparison or not.
   * Comparison happens between D_t the depth texture value in the range [0,1]
   * and with R the interpolated third texture coordinate clamped to range
   * [0,1]. The result of the comparison is noted `r'. If this flag is false,
   * r=D_t.
   * Initial value is false, as in OpenGL spec.
   * Ignored if the texture object is not a depth texture.
   */
  svtkGetMacro(DepthTextureCompare, bool);
  svtkSetMacro(DepthTextureCompare, bool);
  //@}

  //@{
  /**
   * In case DepthTextureCompare is true, specify the comparison function in
   * use. The result of the comparison is noted `r'.
   * Valid values are:
   * - Value
   * - Lequal: r=R<=Dt ? 1.0 : 0.0
   * - Gequal: r=R>=Dt ? 1.0 : 0.0
   * - Less: r=R<D_t ? 1.0 : 0.0
   * - Greater: r=R>Dt ? 1.0 : 0.0
   * - Equal: r=R==Dt ? 1.0 : 0.0
   * - NotEqual: r=R!=Dt ? 1.0 : 0.0
   * - AlwaysTrue: r=1.0
   * - Never: r=0.0
   * If the magnification of minification factor are not nearest, percentage
   * closer filtering (PCF) is used: R is compared to several D_t and r is
   * the average of the comparisons (it is NOT the average of D_t compared
   * once to R).
   * Initial value is Lequal, as in OpenGL spec.
   * Ignored if the texture object is not a depth texture.
   */
  svtkGetMacro(DepthTextureCompareFunction, int);
  svtkSetMacro(DepthTextureCompareFunction, int);
  //@}

  //@{
  /**
   * Tells the hardware to generate mipmap textures from the first texture
   * image at BaseLevel.
   * Initial value is false, as in OpenGL spec.
   */
  svtkGetMacro(GenerateMipmap, bool);
  svtkSetMacro(GenerateMipmap, bool);
  //@}

  //@{
  /**
   * Set/Get the maximum anisotropic filtering to use. 1.0 means use no
   * anisotropic filtering. The default value is 1.0 and a high value would
   * be 16. This might not be supported on all machines.
   */
  svtkSetMacro(MaximumAnisotropicFiltering, float);
  svtkGetMacro(MaximumAnisotropicFiltering, float);
  //@}

  //@{
  /**
   * Query and return maximum texture size (dimension) supported by the
   * OpenGL driver for a particular context. It should be noted that this
   * size does not consider the internal format of the texture and therefore
   * there is no guarantee that a texture of this size will be allocated by
   * the driver. Also, the method does not make the context current so
   * if the passed context is not valid or current, a value of -1 will
   * be returned.
   */
  static int GetMaximumTextureSize(svtkOpenGLRenderWindow* context);
  static int GetMaximumTextureSize3D(svtkOpenGLRenderWindow* context);

  /**
   * Overload which uses the internal context to query the maximum 3D
   * texture size. Will make the internal context current, returns -1 if
   * anything fails.
   */
  int GetMaximumTextureSize3D();
  //@}

  /**
   * Returns if the context supports the required extensions. If flags
   * for optional extensions are set then the test fails when support
   * for them is not found.
   */
  static bool IsSupported(svtkOpenGLRenderWindow*, bool /* requireTexFloat */,
    bool /* requireDepthFloat */, bool /* requireTexInt */)
  {
    return true;
  }

  /**
   * Check for feature support, without any optional features.
   */
  static bool IsSupported(svtkOpenGLRenderWindow*) { return true; }

  //@{
  /**
   * Copy the texture (src) in the current framebuffer.  A variety of
   * signatures based on what you want to do
   * Copy the entire texture to the entire current viewport
   */
  void CopyToFrameBuffer(svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao);
  // part of a texture to part of a viewport, scaling as needed
  void CopyToFrameBuffer(int srcXmin, int srcYmin, int srcXmax, int srcYmax, int dstXmin,
    int dstYmin, int dstXmax, int dstYmax, int dstSizeX, int dstSizeY, svtkShaderProgram* program,
    svtkOpenGLVertexArrayObject* vao);
  // copy part of a texure to part of a viewport, no scalaing
  void CopyToFrameBuffer(int srcXmin, int srcYmin, int srcXmax, int srcYmax, int dstXmin,
    int dstYmin, int dstSizeX, int dstSizeY, svtkShaderProgram* program,
    svtkOpenGLVertexArrayObject* vao);
  // copy a texture to a quad using the provided tcoords and verts
  void CopyToFrameBuffer(
    float* tcoords, float* verts, svtkShaderProgram* program, svtkOpenGLVertexArrayObject* vao);
  //@}

  /**
   * Copy a sub-part of a logical buffer of the framebuffer (color or depth)
   * to the texture object. src is the framebuffer, dst is the texture.
   * (srcXmin,srcYmin) is the location of the lower left corner of the
   * rectangle in the framebuffer. (dstXmin,dstYmin) is the location of the
   * lower left corner of the rectangle in the texture. width and height
   * specifies the size of the rectangle in pixels.
   * If the logical buffer is a color buffer, it has to be selected first with
   * glReadBuffer().
   * \pre is2D: GetNumberOfDimensions()==2
   */
  void CopyFromFrameBuffer(
    int srcXmin, int srcYmin, int dstXmin, int dstYmin, int width, int height);

  /**
   * Get the shift and scale required in the shader to
   * return the texture values to their original range.
   * This is useful when for example you have unsigned char
   * data and it is being accessed using the floating point
   * texture calls. In that case OpenGL maps the uchar
   * range to a different floating point range under the hood.
   * Applying the shift and scale will return the data to
   * its original values in the shader. The texture's
   * internal format must be set before calling these
   * routines. Creating the texture does set it.
   */
  void GetShiftAndScale(float& shift, float& scale);

  // resizes an existing texture, any existing
  // data values are lost
  void Resize(unsigned int width, unsigned int height);

  //@{
  /**
   * Is this texture using the sRGB color space. If you are using a
   * sRGB framebuffer or window then you probably also want to be
   * using sRGB color textures for proper handling of gamma and
   * associated color mixing.
   */
  svtkGetMacro(UseSRGBColorSpace, bool);
  svtkSetMacro(UseSRGBColorSpace, bool);
  svtkBooleanMacro(UseSRGBColorSpace, bool);
  //@}

  /**
   * Assign the TextureObject to a externally provided
   * Handle and Target. This class will not delete the texture
   * referenced by the handle upon releasing. That is up to
   * whoever created it originally. Note that activating
   * and binding will work. Properties such as wrap/interpolate
   * will also work. But width/height/format etc are left unset.
   */
  void AssignToExistingTexture(unsigned int handle, unsigned int target);

protected:
  svtkTextureObject();
  ~svtkTextureObject() override;

  svtkGenericOpenGLResourceFreeCallback* ResourceCallback;

  /**
   * Creates a texture handle if not already created.
   */
  void CreateTexture();

  /**
   * Destroy the texture.
   */
  void DestroyTexture();

  int NumberOfDimensions;
  unsigned int Width;
  unsigned int Height;
  unsigned int Depth;
  unsigned int Samples;
  bool UseSRGBColorSpace;

  float MaximumAnisotropicFiltering;

  unsigned int Target;         // GLenum
  unsigned int Format;         // GLenum
  unsigned int InternalFormat; // GLenum
  unsigned int Type;           // GLenum
  int Components;

  svtkWeakPointer<svtkOpenGLRenderWindow> Context;
  unsigned int Handle;
  bool OwnHandle;
  bool RequireTextureInteger;
  bool SupportsTextureInteger;
  bool RequireTextureFloat;
  bool SupportsTextureFloat;
  bool RequireDepthBufferFloat;
  bool SupportsDepthBufferFloat;

  int WrapS;
  int WrapT;
  int WrapR;
  int MinificationFilter;
  int MagnificationFilter;

  float MinLOD;
  float MaxLOD;
  int BaseLevel;
  int MaxLevel;
  float BorderColor[4];

  bool DepthTextureCompare;
  int DepthTextureCompareFunction;

  bool GenerateMipmap;

  int AutoParameters;
  svtkTimeStamp SendParametersTime;

  // used for copying to framebuffer
  svtkOpenGLHelper* ShaderProgram;

  // for texturebuffers we hold on to the Buffer
  svtkOpenGLBufferObject* BufferObject;

private:
  svtkTextureObject(const svtkTextureObject&) = delete;
  void operator=(const svtkTextureObject&) = delete;
};

#endif
