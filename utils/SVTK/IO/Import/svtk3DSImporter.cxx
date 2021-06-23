/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtk3DSImporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtk3DSImporter.h"

#include "svtkActor.h"
#include "svtkByteSwap.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkLight.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkStripper.h"
#include "svtksys/SystemTools.hxx"

#include <sstream>

svtkStandardNewMacro(svtk3DSImporter);

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// This file just has too many of them.
// This is due to the use of (svtk3DSList **)&root in SVTK_LIST_* macros
// defined in svtk3DS.h
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

// Wrap these globals in a namespace to prevent identifier collisions with
// inline code on MSVC2015:
namespace svtk3DS
{
static svtk3DSColour black = { 0.0f, 0.0f, 0.0f };
static char objName[80] = "";
static svtk3DSColour fogColour = { 0.0f, 0.0f, 0.0f };
static svtk3DSColour col = { 0.0f, 0.0f, 0.0f };
static svtk3DSColour globalAmb = { 0.1f, 0.1f, 0.1f };
static svtk3DSVector pos = { 0.0, 0.0, 0.0 };
static svtk3DSVector target = { 0.0, 0.0, 0.0 };
static float hotspot = -1;
static float falloff = -1;

/* Default material property */
static svtk3DSMatProp defaultMaterial = { "Default", nullptr, { 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 },
  { 1.0, 1.0, 1.0 },
  70.0,      // shininess
  0.0,       // transparency
  0.0,       // reflection
  0,         // self illumination
  "",        // tex_map
  0.0,       // tex_strength
  "",        // bump_map
  0.0,       // bump_strength
  nullptr }; // svtkProperty

} // end namespace svtk3DS

static void cleanup_name(char*);
static void list_insert(svtk3DSList** root, svtk3DSList* new_node);
static void* list_find(svtk3DSList** root, const char* name);
static void list_kill(svtk3DSList** root);
static svtk3DSMatProp* create_mprop();
static svtk3DSMesh* create_mesh(char* name, int vertices, int faces);
static int parse_3ds_file(svtk3DSImporter* importer);
static void parse_3ds(svtk3DSImporter* importer, svtk3DSChunk* mainchunk);
static void parse_mdata(svtk3DSImporter* importer, svtk3DSChunk* mainchunk);
static void parse_fog(svtk3DSImporter* importer, svtk3DSChunk* mainchunk);
static void parse_fog_bgnd(svtk3DSImporter* importer);
static void parse_mat_entry(svtk3DSImporter* importer, svtk3DSChunk* mainchunk);
static char* parse_mapname(svtk3DSImporter* importer, svtk3DSChunk* mainchunk);
static void parse_named_object(svtk3DSImporter* importer, svtk3DSChunk* mainchunk);
static void parse_n_tri_object(svtk3DSImporter* importer, svtk3DSChunk* mainchunk);
static void parse_point_array(svtk3DSImporter* importer, svtk3DSMesh* mesh);
static void parse_face_array(svtk3DSImporter* importer, svtk3DSMesh* mesh, svtk3DSChunk* mainchunk);
static void parse_msh_mat_group(svtk3DSImporter* importer, svtk3DSMesh* mesh);
static void parse_smooth_group(svtk3DSImporter* importer);
static void parse_mesh_matrix(svtk3DSImporter* importer, svtk3DSMesh* mesh);
static void parse_n_direct_light(svtk3DSImporter* importer, svtk3DSChunk* mainchunk);
static void parse_dl_spotlight(svtk3DSImporter* importer);
static void parse_n_camera(svtk3DSImporter* importer);
static void parse_colour(svtk3DSImporter* importer, svtk3DSColour* colour);
static void parse_colour_f(svtk3DSImporter* importer, svtk3DSColour* colour);
static void parse_colour_24(svtk3DSImporter* importer, svtk3DSColour_24* colour);
static float parse_percentage(svtk3DSImporter* importer);
static short parse_int_percentage(svtk3DSImporter* importer);
static float parse_float_percentage(svtk3DSImporter* importer);
static svtk3DSMaterial* update_materials(
  svtk3DSImporter* importer, const char* new_material, int ext);
static void start_chunk(svtk3DSImporter* importer, svtk3DSChunk* chunk);
static void end_chunk(svtk3DSImporter* importer, svtk3DSChunk* chunk);
static byte read_byte(svtk3DSImporter* importer);
static word read_word(svtk3DSImporter* importer);
static word peek_word(svtk3DSImporter* importer);
static dword peek_dword(svtk3DSImporter* importer);
static float read_float(svtk3DSImporter* importer);
static void read_point(svtk3DSImporter* importer, svtk3DSVector v);
static char* read_string(svtk3DSImporter* importer);

