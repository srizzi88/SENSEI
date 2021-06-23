/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGLTFExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGLTFExporter.h"

#include <memory>
#include <sstream>
#include <stdio.h>

#include "svtk_jsoncpp.h"

#include "svtkAssemblyPath.h"
#include "svtkBase64OutputStream.h"
#include "svtkCamera.h"
#include "svtkCollectionRange.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkImageFlip.h"
#include "svtkMapper.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPNGWriter.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRendererCollection.h"
#include "svtkTexture.h"
#include "svtkTriangleFilter.h"
#include "svtkTrivialProducer.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"

#include "svtksys/FStream.hxx"
#include "svtksys/SystemTools.hxx"

svtkStandardNewMacro(svtkGLTFExporter);

svtkGLTFExporter::svtkGLTFExporter()
{
  this->FileName = nullptr;
  this->InlineData = false;
  this->SaveNormal = false;
  this->SaveBatchId = false;
}

svtkGLTFExporter::~svtkGLTFExporter()
{
  delete[] this->FileName;
}

namespace
{

svtkPolyData* findPolyData(svtkDataObject* input)
{
  // do we have polydata?
  svtkPolyData* pd = svtkPolyData::SafeDownCast(input);
  if (pd)
  {
    return pd;
  }
  svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(input);
  if (cd)
  {
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      pd = svtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        return pd;
      }
    }
  }
  return nullptr;
}

void WriteValues(svtkDataArray* ca, ostream& myFile)
{
  myFile.write(reinterpret_cast<char*>(ca->GetVoidPointer(0)),
    ca->GetNumberOfTuples() * ca->GetNumberOfComponents() * ca->GetElementComponentSize());
}

void WriteValues(svtkDataArray* ca, svtkBase64OutputStream* ostr)
{
  ostr->Write(reinterpret_cast<char*>(ca->GetVoidPointer(0)),
    ca->GetNumberOfTuples() * ca->GetNumberOfComponents() * ca->GetElementComponentSize());
}

void WriteBufferAndView(svtkDataArray* inda, const char* fileName, bool inlineData,
  Json::Value& buffers, Json::Value& bufferViews)
{
  svtkDataArray* da = inda;

  // gltf does not support doubles so handle that
  if (inda->GetDataType() == SVTK_DOUBLE)
  {
    da = svtkFloatArray::New();
    da->DeepCopy(inda);
  }

  // if inline then base64 encode the data
  std::string result;
  if (inlineData)
  {
    result = "data:application/octet-stream;base64,";
    std::ostringstream toString;
    svtkNew<svtkBase64OutputStream> ostr;
    ostr->SetStream(&toString);
    ostr->StartWriting();
    WriteValues(da, ostr);
    ostr->EndWriting();
    result += toString.str();
  }
  else
  {
    // otherwise write binary files
    std::ostringstream toString;
    toString << "buffer" << da->GetMTime() << ".bin";
    result = toString.str();

    std::string fullPath = svtksys::SystemTools::GetFilenamePath(fileName);
    if (fullPath.size() > 0)
    {
      fullPath += "/";
    }
    fullPath += result;

    // now write the data
    svtksys::ofstream myFile(fullPath.c_str(), ios::out | ios::binary);

    WriteValues(da, myFile);
    myFile.close();
  }

  Json::Value buffer;
  Json::Value view;

  unsigned int count = da->GetNumberOfTuples() * da->GetNumberOfComponents();
  unsigned int byteLength = da->GetElementComponentSize() * count;
  buffer["byteLength"] = static_cast<Json::Value::Int64>(byteLength);
  buffer["uri"] = result;
  buffers.append(buffer);

  // write the buffer views
  view["buffer"] = buffers.size() - 1;
  view["byteOffset"] = 0;
  view["byteLength"] = static_cast<Json::Value::Int64>(byteLength);
  bufferViews.append(view);

  // delete double to float conversion array
  if (da != inda)
  {
    da->Delete();
  }
}

void WriteBufferAndView(svtkCellArray* ca, const char* fileName, bool inlineData,
  Json::Value& buffers, Json::Value& bufferViews)
{
  svtkUnsignedIntArray* ia = svtkUnsignedIntArray::New();
  svtkIdType npts;
  const svtkIdType* indx;
  for (ca->InitTraversal(); ca->GetNextCell(npts, indx);)
  {
    for (int j = 0; j < npts; ++j)
    {
      unsigned int value = static_cast<unsigned int>(indx[j]);
      ia->InsertNextValue(value);
    }
  }

  WriteBufferAndView(ia, fileName, inlineData, buffers, bufferViews);
  ia->Delete();
}

// gltf uses hard coded numbers to represent data types
// they match the definitions from gl.h but for your convenience
// some of the common values we use are listed below to make
// the code more readable without including gl.h

