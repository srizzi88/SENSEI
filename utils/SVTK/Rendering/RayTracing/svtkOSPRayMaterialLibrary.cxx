/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayMaterialLibrary.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOSPRayMaterialLibrary.h"

#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkImageData.h"
#include "svtkJPEGReader.h"
#include "svtkOSPRayMaterialHelpers.h"
#include "svtkObjectFactory.h"
#include "svtkPNGReader.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"
#include "svtkXMLImageDataReader.h"
#include "svtkXMLImageDataWriter.h"
#include "svtk_jsoncpp.h"
#include "svtksys/FStream.hxx"
#include "svtksys/SystemTools.hxx"

#include <fstream>
#include <string>
#include <vector>

#include <sys/types.h>

namespace
{
const std::map<std::string, std::map<std::string, std::string> > Aliases = {
  { "OBJMaterial",
    { { "colorMap", "map_Kd" }, { "map_kd", "map_Kd" }, { "map_ks", "map_Ks" },
      { "map_ns", "map_Ns" }, { "map_bump", "map_Bump" }, { "normalMap", "map_Bump" },
      { "BumpMap", "map_Bump" }, { "color", "Kd" }, { "kd", "Kd" }, { "alpha", "d" },
      { "ks", "Ks" }, { "ns", "Ns" }, { "tf", "Tf" } } },
  { "ThinGlass", { { "color", "attenuationColor" }, { "transmission", "attenuationColor" } } },
  { "MetallicPaint", { { "color", "baseColor" } } },
  { "Glass",
    { { "etaInside", "eta" }, { "etaOutside", "eta" },
      { "attenuationColorOutside", "attenuationColor" } } },
  { "Principled", {} }, { "CarPaint", {} }, { "Metal", {} }, { "Alloy", {} }, { "Luminous", {} }
};

std::string FindRealName(const std::string& materialType, const std::string& alias)
{
  auto matAliasesIt = ::Aliases.find(materialType);
  if (matAliasesIt != ::Aliases.end())
  {
    auto realNameIt = matAliasesIt->second.find(alias);
    if (realNameIt != matAliasesIt->second.end())
    {
      return realNameIt->second;
    }
  }
  return alias;
}
}

typedef std::map<std::string, std::vector<double> > NamedVariables;
typedef std::map<std::string, svtkSmartPointer<svtkTexture> > NamedTextures;

class svtkOSPRayMaterialLibraryInternals
{
public:
  svtkOSPRayMaterialLibraryInternals() {}
  ~svtkOSPRayMaterialLibraryInternals() {}

  std::set<std::string> NickNames;
  std::map<std::string, std::string> ImplNames;
  std::map<std::string, NamedVariables> VariablesFor;
  std::map<std::string, NamedTextures> TexturesFor;
};

// ----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOSPRayMaterialLibrary);

// ----------------------------------------------------------------------------
svtkOSPRayMaterialLibrary::svtkOSPRayMaterialLibrary()
{
  this->Internal = new svtkOSPRayMaterialLibraryInternals;
}