svtk3DSImporter::svtk3DSImporter()
{
  this->OmniList = nullptr;
  this->SpotLightList = nullptr;
  this->CameraList = nullptr;
  this->MeshList = nullptr;
  this->MaterialList = nullptr;
  this->MatPropList = nullptr;
  this->FileName = nullptr;
  this->FileFD = nullptr;
  this->ComputeNormals = 0;
}

int svtk3DSImporter::ImportBegin()
{
  svtkDebugMacro(<< "Opening import file as binary");
  this->FileFD = svtksys::SystemTools::Fopen(this->FileName, "rb");
  if (this->FileFD == nullptr)
  {
    svtkErrorMacro(<< "Unable to open file: " << this->FileName);
    return 0;
  }
  return this->Read3DS();
}

void svtk3DSImporter::ImportEnd()
{
  svtkDebugMacro(<< "Closing import file");
  if (this->FileFD != nullptr)
  {
    fclose(this->FileFD);
  }
  this->FileFD = nullptr;
}

//----------------------------------------------------------------------------
std::string svtk3DSImporter::GetOutputsDescription()
{
  std::stringstream ss;
  size_t idx = 0;
  for (auto mesh = this->MeshList; mesh != (svtk3DSMesh*)nullptr;
       mesh = (svtk3DSMesh*)mesh->next, idx++)
  {
    if (mesh->aPolyData)
    {
      ss << "Mesh " << idx << " polydata:\n";
      ss << svtkImporter::GetDataSetDescription(mesh->aPolyData, svtkIndent(1));
    }
  }
  return ss.str();
}

int svtk3DSImporter::Read3DS()
{
  svtk3DSMatProp* aMaterial;

  if (parse_3ds_file(this) == 0)
  {
    svtkErrorMacro(<< "Error readings .3ds file: " << this->FileName << "\n");
    return 0;
  }

  // create a svtk3DSMatProp and fill if in with default
  aMaterial = (svtk3DSMatProp*)malloc(sizeof(svtk3DSMatProp));
  *aMaterial = svtk3DS::defaultMaterial;
  aMaterial->aProperty = svtkProperty::New();
  SVTK_LIST_INSERT(this->MatPropList, aMaterial);
  return 1;
}

void svtk3DSImporter::ImportActors(svtkRenderer* renderer)
{
  svtk3DSMatProp* material;
  svtk3DSMesh* mesh;
  svtkStripper* polyStripper;
  svtkPolyDataNormals* polyNormals;
  svtkPolyDataMapper* polyMapper;
  svtkPolyData* polyData;
  svtkActor* actor;

  // walk the list of meshes, creating actors
  for (mesh = this->MeshList; mesh != (svtk3DSMesh*)nullptr; mesh = (svtk3DSMesh*)mesh->next)
  {
    if (mesh->faces == 0)
    {
      svtkWarningMacro(<< "part " << mesh->name << " has zero faces... skipping\n");
      continue;
    }

    polyData = this->GeneratePolyData(mesh);
    mesh->aMapper = polyMapper = svtkPolyDataMapper::New();
    mesh->aStripper = polyStripper = svtkStripper::New();

    // if ComputeNormals is on, insert a svtkPolyDataNormals filter
    if (this->ComputeNormals)
    {
      mesh->aNormals = polyNormals = svtkPolyDataNormals::New();
      polyNormals->SetInputData(polyData);
      polyStripper->SetInputConnection(polyNormals->GetOutputPort());
    }
    else
    {
      polyStripper->SetInputData(polyData);
    }

    polyMapper->SetInputConnection(polyStripper->GetOutputPort());
    svtkDebugMacro(<< "Importing Actor: " << mesh->name);
    mesh->anActor = actor = svtkActor::New();
    actor->SetMapper(polyMapper);
    material = (svtk3DSMatProp*)SVTK_LIST_FIND(this->MatPropList, mesh->mtl[0]->name);
    actor->SetProperty(material->aProperty);
    renderer->AddActor(actor);
  }
}

svtkPolyData* svtk3DSImporter::GeneratePolyData(svtk3DSMesh* mesh)
{
  int i;
  svtk3DSFace* face;
  svtkCellArray* triangles;
  svtkPoints* vertices;
  svtkPolyData* polyData;

  face = mesh->face;
  mesh->aCellArray = triangles = svtkCellArray::New();
  triangles->AllocateEstimate(mesh->faces, 3);
  for (i = 0; i < mesh->faces; i++, face++)
  {
    triangles->InsertNextCell(3);
    triangles->InsertCellPoint(face->a);
    triangles->InsertCellPoint(face->b);
    triangles->InsertCellPoint(face->c);
  }

  mesh->aPoints = vertices = svtkPoints::New();
  vertices->Allocate(mesh->vertices);
  for (i = 0; i < mesh->vertices; i++)
  {
    vertices->InsertPoint(i, (float*)mesh->vertex[i]);
  }
  mesh->aPolyData = polyData = svtkPolyData::New();
  polyData->SetPolys(triangles);
  polyData->SetPoints(vertices);

  return polyData;
}

