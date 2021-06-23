/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProperty
 * @brief   represent surface properties of a geometric object
 *
 * svtkProperty is an object that represents lighting and other surface
 * properties of a geometric object. The primary properties that can be
 * set are colors (overall, ambient, diffuse, specular, and edge color);
 * specular power; opacity of the object; the representation of the
 * object (points, wireframe, or surface); and the shading method to be
 * used (flat, Gouraud, and Phong). Also, some special graphics features
 * like backface properties can be set and manipulated with this object.
 *
 * @sa
 * svtkActor svtkPropertyDevice
 */

#ifndef svtkProperty_h
#define svtkProperty_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include <map>                      // used for ivar
#include <string>                   // used for ivar

// shading models
#define SVTK_FLAT 0
#define SVTK_GOURAUD 1
#define SVTK_PHONG 2
#define SVTK_PBR 3

// representation models
#define SVTK_POINTS 0
#define SVTK_WIREFRAME 1
#define SVTK_SURFACE 2

class svtkActor;
class svtkInformation;
class svtkRenderer;
class svtkShaderProgram;
class svtkTexture;
class svtkWindow;
class svtkXMLDataElement;
class svtkXMLMaterial;

class svtkPropertyInternals;

class SVTKRENDERINGCORE_EXPORT svtkProperty : public svtkObject
{
public:
  svtkTypeMacro(svtkProperty, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with object color, ambient color, diffuse color,
   * specular color, and edge color white; ambient coefficient=0; diffuse
   * coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
   * and surface representation. Backface and frontface culling are off.
   */
  static svtkProperty* New();

  /**
   * Assign one property to another.
   */
  void DeepCopy(svtkProperty* p);

  /**
   * This method causes the property to set up whatever is required for
   * its instance variables. This is actually handled by a subclass of
   * svtkProperty, which is created automatically. This
   * method includes the invoking actor as an argument which can
   * be used by property devices that require the actor.
   */
  virtual void Render(svtkActor*, svtkRenderer*);

  /**
   * This method renders the property as a backface property. TwoSidedLighting
   * must be turned off to see any backface properties. Note that only
   * colors and opacity are used for backface properties. Other properties
   * such as Representation, Culling are specified by the Property.
   */
  virtual void BackfaceRender(svtkActor*, svtkRenderer*) {}

  /**
   * This method is called after the actor has been rendered.
   * Don't call this directly. This method cleans up
   * any shaders allocated.
   */
  virtual void PostRender(svtkActor*, svtkRenderer*);

  //@{
  /**
   * Set/Get lighting flag for an object. Initial value is true.
   */
  svtkGetMacro(Lighting, bool);
  svtkSetMacro(Lighting, bool);
  svtkBooleanMacro(Lighting, bool);
  //@}

  //@{
  /**
   * Set/Get rendering of points as spheres. The size of the
   * sphere in pixels is controlled by the PointSize
   * attribute. Note that half spheres may be rendered
   * instead of spheres.
   */
  svtkGetMacro(RenderPointsAsSpheres, bool);
  svtkSetMacro(RenderPointsAsSpheres, bool);
  svtkBooleanMacro(RenderPointsAsSpheres, bool);
  //@}

  //@{
  /**
   * Set/Get rendering of lines as tubes. The width of the
   * line in pixels is controlled by the LineWidth
   * attribute. May not be supported on every platform
   * and the implementation may be half tubes, or something
   * only tube like in appearance.
   */
  svtkGetMacro(RenderLinesAsTubes, bool);
  svtkSetMacro(RenderLinesAsTubes, bool);
  svtkBooleanMacro(RenderLinesAsTubes, bool);
  //@}

  //@{
  /**
   * Set the shading interpolation method for an object.
   */
  svtkSetClampMacro(Interpolation, int, SVTK_FLAT, SVTK_PBR);
  svtkGetMacro(Interpolation, int);
  void SetInterpolationToFlat() { this->SetInterpolation(SVTK_FLAT); }
  void SetInterpolationToGouraud() { this->SetInterpolation(SVTK_GOURAUD); }
  void SetInterpolationToPhong() { this->SetInterpolation(SVTK_PHONG); }
  void SetInterpolationToPBR() { this->SetInterpolation(SVTK_PBR); }
  const char* GetInterpolationAsString();
  //@}

  //@{
  /**
   * Control the surface geometry representation for the object.
   */
  svtkSetClampMacro(Representation, int, SVTK_POINTS, SVTK_SURFACE);
  svtkGetMacro(Representation, int);
  void SetRepresentationToPoints() { this->SetRepresentation(SVTK_POINTS); }
  void SetRepresentationToWireframe() { this->SetRepresentation(SVTK_WIREFRAME); }
  void SetRepresentationToSurface() { this->SetRepresentation(SVTK_SURFACE); }
  const char* GetRepresentationAsString();
  //@}

  //@{
  /**
   * Set the color of the object. Has the side effect of setting the
   * ambient diffuse and specular colors as well. This is basically
   * a quick overall color setting method.
   */
  virtual void SetColor(double r, double g, double b);
  virtual void SetColor(double a[3]);
  double* GetColor() SVTK_SIZEHINT(3);
  void GetColor(double rgb[3]);
  void GetColor(double& r, double& g, double& b);
  //@}

  //@{
  /**
   * Set/Get the metallic coefficient.
   * Usually this value is either 0 or 1 for real material but any value in between is valid.
   * This parameter is only used by PBR Interpolation.
   * Default value is 0.0
   */
  svtkSetClampMacro(Metallic, double, 0.0, 1.0);
  svtkGetMacro(Metallic, double);
  //@}

  //@{
  /**
   * Set/Get the roughness coefficient.
   * This value have to be between 0 (glossy) and 1 (rough).
   * A glossy material have reflections and a high specular part.
   * This parameter is only used by PBR Interpolation.
   * Default value is 0.5
   */
  svtkSetClampMacro(Roughness, double, 0.0, 1.0);
  svtkGetMacro(Roughness, double);
  //@}

  //@{
  /**
   * Set/Get the normal scale coefficient.
   * This value affects the strength of the normal deviation from the texture.
   * Default value is 1.0
   */
  svtkSetMacro(NormalScale, double);
  svtkGetMacro(NormalScale, double);
  //@}

  //@{
  /**
   * Set/Get the occlusion strength coefficient.
   * This value affects the strength of the occlusion if a material texture is present.
   * This parameter is only used by PBR Interpolation.
   * Default value is 1.0
   */
  svtkSetClampMacro(OcclusionStrength, double, 0.0, 1.0);
  svtkGetMacro(OcclusionStrength, double);
  //@}

  //@{
  /**
   * Set/Get the emissive factor.
   * This value is multiplied with the emissive color when an emissive texture is present.
   * This parameter is only used by PBR Interpolation.
   * Default value is [1.0, 1.0, 1.0]
   */
  svtkSetVector3Macro(EmissiveFactor, double);
  svtkGetVector3Macro(EmissiveFactor, double);
  //@}

  //@{
  /**
   * Set/Get the ambient lighting coefficient.
   */
  svtkSetClampMacro(Ambient, double, 0.0, 1.0);
  svtkGetMacro(Ambient, double);
  //@}

  //@{
  /**
   * Set/Get the diffuse lighting coefficient.
   */
  svtkSetClampMacro(Diffuse, double, 0.0, 1.0);
  svtkGetMacro(Diffuse, double);
  //@}

  //@{
  /**
   * Set/Get the specular lighting coefficient.
   */
  svtkSetClampMacro(Specular, double, 0.0, 1.0);
  svtkGetMacro(Specular, double);
  //@}

  //@{
  /**
   * Set/Get the specular power.
   */
  svtkSetClampMacro(SpecularPower, double, 0.0, 128.0);
  svtkGetMacro(SpecularPower, double);
  //@}

  //@{
  /**
   * Set/Get the object's opacity. 1.0 is totally opaque and 0.0 is completely
   * transparent.
   */
  svtkSetClampMacro(Opacity, double, 0.0, 1.0);
  svtkGetMacro(Opacity, double);
  //@}

  //@{
  /**
   * Set/Get the ambient surface color. Not all renderers support separate
   * ambient and diffuse colors. From a physical standpoint it really
   * doesn't make too much sense to have both. For the rendering
   * libraries that don't support both, the diffuse color is used.
   */
  svtkSetVector3Macro(AmbientColor, double);
  svtkGetVector3Macro(AmbientColor, double);
  //@}

  //@{
  /**
   * Set/Get the diffuse surface color.
   * For PBR Interpolation, DiffuseColor is used as the base color
   */
  svtkSetVector3Macro(DiffuseColor, double);
  svtkGetVector3Macro(DiffuseColor, double);
  //@}

  //@{
  /**
   * Set/Get the specular surface color.
   */
  svtkSetVector3Macro(SpecularColor, double);
  svtkGetVector3Macro(SpecularColor, double);
  //@}

  //@{
  /**
   * Turn on/off the visibility of edges. On some renderers it is
   * possible to render the edges of geometric primitives separately
   * from the interior.
   */
  svtkGetMacro(EdgeVisibility, svtkTypeBool);
  svtkSetMacro(EdgeVisibility, svtkTypeBool);
  svtkBooleanMacro(EdgeVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the color of primitive edges (if edge visibility is enabled).
   */
  svtkSetVector3Macro(EdgeColor, double);
  svtkGetVector3Macro(EdgeColor, double);
  //@}

  //@{
  /**
   * Turn on/off the visibility of vertices. On some renderers it is
   * possible to render the vertices of geometric primitives separately
   * from the interior.
   */
  svtkGetMacro(VertexVisibility, svtkTypeBool);
  svtkSetMacro(VertexVisibility, svtkTypeBool);
  svtkBooleanMacro(VertexVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the color of primitive vertices (if vertex visibility is enabled).
   */
  svtkSetVector3Macro(VertexColor, double);
  svtkGetVector3Macro(VertexColor, double);
  //@}

  //@{
  /**
   * Set/Get the width of a Line. The width is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  svtkSetClampMacro(LineWidth, float, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(LineWidth, float);
  //@}

  //@{
  /**
   * Set/Get the stippling pattern of a Line, as a 16-bit binary pattern
   * (1 = pixel on, 0 = pixel off).
   * This is only implemented for OpenGL, not OpenGL2. The default is 0xFFFF.
   */
  svtkSetMacro(LineStipplePattern, int);
  svtkGetMacro(LineStipplePattern, int);
  //@}

  //@{
  /**
   * Set/Get the stippling repeat factor of a Line, which specifies how
   * many times each bit in the pattern is to be repeated.
   * This is only implemented for OpenGL, not OpenGL2. The default is 1.
   */
  svtkSetClampMacro(LineStippleRepeatFactor, int, 1, SVTK_INT_MAX);
  svtkGetMacro(LineStippleRepeatFactor, int);
  //@}

  //@{
  /**
   * Set/Get the diameter of a point. The size is expressed in screen units.
   * This is only implemented for OpenGL. The default is 1.0.
   */
  svtkSetClampMacro(PointSize, float, 0, SVTK_FLOAT_MAX);
  svtkGetMacro(PointSize, float);
  //@}

  //@{
  /**
   * Turn on/off fast culling of polygons based on orientation of normal
   * with respect to camera. If backface culling is on, polygons facing
   * away from camera are not drawn.
   */
  svtkGetMacro(BackfaceCulling, svtkTypeBool);
  svtkSetMacro(BackfaceCulling, svtkTypeBool);
  svtkBooleanMacro(BackfaceCulling, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off fast culling of polygons based on orientation of normal
   * with respect to camera. If frontface culling is on, polygons facing
   * towards camera are not drawn.
   */
  svtkGetMacro(FrontfaceCulling, svtkTypeBool);
  svtkSetMacro(FrontfaceCulling, svtkTypeBool);
  svtkBooleanMacro(FrontfaceCulling, svtkTypeBool);
  //@}

  //@{
  /**
   * Returns the name of the material currently loaded, if any.
   */
  svtkSetStringMacro(MaterialName);
  svtkGetStringMacro(MaterialName);
  //@}

  //@{
  /**
   * Enable/Disable shading. When shading is enabled, the
   * Material must be set.
   */
  svtkSetMacro(Shading, svtkTypeBool);
  svtkGetMacro(Shading, svtkTypeBool);
  svtkBooleanMacro(Shading, svtkTypeBool);
  //@}

  //@{
  /**
   * Provide values to initialize shader variables.
   * Useful to initialize shader variables that change over time
   * (animation, GUI widgets inputs, etc. )
   * - \p name - hardware name of the uniform variable
   * - \p numVars - number of variables being set
   * - \p x - values
   */
  virtual void AddShaderVariable(const char* name, int numVars, int* x);
  virtual void AddShaderVariable(const char* name, int numVars, float* x);
  virtual void AddShaderVariable(const char* name, int numVars, double* x);
  //@}

  //@{
  /**
   * Methods to provide to add shader variables from wrappers.
   */
  void AddShaderVariable(const char* name, int v) { this->AddShaderVariable(name, 1, &v); }
  void AddShaderVariable(const char* name, float v) { this->AddShaderVariable(name, 1, &v); }
  void AddShaderVariable(const char* name, double v) { this->AddShaderVariable(name, 1, &v); }
  void AddShaderVariable(const char* name, int v1, int v2)
  {
    int v[2] = { v1, v2 };
    this->AddShaderVariable(name, 2, v);
  }
  void AddShaderVariable(const char* name, float v1, float v2)
  {
    float v[2] = { v1, v2 };
    this->AddShaderVariable(name, 2, v);
  }
  void AddShaderVariable(const char* name, double v1, double v2)
  {
    double v[2] = { v1, v2 };
    this->AddShaderVariable(name, 2, v);
  }
  void AddShaderVariable(const char* name, int v1, int v2, int v3)
  {
    int v[3] = { v1, v2, v3 };
    this->AddShaderVariable(name, 3, v);
  }
  void AddShaderVariable(const char* name, float v1, float v2, float v3)
  {
    float v[3] = { v1, v2, v3 };
    this->AddShaderVariable(name, 3, v);
  }
  void AddShaderVariable(const char* name, double v1, double v2, double v3)
  {
    double v[3] = { v1, v2, v3 };
    this->AddShaderVariable(name, 3, v);
  }
  //@}

  //@{
  /**
   * Set/Get the texture object to control rendering texture maps. This will
   * be a svtkTexture object. A property does not need to have an associated
   * texture map and multiple properties can share one texture. Textures
   * must be assigned unique names. Note that for texture blending the
   * textures will be rendering is alphabetical order and after any texture
   * defined in the actor.
   * There exists 4 special textures with reserved names: "albedoTex", "materialTex", "normalTex"
   * and "emissiveTex". While these textures can be added with the regular SetTexture method, it is
   * prefered to use to method SetBaseColorTexture, SetORMTexture, SetNormalTexture and
   * SetEmissiveTexture respectively.
   */
  void SetTexture(const char* name, svtkTexture* texture);
  svtkTexture* GetTexture(const char* name);
  //@}

  /**
   * Set the base color texture. Also called albedo, this texture is only used while rendering
   * with PBR interpolation. This is the color of the object.
   * This texture must be in sRGB color space.
   * @sa SetInterpolationToPBR svtkTexture::UseSRGBColorSpaceOn
   */
  void SetBaseColorTexture(svtkTexture* texture) { this->SetTexture("albedoTex", texture); }

  /**
   * Set the ORM texture. This texture contains three RGB independent components corresponding to
   * the Occlusion value, Roughness value and Metallic value respectively.
   * Each texture value is scaled by the Occlusion strength, roughness coefficient and metallic
   * coefficient.
   * This texture must be in linear color space.
   * This is only used by the PBR shading model.
   * @sa SetInterpolationToPBR SetOcclusionStrength SetMetallic SetRoughness
   */
  void SetORMTexture(svtkTexture* texture) { this->SetTexture("materialTex", texture); }

  /**
   * Set the normal texture. This texture is required for normal mapping. It is valid for both PBR
   * and Phong interpolation.
   * The normal mapping is enabled if this texture is present and both normals and tangents are
   * presents in the svtkPolyData.
   * This texture must be in linear color space.
   * @sa svtkPolyDataTangents SetNormalScale
   */
  void SetNormalTexture(svtkTexture* texture) { this->SetTexture("normalTex", texture); }

  /**
   * Set the emissive texture. When present, this RGB texture provides location and color to the
   * shader where the svtkPolyData should emit light. Emited light is scaled by EmissiveFactor.
   * This is only supported by PBR interpolation model.
   * This texture must be in sRGB color space.
   * @sa SetInterpolationToPBR SetEmissiveFactor svtkTexture::UseSRGBColorSpaceOn
   */
  void SetEmissiveTexture(svtkTexture* texture) { this->SetTexture("emissiveTex", texture); }

  /**
   * Remove a texture from the collection.
   */
  void RemoveTexture(const char* name);

  /**
   * Remove all the textures.
   */
  void RemoveAllTextures();

  /**
   * Returns the number of textures in this property.
   */
  int GetNumberOfTextures();

  /**
   * Returns all the textures in this property and their names
   */
  std::map<std::string, svtkTexture*>& GetAllTextures() { return this->Textures; }

  /**
   * Release any graphics resources that are being consumed by this
   * property. The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(svtkWindow* win);

  //@{
  /**
   * Set/Get the information object associated with the Property.
   */
  svtkGetObjectMacro(Information, svtkInformation);
  virtual void SetInformation(svtkInformation*);
  //@}

protected:
  svtkProperty();
  ~svtkProperty() override;

  /**
   * Computes composite color. Used by GetColor().
   */
  static void ComputeCompositeColor(double result[3], double ambient, const double ambient_color[3],
    double diffuse, const double diffuse_color[3], double specular, const double specular_color[3]);

  double Color[3];
  double AmbientColor[3];
  double DiffuseColor[3];
  double SpecularColor[3];
  double EdgeColor[3];
  double VertexColor[3];
  double Ambient;
  double Diffuse;
  double Metallic;
  double Roughness;
  double NormalScale;
  double OcclusionStrength;
  double EmissiveFactor[3];
  double Specular;
  double SpecularPower;
  double Opacity;
  float PointSize;
  float LineWidth;
  int LineStipplePattern;
  int LineStippleRepeatFactor;
  int Interpolation;
  int Representation;
  svtkTypeBool EdgeVisibility;
  svtkTypeBool VertexVisibility;
  svtkTypeBool BackfaceCulling;
  svtkTypeBool FrontfaceCulling;
  bool Lighting;
  bool RenderPointsAsSpheres;
  bool RenderLinesAsTubes;

  svtkTypeBool Shading;

  char* MaterialName;

  typedef std::map<std::string, svtkTexture*> MapOfTextures;
  MapOfTextures Textures;

  // Arbitrary extra information associated with this Property.
  svtkInformation* Information;

private:
  svtkProperty(const svtkProperty&) = delete;
  void operator=(const svtkProperty&) = delete;
};

//@{
/**
 * Return the method of shading as a descriptive character string.
 */
inline const char* svtkProperty::GetInterpolationAsString(void)
{
  if (this->Interpolation == SVTK_FLAT)
  {
    return "Flat";
  }
  else if (this->Interpolation == SVTK_GOURAUD)
  {
    return "Gouraud";
  }
  else if (this->Interpolation == SVTK_PHONG)
  {
    return "Phong";
  }
  else // if (this->Interpolation == SVTK_PBR)
  {
    return "Physically based rendering";
  }
}
//@}

//@{
/**
 * Return the method of shading as a descriptive character string.
 */
inline const char* svtkProperty::GetRepresentationAsString(void)
{
  if (this->Representation == SVTK_POINTS)
  {
    return "Points";
  }
  else if (this->Representation == SVTK_WIREFRAME)
  {
    return "Wireframe";
  }
  else
  {
    return "Surface";
  }
}
//@}

#endif