#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406

#define GL_CLAMP_TO_EDGE 0x812F
#define GL_REPEAT 0x2901

#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601

void WriteMesh(Json::Value& accessors, Json::Value& buffers, Json::Value& bufferViews,
  Json::Value& meshes, Json::Value& nodes, svtkPolyData* pd, svtkActor* aPart, const char* fileName,
  bool inlineData, bool saveNormal, bool saveBatchId)
{
  svtkNew<svtkTriangleFilter> trif;
  trif->SetInputData(pd);
  trif->Update();
  svtkPolyData* tris = trif->GetOutput();

  // write the point locations
  int pointAccessor = 0;
  {
    svtkDataArray* da = tris->GetPoints()->GetData();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC3";
    acc["componentType"] = GL_FLOAT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    double range[6];
    tris->GetPoints()->GetBounds(range);
    Json::Value mins;
    mins.append(range[0]);
    mins.append(range[2]);
    mins.append(range[4]);
    Json::Value maxs;
    maxs.append(range[1]);
    maxs.append(range[3]);
    maxs.append(range[5]);
    acc["min"] = mins;
    acc["max"] = maxs;
    pointAccessor = accessors.size();
    accessors.append(acc);
  }

  std::vector<svtkDataArray*> arraysToSave;
  if (saveBatchId)
  {
    svtkDataArray* a;
    if ((a = pd->GetPointData()->GetArray("_BATCHID")))
    {
      arraysToSave.push_back(a);
    }
  }
  if (saveNormal)
  {
    svtkDataArray* a;
    if ((a = pd->GetPointData()->GetArray("NORMAL")))
    {
      arraysToSave.push_back(a);
    }
  }
  int userAccessorsStart = accessors.size();
  for (size_t i = 0; i < arraysToSave.size(); ++i)
  {
    svtkDataArray* da = arraysToSave[i];
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = da->GetNumberOfComponents() == 3 ? "VEC3" : "SCALAR";
    acc["componentType"] = GL_FLOAT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    accessors.append(acc);
  }

  // if we have vertex colors then write them out
  int vertColorAccessor = -1;
  aPart->GetMapper()->MapScalars(tris, 1.0);
  if (aPart->GetMapper()->GetColorMapColors())
  {
    svtkUnsignedCharArray* da = aPart->GetMapper()->GetColorMapColors();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC4";
    acc["componentType"] = GL_UNSIGNED_BYTE;
    acc["normalized"] = true;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    vertColorAccessor = accessors.size();
    accessors.append(acc);
  }

  // if we have tcoords then write them out
  // first check for colortcoords
  int tcoordAccessor = -1;
  svtkFloatArray* tcoords = aPart->GetMapper()->GetColorCoordinates();
  if (!tcoords)
  {
    tcoords = svtkFloatArray::SafeDownCast(tris->GetPointData()->GetTCoords());
  }
  if (tcoords)
  {
    svtkFloatArray* da = tcoords;
    WriteBufferAndView(tcoords, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = da->GetNumberOfComponents() == 3 ? "VEC3" : "VEC2";
    acc["componentType"] = GL_FLOAT;
    acc["normalized"] = false;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    tcoordAccessor = accessors.size();
    accessors.append(acc);
  }

  // to store the primitives
  Json::Value prims;

  // write out the verts
  if (tris->GetVerts() && tris->GetVerts()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 0;
    Json::Value attribs;

    svtkCellArray* da = tris->GetVerts();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells());
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    int userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (vertColorAccessor >= 0)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  // write out the lines
  if (tris->GetLines() && tris->GetLines()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 1;
    Json::Value attribs;

    svtkCellArray* da = tris->GetLines();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells() * 2);
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    int userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (vertColorAccessor >= 0)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  // write out the triangles
  if (tris->GetPolys() && tris->GetPolys()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 4;
    Json::Value attribs;

    svtkCellArray* da = tris->GetPolys();
    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells() * 3);
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    int userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (vertColorAccessor >= 0)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  Json::Value amesh;
  char meshNameBuffer[32];
  sprintf(meshNameBuffer, "mesh%d", meshes.size());
  amesh["name"] = meshNameBuffer;
  amesh["primitives"] = prims;
  meshes.append(amesh);

  // write out an actor
  Json::Value child;
  svtkMatrix4x4* amat = aPart->GetMatrix();
  if (!amat->IsIdentity())
  {
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        child["matrix"].append(amat->GetElement(j, i));
      }
    }
  }
  child["mesh"] = meshes.size() - 1;
  child["name"] = meshNameBuffer;
  nodes.append(child);
}