void svtk3DSImporter::ImportCameras(svtkRenderer* renderer)
{
  svtkCamera* aCamera;
  svtk3DSCamera* camera;

  // walk the list of cameras and create svtk cameras
  for (camera = this->CameraList; camera != (svtk3DSCamera*)nullptr;
       camera = (svtk3DSCamera*)camera->next)
  {
    camera->aCamera = aCamera = svtkCamera::New();
    aCamera->SetPosition(camera->pos[0], camera->pos[1], camera->pos[2]);
    aCamera->SetFocalPoint(camera->target[0], camera->target[1], camera->target[2]);
    aCamera->SetViewUp(0, 0, 1);
    aCamera->SetClippingRange(.1, 10000);
    aCamera->Roll(camera->bank);
    renderer->SetActiveCamera(aCamera);
    svtkDebugMacro(<< "Importing Camera: " << camera->name);
  }
}

void svtk3DSImporter::ImportLights(svtkRenderer* renderer)
{
  svtk3DSOmniLight* omniLight;
  svtk3DSSpotLight* spotLight;
  svtkLight* aLight;

  // just walk the list of omni lights, creating svtk lights
  for (omniLight = this->OmniList; omniLight != (svtk3DSOmniLight*)nullptr;
       omniLight = (svtk3DSOmniLight*)omniLight->next)
  {
    omniLight->aLight = aLight = svtkLight::New();
    aLight->SetPosition(omniLight->pos[0], omniLight->pos[1], omniLight->pos[2]);
    aLight->SetFocalPoint(0, 0, 0);
    aLight->SetColor(omniLight->col.red, omniLight->col.green, omniLight->col.blue);
    renderer->AddLight(aLight);
    svtkDebugMacro(<< "Importing Omni Light: " << omniLight->name);
  }

  // now walk the list of spot lights, creating svtk lights
  for (spotLight = this->SpotLightList; spotLight != (svtk3DSSpotLight*)nullptr;
       spotLight = (svtk3DSSpotLight*)spotLight->next)
  {
    spotLight->aLight = aLight = svtkLight::New();
    aLight->PositionalOn();
    aLight->SetPosition(spotLight->pos[0], spotLight->pos[1], spotLight->pos[2]);
    aLight->SetFocalPoint(spotLight->target[0], spotLight->target[1], spotLight->target[2]);
    aLight->SetColor(spotLight->col.red, spotLight->col.green, spotLight->col.blue);
    aLight->SetConeAngle(spotLight->falloff);
    renderer->AddLight(aLight);
    svtkDebugMacro(<< "Importing Spot Light: " << spotLight->name);
  }
}

void svtk3DSImporter::ImportProperties(svtkRenderer* svtkNotUsed(renderer))
{
  float amb = 0.1, dif = 0.9;
  float dist_white, dist_diff, phong, phong_size;
  svtkProperty* property;
  svtk3DSMatProp* m;

  // just walk the list of material properties, creating svtk properties
  for (m = this->MatPropList; m != (svtk3DSMatProp*)nullptr; m = (svtk3DSMatProp*)m->next)
  {
    if (m->self_illum)
    {
      amb = 0.9;
      dif = 0.1;
    }

    dist_white =
      fabs(1.0 - m->specular.red) + fabs(1.0 - m->specular.green) + fabs(1.0 - m->specular.blue);

    dist_diff = fabs(m->diffuse.red - m->specular.red) +
      fabs(m->diffuse.green - m->specular.green) + fabs(m->diffuse.blue - m->specular.blue);

    if (dist_diff < dist_white)
    {
      dif = .1;
      amb = .8;
    }

    phong_size = 0.7 * m->shininess;
    if (phong_size < 1.0)
    {
      phong_size = 1.0;
    }
    if (phong_size > 30.0)
    {
      phong = 1.0;
    }
    else
    {
      phong = phong_size / 30.0;
    }
    property = m->aProperty;
    property->SetAmbientColor(m->ambient.red, m->ambient.green, m->ambient.blue);
    property->SetAmbient(amb);
    property->SetDiffuseColor(m->diffuse.red, m->diffuse.green, m->diffuse.blue);
    property->SetDiffuse(dif);
    property->SetSpecularColor(m->specular.red, m->specular.green, m->specular.blue);
    property->SetSpecular(phong);
    property->SetSpecularPower(phong_size);
    property->SetOpacity(1.0 - m->transparency);
    svtkDebugMacro(<< "Importing Property: " << m->name);

    m->aProperty = property;
  }
}

/* Insert a new node into the list */
static void list_insert(svtk3DSList** root, svtk3DSList* new_node)
{
  new_node->next = *root;
  *root = new_node;
}

