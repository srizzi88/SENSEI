/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProperty.h"

#include "svtkActor.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"

#include <cstdlib>
#include <sstream>

#include <svtksys/SystemTools.hxx>

svtkCxxSetObjectMacro(svtkProperty, Information, svtkInformation);

//----------------------------------------------------------------------------
// Return nullptr if no override is supplied.
svtkObjectFactoryNewMacro(svtkProperty);

// Construct object with object color, ambient color, diffuse color,
// specular color, and edge color white; ambient coefficient=0; diffuse
// coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
// and surface representation. Backface and frontface culling are off.
svtkProperty::svtkProperty()
{
  // Really should initialize all colors including Color[3]
  this->Color[0] = 1;
  this->Color[1] = 1;
  this->Color[2] = 1;

  this->AmbientColor[0] = 1;
  this->AmbientColor[1] = 1;
  this->AmbientColor[2] = 1;

  this->DiffuseColor[0] = 1;
  this->DiffuseColor[1] = 1;
  this->DiffuseColor[2] = 1;

  this->SpecularColor[0] = 1;
  this->SpecularColor[1] = 1;
  this->SpecularColor[2] = 1;

  this->EdgeColor[0] = 0;
  this->EdgeColor[1] = 0;
  this->EdgeColor[2] = 0;

  this->VertexColor[0] = 0.5;
  this->VertexColor[1] = 1.0;
  this->VertexColor[2] = 0.5;

  this->EmissiveFactor[0] = 1.0;
  this->EmissiveFactor[1] = 1.0;
  this->EmissiveFactor[2] = 1.0;

  this->NormalScale = 1.0;
  this->OcclusionStrength = 1.0;
  this->Metallic = 0.0;
  this->Roughness = 0.5;
  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.0;
  this->SpecularPower = 1.0;
  this->Opacity = 1.0;
  this->Interpolation = SVTK_GOURAUD;
  this->Representation = SVTK_SURFACE;
  this->EdgeVisibility = 0;
  this->VertexVisibility = 0;
  this->BackfaceCulling = 0;
  this->FrontfaceCulling = 0;
  this->PointSize = 1.0;
  this->LineWidth = 1.0;
  this->LineStipplePattern = 0xFFFF;
  this->LineStippleRepeatFactor = 1;
  this->Lighting = true;
  this->RenderPointsAsSpheres = false;
  this->RenderLinesAsTubes = false;

  this->Shading = 0;
  this->MaterialName = nullptr;

  this->Information = svtkInformation::New();
  this->Information->Register(this);
  this->Information->Delete();
}

//----------------------------------------------------------------------------
svtkProperty::~svtkProperty()
{
  this->RemoveAllTextures();
  this->SetMaterialName(nullptr);

  this->SetInformation(nullptr);
}

//----------------------------------------------------------------------------
// Assign one property to another.
void svtkProperty::DeepCopy(svtkProperty* p)
{
  if (p != nullptr)
  {
    this->SetColor(p->GetColor());
    this->SetAmbientColor(p->GetAmbientColor());
    this->SetDiffuseColor(p->GetDiffuseColor());
    this->SetSpecularColor(p->GetSpecularColor());
    this->SetEdgeColor(p->GetEdgeColor());
    this->SetVertexColor(p->GetVertexColor());
    this->SetAmbient(p->GetAmbient());
    this->SetDiffuse(p->GetDiffuse());
    this->SetSpecular(p->GetSpecular());
    this->SetSpecularPower(p->GetSpecularPower());
    this->SetOpacity(p->GetOpacity());
    this->SetInterpolation(p->GetInterpolation());
    this->SetRepresentation(p->GetRepresentation());
    this->SetEdgeVisibility(p->GetEdgeVisibility());
    this->SetVertexVisibility(p->GetVertexVisibility());
    this->SetBackfaceCulling(p->GetBackfaceCulling());
    this->SetFrontfaceCulling(p->GetFrontfaceCulling());
    this->SetPointSize(p->GetPointSize());
    this->SetLineWidth(p->GetLineWidth());
    this->SetLineStipplePattern(p->GetLineStipplePattern());
    this->SetLineStippleRepeatFactor(p->GetLineStippleRepeatFactor());
    this->SetLighting(p->GetLighting());
    this->SetRenderPointsAsSpheres(p->GetRenderPointsAsSpheres());
    this->SetRenderLinesAsTubes(p->GetRenderLinesAsTubes());
    this->SetShading(p->GetShading());

    this->RemoveAllTextures();
    auto iter = p->Textures.begin();
    for (; iter != p->Textures.end(); ++iter)
    {
      this->Textures[iter->first] = iter->second;
    }
    // TODO: need to pass shader variables.
  }
}

