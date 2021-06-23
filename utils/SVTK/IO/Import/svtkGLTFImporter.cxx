/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGLTFImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGLTFImporter.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkEventForwarderCommand.h"
#include "svtkFloatArray.h"
#include "svtkGLTFDocumentLoader.h"
#include "svtkImageAppendComponents.h"
#include "svtkImageData.h"
#include "svtkImageExtractComponents.h"
#include "svtkImageResize.h"
#include "svtkInformation.h"
#include "svtkLight.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataTangents.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"
#include "svtkTransform.h"
#include "svtksys/SystemTools.hxx"

#include <algorithm>
#include <array>
#include <stack>

svtkStandardNewMacro(svtkGLTFImporter);

namespace
{
// Desired attenuation value when distanceToLight == lightRange
static const float MIN_LIGHT_ATTENUATION = 0.01;

/**
 * Builds a new svtkCamera object with properties from a glTF Camera struct
 */
//----------------------------------------------------------------------------
svtkSmartPointer<svtkCamera> GLTFCameraToSVTKCamera(const svtkGLTFDocumentLoader::Camera& gltfCam)
{
  svtkNew<svtkCamera> svtkCam;
  svtkCam->SetClippingRange(gltfCam.Znear, gltfCam.Zfar);
  if (gltfCam.IsPerspective)
  {
    svtkCam->SetParallelProjection(false);
    svtkCam->SetViewAngle(svtkMath::DegreesFromRadians(gltfCam.Yfov));
  }
  else
  {
    svtkCam->SetParallelProjection(true);
    svtkCam->SetParallelScale(gltfCam.Ymag);
  }
  return svtkCam;
}

/**
 * Create a svtkTexture object with a glTF texture as model. Sampling options are approximated.
 */
//----------------------------------------------------------------------------
svtkSmartPointer<svtkTexture> CreateSVTKTextureFromGLTFTexture(
  std::shared_ptr<svtkGLTFDocumentLoader::Model> model, int textureIndex,
  std::map<int, svtkSmartPointer<svtkTexture> >& existingTextures)
{

  if (existingTextures.count(textureIndex))
  {
    return existingTextures[textureIndex];
  }

  const svtkGLTFDocumentLoader::Texture& glTFTex = model->Textures[textureIndex];
  if (glTFTex.Source < 0 || glTFTex.Source >= static_cast<int>(model->Images.size()))
  {
    svtkErrorWithObjectMacro(nullptr, "Image not found");
    return nullptr;
  }

  const svtkGLTFDocumentLoader::Image& image = model->Images[glTFTex.Source];

  svtkNew<svtkTexture> texture;
  texture->SetColorModeToDirectScalars();
  texture->SetBlendingMode(svtkTexture::SVTK_TEXTURE_BLENDING_MODE_MODULATE);
  // Approximate filtering settings
  int nbSamplers = static_cast<int>(model->Samplers.size());
  if (glTFTex.Sampler >= 0 && glTFTex.Sampler < nbSamplers)
  {
    const svtkGLTFDocumentLoader::Sampler& sampler = model->Samplers[glTFTex.Sampler];
    if ((sampler.MinFilter == svtkGLTFDocumentLoader::Sampler::FilterType::NEAREST ||
          sampler.MinFilter == svtkGLTFDocumentLoader::Sampler::FilterType::LINEAR) &&
      (sampler.MagFilter == svtkGLTFDocumentLoader::Sampler::FilterType::NEAREST ||
        sampler.MagFilter == svtkGLTFDocumentLoader::Sampler::FilterType::LINEAR))
    {
      texture->MipmapOn();
    }
    else
    {
      texture->MipmapOff();
    }

    if (sampler.WrapS == svtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE ||
      sampler.WrapT == svtkGLTFDocumentLoader::Sampler::WrapType::CLAMP_TO_EDGE)
    {
      texture->RepeatOff();
      texture->EdgeClampOn();
    }
    else if (sampler.WrapS == svtkGLTFDocumentLoader::Sampler::WrapType::REPEAT ||
      sampler.WrapT == svtkGLTFDocumentLoader::Sampler::WrapType::REPEAT)
    {
      texture->RepeatOn();
      texture->EdgeClampOff();
    }
    else
    {
      svtkWarningWithObjectMacro(nullptr, "Mirrored texture wrapping is not supported!");
    }

    if (sampler.MinFilter == svtkGLTFDocumentLoader::Sampler::FilterType::LINEAR ||
      sampler.MinFilter == svtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_NEAREST ||
      sampler.MinFilter == svtkGLTFDocumentLoader::Sampler::FilterType::NEAREST_MIPMAP_LINEAR ||
      sampler.MinFilter == svtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_LINEAR ||
      sampler.MagFilter == svtkGLTFDocumentLoader::Sampler::FilterType::LINEAR ||
      sampler.MagFilter == svtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_NEAREST ||
      sampler.MagFilter == svtkGLTFDocumentLoader::Sampler::FilterType::NEAREST_MIPMAP_LINEAR ||
      sampler.MagFilter == svtkGLTFDocumentLoader::Sampler::FilterType::LINEAR_MIPMAP_LINEAR)
    {
      texture->InterpolateOn();
    }
  }
  else
  {
    texture->MipmapOn();
    texture->InterpolateOn();
    texture->EdgeClampOn();
  }

  svtkNew<svtkImageData> imageData;
  imageData->ShallowCopy(image.ImageData);

  texture->SetInputData(imageData);
  existingTextures[textureIndex] = texture;
  return texture;
}

//----------------------------------------------------------------------------
bool MaterialHasMultipleUVs(const svtkGLTFDocumentLoader::Material& material)
{
  int firstUV = material.PbrMetallicRoughness.BaseColorTexture.TexCoord;
  return (material.EmissiveTexture.Index >= 0 && material.EmissiveTexture.TexCoord != firstUV) ||
    (material.NormalTexture.Index >= 0 && material.NormalTexture.TexCoord != firstUV) ||
    (material.OcclusionTexture.Index >= 0 && material.OcclusionTexture.TexCoord != firstUV) ||
    (material.PbrMetallicRoughness.MetallicRoughnessTexture.Index >= 0 &&
      material.PbrMetallicRoughness.MetallicRoughnessTexture.TexCoord != firstUV);
}

//----------------------------------------------------------------------------
bool PrimitiveNeedsTangents(const std::shared_ptr<svtkGLTFDocumentLoader::Model> model,
  const svtkGLTFDocumentLoader::Primitive& primitive)
{
  // If no material is present, we don't need to generate tangents
  if (primitive.Material < 0 || primitive.Material >= static_cast<int>(model->Materials.size()))
  {
    return false;
  }
  svtkGLTFDocumentLoader::Material& material = model->Materials[primitive.Material];
  // If a normal map is present, we do need tangents
  int normalMapIndex = material.NormalTexture.Index;
  return normalMapIndex >= 0 && normalMapIndex < static_cast<int>(model->Textures.size());
}

//----------------------------------------------------------------------------
void ApplyGLTFMaterialToSVTKActor(std::shared_ptr<svtkGLTFDocumentLoader::Model> model,
  svtkGLTFDocumentLoader::Primitive& primitive, svtkSmartPointer<svtkActor> actor,
  std::map<int, svtkSmartPointer<svtkTexture> >& existingTextures)
{
  svtkGLTFDocumentLoader::Material& material = model->Materials[primitive.Material];

  bool hasMultipleUVs = MaterialHasMultipleUVs(material);
  if (hasMultipleUVs)
  {
    svtkWarningWithObjectMacro(
      nullptr, "Using multiple texture coordinates for the same model is not supported.");
  }
  auto property = actor->GetProperty();
  property->SetInterpolationToPBR();
  if (!material.PbrMetallicRoughness.BaseColorFactor.empty())
  {
    // Apply base material color
    actor->GetProperty()->SetColor(material.PbrMetallicRoughness.BaseColorFactor.data());
    actor->GetProperty()->SetMetallic(material.PbrMetallicRoughness.MetallicFactor);
    actor->GetProperty()->SetRoughness(material.PbrMetallicRoughness.RoughnessFactor);
    actor->GetProperty()->SetEmissiveFactor(material.EmissiveFactor.data());
  }

  if (material.AlphaMode != svtkGLTFDocumentLoader::Material::AlphaModeType::OPAQUE)
  {
    actor->ForceTranslucentOn();
  }

  // flip texture coordinates
  if (actor->GetPropertyKeys() == nullptr)
  {
    svtkNew<svtkInformation> info;
    actor->SetPropertyKeys(info);
  }
  double mat[] = { 1, 0, 0, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1 };
  actor->GetPropertyKeys()->Set(svtkProp::GeneralTextureTransform(), mat, 16);

  if (!material.DoubleSided)
  {
    actor->GetProperty()->BackfaceCullingOn();
  }

  int texIndex = material.PbrMetallicRoughness.BaseColorTexture.Index;
  if (texIndex >= 0 && texIndex < static_cast<int>(model->Textures.size()))
  {
    // set albedo texture
    svtkSmartPointer<svtkTexture> baseColorTex;
    baseColorTex = CreateSVTKTextureFromGLTFTexture(model, texIndex, existingTextures);
    baseColorTex->UseSRGBColorSpaceOn();
    property->SetBaseColorTexture(baseColorTex);

    // merge ambient occlusion and metallic/roughness, then set material texture
    int pbrTexIndex = material.PbrMetallicRoughness.MetallicRoughnessTexture.Index;
    if (pbrTexIndex >= 0 && pbrTexIndex < static_cast<int>(model->Textures.size()))
    {
      const svtkGLTFDocumentLoader::Texture& pbrTexture = model->Textures[pbrTexIndex];
      if (pbrTexture.Source >= 0 && pbrTexture.Source < static_cast<int>(model->Images.size()))
      {
        const svtkGLTFDocumentLoader::Image& pbrImage = model->Images[pbrTexture.Source];
        // While glTF 2.0 uses two different textures for Ambient Occlusion and Metallic/Roughness
        // values, SVTK only uses one, so we merge both textures into one.
        // If an Ambient Occlusion texture is present, we merge its first channel into the
        // metallic/roughness texture (AO is r, Roughness g and Metallic b) If no Ambient
        // Occlusion texture is present, we need to fill the metallic/roughness texture's first
        // channel with 255
        int aoTexIndex = material.OcclusionTexture.Index;
        if (!hasMultipleUVs && aoTexIndex >= 0 &&
          aoTexIndex < static_cast<int>(model->Textures.size()))
        {
          actor->GetProperty()->SetOcclusionStrength(material.OcclusionTextureStrength);
          const svtkGLTFDocumentLoader::Texture& aoTexture = model->Textures[aoTexIndex];
          const svtkGLTFDocumentLoader::Image& aoImage = model->Images[aoTexture.Source];
          svtkNew<svtkImageExtractComponents> redAO;
          // If sizes are different, resize the AO texture to the R/M texture's size
          std::array<svtkIdType, 3> aoSize = { { 0 } };
          std::array<svtkIdType, 3> pbrSize = { { 0 } };
          aoImage.ImageData->GetDimensions(aoSize.data());
          pbrImage.ImageData->GetDimensions(pbrSize.data());
          // compare dimensions
          if (aoSize != pbrSize)
          {
            svtkNew<svtkImageResize> resize;
            resize->SetInputData(aoImage.ImageData);
            resize->SetOutputDimensions(pbrSize[0], pbrSize[1], pbrSize[2]);
            resize->Update();
            redAO->SetInputConnection(resize->GetOutputPort(0));
          }
          else
          {
            redAO->SetInputData(aoImage.ImageData);
          }
          redAO->SetComponents(0);
          svtkNew<svtkImageExtractComponents> gbPbr;
          gbPbr->SetInputData(pbrImage.ImageData);
          gbPbr->SetComponents(1, 2);
          svtkNew<svtkImageAppendComponents> append;
          append->AddInputConnection(redAO->GetOutputPort());
          append->AddInputConnection(gbPbr->GetOutputPort());
          append->SetOutput(pbrImage.ImageData);
          append->Update();
        }
        else
        {
          pbrImage.ImageData->GetPointData()->GetScalars()->FillComponent(0, 255);
        }
        auto materialTex = CreateSVTKTextureFromGLTFTexture(model, pbrTexIndex, existingTextures);
        property->SetORMTexture(materialTex);
      }
    }

    // Set emissive texture
    int emissiveTexIndex = material.EmissiveTexture.Index;
    if (emissiveTexIndex >= 0 && emissiveTexIndex < static_cast<int>(model->Textures.size()))
    {
      auto emissiveTex = CreateSVTKTextureFromGLTFTexture(model, emissiveTexIndex, existingTextures);
      emissiveTex->UseSRGBColorSpaceOn();
      property->SetEmissiveTexture(emissiveTex);
    }
    // Set normal map
    int normalMapIndex = material.NormalTexture.Index;
    if (normalMapIndex >= 0 && normalMapIndex < static_cast<int>(model->Textures.size()))
    {
      actor->GetProperty()->SetNormalScale(material.NormalTextureScale);
      auto normalTex = CreateSVTKTextureFromGLTFTexture(model, normalMapIndex, existingTextures);
      property->SetNormalTexture(normalTex);
    }
  }
};

//----------------------------------------------------------------------------
void ApplyTransformToCamera(svtkSmartPointer<svtkCamera> cam, svtkSmartPointer<svtkTransform> transform)
{
  if (!cam || !transform)
  {
    return;
  }

  double position[3] = { 0.0 };
  double viewUp[3] = { 0.0 };
  double focus[3] = { 0.0 };

  transform->TransformPoint(cam->GetPosition(), position);
  transform->TransformVector(cam->GetViewUp(), viewUp);
  transform->TransformVector(cam->GetDirectionOfProjection(), focus);
  focus[0] -= position[0];
  focus[1] -= position[1];
  focus[2] -= position[2];

  cam->SetPosition(position);
  cam->SetFocalPoint(focus);
  cam->SetViewUp(viewUp);
}
}