/* Find the node with the specified name */
static void* list_find(svtk3DSList** root, const char* name)
{
  svtk3DSList* p;
  for (p = *root; p != (svtk3DSList*)nullptr; p = (svtk3DSList*)p->next)
  {
    if (strcmp(p->name, name) == 0)
    {
      break;
    }
  }
  return (void*)p;
}

/* Delete the entire list */
static void list_kill(svtk3DSList** root)
{
  svtk3DSList* temp;

  while (*root != (svtk3DSList*)nullptr)
  {
    temp = *root;
    *root = (svtk3DSList*)(*root)->next;
    free(temp);
  }
}

/* Add a new material to the material list */
static svtk3DSMaterial* update_materials(svtk3DSImporter* importer, const char* new_material, int ext)
{
  svtk3DSMaterial* p;

  p = (svtk3DSMaterial*)SVTK_LIST_FIND(importer->MaterialList, new_material);

  if (p == nullptr)
  {
    p = (svtk3DSMaterial*)malloc(sizeof(*p));
    strcpy(p->name, new_material);
    p->external = ext;
    SVTK_LIST_INSERT(importer->MaterialList, p);
  }
  return p;
}

static svtk3DSMatProp* create_mprop()
{
  svtk3DSMatProp* new_mprop;

  new_mprop = (svtk3DSMatProp*)malloc(sizeof(*new_mprop));
  strcpy(new_mprop->name, "");
  new_mprop->ambient = svtk3DS::black;
  new_mprop->diffuse = svtk3DS::black;
  new_mprop->specular = svtk3DS::black;
  new_mprop->shininess = 0.0;
  new_mprop->transparency = 0.0;
  new_mprop->reflection = 0.0;
  new_mprop->self_illum = 0;

  strcpy(new_mprop->tex_map, "");
  new_mprop->tex_strength = 0.0;

  strcpy(new_mprop->bump_map, "");
  new_mprop->bump_strength = 0.0;

  new_mprop->aProperty = svtkProperty::New();
  return new_mprop;
}

/* Create a new mesh */
static svtk3DSMesh* create_mesh(char* name, int vertices, int faces)
{
  svtk3DSMesh* new_mesh;

  new_mesh = (svtk3DSMesh*)malloc(sizeof(*new_mesh));
  strcpy(new_mesh->name, name);

  new_mesh->vertices = vertices;

  if (vertices <= 0)
  {
    new_mesh->vertex = nullptr;
  }
  else
  {
    new_mesh->vertex = (svtk3DSVector*)malloc(vertices * sizeof(*new_mesh->vertex));
  }

  new_mesh->faces = faces;

  if (faces <= 0)
  {
    new_mesh->face = nullptr;
    new_mesh->mtl = nullptr;
  }
  else
  {
    new_mesh->face = (svtk3DSFace*)malloc(faces * sizeof(*new_mesh->face));
    new_mesh->mtl = (svtk3DSMaterial**)malloc(faces * sizeof(*new_mesh->mtl));
  }

  new_mesh->hidden = 0;
  new_mesh->shadow = 1;

  new_mesh->anActor = nullptr;
  new_mesh->aMapper = nullptr;
  new_mesh->aNormals = nullptr;
  new_mesh->aStripper = nullptr;
  new_mesh->aPoints = nullptr;
  new_mesh->aCellArray = nullptr;
  new_mesh->aPolyData = nullptr;
  return new_mesh;
}

static int parse_3ds_file(svtk3DSImporter* importer)
{
  svtk3DSChunk chunk;

  start_chunk(importer, &chunk);

  if (chunk.tag == 0x4D4D)
  {
    parse_3ds(importer, &chunk);
  }
  else
  {
    svtkGenericWarningMacro(<< "Error: Input file is not .3DS format\n");
    return 0;
  }

  end_chunk(importer, &chunk);
  return 1;
}

static void parse_3ds(svtk3DSImporter* importer, svtk3DSChunk* mainchunk)
{
  svtk3DSChunk chunk;

  do
  {
    start_chunk(importer, &chunk);

    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0x3D3D:
          parse_mdata(importer, &chunk);
          break;
      }
    }
    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);
}

static void parse_mdata(svtk3DSImporter* importer, svtk3DSChunk* mainchunk)
{
  svtk3DSChunk chunk;
  svtk3DSColour bgnd_colour;

  do
  {
    start_chunk(importer, &chunk);

    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0x2100:
          parse_colour(importer, &svtk3DS::globalAmb);
          break;
        case 0x1200:
          parse_colour(importer, &bgnd_colour);
          break;
        case 0x2200:
          parse_fog(importer, &chunk);
          break;
        case 0x2210:
          parse_fog_bgnd(importer);
          break;
        case 0xAFFF:
          parse_mat_entry(importer, &chunk);
          break;
        case 0x4000:
          parse_named_object(importer, &chunk);
          break;
      }
    }

    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);
}

