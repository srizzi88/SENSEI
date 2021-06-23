/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGLTFUtils.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGLTFUtils.h"

#include "svtkBase64Utilities.h"
#include "svtk_jsoncpp.h"
#include "svtksys/FStream.hxx"
#include "svtksys/RegularExpression.hxx"
#include "svtksys/SystemTools.hxx"

#include <fstream>
#include <iostream>

#define MIN_GLTF_VERSION "2.0"
//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetBoolValue(const Json::Value& root, bool& value)
{
  if (root.empty() || !root.isBool())
  {
    return false;
  }
  value = root.asBool();
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetIntValue(const Json::Value& root, int& value)
{
  if (root.empty() || !root.isInt())
  {
    return false;
  }
  value = root.asInt();
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetUIntValue(const Json::Value& root, unsigned int& value)
{
  if (root.empty() || !root.isUInt())
  {
    return false;
  }
  value = root.asUInt();
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetDoubleValue(const Json::Value& root, double& value)
{
  if (root.empty() || !root.isDouble())
  {
    return false;
  }
  value = root.asDouble();
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetIntArray(const Json::Value& root, std::vector<int>& value)
{
  if (root.empty() || !root.isArray())
  {
    return false;
  }
  value.reserve(root.size());
  for (const auto& intValue : root)
  {
    if (intValue.empty() && !intValue.isInt())
    {
      value.clear();
      return false;
    }
    value.push_back(intValue.asInt());
  }
  if (value.size() == 0)
  {
    value.clear();
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetUIntArray(const Json::Value& root, std::vector<unsigned int>& value)
{
  if (root.empty() || !root.isArray())
  {
    return false;
  }
  value.reserve(root.size());
  for (const auto& uIntValue : root)
  {
    if (uIntValue.empty() && !uIntValue.isUInt())
    {
      value.clear();
      return false;
    }
    value.push_back(uIntValue.asUInt());
  }
  if (value.size() == 0)
  {
    value.clear();
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetFloatArray(const Json::Value& root, std::vector<float>& value)
{
  if (root.empty() || !root.isArray())
  {
    return false;
  }
  value.reserve(root.size());
  for (const auto& floatValue : root)
  {
    if (floatValue.empty() && !floatValue.isDouble())
    {
      value.clear();
      return false;
    }
    value.push_back(floatValue.asDouble());
  }
  if (value.size() == 0)
  {
    value.clear();
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetDoubleArray(const Json::Value& root, std::vector<double>& value)
{
  if (root.empty() || !root.isArray())
  {
    return false;
  }
  value.reserve(root.size());
  for (const auto& doubleValue : root)
  {
    if (doubleValue.empty() && !doubleValue.isDouble())
    {
      value.clear();
      return false;
    }
    value.push_back(doubleValue.asDouble());
  }
  if (value.size() == 0)
  {
    value.clear();
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetStringValue(const Json::Value& root, std::string& value)
{
  if (root.empty() || !root.isString())
  {
    return false;
  }
  value = root.asString();
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::CheckVersion(const Json::Value& glTFAsset)
{
  Json::Value assetMinVersion = glTFAsset["minVersion"];
  Json::Value assetVersion = glTFAsset["version"];

  if (!assetMinVersion.empty())
  {
    if (assetMinVersion != MIN_GLTF_VERSION)
    {
      return false;
    }
  }
  else if (!assetVersion.empty())
  {
    std::string assetVersionStr = glTFAsset["version"].asString();
    if (assetVersionStr != MIN_GLTF_VERSION)
    {
      return false;
    }
  }
  else
  {
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
std::string svtkGLTFUtils::GetResourceFullPath(
  const std::string& resourcePath, const std::string& glTFFilePath)
{
  // Check for relative path
  if (!svtksys::SystemTools::FileIsFullPath(resourcePath.c_str()))
  {
    // Append relative path to base dir
    std::string baseDirPath = svtksys::SystemTools::GetParentDirectory(glTFFilePath);
    return svtksys::SystemTools::CollapseFullPath(resourcePath, baseDirPath);
  }
  return resourcePath;
}

//----------------------------------------------------------------------------
std::string svtkGLTFUtils::GetDataUriMimeType(const std::string& uri)
{
  svtksys::RegularExpression regex("^data:.*;");
  if (regex.find(uri))
  {
    // Remove preceding 'data:' and trailing semicolon
    size_t start = regex.start(0) + 5;
    size_t end = regex.end(0) - 1;
    return uri.substr(start, end - start);
  }
  return std::string();
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::GetBinaryBufferFromUri(const std::string& uri, const std::string& glTFFilePath,
  std::vector<char>& buffer, size_t bufferSize)
{
  // Check for data-uri
  if (svtksys::SystemTools::StringStartsWith(uri, "data:"))
  {
    // Extract base64 buffer
    std::vector<std::string> tokens;
    svtksys::SystemTools::Split(uri, tokens, ',');
    std::string base64Buffer = *(tokens.end() - 1); // Last token contains the base64 data
    buffer.resize(bufferSize);
    svtkBase64Utilities::DecodeSafely(reinterpret_cast<const unsigned char*>(base64Buffer.c_str()),
      base64Buffer.size(), reinterpret_cast<unsigned char*>(buffer.data()), bufferSize);
  }
  // Load buffer from file
  else
  {
    svtksys::ifstream fin;

    std::string bufferPath = GetResourceFullPath(uri, glTFFilePath);

    // Open file
    fin.open(bufferPath.c_str(), ios::binary);
    if (!fin.is_open())
    {
      return false;
    }
    // Check file length
    unsigned int len = svtksys::SystemTools::FileLength(bufferPath);
    if (len != bufferSize)
    {
      fin.close();
      return false;
    }
    // Load data
    buffer.resize(bufferSize);
    fin.read(buffer.data(), bufferSize);
    fin.close();
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::ExtractGLBFileInformation(const std::string& fileName, std::string& magic,
  uint32_t& version, uint32_t& fileLength, std::vector<svtkGLTFUtils::ChunkInfoType>& chunkInfo)
{
  svtksys::ifstream fin;
  fin.open(fileName.c_str(), std::ios::binary | std::ios::in);
  if (!fin.is_open())
  {
    return false;
  }

  // Load glb header
  // Read first word ("magic")
  char magicBuffer[svtkGLTFUtils::GLBWordSize];
  fin.read(magicBuffer, svtkGLTFUtils::GLBWordSize);
  magic = std::string(magicBuffer, magicBuffer + svtkGLTFUtils::GLBWordSize);
  // Read version
  fin.read(reinterpret_cast<char*>(&version), svtkGLTFUtils::GLBWordSize);
  // Read file length
  fin.read(reinterpret_cast<char*>(&fileLength), svtkGLTFUtils::GLBWordSize);
  // Check equality between extracted and actual file lengths
  fin.seekg(0, std::ios::end);
  if (fin.tellg() != std::streampos(fileLength))
  {
    return false;
  }
  fin.seekg(svtkGLTFUtils::GLBHeaderSize);
  // Read chunks until end of file
  while (fin.tellg() < fileLength)
  {
    // Read chunk length
    uint32_t chunkDataSize;
    fin.read(reinterpret_cast<char*>(&chunkDataSize), svtkGLTFUtils::GLBWordSize);

    // Read chunk type
    char chunkTypeBuffer[svtkGLTFUtils::GLBWordSize];
    fin.read(chunkTypeBuffer, svtkGLTFUtils::GLBWordSize);
    std::string chunkType(chunkTypeBuffer, chunkTypeBuffer + svtkGLTFUtils::GLBWordSize);
    chunkInfo.push_back(svtkGLTFUtils::ChunkInfoType(chunkType, chunkDataSize));
    // Jump to next chunk
    fin.seekg(chunkDataSize, std::ios::cur);
  }
  return true;
}

//----------------------------------------------------------------------------
bool svtkGLTFUtils::ValidateGLBFile(const std::string& magic, uint32_t version, uint32_t fileLength,
  std::vector<svtkGLTFUtils::ChunkInfoType> chunkInfo)
{
  // Check header
  if (magic != "glTF" || version != svtkGLTFUtils::GLBVersion)
  {
    return false;
  }
  if (chunkInfo.size() == 0)
  {
    return false;
  }
  // Compute sum of chunk sizes and check that first chunk is json
  size_t lengthSum = 0;
  for (size_t chunkNumber = 0; chunkNumber < chunkInfo.size(); chunkNumber++)
  {
    if (chunkNumber == 0)
    {
      if (chunkInfo[chunkNumber].first != "JSON")
      {
        return false;
      }
    }
    lengthSum += chunkInfo[chunkNumber].second;
  }
  // Compute total size
  lengthSum += svtkGLTFUtils::GLBHeaderSize + chunkInfo.size() * svtkGLTFUtils::GLBChunkHeaderSize;
  // Check for inconsistent chunk sizes
  return fileLength == lengthSum;
}