//----------------------------------------------------------------------------
svtkGLTFImporter::~svtkGLTFImporter()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
int svtkGLTFImporter::ImportBegin()
{
  // Make sure we have a file to read.
  if (!this->FileName)
  {
    svtkErrorMacro("A FileName must be specified.");
    return 0;
  }

  this->Textures.clear();

  this->Loader = svtkSmartPointer<svtkGLTFDocumentLoader>::New();

  svtkNew<svtkEventForwarderCommand> forwarder;
  forwarder->SetTarget(this);
  this->Loader->AddObserver(svtkCommand::ProgressEvent, forwarder);

  // Check extension
  std::vector<char> glbBuffer;
  std::string extension = svtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  if (extension == ".glb")
  {
    if (!this->Loader->LoadFileBuffer(this->FileName, glbBuffer))
    {
      svtkErrorMacro("Error loading binary data");
      return 0;
    }
  }

  if (!this->Loader->LoadModelMetaDataFromFile(this->FileName))
  {
    svtkErrorMacro("Error loading model metadata");
    return 0;
  }
  if (!this->Loader->LoadModelData(glbBuffer))
  {
    svtkErrorMacro("Error loading model data");
    return 0;
  }
  if (!this->Loader->BuildModelSVTKGeometry())
  {
    svtkErrorMacro("Error building model svtk data");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkGLTFImporter::ImportActors(svtkRenderer* renderer)
{
  auto model = this->Loader->GetInternalModel();

  int scene = model->DefaultScene;

  // List of nodes to import
  std::stack<int> nodeIdStack;

  // Add root nodes to the stack
  for (int nodeId : model->Scenes[scene].Nodes)
  {
    nodeIdStack.push(nodeId);
  }

  this->OutputsDescription = "";

  // Iterate over tree
  while (!nodeIdStack.empty())
  {
    // Get current node
    int nodeId = nodeIdStack.top();
    nodeIdStack.pop();
    svtkGLTFDocumentLoader::Node& node = model->Nodes[nodeId];

    // Import node's geometry
    if (node.Mesh >= 0)
    {
      auto mesh = model->Meshes[node.Mesh];
      for (auto primitive : mesh.Primitives)
      {
        auto pointData = primitive.Geometry->GetPointData();

        svtkNew<svtkActor> actor;
        svtkNew<svtkPolyDataMapper> mapper;
        mapper->SetColorModeToDirectScalars();
        mapper->SetInterpolateScalarsBeforeMapping(true);

        if (pointData->GetTangents() == nullptr && PrimitiveNeedsTangents(model, primitive))
        {
          // Generate tangents
          svtkNew<svtkPolyDataTangents> tangents;
          tangents->SetInputData(primitive.Geometry);
          tangents->Update();
          mapper->SetInputConnection(tangents->GetOutputPort(0));
        }
        else
        {
          mapper->SetInputData(primitive.Geometry);
        }

        actor->SetMapper(mapper);
        actor->SetUserTransform(node.GlobalTransform);

        if (!mesh.Name.empty())
        {
          this->OutputsDescription += mesh.Name + " ";
        }
        this->OutputsDescription += "Primitive Geometry:\n";
        this->OutputsDescription +=
          svtkImporter::GetDataSetDescription(primitive.Geometry, svtkIndent(1));

        if (primitive.Material >= 0 &&
          primitive.Material < static_cast<int>(model->Materials.size()))
        {
          ApplyGLTFMaterialToSVTKActor(model, primitive, actor, this->Textures);
        }
        renderer->AddActor(actor);
      }
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
    }
  }
}

//----------------------------------------------------------------------------
void svtkGLTFImporter::ImportCameras(svtkRenderer* renderer)
{
  auto model = this->Loader->GetInternalModel();

  int scene = model->DefaultScene;

  // List of nodes to import
  std::stack<int> nodeIdStack;

  // Add root nodes to the stack
  for (int nodeId : model->Scenes[scene].Nodes)
  {
    nodeIdStack.push(nodeId);
  }

  // Iterate over tree
  while (!nodeIdStack.empty())
  {
    // Get current node
    int nodeId = nodeIdStack.top();
    nodeIdStack.pop();
    const svtkGLTFDocumentLoader::Node& node = model->Nodes[nodeId];

    // Import node's camera
    if (node.Camera >= 0 && node.Camera < static_cast<int>(model->Cameras.size()))
    {
      svtkGLTFDocumentLoader::Camera const& camera = model->Cameras[node.Camera];
      auto svtkCam = GLTFCameraToSVTKCamera(camera);
      ApplyTransformToCamera(svtkCam, node.GlobalTransform);
      renderer->SetActiveCamera(svtkCam);
      // Since the same glTF camera object can be used by multiple nodes (so with different
      // transforms), multiple svtkCameras are generated for the same glTF camera object, but with
      // different transforms.
      this->Cameras.push_back(svtkCam);
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
    }
  }
}

//----------------------------------------------------------------------------
void svtkGLTFImporter::ImportLights(svtkRenderer* renderer)
{
  // Check that lights extension is enabled
  const auto& extensions = this->Loader->GetUsedExtensions();
  if (std::find(extensions.begin(), extensions.end(), "KHR_lights_punctual") == extensions.end())
  {
    return;
  }

  // List of nodes to import
  std::stack<int> nodeIdStack;

  const auto& model = this->Loader->GetInternalModel();
  const auto& lights = model->ExtensionMetaData.KHRLightsPunctualMetaData.Lights;

  // Add root nodes to the stack
  for (int nodeId : model->Scenes[model->DefaultScene].Nodes)
  {
    nodeIdStack.push(nodeId);
  }

  // Iterate over tree
  while (!nodeIdStack.empty())
  {
    // Get current node
    int nodeId = nodeIdStack.top();
    nodeIdStack.pop();

    const svtkGLTFDocumentLoader::Node& node = model->Nodes[nodeId];
    const auto lightId = node.ExtensionMetaData.KHRLightsPunctualMetaData.Light;
    if (lightId >= 0 && lightId < static_cast<int>(lights.size()))
    {
      const auto& glTFLight = lights[lightId];
      // Add light
      svtkNew<svtkLight> light;
      light->SetColor(glTFLight.Color.data());
      light->SetTransformMatrix(node.GlobalTransform->GetMatrix());
      // Handle range
      if (glTFLight.Range > 0)
      {
        // Set quadratic values to get attenuation(range) ~= MIN_LIGHT_ATTENUATION
        light->SetAttenuationValues(
          1, 0, 1.0 / (glTFLight.Range * glTFLight.Range * MIN_LIGHT_ATTENUATION));
      }
      light->SetIntensity(glTFLight.Intensity);
      switch (glTFLight.Type)
      {
        case svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::DIRECTIONAL:
          light->SetPositional(false);
          break;
        case svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::POINT:
          light->SetPositional(true);
          // Set as point light
          light->SetConeAngle(90);
          break;
        case svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::SPOT:
          light->SetPositional(true);
          light->SetConeAngle(svtkMath::DegreesFromRadians(glTFLight.SpotOuterConeAngle));
          break;
      }
      renderer->AddLight(light);
    }

    // Add node's children to stack
    for (int childNodeId : node.Children)
    {
      nodeIdStack.push(childNodeId);
    }
  }
}

//----------------------------------------------------------------------------
void svtkGLTFImporter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkCamera> svtkGLTFImporter::GetCamera(unsigned int id)
{
  if (id >= this->Cameras.size())
  {
    svtkErrorMacro("Out of range camera index");
    return nullptr;
  }
  return this->Cameras[id];
}

//----------------------------------------------------------------------------
size_t svtkGLTFImporter::GetNumberOfCameras()
{
  return this->Cameras.size();
}