static void parse_fog(svtk3DSImporter* importer, svtk3DSChunk* mainchunk)
{
  svtk3DSChunk chunk;

  (void)read_float(importer);
  (void)read_float(importer);
  (void)read_float(importer);
  (void)read_float(importer);

  parse_colour(importer, &svtk3DS::fogColour);

  do
  {
    start_chunk(importer, &chunk);

    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0x2210:
          parse_fog_bgnd(importer);
          break;
      }
    }

    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);
}

static void parse_fog_bgnd(svtk3DSImporter* svtkNotUsed(importer)) {}

static void parse_mat_entry(svtk3DSImporter* importer, svtk3DSChunk* mainchunk)
{
  svtk3DSChunk chunk;
  svtk3DSMatProp* mprop;

  mprop = create_mprop();

  do
  {
    start_chunk(importer, &chunk);
    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0xA000:
          strcpy(mprop->name, read_string(importer));
          cleanup_name(mprop->name);
          break;

        case 0xA010:
          parse_colour(importer, &mprop->ambient);
          break;

        case 0xA020:
          parse_colour(importer, &mprop->diffuse);
          break;

        case 0xA030:
          parse_colour(importer, &mprop->specular);
          break;

        case 0xA040:
          mprop->shininess = 100.0 * parse_percentage(importer);
          break;

        case 0xA050:
          mprop->transparency = parse_percentage(importer);
          break;

        case 0xA080:
          mprop->self_illum = 1;
          break;

        case 0xA220:
          mprop->reflection = parse_percentage(importer);
          (void)parse_mapname(importer, &chunk);
          break;

        case 0xA310:
          if (mprop->reflection == 0.0)
          {
            mprop->reflection = 1.0;
          }
          break;

        case 0xA200:
          mprop->tex_strength = parse_percentage(importer);
          strcpy(mprop->tex_map, parse_mapname(importer, &chunk));
          break;

        case 0xA230:
          mprop->bump_strength = parse_percentage(importer);
          strcpy(mprop->bump_map, parse_mapname(importer, &chunk));
          break;
      }
    }

    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);

  SVTK_LIST_INSERT(importer->MatPropList, mprop);
}

static char* parse_mapname(svtk3DSImporter* importer, svtk3DSChunk* mainchunk)
{
  static char name[80] = "";
  svtk3DSChunk chunk;

  do
  {
    start_chunk(importer, &chunk);

    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0xA300:
          strcpy(name, read_string(importer));
          break;
      }
    }

    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);

  return name;
}

static void parse_named_object(svtk3DSImporter* importer, svtk3DSChunk* mainchunk)
{
  svtk3DSMesh* mesh;
  svtk3DSChunk chunk;

  strcpy(svtk3DS::objName, read_string(importer));
  cleanup_name(svtk3DS::objName);

  mesh = nullptr;

  do
  {
    start_chunk(importer, &chunk);
    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0x4100:
          parse_n_tri_object(importer, &chunk);
          break;
        case 0x4600:
          parse_n_direct_light(importer, &chunk);
          break;
        case 0x4700:
          parse_n_camera(importer);
          break;
        case 0x4010:
          if (mesh != nullptr)
          {
            mesh->hidden = 1;
          }
          break;
        case 0x4012:
          if (mesh != nullptr)
          {
            mesh->shadow = 0;
          }
          break;
      }
    }

    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);
}

static void parse_n_tri_object(svtk3DSImporter* importer, svtk3DSChunk* mainchunk)
{
  svtk3DSMesh* mesh;
  svtk3DSChunk chunk;

  mesh = create_mesh(svtk3DS::objName, 0, 0);

  do
  {
    start_chunk(importer, &chunk);

    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0x4110:
          parse_point_array(importer, mesh);
          break;
        case 0x4120:
          parse_face_array(importer, mesh, &chunk);
          break;
        case 0x4160:
          parse_mesh_matrix(importer, mesh);
          break;
      }
    }

    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);

  SVTK_LIST_INSERT(importer->MeshList, mesh);
}

static void parse_point_array(svtk3DSImporter* importer, svtk3DSMesh* mesh)
{
  int i;

  mesh->vertices = read_word(importer);
  mesh->vertex = (svtk3DSVector*)malloc(mesh->vertices * sizeof(*(mesh->vertex)));
  for (i = 0; i < mesh->vertices; i++)
  {
    read_point(importer, mesh->vertex[i]);
  }
}

