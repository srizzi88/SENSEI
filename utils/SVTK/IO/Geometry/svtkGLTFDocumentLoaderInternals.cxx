/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGLTFDocumentLoader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGLTFDocumentLoaderInternals.h"

#include "svtkGLTFUtils.h"
#include "svtkMath.h"
#include "svtkMathUtilities.h"
#include "svtkTransform.h"
#include "svtk_jsoncpp.h"
#include "svtksys/FStream.hxx"
#include "svtksys/SystemTools.hxx"

#include <algorithm>
#include <sstream>

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadBuffer(
  const Json::Value& root, std::vector<char>& buffer, const std::string& glTFFileName)
{
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid buffer value");
    return false;
  }

  int byteLength = 0;
  svtksys::ifstream fin;

  std::string name = "";
  svtkGLTFUtils::GetStringValue(root["name"], name);

  if (!svtkGLTFUtils::GetIntValue(root["byteLength"], byteLength))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid buffer.byteLength value for buffer " << name);
    return false;
  }
  Json::Value uriRoot = root["uri"];
  if (uriRoot.empty())
  {
    return true;
  }
  std::string uri = root["uri"].asString();

  // Load buffer data
  if (!svtkGLTFUtils::GetBinaryBufferFromUri(uri, glTFFileName, buffer, byteLength))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid buffer.uri value for buffer " << name);
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadBuffers(bool firstBufferIsGLB)
{
  Json::Value bufferRoot;
  Json::CharReaderBuilder reader;
  std::string errs;
  std::stringstream JSONStream(this->Self->GetInternalModel()->BufferMetaData);
  // Parse json
  if (!Json::parseFromStream(reader, JSONStream, &bufferRoot, &errs))
  {
    svtkErrorWithObjectMacro(this->Self, "Could not parse JSON: " << errs);
    return false;
  }
  // Load buffers from disk
  for (const auto& glTFBuffer : bufferRoot)
  {
    std::vector<char> buffer;
    if (this->LoadBuffer(glTFBuffer, buffer, this->Self->GetInternalModel()->FileName))
    {
      if (buffer.empty() && this->Self->GetInternalModel()->Buffers.empty() && !firstBufferIsGLB)
      {
        svtkErrorWithObjectMacro(this->Self,
          "Invalid first buffer value for glb file. No buffer was loaded from the file.");
        return false;
      }
      if (firstBufferIsGLB && this->Self->GetInternalModel()->Buffers.size() == 1 &&
        !buffer.empty())
      {
        svtkErrorWithObjectMacro(
          this->Self, "Invalid first buffer value for glb file. buffer.uri should be undefined");
        return false;
      }
      this->Self->GetInternalModel()->Buffers.emplace_back(std::move(buffer));
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadFileMetaData(
  const std::string& fileName, Json::Value& gltfRoot)
{
  // Expect extension to be either .gltf or .glb
  std::string extension = svtksys::SystemTools::GetFilenameLastExtension(fileName);
  if (extension != ".gltf" && extension != ".glb")
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid file extension: " << extension << ". Expected '.gltf' or '.glb'");
    return false;
  }

  std::stringstream JSONstream;
  svtksys::ifstream fin;
  if (extension == ".glb")
  {
    // Get base information
    std::string magic;
    uint32_t version;
    uint32_t fileLength;
    std::vector<svtkGLTFUtils::ChunkInfoType> chunkInfo;
    if (!svtkGLTFUtils::ExtractGLBFileInformation(fileName, magic, version, fileLength, chunkInfo))
    {
      svtkErrorWithObjectMacro(this->Self, "Invalid binary glTF file");
      return false;
    }
    if (!svtkGLTFUtils::ValidateGLBFile(magic, version, fileLength, chunkInfo))
    {
      svtkErrorWithObjectMacro(this->Self, "Invalid binary glTF file");
      return false;
    }

    // Open the file in binary mode
    fin.open(fileName.c_str(), std::ios::binary | std::ios::in);
    if (!fin.is_open())
    {
      svtkErrorWithObjectMacro(this->Self, "Error opening file " << fileName);
      return false;
    }
    // Get JSON chunk's information (we know it exists and it's the first chunk)
    svtkGLTFUtils::ChunkInfoType& JSONChunkInfo = chunkInfo[0];
    // Jump to chunk data start
    fin.seekg(svtkGLTFUtils::GLBHeaderSize + svtkGLTFUtils::GLBChunkHeaderSize);
    // Read chunk data
    std::vector<char> JSONDataBuffer(JSONChunkInfo.second);
    fin.read(JSONDataBuffer.data(), JSONChunkInfo.second);
    JSONstream.write(JSONDataBuffer.data(), JSONChunkInfo.second);
  }
  else
  {
    // Copy whole file into string
    fin.open(fileName.c_str());
    if (!fin.is_open())
    {
      svtkErrorWithObjectMacro(this->Self, "Error opening file " << fileName);
      return false;
    }
    JSONstream << fin.rdbuf();
  }
  Json::CharReaderBuilder reader;
  std::string errs;
  // Parse json
  if (!Json::parseFromStream(reader, JSONstream, &gltfRoot, &errs))
  {
    svtkErrorWithObjectMacro(this->Self, << errs.c_str());
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadAccessor(
  const Json::Value& root, svtkGLTFDocumentLoader::Accessor& accessor)
{
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor value");
    return false;
  }

  accessor.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], accessor.Name);

  accessor.BufferView = -1;
  svtkGLTFUtils::GetIntValue(root["bufferView"], accessor.BufferView);
  accessor.ByteOffset = 0;
  svtkGLTFUtils::GetIntValue(root["byteOffset"], accessor.ByteOffset);
  if (accessor.ByteOffset < 0)
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid accessor.byteOffset value for accessor " << accessor.Name);
    return false;
  }
  int integerComponentType = 0;
  if (!svtkGLTFUtils::GetIntValue(root["componentType"], integerComponentType))
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid accessor.componentType value for accessor " << accessor.Name);
    return false;
  }

  accessor.ComponentTypeValue =
    static_cast<svtkGLTFDocumentLoader::ComponentType>(integerComponentType);

  switch (accessor.ComponentTypeValue)
  {
    case svtkGLTFDocumentLoader::ComponentType::BYTE:
    case svtkGLTFDocumentLoader::ComponentType::UNSIGNED_BYTE:
    case svtkGLTFDocumentLoader::ComponentType::SHORT:
    case svtkGLTFDocumentLoader::ComponentType::UNSIGNED_SHORT:
    case svtkGLTFDocumentLoader::ComponentType::UNSIGNED_INT:
    case svtkGLTFDocumentLoader::ComponentType::FLOAT:
      break;
    default:
      svtkErrorWithObjectMacro(
        this->Self, "Invalid accessor.componentType value for accessor " << accessor.Name);
      return false;
  }

  accessor.Normalized = false;
  svtkGLTFUtils::GetBoolValue(root["normalized"], accessor.Normalized);
  if (!svtkGLTFUtils::GetIntValue(root["count"], accessor.Count))
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid accessor.count value for accessor " << accessor.Name);
    return false;
  }
  if (accessor.Count < 1)
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid accessor.count value for accessor " << accessor.Name);
    return false;
  }

  std::string accessorTypeString;
  if (!svtkGLTFUtils::GetStringValue(root["type"], accessorTypeString))
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid accessor.type value for accessor " << accessor.Name);
    return false;
  }
  accessor.Type = AccessorTypeStringToEnum(accessorTypeString);
  if (accessor.Type == svtkGLTFDocumentLoader::AccessorType::INVALID)
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid accessor.type value for accessor " << accessor.Name);
    return false;
  }
  accessor.NumberOfComponents = svtkGLTFDocumentLoader::GetNumberOfComponentsForType(accessor.Type);
  if (accessor.NumberOfComponents == 0)
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid accessor.type value for accessor " << accessor.Name);
    return false;
  }
  // Load max and min
  if (!root["max"].empty() && !root["min"].empty())
  {
    if (!this->LoadAccessorBounds(root, accessor))
    {
      svtkErrorWithObjectMacro(this->Self,
        "Error loading accessor.max and accessor.min fields for accessor " << accessor.Name);
      return false;
    }
  }
  if (!root["sparse"].isNull())
  {
    if (!this->LoadSparse(root["sparse"], accessor.SparseObject))
    {
      svtkErrorWithObjectMacro(this->Self, "Invalid accessor object.");
      return false;
    }
    accessor.IsSparse = true;
  }
  else
  {
    accessor.IsSparse = false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadAccessorBounds(
  const Json::Value& root, svtkGLTFDocumentLoader::Accessor& accessor)
{
  // min
  const Json::Value& minArray = root["min"];
  if (!minArray.empty() && minArray.isArray())
  {
    if (minArray.size() != accessor.NumberOfComponents)
    {
      svtkErrorWithObjectMacro(
        this->Self, "Invalid accessor.min array size for accessor " << accessor.Name);
      return false;
    }
    svtkGLTFUtils::GetDoubleArray(minArray, accessor.Min);
    if (accessor.Min.size() != accessor.NumberOfComponents)
    {
      svtkErrorWithObjectMacro(this->Self, "Error loading accessor.min");
      return false;
    }
  }
  // max
  const Json::Value& maxArray = root["max"];
  if (!maxArray.empty() && maxArray.isArray())
  {
    if (maxArray.size() != accessor.NumberOfComponents)
    {
      svtkErrorWithObjectMacro(
        this->Self, "Invalid accessor.max array size for accessor " << accessor.Name);
      return false;
    }
    svtkGLTFUtils::GetDoubleArray(maxArray, accessor.Max);
    if (accessor.Max.size() != accessor.NumberOfComponents)
    {
      svtkErrorWithObjectMacro(this->Self, "Error loading accessor.max");
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadAnimation(
  const Json::Value& root, svtkGLTFDocumentLoader::Animation& animation)
{
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid animation value");
    return false;
  }

  animation.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], animation.Name);

  if ((root["channels"].empty() && !root["channels"].isArray()) ||
    (root["samplers"].empty() && !root["samplers"].isArray()))
  {
    svtkErrorWithObjectMacro(this->Self,
      "Invalid animation.channels and animation.samplers for animation " << animation.Name);
    return false;
  }

  // Load channel metadata
  for (const auto& channelRoot : root["channels"])
  {
    svtkGLTFDocumentLoader::Animation::Channel channel;
    if (!svtkGLTFUtils::GetIntValue(channelRoot["sampler"], channel.Sampler))
    {
      svtkErrorWithObjectMacro(
        this->Self, "Invalid animation.channel.sampler value for animation " << animation.Name);
      return false;
    }
    channel.TargetNode = -1;
    svtkGLTFUtils::GetIntValue(channelRoot["target"]["node"], channel.TargetNode);

    std::string targetPathString;
    if (!svtkGLTFUtils::GetStringValue(channelRoot["target"]["path"], targetPathString))
    {
      svtkErrorWithObjectMacro(
        this->Self, "Invalid animation.channel.target.path value for animation " << animation.Name);
      return false;
    }
    if (targetPathString == "translation")
    {
      channel.TargetPath = svtkGLTFDocumentLoader::Animation::Channel::PathType::TRANSLATION;
    }
    else if (targetPathString == "rotation")
    {
      channel.TargetPath = svtkGLTFDocumentLoader::Animation::Channel::PathType::ROTATION;
    }
    else if (targetPathString == "scale")
    {
      channel.TargetPath = svtkGLTFDocumentLoader::Animation::Channel::PathType::SCALE;
    }
    else if (targetPathString == "weights")
    {
      channel.TargetPath = svtkGLTFDocumentLoader::Animation::Channel::PathType::WEIGHTS;
    }
    else
    {
      svtkErrorWithObjectMacro(
        this->Self, "Invalid animation.channel.target.path value for animation " << animation.Name);
      return false;
    }
    animation.Channels.push_back(channel);
  }

  float maxDuration = 0;
  // Load sampler metadata
  for (const auto& samplerRoot : root["samplers"])
  {
    svtkGLTFDocumentLoader::Animation::Sampler sampler;
    if (!svtkGLTFUtils::GetUIntValue(samplerRoot["input"], sampler.Input))
    {
      svtkErrorWithObjectMacro(
        this->Self, "Invalid animation.sampler.input value for animation " << animation.Name);
      return false;
    }
    // Fetching animation duration from metadata
    if (sampler.Input < this->Self->GetInternalModel()->Accessors.size())
    {
      svtkGLTFDocumentLoader::Accessor& accessor =
        this->Self->GetInternalModel()->Accessors[sampler.Input];
      if (accessor.Max.empty())
      {
        svtkErrorWithObjectMacro(this->Self,
          "Empty accessor.max value for sampler input accessor. Max is mandatory in this case.");
        return false;
      }
      if (accessor.Max[0] > maxDuration)
      {
        maxDuration = accessor.Max[0];
      }
    }
    else
    {
      svtkErrorWithObjectMacro(this->Self, "Invalid sampler.input value.");
      return false;
    }
    if (!svtkGLTFUtils::GetUIntValue(samplerRoot["output"], sampler.Output))
    {
      svtkErrorWithObjectMacro(
        this->Self, "Invalid animation.sampler.output value for animation " << animation.Name);
      return false;
    }
    std::string interpolationString("LINEAR");
    svtkGLTFUtils::GetStringValue(samplerRoot["interpolation"], interpolationString);
    if (interpolationString == "LINEAR")
    {
      sampler.Interpolation = svtkGLTFDocumentLoader::Animation::Sampler::InterpolationMode::LINEAR;
    }
    else if (interpolationString == "STEP")
    {
      sampler.Interpolation = svtkGLTFDocumentLoader::Animation::Sampler::InterpolationMode::STEP;
    }
    else if (interpolationString == "CUBICSPLINE")
    {
      sampler.Interpolation =
        svtkGLTFDocumentLoader::Animation::Sampler::InterpolationMode::CUBICSPLINE;
    }
    else
    {
      svtkErrorWithObjectMacro(this->Self,
        "Invalid animation.sampler.interpolation value for animation " << animation.Name);
      return false;
    }
    animation.Samplers.push_back(sampler);
  }
  animation.Duration = maxDuration;
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadBufferView(
  const Json::Value& root, svtkGLTFDocumentLoader::BufferView& bufferView)
{
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid bufferView value");
    return false;
  }
  bufferView.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], bufferView.Name);

  if (!svtkGLTFUtils::GetIntValue(root["buffer"], bufferView.Buffer))
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid bufferView.buffer value for bufferView " << bufferView.Name);
    return false;
  }
  if (!svtkGLTFUtils::GetIntValue(root["byteLength"], bufferView.ByteLength))
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid bufferView.bytelength value for bufferView " << bufferView.Name);
    return false;
  }
  bufferView.ByteOffset = 0;
  bufferView.ByteStride = 0;
  bufferView.Target = 0;
  svtkGLTFUtils::GetIntValue(root["byteOffset"], bufferView.ByteOffset);
  svtkGLTFUtils::GetIntValue(root["byteStride"], bufferView.ByteStride);
  svtkGLTFUtils::GetIntValue(root["target"], bufferView.Target);
  if (bufferView.Target != 0 &&
    bufferView.Target !=
      static_cast<unsigned short>(svtkGLTFDocumentLoader::Target::ELEMENT_ARRAY_BUFFER) &&
    bufferView.Target != static_cast<unsigned short>(svtkGLTFDocumentLoader::Target::ARRAY_BUFFER))
  {
    svtkErrorWithObjectMacro(this->Self,
      "Invalid bufferView.target value. Expecting ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER");
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadCamera(
  const Json::Value& root, svtkGLTFDocumentLoader::Camera& camera)
{
  if (root.isNull() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid camera object");
    return false;
  }

  std::string type;
  if (!svtkGLTFUtils::GetStringValue(root["type"], type))
  {
    svtkErrorWithObjectMacro(this->Self, "camera.type field is required but not found");
    return false;
  }
  camera.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], camera.Name);

  Json::Value camRoot; // used to extract zfar and znear, can be either camera.orthographic or
                       // camera.perspective objects.

  if (type == "orthographic")
  {
    camRoot = root["orthographic"];
    camera.IsPerspective = false;
  }
  else if (type == "perspective")
  {
    camRoot = root["perspective"];
    camera.IsPerspective = true;
  }
  else
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid camera.type value. Expecting 'orthographic' or 'perspective'");
    return false;
  }

  if (!svtkGLTFUtils::GetDoubleValue(camRoot["znear"], camera.Znear))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid camera.znear value.");
    return false;
  }

  // zfar is only required for orthographic cameras
  // znear is required for both types, and has to be positive
  if (!svtkGLTFUtils::GetDoubleValue(camRoot["zfar"], camera.Zfar) && type == "orthographic")
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid camera.zfar value.");
    return false;
  }
  if (camera.Znear <= 0 && type == "orthographic" &&
    (camera.Zfar <= camera.Znear || camera.Zfar <= 0))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid camera.orthographic znear and zfar values");
    return false;
  }

  if (type == "orthographic")
  {
    if (!svtkGLTFUtils::GetDoubleValue(camRoot["xmag"], camera.Xmag))
    {
      svtkErrorWithObjectMacro(
        this->Self, "camera.orthographic.xmag field is required but not found");
      return false;
    }
    if (!svtkGLTFUtils::GetDoubleValue(camRoot["ymag"], camera.Ymag))
    {
      svtkErrorWithObjectMacro(
        this->Self, "camera.orthographic.ymag field is required but not found");
      return false;
    }
  }
  else if (type == "perspective")
  {
    if (svtkGLTFUtils::GetDoubleValue(camRoot["aspectRatio"], camera.AspectRatio))
    {
      if (camera.AspectRatio <= 0)
      {
        svtkErrorWithObjectMacro(this->Self, "Invalid camera.perpective.aspectRatio value");
        return false;
      }
    }
    if (!svtkGLTFUtils::GetDoubleValue(camRoot["yfov"], camera.Yfov))
    {
      svtkErrorWithObjectMacro(this->Self, "Invalid camera.perspective.yfov value");
      return false;
    }
    else if (camera.Yfov <= 0)
    {
      svtkErrorWithObjectMacro(this->Self, "Invalid camera.perspective.yfov value");
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadImage(
  const Json::Value& root, svtkGLTFDocumentLoader::Image& image)
{
  if (root.empty() || !root.isObject())
  {
    return false;
  }

  image.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], image.Name);

  if (!svtkGLTFUtils::GetStringValue(root["mimeType"], image.MimeType))
  {
    image.MimeType.clear();
  }
  else if (image.MimeType != "image/jpeg" && image.MimeType != "image/png")
  {
    svtkErrorWithObjectMacro(this->Self,
      "Invalid image.mimeType value. Must be either image/jpeg or image/png for image "
        << image.Name);
    return false;
  }
  // Read the bufferView index value, if it exists.
  image.BufferView = -1;
  if (svtkGLTFUtils::GetIntValue(root["bufferView"], image.BufferView))
  {
    if (image.MimeType.empty())
    {
      svtkErrorWithObjectMacro(this->Self,
        "Invalid image.mimeType value. It is required as image.bufferView is set for image "
          << image.Name);
      return false;
    }
  }
  else // Don't look for uri when bufferView is specified
  {
    // Read the image uri value if it exists
    if (!svtkGLTFUtils::GetStringValue(root["uri"], image.Uri))
    {
      svtkErrorWithObjectMacro(this->Self, "Invalid image.uri value for image " << image.Name);
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadMaterial(
  const Json::Value& root, svtkGLTFDocumentLoader::Material& material)
{
  double metallicFactor = 1;
  double roughnessFactor = 1;

  const auto& pbrRoot = root["pbrMetallicRoughness"];
  if (!pbrRoot.empty())
  {
    if (svtkGLTFUtils::GetDoubleValue(pbrRoot["metallicFactor"], metallicFactor))
    {
      if (metallicFactor < 0 || metallicFactor > 1)
      {
        svtkWarningWithObjectMacro(this->Self,
          "Invalid material.pbrMetallicRoughness.metallicFactor value. Using default "
          "value instead.");
        metallicFactor = 1;
      }
    }
    if (svtkGLTFUtils::GetDoubleValue(pbrRoot["roughnessFactor"], roughnessFactor))
    {
      if (roughnessFactor < 0 || roughnessFactor > 1)
      {
        svtkWarningWithObjectMacro(this->Self,
          "Invalid material.pbrMetallicRoughness.roughnessFactor value. Using default"
          " value instead.");
        roughnessFactor = 1;
      }
    }
    if (!pbrRoot["baseColorTexture"].isNull())
    {
      this->LoadTextureInfo(
        pbrRoot["baseColorTexture"], material.PbrMetallicRoughness.BaseColorTexture);
    }
    if (!pbrRoot["metallicRoughnessTexture"].isNull())
    {
      this->LoadTextureInfo(pbrRoot["metallicRoughnessTexture"],
        material.PbrMetallicRoughness.MetallicRoughnessTexture);
    }
    svtkGLTFUtils::GetDoubleArray(
      pbrRoot["baseColorFactor"], material.PbrMetallicRoughness.BaseColorFactor);
  }
  if (material.PbrMetallicRoughness.BaseColorFactor.size() !=
    svtkGLTFDocumentLoader::GetNumberOfComponentsForType(svtkGLTFDocumentLoader::AccessorType::VEC4))
  {
    material.PbrMetallicRoughness.BaseColorFactor.clear();
  }
  if (material.PbrMetallicRoughness.BaseColorFactor.empty())
  {
    material.PbrMetallicRoughness.BaseColorFactor.insert(
      material.PbrMetallicRoughness.BaseColorFactor.end(), { 1, 1, 1, 1 });
  }
  material.PbrMetallicRoughness.MetallicFactor = metallicFactor;
  material.PbrMetallicRoughness.RoughnessFactor = roughnessFactor;
  if (!root["normalTexture"].isNull())
  {
    this->LoadTextureInfo(root["normalTexture"], material.NormalTexture);
    material.NormalTextureScale = 1.0;
    svtkGLTFUtils::GetDoubleValue(root["normalTexture"]["scale"], material.NormalTextureScale);
  }
  if (!root["occlusionTexture"].isNull())
  {
    this->LoadTextureInfo(root["occlusionTexture"], material.OcclusionTexture);
    material.OcclusionTextureStrength = 1.0;
    svtkGLTFUtils::GetDoubleValue(
      root["occlusionTexture"]["strength"], material.OcclusionTextureStrength);
  }
  if (!root["emissiveTexture"].isNull())
  {
    this->LoadTextureInfo(root["emissiveTexture"], material.EmissiveTexture);
  }
  svtkGLTFUtils::GetDoubleArray(root["emissiveFactor"], material.EmissiveFactor);
  if (material.EmissiveFactor.size() !=
    svtkGLTFDocumentLoader::GetNumberOfComponentsForType(svtkGLTFDocumentLoader::AccessorType::VEC3))
  {
    material.EmissiveFactor.clear();
  }
  if (material.EmissiveFactor.empty())
  {
    material.EmissiveFactor.insert(material.EmissiveFactor.end(), { 0, 0, 0 });
  }

  std::string alphaMode = "OPAQUE";
  svtkGLTFUtils::GetStringValue(root["alphaMode"], alphaMode);
  material.AlphaMode = this->MaterialAlphaModeStringToEnum(alphaMode);

  material.AlphaCutoff = 0.5;
  svtkGLTFUtils::GetDoubleValue(root["alphaCutoff"], material.AlphaCutoff);
  if (material.AlphaCutoff < 0)
  {
    svtkWarningWithObjectMacro(
      this->Self, "Invalid material.alphaCutoff value. Using default value instead.");
    material.AlphaCutoff = 0.5;
  }

  material.DoubleSided = false;
  svtkGLTFUtils::GetBoolValue(root["doubleSided"], material.DoubleSided);

  material.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], material.Name);

  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadMesh(
  const Json::Value& root, svtkGLTFDocumentLoader::Mesh& mesh)
{
  if (root.empty() || !root.isObject())
  {
    return false;
  }

  if (!svtkGLTFUtils::GetStringValue(root["name"], mesh.Name))
  {
    mesh.Name = "";
  }

  // Load primitives
  for (const auto& glTFPrimitive : root["primitives"])
  {
    svtkGLTFDocumentLoader::Primitive primitive;
    if (this->LoadPrimitive(glTFPrimitive, primitive))
    {
      mesh.Primitives.emplace_back(std::move(primitive));
    }
  }

  // Load morph weights
  if (!svtkGLTFUtils::GetFloatArray(root["weights"], mesh.Weights))
  {
    mesh.Weights.clear();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadNode(
  const Json::Value& root, svtkGLTFDocumentLoader::Node& node)
{
  node.Camera = -1;
  svtkGLTFUtils::GetIntValue(root["camera"], node.Camera);

  node.Children.clear();
  svtkGLTFUtils::GetIntArray(root["children"], node.Children);

  node.Skin = -1;
  svtkGLTFUtils::GetIntValue(root["skin"], node.Skin);

  node.Mesh = -1;
  svtkGLTFUtils::GetIntValue(root["mesh"], node.Mesh);

  // Load matrix value
  std::vector<double> matrixValues;
  node.Matrix = svtkSmartPointer<svtkMatrix4x4>::New();
  node.Matrix->Identity();

  // A node can define either a 'matrix' property, or any of the three 'rotation', 'translation' and
  // 'scale' properties, not both.
  if (svtkGLTFUtils::GetDoubleArray(root["matrix"], matrixValues))
  {
    // If the node has defined a skin, it can't define 'matrix'
    if (node.Skin >= 0)
    {
      svtkErrorWithObjectMacro(this->Self, "Invalid node.matrix value with node.skin defined.");
      return false;
    }
    if (matrixValues.size() ==
      svtkGLTFDocumentLoader::GetNumberOfComponentsForType(
        svtkGLTFDocumentLoader::AccessorType::MAT4))
    {
      node.Matrix->DeepCopy(matrixValues.data());
      node.Matrix->Transpose();
      node.TRSLoaded = false;
    }
  }
  else
  {
    // Load translation, rotation and scale values
    if (svtkGLTFUtils::GetFloatArray(root["scale"], node.InitialScale))
    {
      if (node.InitialScale.size() !=
        svtkGLTFDocumentLoader::GetNumberOfComponentsForType(
          svtkGLTFDocumentLoader::AccessorType::VEC3))
      {
        svtkWarningWithObjectMacro(
          this->Self, "Invalid node.scale array size. Using default scale for node " << node.Name);
        node.InitialScale.clear();
      }
    }
    if (node.InitialScale.empty())
    {
      // Default values
      node.InitialScale.insert(node.InitialScale.end(), { 1, 1, 1 });
    }
    if (svtkGLTFUtils::GetFloatArray(root["translation"], node.InitialTranslation))
    {
      if (node.InitialTranslation.size() != 3)
      {
        svtkWarningWithObjectMacro(this->Self,
          "Invalid node.translation array size. Using default translation for node " << node.Name);
        node.InitialTranslation.clear();
      }
    }
    if (node.InitialTranslation.empty())
    {
      // Default values
      node.InitialTranslation.insert(node.InitialTranslation.end(), { 0, 0, 0 });
    }
    if (svtkGLTFUtils::GetFloatArray(root["rotation"], node.InitialRotation))
    {
      float rotationLengthSquared = 0;
      for (float rotationValue : node.InitialRotation)
      {
        rotationLengthSquared += rotationValue * rotationValue;
      }
      if (!svtkMathUtilities::NearlyEqual<float>(rotationLengthSquared, 1))
      {
        svtkWarningWithObjectMacro(this->Self,
          "Invalid node.rotation value. Using normalized rotation for node " << node.Name);
        float rotationLength = sqrt(rotationLengthSquared);
        for (float& rotationValue : node.InitialRotation)
        {
          rotationValue /= rotationLength;
        }
      }
      if (node.InitialRotation.size() !=
        svtkGLTFDocumentLoader::GetNumberOfComponentsForType(
          svtkGLTFDocumentLoader::AccessorType::VEC4))
      {
        svtkWarningWithObjectMacro(this->Self,
          "Invalid node.rotation array size. Using default rotation for node " << node.Name);
        node.InitialRotation.clear();
      }
    }
    if (node.InitialRotation.empty())
    {
      // Default value
      node.InitialRotation.insert(node.InitialRotation.end(), { 0, 0, 0, 1 });
    }
    node.TRSLoaded = true;
  }

  node.Transform = svtkSmartPointer<svtkTransform>::New();
  // Update the node with its initial transform values
  node.UpdateTransform();

  if (!svtkGLTFUtils::GetFloatArray(root["weights"], node.InitialWeights))
  {
    node.InitialWeights.clear();
  }

  node.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], node.Name);

  // Load extensions if necessary
  if (!this->Self->GetUsedExtensions().empty() && root["extensions"].isObject())
  {
    this->LoadNodeExtensions(root["extensions"], node.ExtensionMetaData);
  }
  return true;
}

//-----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadSampler(
  const Json::Value& root, svtkGLTFDocumentLoader::Sampler& sampler)
{
  using Sampler = svtkGLTFDocumentLoader::Sampler;
  if (!root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid sampler object");
    return false;
  }

  if (root.size() == 0)
  {
    sampler.MagFilter = Sampler::FilterType::LINEAR_MIPMAP_LINEAR;
    sampler.MinFilter = Sampler::FilterType::LINEAR_MIPMAP_LINEAR;
    sampler.WrapT = Sampler::WrapType::REPEAT;
    sampler.WrapS = Sampler::WrapType::REPEAT;
    return true;
  }

  int tempIntValue = 0;
  if (!svtkGLTFUtils::GetIntValue(root["magFilter"], tempIntValue))
  {
    sampler.MagFilter = svtkGLTFDocumentLoader::Sampler::FilterType::NEAREST;
  }
  else
  {
    switch (static_cast<svtkGLTFDocumentLoader::Sampler::FilterType>(tempIntValue))
    {
      case Sampler::FilterType::LINEAR:
      case Sampler::FilterType::NEAREST:
        sampler.MagFilter = static_cast<Sampler::FilterType>(tempIntValue);
        break;
      default:
        sampler.MagFilter = Sampler::FilterType::NEAREST;
        svtkWarningWithObjectMacro(this->Self, "Invalid sampler.magFilter value.");
    }
  }

  if (!svtkGLTFUtils::GetIntValue(root["minFilter"], tempIntValue))
  {
    sampler.MinFilter = Sampler::FilterType::NEAREST;
  }
  else
  {
    switch (static_cast<Sampler::FilterType>(tempIntValue))
    {
      case Sampler::FilterType::LINEAR:
      case Sampler::FilterType::LINEAR_MIPMAP_LINEAR:
      case Sampler::FilterType::LINEAR_MIPMAP_NEAREST:
      case Sampler::FilterType::NEAREST:
      case Sampler::FilterType::NEAREST_MIPMAP_LINEAR:
      case Sampler::FilterType::NEAREST_MIPMAP_NEAREST:
        sampler.MinFilter = static_cast<Sampler::FilterType>(tempIntValue);
        break;
      default:
        sampler.MinFilter = Sampler::FilterType::NEAREST;
        svtkWarningWithObjectMacro(this->Self, "Invalid sampler.minFilter value.");
    }
  }

  if (!svtkGLTFUtils::GetIntValue(root["wrapS"], tempIntValue))
  {
    sampler.WrapS = Sampler::WrapType::REPEAT;
  }
  else
  {
    switch (static_cast<Sampler::WrapType>(tempIntValue))
    {
      case Sampler::WrapType::REPEAT:
      case Sampler::WrapType::MIRRORED_REPEAT:
      case Sampler::WrapType::CLAMP_TO_EDGE:
        sampler.WrapS = static_cast<Sampler::WrapType>(tempIntValue);
        break;
      default:
        sampler.WrapS = Sampler::WrapType::REPEAT;
        svtkWarningWithObjectMacro(this->Self, "Invalid sampler.wrapS value.");
    }
  }

  if (!svtkGLTFUtils::GetIntValue(root["wrapT"], tempIntValue))
  {
    sampler.WrapT = Sampler::WrapType::REPEAT;
  }
  else
  {
    switch (static_cast<Sampler::WrapType>(tempIntValue))
    {
      case Sampler::WrapType::REPEAT:
      case Sampler::WrapType::MIRRORED_REPEAT:
      case Sampler::WrapType::CLAMP_TO_EDGE:
        sampler.WrapT = static_cast<Sampler::WrapType>(tempIntValue);
        break;
      default:
        sampler.WrapT = Sampler::WrapType::REPEAT;
        svtkWarningWithObjectMacro(this->Self, "Invalid sampler.wrapT value.");
    }
  }

  sampler.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], sampler.Name);

  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadPrimitive(
  const Json::Value& root, svtkGLTFDocumentLoader::Primitive& primitive)
{
  if (root.empty() || !root.isObject())
  {
    return false;
  }

  // Load mode
  primitive.Mode = GL_TRIANGLES;
  svtkGLTFUtils::GetIntValue(root["mode"], primitive.Mode);
  switch (primitive.Mode)
  {
    case GL_POINTS:
      primitive.CellSize = 1;
      break;
    case GL_LINES:
    case GL_LINE_LOOP:
    case GL_LINE_STRIP:
      primitive.CellSize = 2;
      break;
    case GL_TRIANGLES:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
      primitive.CellSize = 3;
      break;
  }

  primitive.Material = -1; // default material
  svtkGLTFUtils::GetIntValue(root["material"], primitive.Material);

  primitive.IndicesId = -1;
  svtkGLTFUtils::GetIntValue(root["indices"], primitive.IndicesId);
  const auto& glTFAttributes = root["attributes"];
  if (!glTFAttributes.empty() && glTFAttributes.isObject())
  {
    for (const auto& glTFAttribute : glTFAttributes.getMemberNames())
    {
      int indice;
      if (svtkGLTFUtils::GetIntValue(glTFAttributes[glTFAttribute], indice))
      {
        primitive.AttributeIndices[glTFAttribute] = indice;
      }
    }
  }

  // Load morph targets
  const auto& glTFMorphTargets = root["targets"];
  if (!glTFMorphTargets.empty() && glTFMorphTargets.isArray())
  {
    for (const auto& glTFMorphTarget : glTFMorphTargets)
    {
      svtkGLTFDocumentLoader::MorphTarget target;
      int indice;
      for (const auto& glTFAttribute : glTFMorphTarget.getMemberNames())
      {
        if (svtkGLTFUtils::GetIntValue(glTFMorphTarget[glTFAttribute], indice))
        {
          target.AttributeIndices[glTFAttribute] = indice;
        }
      }
      primitive.Targets.push_back(target);
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadScene(
  const Json::Value& root, svtkGLTFDocumentLoader::Scene& scene)
{
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid scene object");
    return false;
  }
  if (!svtkGLTFUtils::GetUIntArray(root["nodes"], scene.Nodes))
  {
    scene.Nodes.clear();
  }

  scene.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], scene.Name);

  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadSkin(
  const Json::Value& root, svtkGLTFDocumentLoader::Skin& skin)
{
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid skin object");
    return false;
  }

  skin.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], skin.Name);

  skin.Skeleton = -1;
  svtkGLTFUtils::GetIntValue(root["skeleton"], skin.Skeleton);

  skin.InverseBindMatricesAccessorId = -1;
  svtkGLTFUtils::GetIntValue(root["inverseBindMatrices"], skin.InverseBindMatricesAccessorId);

  if (!svtkGLTFUtils::GetIntArray(root["joints"], skin.Joints))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid skin.joints value for skin " << skin.Name);
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadSparse(
  const Json::Value& root, svtkGLTFDocumentLoader::Accessor::Sparse& sparse)
{
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor.sparse object");
    return false;
  }
  if (!svtkGLTFUtils::GetIntValue(root["count"], sparse.Count))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor.sparse.count value");
    return false;
  }
  const Json::Value& indices = root["indices"];
  const Json::Value& values = root["values"];
  if (indices.empty() || values.empty() || !indices.isObject() || !values.isObject())
  {
    svtkErrorWithObjectMacro(
      this->Self, "Invalid accessor.sparse.indices or accessor.sparse.values value");
    return false;
  }
  if (!svtkGLTFUtils::GetIntValue(indices["bufferView"], sparse.IndicesBufferView))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor.sparse.indices.bufferView value");
    return false;
  }
  if (!svtkGLTFUtils::GetIntValue(indices["byteOffset"], sparse.IndicesByteOffset))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor.sparse.indices.byteOffset value");
    return false;
  }
  int intIndicesComponentTypes = 0;
  if (!svtkGLTFUtils::GetIntValue(indices["componentType"], intIndicesComponentTypes))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor.sparse.indices.componentType value");
    return false;
  }
  if (intIndicesComponentTypes < static_cast<int>(svtkGLTFDocumentLoader::ComponentType::BYTE) ||
    intIndicesComponentTypes > static_cast<int>(svtkGLTFDocumentLoader::ComponentType::FLOAT))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor.sparse.componentType value");
    return false;
  }
  sparse.IndicesComponentType =
    static_cast<svtkGLTFDocumentLoader::ComponentType>(intIndicesComponentTypes);
  if (!svtkGLTFUtils::GetIntValue(values["bufferView"], sparse.ValuesBufferView))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor.sparse.values.bufferView value");
    return false;
  }
  if (!svtkGLTFUtils::GetIntValue(values["byteOffset"], sparse.ValuesByteOffset))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid accessor.sparse.values.byteOffset value");
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadTexture(
  const Json::Value& root, svtkGLTFDocumentLoader::Texture& texture)
{
  /**
   * This loads a glTF object from a Json value, no files are loaded by this function.
   * Apart from the 'name' field, glTF texture objects contain two integer indices: one to an
   * image object (the object that references to an actual image file), and one to a sampler
   * object (which specifies filter and wrapping options for a texture).
   */
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid texture object.");
    return false;
  }
  texture.Sampler = -1;
  svtkGLTFUtils::GetIntValue(root["sampler"], texture.Sampler);
  texture.Source = -1;
  svtkGLTFUtils::GetIntValue(root["source"], texture.Source);
  texture.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], texture.Name);

  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadTextureInfo(
  const Json::Value& root, svtkGLTFDocumentLoader::TextureInfo& textureInfo)
{
  if (root.empty() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid textureInfo object");
    return false;
  }
  textureInfo.Index = -1;
  if (!svtkGLTFUtils::GetIntValue(root["index"], textureInfo.Index))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid textureInfo.index value");
    return false;
  }
  if (textureInfo.Index < 0)
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid textureInfo.index value");
    return false;
  }

  textureInfo.TexCoord = 0;
  svtkGLTFUtils::GetIntValue(root["texCoord"], textureInfo.TexCoord);

  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadModelMetaDataFromFile(
  std::string& fileName, std::vector<std::string>& extensionsUsedByLoader)
{
  extensionsUsedByLoader.clear();

  Json::Value root;
  if (!this->LoadFileMetaData(fileName, root))
  {
    svtkErrorWithObjectMacro(this->Self, "Failed to load file: " << fileName);
    return false;
  }

  // Load asset
  Json::Value glTFAsset = root["asset"];
  if (glTFAsset.empty() || !glTFAsset.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid asset value");
    return 0;
  }

  // check minversion and version
  if (!svtkGLTFUtils::CheckVersion(glTFAsset))
  {
    svtkErrorWithObjectMacro(this->Self, "Unsupported or invalid glTF version");
    return 0;
  }

  // Check for extensions
  const auto& supportedExtensions = this->Self->GetSupportedExtensions();
  for (const auto& extensionRequiredByModel : root["extensionsRequired"])
  {
    if (!extensionRequiredByModel.isString())
    {
      svtkWarningWithObjectMacro(
        this->Self, "Invalid extensions.extensionsRequired value. Ignoring this value.");
      continue;
    }
    // This is only for warnings. extensionsRequired is a subset of extensionsUsed, which is what is
    // used to fill extensionsUsedByLoader.
    if (!std::any_of(supportedExtensions.begin(), supportedExtensions.end(),
          [&extensionRequiredByModel](
            const std::string& value) { return value == extensionRequiredByModel.asString(); }))
    {
      svtkWarningWithObjectMacro(this->Self,
        "glTF extension " << extensionRequiredByModel.asString()
                          << " is required in this model, but not supported by this loader. The "
                             "extension will be ignored.");
    }
  }
  for (const auto& extensionUsedByModel : root["extensionsUsed"])
  {
    if (!extensionUsedByModel.isString())
    {
      svtkWarningWithObjectMacro(
        this->Self, "Invalid extensions.extensionsUsed value. Ignoring this value.");
      continue;
    }
    if (std::any_of(supportedExtensions.begin(), supportedExtensions.end(),
          [&extensionUsedByModel](
            const std::string& value) { return value == extensionUsedByModel.asString(); }))
    {
      extensionsUsedByLoader.push_back(extensionUsedByModel.asString());
    }
    else
    {
      svtkWarningWithObjectMacro(this->Self,
        "glTF extension " << extensionUsedByModel.asString()
                          << " is used in this model, but not supported by this loader. The "
                             "extension will be ignored.");
    }
  }

  // Load Accessors
  this->Self->GetInternalModel()->Accessors.reserve(root["accessors"].size());
  for (const auto& gltfAccessor : root["accessors"])
  {
    svtkGLTFDocumentLoader::Accessor accessor;
    if (this->LoadAccessor(gltfAccessor, accessor))
    {
      this->Self->GetInternalModel()->Accessors.emplace_back(std::move(accessor));
    }
  }

  // Load animations
  this->Self->GetInternalModel()->Animations.reserve(root["animations"].size());
  for (const auto& glTFAnimation : root["animations"])
  {
    svtkGLTFDocumentLoader::Animation animation;
    if (this->LoadAnimation(glTFAnimation, animation))
    {
      this->Self->GetInternalModel()->Animations.emplace_back(std::move(animation));
    }
  }

  // Load BufferViews
  this->Self->GetInternalModel()->BufferViews.reserve(root["bufferViews"].size());
  for (const auto& glTFBufferView : root["bufferViews"])
  {
    svtkGLTFDocumentLoader::BufferView bufferView;
    if (this->LoadBufferView(glTFBufferView, bufferView))
    {
      this->Self->GetInternalModel()->BufferViews.emplace_back(std::move(bufferView));
    }
  }

  // Load cameras
  this->Self->GetInternalModel()->Cameras.reserve(root["cameras"].size());
  for (const auto& glTFCamera : root["cameras"])
  {
    svtkGLTFDocumentLoader::Camera camera;
    if (this->LoadCamera(glTFCamera, camera))
    {
      this->Self->GetInternalModel()->Cameras.emplace_back(std::move(camera));
    }
  }

  // Load images
  this->Self->GetInternalModel()->Images.reserve(root["images"].size());
  for (const auto& glTFImage : root["images"])
  {
    svtkGLTFDocumentLoader::Image image;
    if (this->LoadImage(glTFImage, image))
    {
      this->Self->GetInternalModel()->Images.emplace_back(std::move(image));
    }
  }

  // Load materials
  this->Self->GetInternalModel()->Materials.reserve(root["materials"].size());
  for (const auto& glTFMaterial : root["materials"])
  {
    svtkGLTFDocumentLoader::Material material;
    if (this->LoadMaterial(glTFMaterial, material))
    {
      this->Self->GetInternalModel()->Materials.emplace_back(std::move(material));
    }
  }

  // Load meshes
  this->Self->GetInternalModel()->Meshes.reserve(root["meshes"].size());
  for (const auto& glTFMesh : root["meshes"])
  {
    svtkGLTFDocumentLoader::Mesh mesh;
    if (this->LoadMesh(glTFMesh, mesh))
    {
      this->Self->GetInternalModel()->Meshes.emplace_back(std::move(mesh));
    }
  }

  // Load nodes
  this->Self->GetInternalModel()->Nodes.reserve(root["nodes"].size());
  for (const auto& glTFNode : root["nodes"])
  {
    svtkGLTFDocumentLoader::Node node;
    if (this->LoadNode(glTFNode, node))
    {
      this->Self->GetInternalModel()->Nodes.emplace_back(std::move(node));
    }
  }

  // Load samplers
  this->Self->GetInternalModel()->Samplers.reserve(root["samplers"].size());
  for (const auto& glTFSampler : root["samplers"])
  {
    svtkGLTFDocumentLoader::Sampler sampler;
    if (this->LoadSampler(glTFSampler, sampler))
    {
      this->Self->GetInternalModel()->Samplers.emplace_back(std::move(sampler));
    }
  }

  // Load scenes
  this->Self->GetInternalModel()->Scenes.reserve(root["scenes"].size());
  for (const auto& glTFScene : root["scenes"])
  {
    svtkGLTFDocumentLoader::Scene scene;
    if (this->LoadScene(glTFScene, scene))
    {
      this->Self->GetInternalModel()->Scenes.emplace_back(std::move(scene));
    }
  }

  // Load default scene
  this->Self->GetInternalModel()->DefaultScene = 0;
  if (!svtkGLTFUtils::GetIntValue(root["scene"], this->Self->GetInternalModel()->DefaultScene))
  {
    int nbScenes = static_cast<int>(this->Self->GetInternalModel()->Scenes.size());
    if (this->Self->GetInternalModel()->DefaultScene < 0 ||
      this->Self->GetInternalModel()->DefaultScene >= nbScenes)
    {
      this->Self->GetInternalModel()->DefaultScene = 0;
    }
  }

  // Load skins
  this->Self->GetInternalModel()->Skins.reserve(root["skins"].size());
  for (const auto& glTFSkin : root["skins"])
  {
    svtkGLTFDocumentLoader::Skin skin;
    if (this->LoadSkin(glTFSkin, skin))
    {
      this->Self->GetInternalModel()->Skins.emplace_back(std::move(skin));
    }
  }

  // Load textures
  this->Self->GetInternalModel()->Textures.reserve(root["textures"].size());
  for (const auto& glTFTexture : root["textures"])
  {
    svtkGLTFDocumentLoader::Texture texture;
    if (this->LoadTexture(glTFTexture, texture))
    {
      this->Self->GetInternalModel()->Textures.emplace_back(std::move(texture));
    }
  }

  // Load extensions
  if (!this->Self->GetUsedExtensions().empty() && root["extensions"].isObject())
  {
    this->LoadExtensions(root["extensions"], this->Self->GetInternalModel()->ExtensionMetaData);
  }

  // Save buffer metadata but don't load buffers
  if (!root["buffers"].empty() && root["buffers"].isArray())
  {
    this->Self->GetInternalModel()->BufferMetaData = root["buffers"].toStyledString();
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadKHRLightsPunctualNodeExtension(const Json::Value& root,
  svtkGLTFDocumentLoader::Node::Extensions::KHRLightsPunctual& lightsExtension)
{
  if (root.isNull() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid node.extensions.KHR_lights_punctual object");
    return false;
  }
  if (!svtkGLTFUtils::GetIntValue(root["light"], lightsExtension.Light))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid node.extensions.KHR_lights_punctual.light value");
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadKHRLightsPunctualExtension(
  const Json::Value& root, svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual& lightsExtension)
{
  lightsExtension.Lights.reserve(root["lights"].size());
  for (const auto& glTFLight : root["lights"])
  {
    svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light light;
    if (this->LoadKHRLightsPunctualExtensionLight(glTFLight, light))
    {
      lightsExtension.Lights.emplace_back(std::move(light));
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadKHRLightsPunctualExtensionLight(
  const Json::Value& root, svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light& light)
{
  if (root.isNull() || !root.isObject())
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid KHR_lights_punctual.lights object");
    return false;
  }

  light.SpotInnerConeAngle = 0;
  light.SpotOuterConeAngle = 0;

  static const double DefaultSpotOuterConeAngle = svtkMath::Pi() / 4.0;
  static const double DefaultSpotInnerConeAngle = 0;
  static const double MaxSpotInnerConeAngle = svtkMath::Pi() / 2.0;

  // Load name
  light.Name = "";
  svtkGLTFUtils::GetStringValue(root["name"], light.Name);

  // Load type and type-specific values
  std::string type;
  if (!svtkGLTFUtils::GetStringValue(root["type"], type))
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid KHR_lights_punctual.lights.type value.");
    return false;
  }
  if (type == "directional")
  {
    light.Type =
      svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::DIRECTIONAL;
  }
  else if (type == "point")
  {
    light.Type = svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::POINT;
  }
  else if (type == "spot")
  {
    light.Type = svtkGLTFDocumentLoader::Extensions::KHRLightsPunctual::Light::LightType::SPOT;
    // Load innerConeAngle and outerConeAngle
    auto glTFSpot = root["spot"];
    if (glTFSpot.isNull() || !glTFSpot.isObject())
    {
      svtkErrorWithObjectMacro(
        this->Self, "Invalid KHR_lights_punctual.lights.spot object for spot type");
      return false;
    }
    light.SpotOuterConeAngle = DefaultSpotOuterConeAngle;
    if (svtkGLTFUtils::GetDoubleValue(glTFSpot["outerConeAngle"], light.SpotOuterConeAngle))
    {
      if (light.SpotOuterConeAngle <= 0 || light.SpotOuterConeAngle > MaxSpotInnerConeAngle)
      {
        svtkWarningWithObjectMacro(
          this->Self, "Invalid KHR_lights_punctual.lights.spot.outerConeAngle value");
        light.SpotOuterConeAngle = DefaultSpotOuterConeAngle;
      }
    }
    light.SpotInnerConeAngle = DefaultSpotInnerConeAngle;
    if (svtkGLTFUtils::GetDoubleValue(glTFSpot["innerConeAngle"], light.SpotInnerConeAngle))
    {
      if (light.SpotInnerConeAngle < 0 || light.SpotInnerConeAngle >= light.SpotOuterConeAngle)
      {
        svtkWarningWithObjectMacro(
          this->Self, "Invalid KHR_lights_punctual.lights.spot.innerConeAngle value");
        light.SpotInnerConeAngle = DefaultSpotInnerConeAngle;
      }
    }
  }
  else
  {
    svtkErrorWithObjectMacro(this->Self, "Invalid KHR_lights_punctual.lights.type value");
    return false;
  }

  // Load color
  if (!svtkGLTFUtils::GetDoubleArray(root["color"], light.Color) || light.Color.size() != 3)
  {
    light.Color = std::vector<double>(3, 1.0f);
  }

  // Load intensity
  light.Intensity = 1.0f;
  svtkGLTFUtils::GetDoubleValue(root["intensity"], light.Intensity);

  // Load range
  light.Range = 0;
  if (svtkGLTFUtils::GetDoubleValue(root["range"], light.Range))
  {
    // Must be positive
    if (light.Range < 0)
    {
      light.Range = 0;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadNodeExtensions(
  const Json::Value& root, svtkGLTFDocumentLoader::Node::Extensions& nodeExtensions)
{
  for (const std::string& usedExtensionName : this->Self->GetUsedExtensions())
  {
    if (usedExtensionName == "KHR_lights_punctual" && root["KHR_lights_punctual"].isObject())
    {
      this->LoadKHRLightsPunctualNodeExtension(
        root["KHR_lights_punctual"], nodeExtensions.KHRLightsPunctualMetaData);
    }
    // New node extensions should be loaded from here
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFDocumentLoaderInternals::LoadExtensions(
  const Json::Value& root, svtkGLTFDocumentLoader::Extensions& extensions)
{
  for (const std::string& usedExtensionName : this->Self->GetUsedExtensions())
  {
    if (usedExtensionName == "KHR_lights_punctual" && root["KHR_lights_punctual"].isObject())
    {
      this->LoadKHRLightsPunctualExtension(
        root["KHR_lights_punctual"], extensions.KHRLightsPunctualMetaData);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
svtkGLTFDocumentLoader::AccessorType svtkGLTFDocumentLoaderInternals::AccessorTypeStringToEnum(
  std::string typeName)
{
  if (typeName == "VEC2")
  {
    return svtkGLTFDocumentLoader::AccessorType::VEC2;
  }
  else if (typeName == "VEC3")
  {
    return svtkGLTFDocumentLoader::AccessorType::VEC3;
  }
  else if (typeName == "VEC4")
  {
    return svtkGLTFDocumentLoader::AccessorType::VEC4;
  }
  else if (typeName == "MAT2")
  {
    return svtkGLTFDocumentLoader::AccessorType::MAT2;
  }
  else if (typeName == "MAT3")
  {
    return svtkGLTFDocumentLoader::AccessorType::MAT3;
  }
  else if (typeName == "MAT4")
  {
    return svtkGLTFDocumentLoader::AccessorType::MAT4;
  }
  else if (typeName == "SCALAR")
  {
    return svtkGLTFDocumentLoader::AccessorType::SCALAR;
  }
  else
  {
    return svtkGLTFDocumentLoader::AccessorType::INVALID;
  }
}

//----------------------------------------------------------------------------
svtkGLTFDocumentLoader::Material::AlphaModeType
svtkGLTFDocumentLoaderInternals::MaterialAlphaModeStringToEnum(std::string alphaModeString)
{
  if (alphaModeString == "MASK")
  {
    return svtkGLTFDocumentLoader::Material::AlphaModeType::MASK;
  }
  else if (alphaModeString == "BLEND")
  {
    return svtkGLTFDocumentLoader::Material::AlphaModeType::BLEND;
  }
  return svtkGLTFDocumentLoader::Material::AlphaModeType::OPAQUE;
}
