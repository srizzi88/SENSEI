/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTexture
 * @brief   handles properties associated with a texture map
 *
 * svtkTexture is an object that handles loading and binding of texture
 * maps. It obtains its data from an input image data dataset type.
 * Thus you can create visualization pipelines to read, process, and
 * construct textures. Note that textures will only work if texture
 * coordinates are also defined, and if the rendering system supports
 * texture.
 *
 * Instances of svtkTexture are associated with actors via the actor's
 * SetTexture() method. Actors can share texture maps (this is encouraged
 * to save memory resources.)
 *
 * @warning
 * Currently only 2D texture maps are supported, even though the data pipeline
 * supports 1,2, and 3D texture coordinates.
 *
 * @warning
 * Some renderers such as old OpenGL require that the texture map dimensions
 * are a power of two in each direction. If a non-power of two texture map is
 * used, it is automatically resampled to a power of two in one or more
 * directions, at the cost of an expensive computation. If the OpenGL
 * implementation is recent enough (OpenGL>=2.0 or
 * extension GL_ARB_texture_non_power_of_two exists) there is no such
 * restriction and no extra computational cost.
 * @sa
 * svtkActor svtkRenderer svtkOpenGLTexture
 */

#ifndef svtkTexture_h
#define svtkTexture_h

#include "svtkImageAlgorithm.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkSystemIncludes.h"      // For SVTK_COLOR_MODE_*

class svtkImageData;
class svtkScalarsToColors;
class svtkRenderer;
class svtkUnsignedCharArray;
class svtkWindow;
class svtkDataArray;
class svtkTransform;

#define SVTK_TEXTURE_QUALITY_DEFAULT 0
#define SVTK_TEXTURE_QUALITY_16BIT 16
#define SVTK_TEXTURE_QUALITY_32BIT 32