static void parse_face_array(svtk3DSImporter* importer, svtk3DSMesh* mesh, svtk3DSChunk* mainchunk)
{
  svtk3DSChunk chunk;
  int i;

  mesh->faces = read_word(importer);
  mesh->face = (svtk3DSFace*)malloc(mesh->faces * sizeof(*(mesh->face)));
  mesh->mtl = (svtk3DSMaterial**)malloc(mesh->faces * sizeof(*(mesh->mtl)));

  for (i = 0; i < mesh->faces; i++)
  {
    mesh->face[i].a = read_word(importer);
    mesh->face[i].b = read_word(importer);
    mesh->face[i].c = read_word(importer);
    (void)read_word(importer);

    mesh->mtl[i] = nullptr;
  }

  do
  {
    start_chunk(importer, &chunk);
    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0x4130:
          parse_msh_mat_group(importer, mesh);
          break;
        case 0x4150:
          parse_smooth_group(importer);
          break;
      }
    }

    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);

  for (i = 0; i < mesh->faces; i++)
  {
    if (mesh->mtl[i] == (svtk3DSMaterial*)nullptr)
    {
      mesh->mtl[i] = update_materials(importer, "Default", 0);
    }
  }
}

static void parse_msh_mat_group(svtk3DSImporter* importer, svtk3DSMesh* mesh)
{
  svtk3DSMaterial* new_mtl;
  char mtlname[80];
  int mtlcnt;
  int i, face;

  strcpy(mtlname, read_string(importer));
  cleanup_name(mtlname);

  new_mtl = update_materials(importer, mtlname, 0);

  mtlcnt = read_word(importer);

  for (i = 0; i < mtlcnt; i++)
  {
    face = read_word(importer);
    mesh->mtl[face] = new_mtl;
  }
}

static void parse_smooth_group(svtk3DSImporter* svtkNotUsed(importer)) {}

static void parse_mesh_matrix(svtk3DSImporter* svtkNotUsed(importer), svtk3DSMesh* svtkNotUsed(mesh))
{
  //  svtkGenericWarningMacro(<< "mesh matrix detected but not used\n");
}

static void parse_n_direct_light(svtk3DSImporter* importer, svtk3DSChunk* mainchunk)
{
  svtk3DSChunk chunk;
  svtk3DSSpotLight* s;
  svtk3DSOmniLight* o;
  int spot_flag = 0;

  read_point(importer, svtk3DS::pos);
  parse_colour(importer, &svtk3DS::col);

  do
  {
    start_chunk(importer, &chunk);

    if (chunk.end <= mainchunk->end)
    {
      switch (chunk.tag)
      {
        case 0x4620:
          break;
        case 0x4610:
          parse_dl_spotlight(importer);
          spot_flag = 1;
          break;
      }
    }

    end_chunk(importer, &chunk);
  } while (chunk.end <= mainchunk->end);

  if (!spot_flag)
  {
    o = (svtk3DSOmniLight*)SVTK_LIST_FIND(importer->OmniList, svtk3DS::objName);

    if (o != nullptr)
    {
      svtk3DS::pos[0] = o->pos[0];
      svtk3DS::pos[1] = o->pos[1];
      svtk3DS::pos[2] = o->pos[2];
      svtk3DS::col = o->col;
    }
    else
    {
      o = (svtk3DSOmniLight*)malloc(sizeof(*o));
      o->pos[0] = svtk3DS::pos[0];
      o->pos[1] = svtk3DS::pos[1];
      o->pos[2] = svtk3DS::pos[2];
      o->col = svtk3DS::col;
      strcpy(o->name, svtk3DS::objName);
      SVTK_LIST_INSERT(importer->OmniList, o);
    }
  }
  else
  {
    s = (svtk3DSSpotLight*)SVTK_LIST_FIND(importer->SpotLightList, svtk3DS::objName);

    if (s != nullptr)
    {
      svtk3DS::pos[0] = s->pos[0];
      svtk3DS::pos[1] = s->pos[1];
      svtk3DS::pos[2] = s->pos[2];
      svtk3DS::target[0] = s->target[0];
      svtk3DS::target[1] = s->target[1];
      svtk3DS::target[2] = s->target[2];
      svtk3DS::col = s->col;
      svtk3DS::hotspot = s->hotspot;
      svtk3DS::falloff = s->falloff;
    }
    else
    {
      if (svtk3DS::falloff <= 0.0)
      {
        svtk3DS::falloff = 180.0;
      }
      if (svtk3DS::hotspot <= 0.0)
      {
        svtk3DS::hotspot = 0.7 * svtk3DS::falloff;
      }
      s = (svtk3DSSpotLight*)malloc(sizeof(*s));
      s->pos[0] = svtk3DS::pos[0];
      s->pos[1] = svtk3DS::pos[1];
      s->pos[2] = svtk3DS::pos[2];
      s->target[0] = svtk3DS::target[0];
      s->target[1] = svtk3DS::target[1];
      s->target[2] = svtk3DS::target[2];
      s->col = svtk3DS::col;
      s->hotspot = svtk3DS::hotspot;
      s->falloff = svtk3DS::falloff;
      strcpy(s->name, svtk3DS::objName);
      SVTK_LIST_INSERT(importer->SpotLightList, s);
    }
  }
}