// ----------------------------------------------------------------------------
svtkOSPRayMaterialLibrary::~svtkOSPRayMaterialLibrary()
{
  delete this->Internal;
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Materials:\n";
  for (auto mat : this->Internal->NickNames)
  {
    os << indent << "  - " << mat << "( " << this->Internal->ImplNames[mat] << " )" << endl;
    for (auto v : this->Internal->VariablesFor[mat])
    {
      os << indent << "    - " << v.first << endl;
    }
  }
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::AddMaterial(const std::string& nickname, const std::string& implname)
{
  auto& dic = svtkOSPRayMaterialLibrary::GetParametersDictionary();

  if (dic.find(implname) != dic.end())
  {
    this->Internal->NickNames.insert(nickname);
    this->Internal->ImplNames[nickname] = implname;
  }
  else
  {
    svtkGenericWarningMacro(
      "Unknown material type \"" << implname << "\" for material named \"" << nickname << "\"");
  }
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::RemoveMaterial(const std::string& nickname)
{
  this->Internal->NickNames.erase(nickname);
  this->Internal->ImplNames.erase(nickname);
  this->Internal->VariablesFor.erase(nickname);
  this->Internal->TexturesFor.erase(nickname);
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::AddTexture(
  const std::string& nickname, const std::string& texname, svtkTexture* tex)
{
  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], texname);

  auto& dic = svtkOSPRayMaterialLibrary::GetParametersDictionary();
  auto& params = dic.at(this->Internal->ImplNames[nickname]);
  if (params.find(realname) != params.end())
  {
    NamedTextures& tsForNickname = this->Internal->TexturesFor[nickname];
    tsForNickname[realname] = tex;
  }
  else
  {
    svtkGenericWarningMacro("Unknown parameter \"" << texname << "\" for type \""
                                                  << this->Internal->ImplNames[nickname] << "\"");
  }
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::RemoveTexture(
  const std::string& nickname, const std::string& texname)
{
  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], texname);
  this->Internal->TexturesFor[nickname].erase(realname);
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::RemoveAllTextures(const std::string& nickname)
{
  this->Internal->TexturesFor[nickname].clear();
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::AddShaderVariable(
  const std::string& nickname, const std::string& varname, int numVars, const double* x)
{
  std::vector<double> w;
  w.assign(x, x + numVars);

  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], varname);

  auto& dic = svtkOSPRayMaterialLibrary::GetParametersDictionary();
  auto& params = dic.at(this->Internal->ImplNames[nickname]);
  if (params.find(realname) != params.end())
  {
    NamedVariables& vsForNickname = this->Internal->VariablesFor[nickname];
    vsForNickname[realname] = std::move(w);
  }
  else
  {
    svtkGenericWarningMacro("Unknown parameter \"" << varname << "\" for type \""
                                                  << this->Internal->ImplNames[nickname] << "\"");
  }
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::RemoveShaderVariable(
  const std::string& nickname, const std::string& varname)
{
  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], varname);
  this->Internal->VariablesFor[nickname].erase(realname);
}

// ----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::RemoveAllShaderVariables(const std::string& nickname)
{
  this->Internal->VariablesFor[nickname].clear();
}

// ----------------------------------------------------------------------------
bool svtkOSPRayMaterialLibrary::ReadFile(const char* filename)
{
  return this->InternalParse(filename, true);
}

// ----------------------------------------------------------------------------
bool svtkOSPRayMaterialLibrary::ReadBuffer(const char* filename)
{
  return this->InternalParse(filename, false);
}

// ----------------------------------------------------------------------------
bool svtkOSPRayMaterialLibrary::InternalParse(const char* filename, bool fromfile)
{
  if (!filename)
  {
    return false;
  }
  if (fromfile && !svtksys::SystemTools::FileExists(filename, true))
  {
    return false;
  }

  std::istream* doc;
  if (fromfile)
  {
    doc = new svtksys::ifstream(filename, std::ios::binary);
  }
  else
  {
    doc = new std::istringstream(filename);
  }
  bool retOK = false;
  if (std::string(filename).rfind(".mtl") != std::string::npos)
  {
    retOK = this->InternalParseMTL(filename, fromfile, doc);
  }
  else
  {
    retOK = this->InternalParseJSON(filename, fromfile, doc);
  }
  delete doc;
  return retOK;
}