void WriteCamera(Json::Value& cameras, svtkRenderer* ren)
{
  svtkCamera* cam = ren->GetActiveCamera();
  Json::Value acamera;
  Json::Value camValues;
  camValues["znear"] = cam->GetClippingRange()[0];
  camValues["zfar"] = cam->GetClippingRange()[1];
  if (cam->GetParallelProjection())
  {
    acamera["type"] = "orthographic";
    camValues["xmag"] = cam->GetParallelScale() * ren->GetTiledAspectRatio();
    camValues["ymag"] = cam->GetParallelScale();
    acamera["orthographic"] = camValues;
  }
  else
  {
    acamera["type"] = "perspective";
    camValues["yfov"] = svtkMath::RadiansFromDegrees(cam->GetViewAngle());
    camValues["aspectRatio"] = ren->GetTiledAspectRatio();
    acamera["perspective"] = camValues;
  }
  cameras.append(acamera);
}

void WriteTexture(Json::Value& buffers, Json::Value& bufferViews, Json::Value& textures,
  Json::Value& samplers, Json::Value& images, svtkPolyData* pd, svtkActor* aPart,
  const char* fileName, bool inlineData, std::map<svtkUnsignedCharArray*, unsigned int>& textureMap)
{
  // do we have a texture
  aPart->GetMapper()->MapScalars(pd, 1.0);
  svtkImageData* id = aPart->GetMapper()->GetColorTextureMap();
  svtkTexture* t = nullptr;
  if (!id && aPart->GetTexture())
  {
    t = aPart->GetTexture();
    id = t->GetInput();
  }

  svtkUnsignedCharArray* da = nullptr;
  if (id && id->GetPointData()->GetScalars())
  {
    da = svtkUnsignedCharArray::SafeDownCast(id->GetPointData()->GetScalars());
  }
  if (!da)
  {
    return;
  }

  unsigned int textureSource = 0;

  if (textureMap.find(da) == textureMap.end())
  {
    textureMap[da] = textures.size();

    // flip Y
    svtkNew<svtkTrivialProducer> triv;
    triv->SetOutput(id);
    svtkNew<svtkImageFlip> flip;
    flip->SetFilteredAxis(1);
    flip->SetInputConnection(triv->GetOutputPort());

    // convert to png
    svtkNew<svtkPNGWriter> png;
    png->SetCompressionLevel(5);
    png->SetInputConnection(flip->GetOutputPort());
    png->WriteToMemoryOn();
    png->Write();
    da = png->GetResult();

    WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the image
    Json::Value img;
    img["bufferView"] = bufferViews.size() - 1;
    img["mimeType"] = "image/png";
    images.append(img);

    textureSource = images.size() - 1;
  }
  else
  {
    textureSource = textureMap[da];
  }

  // write the sampler
  Json::Value smp;
  smp["magFilter"] = GL_NEAREST;
  smp["minFilter"] = GL_NEAREST;
  smp["wrapS"] = GL_CLAMP_TO_EDGE;
  smp["wrapT"] = GL_CLAMP_TO_EDGE;
  if (t)
  {
    smp["wrapS"] = t->GetRepeat() ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    smp["wrapT"] = t->GetRepeat() ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    smp["magFilter"] = t->GetInterpolate() ? GL_LINEAR : GL_NEAREST;
    smp["minFilter"] = t->GetInterpolate() ? GL_LINEAR : GL_NEAREST;
  }
  samplers.append(smp);

  Json::Value texture;
  texture["source"] = textureSource;
  texture["sampler"] = samplers.size() - 1;
  textures.append(texture);
}

void WriteMaterial(Json::Value& materials, int textureIndex, bool haveTexture, svtkActor* aPart)
{
  Json::Value mat;
  Json::Value model;

  if (haveTexture)
  {
    Json::Value tex;
    tex["texCoord"] = 0; // TEXCOORD_0
    tex["index"] = textureIndex;
    model["baseColorTexture"] = tex;
  }

  svtkProperty* prop = aPart->GetProperty();
  double dcolor[3];
  prop->GetDiffuseColor(dcolor);
  model["baseColorFactor"].append(dcolor[0]);
  model["baseColorFactor"].append(dcolor[1]);
  model["baseColorFactor"].append(dcolor[2]);
  model["baseColorFactor"].append(prop->GetOpacity());
  model["metallicFactor"] = prop->GetSpecular();
  model["roughnessFactor"] = 1.0 / (1.0 + prop->GetSpecular() * 0.2 * prop->GetSpecularPower());
  mat["pbrMetallicRoughness"] = model;
  materials.append(mat);
}

}

std::string svtkGLTFExporter::WriteToString()
{
  std::ostringstream result;

  this->WriteToStream(result);

  return result.str();
}