static void parse_dl_spotlight(svtk3DSImporter* importer)
{
  read_point(importer, svtk3DS::target);

  svtk3DS::hotspot = read_float(importer);
  svtk3DS::falloff = read_float(importer);
}

static void parse_n_camera(svtk3DSImporter* importer)
{
  float bank;
  float lens;
  svtk3DSCamera* c = (svtk3DSCamera*)malloc(sizeof(svtk3DSCamera));

  read_point(importer, svtk3DS::pos);
  read_point(importer, svtk3DS::target);
  bank = read_float(importer);
  lens = read_float(importer);

  strcpy(c->name, svtk3DS::objName);
  c->pos[0] = svtk3DS::pos[0];
  c->pos[1] = svtk3DS::pos[1];
  c->pos[2] = svtk3DS::pos[2];
  c->target[0] = svtk3DS::target[0];
  c->target[1] = svtk3DS::target[1];
  c->target[2] = svtk3DS::target[2];
  c->lens = lens;
  c->bank = bank;

  SVTK_LIST_INSERT(importer->CameraList, c);
}

static void parse_colour(svtk3DSImporter* importer, svtk3DSColour* colour)
{
  svtk3DSChunk chunk;
  svtk3DSColour_24 colour_24;

  start_chunk(importer, &chunk);

  switch (chunk.tag)
  {
    case 0x0010:
      parse_colour_f(importer, colour);
      break;

    case 0x0011:
      parse_colour_24(importer, &colour_24);
      colour->red = colour_24.red / 255.0;
      colour->green = colour_24.green / 255.0;
      colour->blue = colour_24.blue / 255.0;
      break;

    default:
      svtkGenericWarningMacro(<< "Error parsing colour");
  }

  end_chunk(importer, &chunk);
}

static void parse_colour_f(svtk3DSImporter* importer, svtk3DSColour* colour)
{
  colour->red = read_float(importer);
  colour->green = read_float(importer);
  colour->blue = read_float(importer);
}

static void parse_colour_24(svtk3DSImporter* importer, svtk3DSColour_24* colour)
{
  colour->red = read_byte(importer);
  colour->green = read_byte(importer);
  colour->blue = read_byte(importer);
}

static float parse_percentage(svtk3DSImporter* importer)
{
  svtk3DSChunk chunk;
  float percent = 0.0;

  start_chunk(importer, &chunk);

  switch (chunk.tag)
  {
    case 0x0030:
      percent = parse_int_percentage(importer) / 100.0;
      break;

    case 0x0031:
      percent = parse_float_percentage(importer);
      break;

    default:
      svtkGenericWarningMacro(<< "Error parsing percentage\n");
  }

  end_chunk(importer, &chunk);

  return percent;
}

static short parse_int_percentage(svtk3DSImporter* importer)
{
  word percent = read_word(importer);

  return percent;
}

static float parse_float_percentage(svtk3DSImporter* importer)
{
  float percent = read_float(importer);

  return percent;
}

static void start_chunk(svtk3DSImporter* importer, svtk3DSChunk* chunk)
{
  chunk->start = ftell(importer->GetFileFD());
  chunk->tag = peek_word(importer);
  chunk->length = peek_dword(importer);
  if (chunk->length == 0)
  {
    chunk->length = 1;
  }
  chunk->end = chunk->start + chunk->length;
}

static void end_chunk(svtk3DSImporter* importer, svtk3DSChunk* chunk)
{
  fseek(importer->GetFileFD(), chunk->end, 0);
}

static byte read_byte(svtk3DSImporter* importer)
{
  byte data;

  data = fgetc(importer->GetFileFD());

  return data;
}

static word read_word(svtk3DSImporter* importer)
{
  word data;

  if (fread(&data, 2, 1, importer->GetFileFD()) != 1)
  {
    svtkErrorWithObjectMacro(importer, "Pre-mature end of file in read_word\n");
    data = 0;
  }
  svtkByteSwap::Swap2LE((short*)&data);
  return data;
}

static word peek_word(svtk3DSImporter* importer)
{
  word data;

  if (fread(&data, 2, 1, importer->GetFileFD()) != 1)
  {
    data = 0;
  }
  svtkByteSwap::Swap2LE((short*)&data);
  return data;
}

static dword peek_dword(svtk3DSImporter* importer)
{
  dword data;

  if (fread(&data, 4, 1, importer->GetFileFD()) != 1)
  {
    data = 0;
  }

  svtkByteSwap::Swap4LE((char*)&data);
  return data;
}