//----------------------------------------------------------------------------
void svtkProperty::SetColor(double r, double g, double b)
{
  double newColor[3] = { r, g, b };

  // SetColor is shorthand for "set all colors"
  double* color[4] = { this->Color, this->AmbientColor, this->DiffuseColor, this->SpecularColor };

  // Set colors, and check for changes
  bool modified = false;
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (color[i][j] != newColor[j])
      {
        modified = true;
        color[i][j] = newColor[j];
      }
    }
  }

  // Call Modified() if anything changed
  if (modified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkProperty::SetColor(double a[3])
{
  this->SetColor(a[0], a[1], a[2]);
}

//----------------------------------------------------------------------------
void svtkProperty::ComputeCompositeColor(double result[3], double ambient,
  const double ambient_color[3], double diffuse, const double diffuse_color[3], double specular,
  const double specular_color[3])
{
  double norm = 0.0;
  if ((ambient + diffuse + specular) > 0)
  {
    norm = 1.0 / (ambient + diffuse + specular);
  }

  for (int i = 0; i < 3; i++)
  {
    result[i] =
      (ambient * ambient_color[i] + diffuse * diffuse_color[i] + specular * specular_color[i]) *
      norm;
  }
}

//----------------------------------------------------------------------------
// Return composite color of object (ambient + diffuse + specular). Return value
// is a pointer to rgb values.
double* svtkProperty::GetColor()
{
  svtkProperty::ComputeCompositeColor(this->Color, this->Ambient, this->AmbientColor, this->Diffuse,
    this->DiffuseColor, this->Specular, this->SpecularColor);
  return this->Color;
}

//----------------------------------------------------------------------------
// Copy composite color of object (ambient + diffuse + specular) into array
// provided.
void svtkProperty::GetColor(double rgb[3])
{
  this->GetColor();
  rgb[0] = this->Color[0];
  rgb[1] = this->Color[1];
  rgb[2] = this->Color[2];
}

//----------------------------------------------------------------------------
void svtkProperty::GetColor(double& r, double& g, double& b)
{
  this->GetColor();
  r = this->Color[0];
  g = this->Color[1];
  b = this->Color[2];
}

//----------------------------------------------------------------------------
void svtkProperty::SetTexture(const char* name, svtkTexture* tex)
{
  if (tex == nullptr)
  {
    this->RemoveTexture(name);
    return;
  }

  if ((strcmp(name, "albedoTex") == 0 || strcmp(name, "emissiveTex") == 0) &&
    !tex->GetUseSRGBColorSpace())
  {
    svtkErrorMacro("The " << name << " texture is not in sRGB color space.");
    return;
  }
  if ((strcmp(name, "materialTex") == 0 || strcmp(name, "normalTex") == 0) &&
    tex->GetUseSRGBColorSpace())
  {
    svtkErrorMacro("The " << name << " texture is not in linear color space.");
    return;
  }

  auto iter = this->Textures.find(std::string(name));
  if (iter != this->Textures.end())
  {
    // same value?
    if (iter->second == tex)
    {
      return;
    }
    svtkWarningMacro("Texture with name " << name << " exists. It will be replaced.");
    iter->second->UnRegister(this);
  }

  tex->Register(this);
  this->Textures[name] = tex;
  this->Modified();
}

//----------------------------------------------------------------------------
svtkTexture* svtkProperty::GetTexture(const char* name)
{
  auto iter = this->Textures.find(std::string(name));
  if (iter == this->Textures.end())
  {
    return nullptr;
  }

  return iter->second;
}

//----------------------------------------------------------------------------
int svtkProperty::GetNumberOfTextures()
{
  return static_cast<int>(this->Textures.size());
}

//----------------------------------------------------------------------------
void svtkProperty::RemoveTexture(const char* name)
{
  auto iter = this->Textures.find(std::string(name));
  if (iter != this->Textures.end())
  {
    iter->second->UnRegister(this);
    this->Textures.erase(iter);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkProperty::RemoveAllTextures()
{
  while (!this->Textures.empty())
  {
    auto iter = this->Textures.begin();
    iter->second->UnRegister(this);
    this->Textures.erase(iter);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkProperty::Render(svtkActor*, svtkRenderer* renderer)
{
  // subclass would have renderer the property already.
  // this class, just handles the shading.

  if (renderer->GetSelector())
  {
    // nothing to do when rendering for hardware selection.
    return;
  }
}

//----------------------------------------------------------------------------
void svtkProperty::PostRender(svtkActor*, svtkRenderer* renderer)
{
  if (renderer->GetSelector())
  {
    // nothing to do when rendering for hardware selection.
    return;
  }
}

//----------------------------------------------------------------------------
void svtkProperty::AddShaderVariable(const char*, int, int*) {}

//----------------------------------------------------------------------------
void svtkProperty::AddShaderVariable(const char*, int, float*) {}

//----------------------------------------------------------------------------
void svtkProperty::AddShaderVariable(const char*, int, double*) {}

//-----------------------------------------------------------------------------
void svtkProperty::ReleaseGraphicsResources(svtkWindow*)
{
  // svtkOpenGLRenderer releases texture resources, so we don't need to release
  // them here.
}

//----------------------------------------------------------------------------
void svtkProperty::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Ambient: " << this->Ambient << "\n";
  os << indent << "Ambient Color: (" << this->AmbientColor[0] << ", " << this->AmbientColor[1]
     << ", " << this->AmbientColor[2] << ")\n";
  os << indent << "Diffuse: " << this->Diffuse << "\n";
  os << indent << "Diffuse Color: (" << this->DiffuseColor[0] << ", " << this->DiffuseColor[1]
     << ", " << this->DiffuseColor[2] << ")\n";
  os << indent << "Edge Color: (" << this->EdgeColor[0] << ", " << this->EdgeColor[1] << ", "
     << this->EdgeColor[2] << ")\n";
  os << indent << "Edge Visibility: " << (this->EdgeVisibility ? "On\n" : "Off\n");
  os << indent << "Vertex Color: (" << this->VertexColor[0] << ", " << this->VertexColor[1] << ", "
     << this->VertexColor[2] << ")\n";
  os << indent << "Vertex Visibility: " << (this->VertexVisibility ? "On\n" : "Off\n");
  os << indent << "Interpolation: ";
  switch (this->Interpolation)
  {
    case SVTK_FLAT:
      os << "SVTK_FLAT\n";
      break;
    case SVTK_GOURAUD:
      os << "SVTK_GOURAUD\n";
      break;
    case SVTK_PHONG:
      os << "SVTK_PHONG\n";
      break;
    case SVTK_PBR:
      os << "SVTK_PBR\n";
      break;
    default:
      os << "unknown\n";
  }
  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Representation: ";
  switch (this->Representation)
  {
    case SVTK_POINTS:
      os << "SVTK_POINTS\n";
      break;
    case SVTK_WIREFRAME:
      os << "SVTK_WIREFRAME\n";
      break;
    case SVTK_SURFACE:
      os << "SVTK_SURFACE\n";
      break;
    default:
      os << "unknown\n";
  }
  os << indent << "Specular: " << this->Specular << "\n";
  os << indent << "Specular Color: (" << this->SpecularColor[0] << ", " << this->SpecularColor[1]
     << ", " << this->SpecularColor[2] << ")\n";
  os << indent << "Specular Power: " << this->SpecularPower << "\n";
  os << indent << "Backface Culling: " << (this->BackfaceCulling ? "On\n" : "Off\n");
  os << indent << "Frontface Culling: " << (this->FrontfaceCulling ? "On\n" : "Off\n");
  os << indent << "Point size: " << this->PointSize << "\n";
  os << indent << "Line width: " << this->LineWidth << "\n";
  os << indent << "Line stipple pattern: " << this->LineStipplePattern << "\n";
  os << indent << "Line stipple repeat factor: " << this->LineStippleRepeatFactor << "\n";
  os << indent << "Lighting: ";
  if (this->Lighting)
  {
    os << "On" << endl;
  }
  else
  {
    os << "Off" << endl;
  }
  os << indent << "RenderPointsAsSpheres: " << (this->RenderPointsAsSpheres ? "On" : "Off") << endl;
  os << indent << "RenderLinesAsTubes: " << (this->RenderLinesAsTubes ? "On" : "Off") << endl;

  os << indent << "Shading: " << (this->Shading ? "On" : "Off") << endl;

  os << indent << "MaterialName: " << (this->MaterialName ? this->MaterialName : "(none)") << endl;

  os << indent << "Color: (" << this->Color[0] << ", " << this->Color[1] << ", " << this->Color[2]
     << ")" << endl;
  os << indent << "EmissiveFactor: (" << this->EmissiveFactor[0] << ", " << this->EmissiveFactor[1]
     << ", " << this->EmissiveFactor[2] << ")" << endl;
  os << indent << "NormalScale: " << this->NormalScale << endl;
  os << indent << "OcclusionStrength: " << this->OcclusionStrength << endl;
  os << indent << "Metallic: " << this->Metallic << endl;
  os << indent << "Roughness: " << this->Roughness << endl;
}