void svtkGLTFExporter::WriteData()
{
  svtksys::ofstream output;

  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr)
  {
    svtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  // try opening the files
  output.open(this->FileName);
  if (!output.is_open())
  {
    svtkErrorMacro("Unable to open file for gltf output.");
    return;
  }

  this->WriteToStream(output);
  output.close();
}

void svtkGLTFExporter::WriteToStream(ostream& output)
{
  Json::Value cameras;
  Json::Value bufferViews;
  Json::Value buffers;
  Json::Value accessors;
  Json::Value nodes;
  Json::Value meshes;
  Json::Value textures;
  Json::Value images;
  Json::Value samplers;
  Json::Value materials;

  std::vector<unsigned int> topNodes;

  // support sharing texture maps
  std::map<svtkUnsignedCharArray*, unsigned int> textureMap;

  for (auto ren : svtk::Range(this->RenderWindow->GetRenderers()))
  {
    if (this->ActiveRenderer && ren != this->ActiveRenderer)
    {
      // If ActiveRenderer is specified then ignore all other renderers
      continue;
    }
    if (!ren->GetDraw())
    {
      continue;
    }

    // setup the camera data in case we need to use it later
    Json::Value anode;
    anode["camera"] = cameras.size(); // camera node
    svtkMatrix4x4* mat = ren->GetActiveCamera()->GetModelViewTransformMatrix();
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        anode["matrix"].append(mat->GetElement(j, i));
      }
    }
    anode["name"] = "Camera Node";

    // setup renderer group node
    Json::Value rendererNode;
    rendererNode["name"] = "Renderer Node";

    svtkPropCollection* pc;
    svtkProp* aProp;
    pc = ren->GetViewProps();
    svtkCollectionSimpleIterator pit;
    bool foundVisibleProp = false;
    for (pc->InitTraversal(pit); (aProp = pc->GetNextProp(pit));)
    {
      if (!aProp->GetVisibility())
      {
        continue;
      }
      svtkNew<svtkActorCollection> ac;
      aProp->GetActors(ac);
      svtkActor* anActor;
      svtkCollectionSimpleIterator ait;
      for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
      {
        svtkAssemblyPath* apath;
        svtkActor* aPart;
        for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
        {
          aPart = static_cast<svtkActor*>(apath->GetLastNode()->GetViewProp());
          if (aPart->GetVisibility() && aPart->GetMapper() &&
            aPart->GetMapper()->GetInputAlgorithm())
          {
            aPart->GetMapper()->GetInputAlgorithm()->Update();
            svtkPolyData* pd = findPolyData(aPart->GetMapper()->GetInputDataObject(0, 0));
            if (pd && pd->GetNumberOfCells() > 0)
            {
              foundVisibleProp = true;
              WriteMesh(accessors, buffers, bufferViews, meshes, nodes, pd, aPart, this->FileName,
                this->InlineData, this->SaveNormal, this->SaveBatchId);
              rendererNode["children"].append(nodes.size() - 1);
              unsigned int oldTextureCount = textures.size();
              WriteTexture(buffers, bufferViews, textures, samplers, images, pd, aPart,
                this->FileName, this->InlineData, textureMap);
              meshes[meshes.size() - 1]["primitives"][0]["material"] = materials.size();
              WriteMaterial(materials, oldTextureCount, oldTextureCount != textures.size(), aPart);
            }
          }
        }
      }
    }
    // only write the camera if we had visible nodes
    if (foundVisibleProp)
    {
      WriteCamera(cameras, ren);
      nodes.append(anode);
      rendererNode["children"].append(nodes.size() - 1);
      nodes.append(rendererNode);
      topNodes.push_back(nodes.size() - 1);
    }
  }

  Json::Value root;
  Json::Value asset;
  asset["generator"] = "SVTK";
  asset["version"] = "2.0";
  root["asset"] = asset;

  root["scene"] = 0;
  root["cameras"] = cameras;
  root["nodes"] = nodes;
  root["meshes"] = meshes;
  root["buffers"] = buffers;
  root["bufferViews"] = bufferViews;
  root["accessors"] = accessors;
  if (images.size() > 0)
    root["images"] = images;
  if (textures.size() > 0)
    root["textures"] = textures;
  if (samplers.size() > 0)
    root["samplers"] = samplers;
  root["materials"] = materials;

  Json::Value ascene;
  ascene["name"] = "Layer 0";
  Json::Value noderefs;
  for (auto i : topNodes)
  {
    noderefs.append(i);
  }
  ascene["nodes"] = noderefs;
  Json::Value scenes;
  scenes.append(ascene);
  root["scenes"] = scenes;

  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "   ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  writer->write(root, &output);
}

void svtkGLTFExporter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "InlineData: " << this->InlineData << "\n";
  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
}