static float read_float(svtk3DSImporter* importer)
{
  float data;

  if (fread(&data, 4, 1, importer->GetFileFD()) != 1)
  {
    svtkErrorWithObjectMacro(importer, "Pre-mature end of file in read_float\n");
    data = 0;
  }

  svtkByteSwap::Swap4LE((char*)&data);
  return data;
}

static void read_point(svtk3DSImporter* importer, svtk3DSVector v)
{
  v[0] = read_float(importer);
  v[1] = read_float(importer);
  v[2] = read_float(importer);
}

static char* read_string(svtk3DSImporter* importer)
{
  static char string[80];
  int i;

  for (i = 0; i < 80; i++)
  {
    string[i] = read_byte(importer);

    if (string[i] == '\0')
    {
      break;
    }
  }

  return string;
}

static void cleanup_name(char* name)
{
  char* tmp = (char*)malloc(strlen(name) + 2);
  int i;

  /* Remove any leading blanks or quotes */
  i = 0;
  while ((name[i] == ' ' || name[i] == '"') && name[i] != '\0')
  {
    i++;
  }
  strcpy(tmp, &name[i]);

  /* Remove any trailing blanks or quotes */
  for (i = static_cast<int>(strlen(tmp)) - 1; i >= 0; i--)
  {
    if (isprint(tmp[i]) && !isspace(tmp[i]) && tmp[i] != '"')
    {
      break;
    }
    else
    {
      tmp[i] = '\0';
    }
  }

  strcpy(name, tmp);

  /* Prefix the letter 'N' to materials that begin with a digit */
  if (!isdigit(name[0]))
  {
    strcpy(tmp, name);
  }
  else
  {
    tmp[0] = 'N';
    strcpy(&tmp[1], name);
  }

  /* Replace all illegal characters in name with underscores */
  for (i = 0; tmp[i] != '\0'; i++)
  {
    if (!isalnum(tmp[i]))
    {
      tmp[i] = '_';
    }
  }

  strcpy(name, tmp);

  free(tmp);
}

svtk3DSImporter::~svtk3DSImporter()
{
  svtk3DSOmniLight* omniLight;
  svtk3DSSpotLight* spotLight;

  // walk the light list and delete svtk objects
  for (omniLight = this->OmniList; omniLight != (svtk3DSOmniLight*)nullptr;
       omniLight = (svtk3DSOmniLight*)omniLight->next)
  {
    omniLight->aLight->Delete();
  }
  SVTK_LIST_KILL(this->OmniList);

  // walk the spot light list and delete svtk objects
  for (spotLight = this->SpotLightList; spotLight != (svtk3DSSpotLight*)nullptr;
       spotLight = (svtk3DSSpotLight*)spotLight->next)
  {
    spotLight->aLight->Delete();
  }
  SVTK_LIST_KILL(this->SpotLightList);

  svtk3DSCamera* camera;
  // walk the camera list and delete svtk objects
  for (camera = this->CameraList; camera != (svtk3DSCamera*)nullptr;
       camera = (svtk3DSCamera*)camera->next)
  {
    camera->aCamera->Delete();
  }
  SVTK_LIST_KILL(this->CameraList);

  // walk the mesh list and delete malloced datra and svtk objects
  svtk3DSMesh* mesh;
  for (mesh = this->MeshList; mesh != (svtk3DSMesh*)nullptr; mesh = (svtk3DSMesh*)mesh->next)
  {
    if (mesh->anActor != nullptr)
    {
      mesh->anActor->Delete();
    }
    if (mesh->aMapper != nullptr)
    {
      mesh->aMapper->Delete();
    }
    if (mesh->aNormals != nullptr)
    {
      mesh->aNormals->Delete();
    }
    if (mesh->aStripper != nullptr)
    {
      mesh->aStripper->Delete();
    }
    if (mesh->aPoints != nullptr)
    {
      mesh->aPoints->Delete();
    }
    if (mesh->aCellArray != nullptr)
    {
      mesh->aCellArray->Delete();
    }
    if (mesh->aPolyData != nullptr)
    {
      mesh->aPolyData->Delete();
    }
    if (mesh->vertex)
    {
      free(mesh->vertex);
    }
    if (mesh->face)
    {
      free(mesh->face);
    }
    if (mesh->mtl)
    {
      free(mesh->mtl);
    }
  }

  // then delete the list structure

  SVTK_LIST_KILL(this->MeshList);
  SVTK_LIST_KILL(this->MaterialList);

  // objects allocated in Material Property List
  svtk3DSMatProp* m;
  // just walk the list of material properties, deleting svtk properties
  for (m = this->MatPropList; m != (svtk3DSMatProp*)nullptr; m = (svtk3DSMatProp*)m->next)
  {
    m->aProperty->Delete();
  }

  // then delete the list structure
  SVTK_LIST_KILL(this->MatPropList);

  delete[] this->FileName;
}

void svtk3DSImporter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "File Name: " << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
}