// ----------------------------------------------------------------------------
bool svtkOSPRayMaterialLibrary::InternalParseJSON(
  const char* filename, bool fromfile, std::istream* doc)
{
  Json::Value root;
  std::string errs;
  Json::CharReaderBuilder jreader;
  bool ok = Json::parseFromStream(jreader, *doc, &root, &errs);
  if (!ok)
  {
    return false;
  }
  if (!root.isMember("family"))
  {
    svtkErrorMacro("Not a materials file. Must have \"family\"=\"...\" entry.");
    return false;
  }
  const Json::Value family = root["family"];
  if (family.asString() != "OSPRay")
  {
    svtkErrorMacro("Unsupported materials file. Family is not \"OSPRay\".");
    return false;
  }
  if (!root.isMember("version"))
  {
    svtkErrorMacro("Not a materials file. Must have \"version\"=\"...\" entry.");
    return false;
  }
  const Json::Value version = root["version"];
  if (version.asString() != "0.0")
  {
    svtkErrorMacro("Unsupported materials file. Version is not \"0.0\".");
    return false;
  }
  if (!root.isMember("materials"))
  {
    svtkErrorMacro("Not a materials file. Must have \"materials\"={...} entry.");
    return false;
  }

  const Json::Value materials = root["materials"];
  std::vector<std::string> ikeys = materials.getMemberNames();
  for (size_t i = 0; i < ikeys.size(); ++i)
  {
    const std::string& nickname = ikeys[i];
    const Json::Value nextmat = materials[nickname];
    if (!nextmat.isMember("type"))
    {
      svtkErrorMacro(
        "Invalid material " << nickname << " must have \"type\"=\"...\" entry, ignoring.");
      continue;
    }

    // keep a record so others know this material is available
    this->Internal->NickNames.insert(nickname);

    const std::string& implname = nextmat["type"].asString();
    this->Internal->ImplNames[nickname] = implname;
    if (nextmat.isMember("textures"))
    {
      std::string parentDir = svtksys::SystemTools::GetParentDirectory(filename);
      const Json::Value textures = nextmat["textures"];
      for (const std::string& tname : textures.getMemberNames())
      {
        const Json::Value nexttext = textures[tname];
        const char* tfname = nexttext.asCString();
        svtkSmartPointer<svtkTexture> textr = svtkSmartPointer<svtkTexture>::New();
        svtkSmartPointer<svtkJPEGReader> jpgReader = svtkSmartPointer<svtkJPEGReader>::New();
        svtkSmartPointer<svtkPNGReader> pngReader = svtkSmartPointer<svtkPNGReader>::New();
        if (fromfile)
        {
          std::string tfullname = parentDir + "/" + tfname;
          if (!svtksys::SystemTools::FileExists(tfullname.c_str(), true))
          {
            cerr << "No such texture file " << tfullname << " skipping" << endl;
            continue;
          }
          if (tfullname.substr(tfullname.length() - 3) == "png")
          {
            pngReader->SetFileName(tfullname.c_str());
            pngReader->Update();
            textr->SetInputConnection(pngReader->GetOutputPort(0));
          }
          else
          {
            jpgReader->SetFileName(tfullname.c_str());
            jpgReader->Update();
            textr->SetInputConnection(jpgReader->GetOutputPort(0));
          }
        }
        else
        {
          svtkSmartPointer<svtkXMLImageDataReader> reader =
            svtkSmartPointer<svtkXMLImageDataReader>::New();
          reader->ReadFromInputStringOn();
          reader->SetInputString(tfname);
          textr->SetInputConnection(reader->GetOutputPort(0));
        }
        textr->Update();
        this->AddTexture(nickname, tname, textr);
      }
    }
    if (nextmat.isMember("doubles"))
    {
      const Json::Value doubles = nextmat["doubles"];
      for (const std::string& vname : doubles.getMemberNames())
      {
        const Json::Value nexttext = doubles[vname];
        std::vector<double> vals(nexttext.size());
        for (size_t k = 0; k < nexttext.size(); ++k)
        {
          const Json::Value nv = nexttext[static_cast<int>(k)];
          vals[k] = nv.asDouble();
        }
        this->AddShaderVariable(nickname, vname, nexttext.size(), vals.data());
      }
    }
  }

  return true;
}

namespace
{
static std::string trim(std::string s)
{
  size_t start = 0;
  while ((start < s.length()) && (isspace(s[start])))
  {
    start++;
  }
  size_t end = s.length();
  while ((end > start) && (isspace(s[end - 1])))
  {
    end--;
  }
  return s.substr(start, end - start);
}
}