class SVTKRENDERINGCORE_EXPORT svtkTexture : public svtkImageAlgorithm
{
public:
  static svtkTexture* New();
  svtkTypeMacro(svtkTexture, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Renders a texture map. It first checks the object's modified time
   * to make sure the texture maps Input is valid, then it invokes the
   * Load() method.
   */
  virtual void Render(svtkRenderer* ren);

  /**
   * Cleans up after the texture rendering to restore the state of the
   * graphics context.
   */
  virtual void PostRender(svtkRenderer*) {}

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(svtkWindow*) {}

  /**
   * Abstract interface to renderer. Each concrete subclass of
   * svtkTexture will load its data into graphics system in response
   * to this method invocation.
   */
  virtual void Load(svtkRenderer*) {}

  //@{
  /**
   * Turn on/off the repetition of the texture map when the texture
   * coords extend beyond the [0,1] range.
   */
  svtkGetMacro(Repeat, svtkTypeBool);
  svtkSetMacro(Repeat, svtkTypeBool);
  svtkBooleanMacro(Repeat, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the clamping of the texture map when the texture
   * coords extend beyond the [0,1] range.
   * Only used when Repeat is off, and edge clamping is supported by
   * the graphics card.
   */
  svtkGetMacro(EdgeClamp, svtkTypeBool);
  svtkSetMacro(EdgeClamp, svtkTypeBool);
  svtkBooleanMacro(EdgeClamp, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off linear interpolation of the texture map when rendering.
   */
  svtkGetMacro(Interpolate, svtkTypeBool);
  svtkSetMacro(Interpolate, svtkTypeBool);
  svtkBooleanMacro(Interpolate, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off use of mipmaps when rendering.
   */
  svtkGetMacro(Mipmap, bool);
  svtkSetMacro(Mipmap, bool);
  svtkBooleanMacro(Mipmap, bool);
  //@}

  //@{
  /**
   * Set/Get the maximum anisotropic filtering to use. 1.0 means use no
   * anisotropic filtering. The default value is 4.0 and a high value would
   * be 16. This setting is only applied when mipmaps are used. This might
   * not be supported on all machines.
   */
  svtkSetMacro(MaximumAnisotropicFiltering, float);
  svtkGetMacro(MaximumAnisotropicFiltering, float);
  //@}

  //@{
  /**
   * Force texture quality to 16-bit or 32-bit.
   * This might not be supported on all machines.
   */
  svtkSetMacro(Quality, int);
  svtkGetMacro(Quality, int);
  void SetQualityToDefault() { this->SetQuality(SVTK_TEXTURE_QUALITY_DEFAULT); }
  void SetQualityTo16Bit() { this->SetQuality(SVTK_TEXTURE_QUALITY_16BIT); }
  void SetQualityTo32Bit() { this->SetQuality(SVTK_TEXTURE_QUALITY_32BIT); }
  //@}

  //@{
  /**
   * Default: ColorModeToDefault. unsigned char scalars are treated
   * as colors, and NOT mapped through the lookup table (set with SetLookupTable),
   * while other kinds of scalars are. ColorModeToDirectScalar extends
   * ColorModeToDefault such that all integer types are treated as
   * colors with values in the range 0-255 and floating types are
   * treated as colors with values in the range 0.0-1.0. Setting
   * ColorModeToMapScalars means that all scalar data will be mapped
   * through the lookup table.
   */
  svtkSetMacro(ColorMode, int);
  svtkGetMacro(ColorMode, int);
  void SetColorModeToDefault() { this->SetColorMode(SVTK_COLOR_MODE_DEFAULT); }
  void SetColorModeToMapScalars() { this->SetColorMode(SVTK_COLOR_MODE_MAP_SCALARS); }
  void SetColorModeToDirectScalars() { this->SetColorMode(SVTK_COLOR_MODE_DIRECT_SCALARS); }
  //@}

  /**
   * Get the input as a svtkImageData object.  This method is for
   * backwards compatibility.
   */
  svtkImageData* GetInput();

  //@{
  /**
   * Specify the lookup table to convert scalars if necessary
   */
  void SetLookupTable(svtkScalarsToColors*);
  svtkGetObjectMacro(LookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * Get Mapped Scalars
   */
  svtkGetObjectMacro(MappedScalars, svtkUnsignedCharArray);
  //@}

  /**
   * Map scalar values into color scalars.
   */
  unsigned char* MapScalarsToColors(svtkDataArray* scalars);

  //@{
  /**
   * Set a transform on the texture which allows one to scale,
   * rotate and translate the texture.
   */
  void SetTransform(svtkTransform* transform);
  svtkGetObjectMacro(Transform, svtkTransform);
  //@}

  /**
   * Used to specify how the texture will blend its RGB and Alpha values
   * with other textures and the fragment the texture is rendered upon.
   */
  enum SVTKTextureBlendingMode
  {
    SVTK_TEXTURE_BLENDING_MODE_NONE = 0,
    SVTK_TEXTURE_BLENDING_MODE_REPLACE,
    SVTK_TEXTURE_BLENDING_MODE_MODULATE,
    SVTK_TEXTURE_BLENDING_MODE_ADD,
    SVTK_TEXTURE_BLENDING_MODE_ADD_SIGNED,
    SVTK_TEXTURE_BLENDING_MODE_INTERPOLATE,
    SVTK_TEXTURE_BLENDING_MODE_SUBTRACT
  };

  //@{
  /**
   * Used to specify how the texture will blend its RGB and Alpha values
   * with other textures and the fragment the texture is rendered upon.
   */
  svtkGetMacro(BlendingMode, int);
  svtkSetMacro(BlendingMode, int);
  //@}

  //@{
  /**
   * Whether the texture colors are premultiplied by alpha.
   * Initial value is false.
   */
  svtkGetMacro(PremultipliedAlpha, bool);
  svtkSetMacro(PremultipliedAlpha, bool);
  svtkBooleanMacro(PremultipliedAlpha, bool);
  //@}

  //@{
  /**
   * When the texture is forced to be a power of 2, the default behavior is
   * for the "new" image's dimensions to be greater than or equal to with
   * respects to the original.  Setting RestrictPowerOf2ImageSmaller to be
   * 1 (or ON) with force the new image's dimensions to be less than or equal
   * to with respects to the original.
   */
  svtkGetMacro(RestrictPowerOf2ImageSmaller, svtkTypeBool);
  svtkSetMacro(RestrictPowerOf2ImageSmaller, svtkTypeBool);
  svtkBooleanMacro(RestrictPowerOf2ImageSmaller, svtkTypeBool);
  //@}

  /**
   * Is this Texture Translucent?
   * returns false (0) if the texture is either fully opaque or has
   * only fully transparent pixels and fully opaque pixels and the
   * Interpolate flag is turn off.
   */
  virtual int IsTranslucent();

  /**
   * Return the texture unit used for this texture
   */
  virtual int GetTextureUnit() { return 0; }

  //@{
  /**
   * Is this texture a cube map, if so it needs 6 inputs
   * one for each side of the cube. You must set this before
   * connecting the inputs. The inputs must all have the same
   * size, data type, and depth
   */
  svtkGetMacro(CubeMap, bool);
  svtkBooleanMacro(CubeMap, bool);
  void SetCubeMap(bool val);
  //@}

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

protected:
  svtkTexture();
  ~svtkTexture() override;

  // A texture is a sink, so there is no need to do anything.
  // This definition avoids a warning when doing Update() on a svtkTexture object.
  void ExecuteData(svtkDataObject*) override {}

  bool Mipmap;
  float MaximumAnisotropicFiltering;
  svtkTypeBool Repeat;
  svtkTypeBool EdgeClamp;
  svtkTypeBool Interpolate;
  int Quality;
  int ColorMode;
  svtkScalarsToColors* LookupTable;
  svtkUnsignedCharArray* MappedScalars;
  svtkTransform* Transform;

  int BlendingMode;
  svtkTypeBool RestrictPowerOf2ImageSmaller;
  // this is to duplicated the previous behavior of SelfCreatedLookUpTable
  int SelfAdjustingTableRange;
  bool PremultipliedAlpha;
  bool CubeMap;
  bool UseSRGBColorSpace;

  // the result of HasTranslucentPolygonalGeometry is cached
  svtkTimeStamp TranslucentComputationTime;
  int TranslucentCachedResult;

private:
  svtkTexture(const svtkTexture&) = delete;
  void operator=(const svtkTexture&) = delete;
};

#endif