// ----------------------------------------------------------------------------
bool svtkOSPRayMaterialLibrary::InternalParseMTL(
  const char* filename, bool fromfile, std::istream* doc)
{
  std::string str;
  std::string nickname = "";
  std::string implname = "OBJMaterial";

  const std::vector<std::string> singles{ "d ", "Ks ", "alpha ", "roughness ", "eta ",
    "thickness " };
  const std::vector<std::string> triples{ "Ka ", "color ", "Kd ", "Ks " };
  const std::vector<std::string> textures{ "map_d ", "map_Kd ", "map_kd ", "colorMap ", "map_Ks ",
    "map_ks ", "map_Ns ", "map_ns ", "map_Bump", "map_bump", "normalMap", "bumpMap" };

  while (getline(*doc, str))
  {
    std::string tstr = trim(str);
    std::string lkey;

    // a new material
    lkey = "newmtl ";
    if (tstr.compare(0, lkey.size(), lkey) == 0)
    {
      nickname = trim(tstr.substr(lkey.size()));
      this->Internal->NickNames.insert(nickname);
      this->Internal->ImplNames[nickname] = "OBJMaterial";
    }

    // ospray type of the material, if not obj
    lkey = "type ";
    if (tstr.compare(0, lkey.size(), lkey) == 0)
    {
      // this non standard entry is a quick way to break out of
      // objmaterial and use one of the ospray specific materials
      implname = trim(tstr.substr(lkey.size()));
      if (implname == "matte")
      {
        implname = "OBJMaterial";
      }
      if (implname == "glass")
      {
        implname = "ThinGlass";
      }
      if (implname == "metal")
      {
        implname = "Metal";
      }
      if (implname == "metallicPaint")
      {
        implname = "MetallicPaint";
      }
      this->Internal->ImplNames[nickname] = implname;
    }

    // grab all the single valued settings we see
    std::vector<std::string>::const_iterator sit1 = singles.begin();
    while (sit1 != singles.end())
    {
      std::string key = *sit1;
      ++sit1;
      if (tstr.compare(0, key.size(), key) == 0)
      {
        std::string v = tstr.substr(key.size());
        double dv = 0.;
        bool OK = false;
        try
        {
          dv = std::stod(v);
          OK = true;
        }
        catch (const std::invalid_argument&)
        {
        }
        catch (const std::out_of_range&)
        {
        }
        if (OK)
        {
          double vals[1] = { dv };
          this->AddShaderVariable(nickname, key.substr(0, key.size() - 1).c_str(), 1, vals);
        }
      }
    }

    // grab all the triple valued settings we see
    std::vector<std::string>::const_iterator sit3 = triples.begin();
    while (sit3 != triples.end())
    {
      std::string key = *sit3;
      ++sit3;
      if (tstr.compare(0, key.size(), key) == 0)
      {
        std::string vs = tstr.substr(key.size());
        size_t loc1 = vs.find(" ");
        size_t loc2 = vs.find(" ", loc1 + 1);
        std::string v1 = vs.substr(0, loc1);
        std::string v2 = vs.substr(loc1 + 1, loc2);
        std::string v3 = vs.substr(loc2 + 1);
        double d1 = 0;
        double d2 = 0;
        double d3 = 0;
        bool OK = false;
        try
        {
          d1 = std::stod(v1);
          d2 = std::stod(v2);
          d3 = std::stod(v3);
          OK = true;
        }
        catch (const std::invalid_argument&)
        {
        }
        catch (const std::out_of_range&)
        {
        }
        if (OK)
        {
          double vals[3] = { d1, d2, d3 };
          this->AddShaderVariable(nickname, key.substr(0, key.size() - 1).c_str(), 3, vals);
        }
      }
    }

    // grab all the textures we see
    std::vector<std::string>::const_iterator tit = textures.begin();
    while (tit != textures.end())
    {
      std::string key = *tit;
      ++tit;

      std::string tfname = "";
      if (tstr.compare(0, key.size(), key) == 0)
      {
        tfname = trim(tstr.substr(key.size()));
      }
      if (tfname != "")
      {
        svtkSmartPointer<svtkTexture> textr = svtkSmartPointer<svtkTexture>::New();
        svtkSmartPointer<svtkJPEGReader> jpgReader = svtkSmartPointer<svtkJPEGReader>::New();
        svtkSmartPointer<svtkPNGReader> pngReader = svtkSmartPointer<svtkPNGReader>::New();
        if (fromfile)
        {
          std::string parentDir = svtksys::SystemTools::GetParentDirectory(filename);
          std::string tfullname = parentDir + "/" + tfname;
          if (!svtksys::SystemTools::FileExists(tfullname.c_str(), true))
          {
            cerr << "No such texture file " << tfullname << " skipping" << endl;
            continue;
          }
          if (tfullname.substr(tfullname.length() - 3) == "png")
          {
            pngReader->SetFileName(tfullname.c_str());
            pngReader->Update();
            textr->SetInputConnection(pngReader->GetOutputPort(0));
          }
          else
          {
            jpgReader->SetFileName(tfullname.c_str());
            jpgReader->Update();
            textr->SetInputConnection(jpgReader->GetOutputPort(0));
          }
        }
        else
        {
          svtkSmartPointer<svtkXMLImageDataReader> reader =
            svtkSmartPointer<svtkXMLImageDataReader>::New();
          reader->ReadFromInputStringOn();
          reader->SetInputString(tfname);
          textr->SetInputConnection(reader->GetOutputPort(0));
        }
        textr->Update();

        this->AddTexture(nickname, key.substr(0, key.size() - 1).c_str(), textr);
      }
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
const char* svtkOSPRayMaterialLibrary::WriteBuffer()
{
  Json::Value root;
  root["family"] = "OSPRay";
  root["version"] = "0.0";
  Json::Value materials;

  svtkSmartPointer<svtkXMLImageDataWriter> idwriter = svtkSmartPointer<svtkXMLImageDataWriter>::New();
  idwriter->WriteToOutputStringOn();

  std::set<std::string>::iterator it = this->Internal->NickNames.begin();
  while (it != this->Internal->NickNames.end())
  {
    std::string nickname = *it;
    Json::Value jnickname;
    std::string implname = this->LookupImplName(nickname);
    jnickname["type"] = implname;

    if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
    {
      Json::Value variables;
      NamedVariables::iterator vit = this->Internal->VariablesFor[nickname].begin();
      while (vit != this->Internal->VariablesFor[nickname].end())
      {
        std::string vname = vit->first;
        std::vector<double> vvals = vit->second;
        Json::Value jvvals;
        for (size_t i = 0; i < vvals.size(); ++i)
        {
          jvvals.append(vvals[i]);
        }
        variables[vname] = jvvals;
        ++vit;
      }

      jnickname["doubles"] = variables;
    }

    if (this->Internal->TexturesFor.find(nickname) != this->Internal->TexturesFor.end())
    {
      Json::Value textures;
      NamedTextures::iterator vit = this->Internal->TexturesFor[nickname].begin();
      while (vit != this->Internal->TexturesFor[nickname].end())
      {
        std::string vname = vit->first;
        svtkSmartPointer<svtkTexture> vvals = vit->second;
        idwriter->SetInputData(vvals->GetInput());
        idwriter->Write();
        std::string os = idwriter->GetOutputString();
        Json::Value jvvals = os;
        textures[vname] = jvvals;
        ++vit;
      }

      jnickname["textures"] = textures;
    }

    materials[nickname] = jnickname;
    ++it;
  }

  root["materials"] = materials;

  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "   ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  std::ostringstream result;
  writer->write(root, &result);
  std::string rstring = result.str();

  if (rstring.size())
  {
    char* buf = new char[rstring.size() + 1];
    memcpy(buf, rstring.c_str(), rstring.size());
    buf[rstring.size()] = 0;
    return buf;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void svtkOSPRayMaterialLibrary::Fire()
{
  this->InvokeEvent(svtkCommand::UpdateDataEvent);
}

//-----------------------------------------------------------------------------
std::set<std::string> svtkOSPRayMaterialLibrary::GetMaterialNames()
{
  return this->Internal->NickNames;
}

//-----------------------------------------------------------------------------
std::string svtkOSPRayMaterialLibrary::LookupImplName(const std::string& nickname)
{
  return this->Internal->ImplNames[nickname];
}

//-----------------------------------------------------------------------------
svtkTexture* svtkOSPRayMaterialLibrary::GetTexture(
  const std::string& nickname, const std::string& texturename)
{
  NamedTextures tsForNickname;
  if (this->Internal->TexturesFor.find(nickname) != this->Internal->TexturesFor.end())
  {
    tsForNickname = this->Internal->TexturesFor[nickname];
    std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], texturename);
    return tsForNickname[realname];
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
std::vector<double> svtkOSPRayMaterialLibrary::GetDoubleShaderVariable(
  const std::string& nickname, const std::string& varname)
{
  if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
  {
    NamedVariables vsForNickname = this->Internal->VariablesFor[nickname];
    std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], varname);
    return vsForNickname[realname];
  }
  return std::vector<double>();
}

//-----------------------------------------------------------------------------
std::vector<std::string> svtkOSPRayMaterialLibrary::GetDoubleShaderVariableList(
  const std::string& nickname)
{
  std::vector<std::string> variableNames;
  if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
  {
    for (auto& v : this->Internal->VariablesFor[nickname])
    {
      variableNames.push_back(v.first);
    }
  }
  return variableNames;
}

//-----------------------------------------------------------------------------
std::vector<std::string> svtkOSPRayMaterialLibrary::GetTextureList(const std::string& nickname)
{
  std::vector<std::string> texNames;
  if (this->Internal->TexturesFor.find(nickname) != this->Internal->TexturesFor.end())
  {
    for (auto& v : this->Internal->TexturesFor[nickname])
    {
      texNames.push_back(v.first);
    }
  }
  return texNames;
}

//-----------------------------------------------------------------------------
const std::map<std::string, svtkOSPRayMaterialLibrary::ParametersMap>&
svtkOSPRayMaterialLibrary::GetParametersDictionary()
{
  // This is the material dictionary from OSPRay 1.8
  // If attribute name changes with new OSPRay version, keep old name aliases support in functions
  // svtkOSPRayMaterialLibrary::AddShaderVariable and svtkOSPRayMaterialLibrary::AddTexture
  static std::map<std::string, svtkOSPRayMaterialLibrary::ParametersMap> dic = {
    { "OBJMaterial",
      {
        { "Ka", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "Kd", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "Ks", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "Ns", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "d", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "Tf", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "map_Bump", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_Bump.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_Bump.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_Bump.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Bump.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Kd", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_Kd.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_Kd.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_Kd.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Kd.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Ks", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_Ks.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_Ks.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_Ks.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Ks.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Ns", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_Ns.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_Ns.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_Ns.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Ns.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_d", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_d.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_d.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_d.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_d.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "Principled",
      {
        { "baseColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "edgeColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "metallic", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "diffuse", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "specular", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "ior", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "transmission", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "transmissionColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "transmissionDepth", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "roughness", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "anisotropy", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "rotation", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "normal", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseNormal", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "thin", svtkOSPRayMaterialLibrary::ParameterType::BOOLEAN },
        { "thickness", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "backlight", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coat", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coatIor", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "coatThickness", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatRoughness", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coatNormal", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheen", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "sheenColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "sheenTint", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenRoughness", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "opacity", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "baseColorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "baseColorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "baseColorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseColorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "baseColorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "edgeColorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "edgeColorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "edgeColorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "edgeColorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "edgeColorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "metallicMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "metallicMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "metallicMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "metallicMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "metallicMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "diffuseMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "diffuseMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "diffuseMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "diffuseMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "diffuseMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "specularMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "specularMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "specularMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "specularMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "specularMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "iorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "iorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "iorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "iorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "iorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "transmissionMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "transmissionMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "transmissionMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionColorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "transmissionColorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "transmissionColorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "transmissionColorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionColorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionDepthMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "transmissionDepthMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "transmissionDepthMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "transmissionDepthMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionDepthMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "roughnessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "roughnessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "roughnessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "roughnessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "roughnessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "anisotropyMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "anisotropyMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "anisotropyMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "anisotropyMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "anisotropyMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "rotationMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "rotationMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "rotationMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "rotationMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "rotationMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "normalMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "normalMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "normalMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "normalMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "normalMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "baseNormalMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "baseNormalMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "baseNormalMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseNormalMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "baseNormalMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "thinMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "thinMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "thinMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "thinMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "thinMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "thicknessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "thicknessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "thicknessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "thicknessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "thicknessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "backlightMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "backlightMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "backlightMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "backlightMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "backlightMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatIorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatIorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatIorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatIorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatIorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatColorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatColorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatColorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatColorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatColorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatThicknessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatThicknessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatThicknessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatThicknessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatThicknessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatRoughnessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatRoughnessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatRoughnessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatRoughnessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatRoughnessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatNormalMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatNormalMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatNormalMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatNormalMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatNormalMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "sheenMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "sheenMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenColorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "sheenColorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "sheenColorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenColorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenColorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenTintMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "sheenTintMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "sheenTintMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenTintMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenTintMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenRoughnessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "sheenRoughnessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "sheenRoughnessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenRoughnessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenRoughnessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "opacityMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "opacityMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "opacityMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "opacityMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "opacityMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "CarPaint",
      {
        { "baseColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "roughness", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "normal", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeDensity", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "flakeScale", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeSpread", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "flakeJitter", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "flakeRoughness", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coat", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coatIor", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "coatThickness", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatRoughness", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coatNormal", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flipflopColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "flipflopFalloff", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseColorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "baseColorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "baseColorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseColorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "baseColorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "roughnessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "roughnessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "roughnessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "roughnessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "roughnessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "normalMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "normalMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "normalMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "normalMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "normalMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeDensityMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeDensityMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeDensityMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeDensityMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeDensityMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeScaleMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeScaleMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeScaleMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeScaleMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeScaleMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeSpreadMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeSpreadMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeSpreadMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeSpreadMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeSpreadMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeJitterMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeJitterMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeJitterMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeJitterMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeJitterMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeRoughnessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeRoughnessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeRoughnessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeRoughnessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeRoughnessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatIorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatIorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatIorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatIorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatIorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatColorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatColorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatColorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatColorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatColorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatThicknessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatThicknessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatThicknessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatThicknessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatThicknessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatRoughnessMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatRoughnessMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatRoughnessMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatRoughnessMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatRoughnessMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatNormalMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatNormalMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatNormalMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatNormalMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatNormalMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flipflopColorMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flipflopColorMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flipflopColorMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flipflopColorMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flipflopColorMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flipflopFalloffMap", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flipflopFalloffMap.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flipflopFalloffMap.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flipflopFalloffMap.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flipflopFalloffMap.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "Metal",
      {
        { "ior", svtkOSPRayMaterialLibrary::ParameterType::FLOAT_DATA },
        { "eta", svtkOSPRayMaterialLibrary::ParameterType::VEC3 },
        { "k", svtkOSPRayMaterialLibrary::ParameterType::VEC3 },
        { "roughness", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_roughness", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_roughness.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_roughness.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_roughness.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "Alloy",
      {
        { "color", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "edgeColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "roughness", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_color", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_color.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_color.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_color.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_color.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_edgeColor", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_edgeColor.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_edgeColor.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_edgeColor.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_edgeColor.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_roughness.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_roughness.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_roughness.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "Glass",
      {
        { "eta", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "attenuationColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "attenuationDistance", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
      } },
    { "ThinGlass",
      {
        { "eta", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "attenuationColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "attenuationDistance", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "thickness", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_attenuationColor", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_attenuationColor.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_attenuationColor.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_attenuationColor.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_attenuationColor.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "MetallicPaint",
      {
        { "baseColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "flakeAmount", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "flakeColor", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "flakeSpread", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "eta", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_baseColor", svtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_baseColor.transform", svtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_baseColor.rotation", svtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_baseColor.scale", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_baseColor.translation", svtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "Luminous",
      {
        { "color", svtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "intensity", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "transparency", svtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
      } },
  };
  return dic;
}
