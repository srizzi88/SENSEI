/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVtkJSSceneGraphSerializerGraphSerializer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVtkJSSceneGraphSerializer.h"

#include "svtksys/MD5.h"
#include <svtkActor.h>
#include <svtkAlgorithm.h>
#include <svtkCamera.h>
#include <svtkCellData.h>
#include <svtkCollectionIterator.h>
#include <svtkCompositeDataIterator.h>
#include <svtkCompositeDataSet.h>
#include <svtkCompositePolyDataMapper.h>
#include <svtkGlyph3DMapper.h>
#include <svtkIdTypeArray.h>
#include <svtkImageData.h>
#include <svtkLight.h>
#include <svtkLightCollection.h>
#include <svtkLookupTable.h>
#include <svtkMapper.h>
#include <svtkObjectFactory.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderer.h>
#include <svtkTexture.h>
#include <svtkTransform.h>
#include <svtkViewNode.h>
#include <svtkViewNodeCollection.h>
#include <svtksys/SystemTools.hxx>

#if SVTK_MODULE_ENABLE_SVTK_RenderingOpenGL2
#include <svtkCompositeDataDisplayAttributes.h>
#include <svtkCompositePolyDataMapper2.h>
#endif

#include <array>
#include <ios>
#include <sstream>
#include <unordered_map>

//----------------------------------------------------------------------------
namespace
{
static const std::array<char, 13> arrayTypes = {
  ' ', // SVTK_VOID            0
  ' ', // SVTK_BIT             1
  'b', // SVTK_CHAR            2
  'B', // SVTK_UNSIGNED_CHAR   3
  'h', // SVTK_SHORT           4
  'H', // SVTK_UNSIGNED_SHORT  5
  'i', // SVTK_INT             6
  'I', // SVTK_UNSIGNED_INT    7
  'l', // SVTK_LONG            8
  'L', // SVTK_UNSIGNED_LONG   9
  'f', // SVTK_FLOAT          10
  'd', // SVTK_DOUBLE         11
  'L'  // SVTK_ID_TYPE        12
};

static const std::unordered_map<char, std::string> javascriptMapping = { { 'b', "Int8Array" },
  { 'B', "Uint8Array" }, { 'h', "Int16Array" }, { 'H', "Int16Array" }, { 'i', "Int32Array" },
  { 'I', "Uint32Array" }, { 'l', "Int32Array" }, { 'L', "Uint32Array" }, { 'f', "Float32Array" },
  { 'd', "Float64Array" } };

static const std::string getJSArrayType(svtkDataArray* array)
{
  return javascriptMapping.at(arrayTypes.at(array->GetDataType()));
}

static const Json::Value getRangeInfo(svtkDataArray* array, svtkIdType component)
{
  double r[2];
  array->GetRange(r, component);
  Json::Value compRange;
  compRange["min"] = r[0];
  compRange["max"] = r[1];
  compRange["component"] =
    array->GetComponentName(component) ? array->GetComponentName(component) : Json::Value();
  return compRange;
}

void computeMD5(const unsigned char* content, int size, std::string& hash)
{
  unsigned char digest[16];
  char md5Hash[33];
  md5Hash[32] = '\0';

  svtksysMD5* md5 = svtksysMD5_New();
  svtksysMD5_Initialize(md5);
  svtksysMD5_Append(md5, content, size);
  svtksysMD5_Finalize(md5, digest);
  svtksysMD5_DigestToHex(digest, md5Hash);
  svtksysMD5_Delete(md5);

  hash = md5Hash;
}

std::string ptrToString(void* ptr)
{
  std::stringstream s;
  s << std::hex << reinterpret_cast<uintptr_t>(ptr);
  return s.str();
}
}

//----------------------------------------------------------------------------
struct svtkVtkJSSceneGraphSerializer::Internal
{
  Internal()
    : UniqueIdCount(0)
  {
  }

  Json::Value Root;
  std::unordered_map<void*, Json::ArrayIndex> UniqueIds;
  std::size_t UniqueIdCount;
  std::vector<std::pair<Json::ArrayIndex, svtkDataObject*> > DataObjects;
  std::vector<std::pair<std::string, svtkDataArray*> > DataArrays;

  Json::Value* entry(const std::string& index, Json::Value* node);
  Json::Value* entry(const Json::ArrayIndex index) { return entry(std::to_string(index), &Root); }
  Json::Value* entry(void* address) { return entry(UniqueIds.at(address)); }

  Json::ArrayIndex uniqueId(void* ptr = nullptr);
};

Json::Value* svtkVtkJSSceneGraphSerializer::Internal::entry(
  const std::string& index, Json::Value* node)
{
  if (node == nullptr || (*node)["id"] == index)
  {
    return node;
  }

  if (node->isMember("dependencies"))
  {
    for (Json::ArrayIndex i = 0; i < (*node)["dependencies"].size(); ++i)
    {
      Json::Value* n = entry(index, &(*node)["dependencies"][i]);
      if (n != nullptr)
      {
        return n;
      }
    }
  }

  return nullptr;
}

Json::ArrayIndex svtkVtkJSSceneGraphSerializer::Internal::uniqueId(void* ptr)
{
  Json::ArrayIndex id;
  if (ptr == nullptr)
  {
    // There is no associated address for this unique id.
    id = Json::ArrayIndex(UniqueIdCount++);
  }
  else
  {
    // There is an associated address for this unique id, so we use it to ensure
    // that subsequent calls will return the same id.
    auto search = UniqueIds.find(ptr);
    if (search != UniqueIds.end())
    {
      id = search->second;
    }
    else
    {
      id = Json::ArrayIndex(UniqueIdCount++);
      UniqueIds[ptr] = id;
    }
  }
  return id;
}

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkVtkJSSceneGraphSerializer);
//----------------------------------------------------------------------------
svtkVtkJSSceneGraphSerializer::svtkVtkJSSceneGraphSerializer()
  : Internals(new svtkVtkJSSceneGraphSerializer::Internal)
{
}

//----------------------------------------------------------------------------
svtkVtkJSSceneGraphSerializer::~svtkVtkJSSceneGraphSerializer()
{
  delete Internals;
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Reset()
{
  this->Internals->Root = Json::Value();
  this->Internals->UniqueIds.clear();
  this->Internals->UniqueIdCount = 0;
  this->Internals->DataObjects.clear();
  this->Internals->DataArrays.clear();
}

//----------------------------------------------------------------------------
const Json::Value& svtkVtkJSSceneGraphSerializer::GetRoot() const
{
  return this->Internals->Root;
}

//----------------------------------------------------------------------------
svtkIdType svtkVtkJSSceneGraphSerializer::GetNumberOfDataObjects() const
{
  return svtkIdType(this->Internals->DataObjects.size());
}

//----------------------------------------------------------------------------
Json::ArrayIndex svtkVtkJSSceneGraphSerializer::GetDataObjectId(svtkIdType i) const
{
  return this->Internals->DataObjects.at(i).first;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkVtkJSSceneGraphSerializer::GetDataObject(svtkIdType i) const
{
  return this->Internals->DataObjects.at(i).second;
}

//----------------------------------------------------------------------------
svtkIdType svtkVtkJSSceneGraphSerializer::GetNumberOfDataArrays() const
{
  return svtkIdType(this->Internals->DataArrays.size());
}

//----------------------------------------------------------------------------
std::string svtkVtkJSSceneGraphSerializer::GetDataArrayId(svtkIdType i) const
{
  return this->Internals->DataArrays.at(i).first;
}

//----------------------------------------------------------------------------
svtkDataArray* svtkVtkJSSceneGraphSerializer::GetDataArray(svtkIdType i) const
{
  return this->Internals->DataArrays.at(i).second;
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Add(svtkViewNode* node, svtkActor* actor)
{
  // Skip actors that are connected to composite mappers (they are dealt with
  // when the mapper is traversed).
  //
  // TODO: this is an awkward consequence of an external scene graph traversal
  //       mechanism where we cannot abort the traversal of subordinate nodes
  //       and an imperfect parity between SVTK and svtk-js (namely the lack of
  //       support in svtk-js for composite data structures). This logic should
  //       be removed when svtk-js support for composite data structures is in
  //       place.
  {
    svtkViewNodeCollection* children = node->GetChildren();
    if (children->GetNumberOfItems() > 0)
    {
      children->InitTraversal();

      for (svtkViewNode* child = children->GetNextItem(); child != nullptr;
           child = children->GetNextItem())
      {
        if (svtkCompositePolyDataMapper::SafeDownCast(child->GetRenderable())
#if SVTK_MODULE_ENABLE_SVTK_RenderingOpenGL2
          || svtkCompositePolyDataMapper2::SafeDownCast(child->GetRenderable())
#endif
        )
        {
          return;
        }
      }
    }
  }

  Json::Value* parent = this->Internals->entry(node->GetParent()->GetRenderable());
  Json::Value val = this->ToJson(*parent, actor);
  (*parent)["dependencies"].append(val);

  Json::Value v = Json::arrayValue;
  v.append("addViewProp");
  Json::Value w = Json::arrayValue;
  w.append("instance:${" + std::to_string(this->UniqueId(node->GetRenderable())) + "}");
  v.append(w);
  (*parent)["calls"].append(v);
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Add(Json::Value* self, svtkAlgorithm* algorithm)
{
  algorithm->Update();

  // Algorithms have data associated with them, so we construct a unique id for
  // each port and associate it with the data object.
  for (int inputPort = 0; inputPort < algorithm->GetNumberOfInputPorts(); ++inputPort)
  {
    // svtk-js does not support multiple connections, so we always look at
    // connection 0
    static const int connection = 0;
    svtkDataObject* dataObject = algorithm->GetInputDataObject(inputPort, connection);
    Json::ArrayIndex dataId = this->UniqueId(dataObject);
    this->Internals->DataObjects.push_back(std::make_pair(dataId, dataObject));

    (*self)["dependencies"].append(this->ToJson((*self), algorithm, dataObject));
    Json::Value v = Json::arrayValue;
    v.append("setInputData");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + std::to_string(this->UniqueId(dataObject)) + "}");
    w.append(inputPort);
    v.append(w);
    (*self)["calls"].append(v);
  }
}

//----------------------------------------------------------------------------
namespace
{
#if SVTK_MODULE_ENABLE_SVTK_RenderingOpenGL2
// svtkCompositePolyDataMapper2 provides an API for assigning color and opacity
// to each block in the dataset, but svtkCompositePolyDataMapper does not. This
// logic splits the code to apply per-block coloring when it is available.
template <typename CompositeMapper>
typename std::enable_if<std::is_base_of<svtkCompositePolyDataMapper2, CompositeMapper>::value>::type
SetColorAndOpacity(Json::Value& property, CompositeMapper* mapper, svtkDataObject* block)
{
  static const std::array<std::string, 4> colorProperties = { "ambientColor", "color",
    "diffuseColor", "specularColor" };

  // Set the color and opacity according to the dataset's corresponding block
  // information.
  svtkCompositeDataDisplayAttributes* atts = mapper->GetCompositeDataDisplayAttributes();
  if (atts->HasBlockColor(block))
  {
    for (int i = 0; i < 3; i++)
    {
      for (auto& colorProperty : colorProperties)
      {
        property["properties"][colorProperty.c_str()][i] = atts->GetBlockColor(block)[i];
      }
    }
  }
  if (atts->HasBlockOpacity(block))
  {
    property["properties"]["opacity"] = atts->GetBlockOpacity(block);
  }
  if (atts->HasBlockVisibility(block))
  {
    property["properties"]["visibility"] = atts->GetBlockVisibility(block);
  }
}
#endif

template <typename CompositeMapper>
typename std::enable_if<std::is_base_of<svtkCompositePolyDataMapper, CompositeMapper>::value>::type
SetColorAndOpacity(Json::Value&, CompositeMapper*, svtkDataObject*)
{
}
}

//----------------------------------------------------------------------------
template <typename CompositeMapper>
void svtkVtkJSSceneGraphSerializer::Add(
  svtkViewNode* node, svtkDataObject* dataObject, CompositeMapper* mapper)
{
  if (svtkPolyData::SafeDownCast(dataObject) != nullptr)
  {
    // If the data object associated with the composite mapper is a polydata,
    // treat the mapper as a svtk-js Mapper.

    // First, add an actor for the mapper
    Json::Value* parent;
    {
      Json::Value* renderer =
        this->Internals->entry(node->GetParent()->GetParent()->GetRenderable());
      Json::Value actor =
        this->ToJson(*renderer, svtkActor::SafeDownCast(node->GetParent()->GetRenderable()), true);
      Json::ArrayIndex actorId = this->UniqueId();
      actor["id"] = std::to_string(actorId);

      {
        // Locate the actor's entry for its svtkProperty
        for (Json::Value::iterator it = actor["dependencies"].begin();
             it != actor["dependencies"].end(); ++it)
        {
          if ((*it)["type"] == "svtkProperty")
          {
            // Color the actor using the block color, if available
            SetColorAndOpacity(*it, mapper, dataObject);
            break;
          }
        }
      }

      parent = &(*renderer)["dependencies"].append(actor);
      Json::Value v = Json::arrayValue;
      v.append("addViewProp");
      Json::Value w = Json::arrayValue;
      w.append("instance:${" + actor["id"].asString() + "}");
      v.append(w);
      (*renderer)["calls"].append(v);
    }

    // Then, add a new mapper
    {
      Json::ArrayIndex id = this->UniqueId();
      Json::Value value = ToJson(*parent, id, static_cast<svtkMapper*>(mapper), true);

      Json::Value v = Json::arrayValue;
      v.append("setMapper");
      Json::Value w = Json::arrayValue;
      w.append("instance:${" + std::to_string(id) + "}");
      v.append(w);
      (*parent)["calls"].append(v);
      parent = &(*parent)["dependencies"].append(value);
    }

    // Finally, add the data object for the mapper
    {
      // Assign the data object a unique id and record it
      Json::ArrayIndex dataId = this->UniqueId(dataObject);
      this->Internals->DataObjects.push_back(std::make_pair(dataId, dataObject));

      (*parent)["dependencies"].append(
        this->ToJson(*parent, static_cast<svtkMapper*>(mapper), dataObject));
      Json::Value v = Json::arrayValue;
      v.append("setInputData");
      Json::Value w = Json::arrayValue;
      w.append("instance:${" + std::to_string(dataId) + "}");
      v.append(w);
      (*parent)["calls"].append(v);
    }
  }
  else
  {
    // Otherwise, we must construct a svtk-js Mapper for each nonempty node in
    // the composite dataset.
    svtkCompositeDataSet* composite = svtkCompositeDataSet::SafeDownCast(dataObject);
    svtkSmartPointer<svtkCompositeDataIterator> iter = composite->NewIterator();
    iter->SkipEmptyNodesOn();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      this->Add<CompositeMapper>(node, iter->GetCurrentDataObject(), mapper);
      iter->GoToNextItem();
    }
  }
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Add(svtkViewNode* node, svtkCompositePolyDataMapper* mapper)
{
  this->Add<svtkCompositePolyDataMapper>(node, mapper->GetInputDataObject(0, 0), mapper);
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Add(svtkViewNode* node, svtkCompositePolyDataMapper2* mapper)
{
#if SVTK_MODULE_ENABLE_SVTK_RenderingOpenGL2
  this->Add<svtkCompositePolyDataMapper2>(node, mapper->GetInputDataObject(0, 0), mapper);
#else
  (void)node;
  (void)mapper;
#endif
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Add(svtkViewNode* node, svtkGlyph3DMapper* mapper)
{
  // TODO: svtkGlyph3DMapper and its derived implementation
  //       svtkOpenGLGlyph3DMapper may have composite datasets for both the glyph
  //       representations and instances. The logic for handling this is rather
  //       complex and is currently inaccessible outside of its implementation.
  //       Rather than duplicate that logic here, there should be exposed
  //       methods on svtkGlyph3DMapper to "flatten" a mapper with composite
  //       inputs into a collection of glyph mappers that use svtkPolyData (as is
  //       currently in the implementation). Until then, we only handle the case
  //       with svtkPolyData for the glyph representations and indices.
  for (int inputPort = 0; inputPort < mapper->GetNumberOfInputPorts(); ++inputPort)
  {
    // svtk-js does not support multiple connections, so we always look at
    // connection 0
    static const int connection = 0;
    svtkDataObject* dataObject = mapper->GetInputDataObject(inputPort, connection);
    if (svtkCompositeDataSet::SafeDownCast(dataObject) != nullptr)
    {
      svtkErrorMacro(<< "Composite data sets are not currently supported for svtk-js glyph mappers.");
      return;
    }
  }

  Json::Value* parent = this->Internals->entry(node->GetParent()->GetRenderable());
  Json::Value val = this->ToJson(*parent, this->UniqueId(mapper), mapper);
  (*parent)["dependencies"].append(val);

  Json::Value v = Json::arrayValue;
  v.append("setMapper");
  Json::Value w = Json::arrayValue;
  w.append("instance:${" + std::to_string(this->UniqueId(node->GetRenderable())) + "}");
  v.append(w);
  (*parent)["calls"].append(v);

  this->Add(this->Internals->entry(node->GetRenderable()), svtkAlgorithm::SafeDownCast(mapper));
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Add(svtkViewNode* node, svtkMapper* mapper)
{
  Json::Value* parent = this->Internals->entry(node->GetParent()->GetRenderable());
  Json::Value val = this->ToJson(*parent, this->UniqueId(mapper), mapper);
  (*parent)["dependencies"].append(val);

  Json::Value v = Json::arrayValue;
  v.append("setMapper");
  Json::Value w = Json::arrayValue;
  w.append("instance:${" + std::to_string(this->UniqueId(node->GetRenderable())) + "}");
  v.append(w);
  (*parent)["calls"].append(v);

  this->Add(this->Internals->entry(node->GetRenderable()), svtkAlgorithm::SafeDownCast(mapper));
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Add(svtkViewNode* node, svtkRenderer* renderer)
{
  Json::Value* parent = this->Internals->entry(node->GetParent()->GetRenderable());
  Json::Value val = this->ToJson(*parent, renderer);
  (*parent)["dependencies"].append(val);

  Json::Value v = Json::arrayValue;
  v.append("addRenderer");
  Json::Value w = Json::arrayValue;
  w.append("instance:${" + std::to_string(this->UniqueId(node->GetRenderable())) + "}");
  v.append(w);
  (*parent)["calls"].append(v);
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::Add(svtkViewNode*, svtkRenderWindow* window)
{
  this->Internals->Root = this->ToJson(window);
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, svtkAlgorithm* algorithm, svtkDataObject* dataObject)
{
  if (svtkImageData* imageData = svtkImageData::SafeDownCast(dataObject))
  {
    return this->ToJson(parent, algorithm, imageData);
  }
  else if (svtkPolyData* polyData = svtkPolyData::SafeDownCast(dataObject))
  {
    return this->ToJson(parent, algorithm, polyData);
  }
  else
  {
    svtkErrorMacro(<< "Cannot export data object of type \"" << dataObject->GetClassName() << "\".");
    return Json::Value();
  }
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, svtkAlgorithm* algorithm, svtkImageData* imageData)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(imageData));
  val["type"] = "svtkImageData";

  Json::Value properties;

  properties["address"] = ptrToString(imageData);

  for (int i = 0; i < 3; i++)
  {
    properties["spacing"][i] = imageData->GetSpacing()[i];
    properties["origin"][i] = imageData->GetOrigin()[i];
  }
  for (int i = 0; i < 6; i++)
  {
    properties["extent"][i] = imageData->GetExtent()[i];
  }

  properties["fields"] = Json::arrayValue;
  this->extractRequiredFields(properties["fields"], svtkMapper::SafeDownCast(algorithm), imageData);

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(svtkDataArray* array)
{
  Json::Value val;
  std::string hash;
  {
    const unsigned char* content = (const unsigned char*)array->GetVoidPointer(0);
    int size = array->GetNumberOfValues() * array->GetDataTypeSize();
    computeMD5(content, size, hash);
  }
  this->Internals->DataArrays.push_back(std::make_pair(hash, array));
  val["hash"] = hash;
  val["svtkClass"] = "svtkDataArray";
  val["name"] = array->GetName() ? array->GetName() : Json::Value();
  val["dataType"] = getJSArrayType(array);
  val["numberOfComponents"] = array->GetNumberOfComponents();
  val["size"] = Json::Value::UInt64(array->GetNumberOfComponents() * array->GetNumberOfTuples());
  val["ranges"] = Json::arrayValue;
  if (array->GetNumberOfComponents() > 1)
  {
    for (int i = 0; i < array->GetNumberOfComponents(); ++i)
    {
      val["ranges"].append(getRangeInfo(array, i));
    }
    val["ranges"].append(getRangeInfo(array, -1));
  }
  else
  {
    val["ranges"].append(getRangeInfo(array, 0));
  }
  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, svtkAlgorithm* algorithm, svtkPolyData* polyData)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(polyData));
  val["type"] = "svtkPolyData";

  Json::Value properties;

  properties["address"] = ptrToString(polyData);

  {
    properties["points"] = this->ToJson(polyData->GetPoints()->GetData());
    properties["points"]["svtkClass"] = "svtkPoints";
  }

  if (polyData->GetVerts() && polyData->GetVerts()->GetData()->GetNumberOfTuples() > 0)
  {
    properties["verts"] = this->ToJson(polyData->GetVerts()->GetData());
    properties["verts"]["svtkClass"] = "svtkCellArray";
  }

  if (polyData->GetLines() && polyData->GetLines()->GetData()->GetNumberOfTuples() > 0)
  {
    properties["lines"] = this->ToJson(polyData->GetLines()->GetData());
    properties["lines"]["svtkClass"] = "svtkCellArray";
  }

  if (polyData->GetPolys() && polyData->GetPolys()->GetData()->GetNumberOfTuples() > 0)
  {
    properties["polys"] = this->ToJson(polyData->GetPolys()->GetData());
    properties["polys"]["svtkClass"] = "svtkCellArray";
  }

  if (polyData->GetStrips() && polyData->GetStrips()->GetData()->GetNumberOfTuples() > 0)
  {
    properties["strips"] = this->ToJson(polyData->GetStrips()->GetData());
    properties["strips"]["svtkClass"] = "svtkCellArray";
  }

  properties["fields"] = Json::arrayValue;
  this->extractRequiredFields(properties["fields"], svtkMapper::SafeDownCast(algorithm), polyData);

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, svtkProperty* property)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(property));
  val["type"] = "svtkProperty";

  Json::Value properties;

  properties["address"] = ptrToString(property);
  properties["representation"] = property->GetRepresentation();
  for (int i = 0; i < 3; i++)
  {
    properties["diffuseColor"][i] = property->GetDiffuseColor()[i];
    properties["color"][i] = property->GetColor()[i];
    properties["ambientColor"][i] = property->GetAmbientColor()[i];
    properties["specularColor"][i] = property->GetSpecularColor()[i];
    properties["edgeColor"][i] = property->GetEdgeColor()[i];
  }
  properties["ambient"] = property->GetAmbient();
  properties["diffuse"] = property->GetDiffuse();
  properties["specular"] = property->GetSpecular();
  properties["specularPower"] = property->GetSpecularPower();
  properties["opacity"] = property->GetOpacity();
  properties["interpolation"] = property->GetInterpolation();
  properties["edgeVisibility"] = property->GetEdgeVisibility();
  properties["backfaceCulling"] = property->GetBackfaceCulling();
  properties["frontfaceCulling"] = property->GetFrontfaceCulling();
  properties["pointSize"] = property->GetPointSize();
  properties["lineWidth"] = property->GetLineWidth();
  properties["lighting"] = property->GetLighting();

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, svtkTransform* transform)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(transform));
  val["type"] = "svtkTransform";

  Json::Value properties;

  properties["address"] = ptrToString(transform);
  std::array<double, 3> scale;
  transform->GetScale(scale.data());
  for (int i = 0; i < 3; i++)
  {
    properties["scale"][i] = scale[i];
  }
  std::array<double, 4> orientation;
  transform->GetOrientationWXYZ(orientation.data());
  for (int i = 0; i < 4; i++)
  {
    properties["orientationWXYZ"][i] = orientation[i];
  }

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, svtkTexture* texture)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(texture));
  val["type"] = "svtkTexture";

  Json::Value properties;

  properties["address"] = ptrToString(texture);
  properties["repeat"] = texture->GetRepeat();
  properties["edgeClamp"] = texture->GetEdgeClamp();
  properties["interpolate"] = texture->GetInterpolate();
  properties["mipmap"] = texture->GetMipmap();
  properties["maximumAnisotropicFiltering"] = texture->GetMaximumAnisotropicFiltering();
  properties["quality"] = texture->GetQuality();
  properties["colorMode"] = texture->GetColorMode();
  properties["blendingMode"] = texture->GetBlendingMode();
  properties["premulipliedAlpha"] = texture->GetPremultipliedAlpha();
  properties["restrictPowerOf2ImageSmaller"] = texture->GetRestrictPowerOf2ImageSmaller();
  properties["cubeMap"] = texture->GetCubeMap();
  properties["useSRGBColorSpace"] = texture->GetUseSRGBColorSpace();

  svtkLookupTable* lookupTable = svtkLookupTable::SafeDownCast(texture->GetLookupTable());
  if (lookupTable != nullptr)
  {
    Json::Value lut = this->ToJson(val, lookupTable);
    std::string lutId = std::to_string(this->UniqueId(lookupTable));
    lut["id"] = lutId;
    val["dependencies"].append(lut);
    Json::Value v = Json::arrayValue;
    v.append("setLookupTable");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + lutId + "}");
    v.append(w);
    val["calls"].append(v);
  }

  svtkTransform* transform = texture->GetTransform();
  if (transform != nullptr)
  {
    Json::Value trans = this->ToJson(val, transform);
    std::string transId = std::to_string(this->UniqueId(lookupTable));
    trans["id"] = transId;
    val["dependencies"].append(trans);
    Json::Value v = Json::arrayValue;
    v.append("setTransform");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + transId + "}");
    v.append(w);
    val["calls"].append(v);
  }

  val["properties"] = properties;

  this->Add(&val, static_cast<svtkAlgorithm*>(texture));

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, svtkActor* actor, bool newPropertyId)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(actor));
  val["type"] = "svtkActor";

  Json::Value properties;
  properties["address"] = ptrToString(actor);
  for (int i = 0; i < 3; i++)
  {
    properties["origin"][i] = actor->GetOrigin()[i];
    properties["scale"][i] = actor->GetScale()[i];
    properties["position"][i] = actor->GetPosition()[i];
    properties["orientation"][i] = actor->GetOrientation()[i];
  }
  properties["visibility"] = actor->GetVisibility();
  properties["pickable"] = actor->GetPickable();
  properties["dragable"] = actor->GetDragable();
  properties["useBounds"] = actor->GetUseBounds();
  properties["renderTimeMultiplier"] = actor->GetRenderTimeMultiplier();

  val["properties"] = properties;
  val["dependencies"] = Json::arrayValue;
  val["calls"] = Json::arrayValue;

  svtkProperty* property = svtkProperty::SafeDownCast(actor->GetProperty());
  if (property != nullptr)
  {
    Json::Value prop = this->ToJson(val, property);
    std::string propertyId =
      (newPropertyId ? std::to_string(this->UniqueId()) : std::to_string(this->UniqueId(property)));
    prop["id"] = propertyId;
    val["dependencies"].append(prop);
    Json::Value v = Json::arrayValue;
    v.append("setProperty");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + propertyId + "}");
    v.append(w);
    val["calls"].append(v);
  }

  svtkTexture* texture = actor->GetTexture();
  if (texture != nullptr)
  {
    Json::Value tex = this->ToJson(val, texture);
    std::string textureId = std::to_string(this->UniqueId(texture));
    tex["id"] = textureId;
    val["dependencies"].append(tex);
    Json::Value v = Json::arrayValue;
    v.append("addTexture");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + textureId + "}");
    v.append(w);
    val["calls"].append(v);
  }

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, svtkLookupTable* lookupTable)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(lookupTable));
  val["type"] = "svtkLookupTable";

  Json::Value properties;

  properties["address"] = ptrToString(lookupTable);
  properties["numberOfColors"] = static_cast<Json::Value::Int64>(lookupTable->GetNumberOfColors());
  for (int i = 0; i < 2; ++i)
  {
    properties["alphaRange"][i] = lookupTable->GetAlphaRange()[i];
    properties["hueRange"][i] = lookupTable->GetHueRange()[i];
    properties["saturationRange"][i] = lookupTable->GetSaturationRange()[i];
    properties["valueRange"][i] = lookupTable->GetValueRange()[i];
  }
  for (int i = 0; i < 4; ++i)
  {
    properties["nanColor"][i] = lookupTable->GetNanColor()[i];
    properties["belowRangeColor"][i] = lookupTable->GetBelowRangeColor()[i];
    properties["aboveRangeColor"][i] = lookupTable->GetAboveRangeColor()[i];
  }

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, Json::ArrayIndex id, svtkMapper* mapper, bool newLUTId)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(id);
  val["type"] = "svtkMapper";

  Json::Value properties;

  properties["address"] = ptrToString(mapper);
  properties["colorByArrayName"] = mapper->GetArrayName();
  properties["arrayAccessMode"] = mapper->GetArrayAccessMode();
  properties["colorMode"] = mapper->GetColorMode();
  properties["fieldDataTupleId"] = static_cast<Json::Value::Int64>(mapper->GetFieldDataTupleId());
  properties["interpolateScalarsBeforeMapping"] = mapper->GetInterpolateScalarsBeforeMapping();
  properties["renderTime"] = mapper->GetRenderTime();
  properties["resolveCoincidentTopology"] = mapper->GetResolveCoincidentTopology();
  properties["scalarMode"] = mapper->GetScalarMode();
  properties["scalarVisibility"] = mapper->GetScalarVisibility();
  properties["static"] = mapper->GetStatic();
  properties["useLookupTableScalarRange"] = mapper->GetUseLookupTableScalarRange();

  val["properties"] = properties;
  val["dependencies"] = Json::arrayValue;
  val["calls"] = Json::arrayValue;

  svtkLookupTable* lookupTable = svtkLookupTable::SafeDownCast(mapper->GetLookupTable());
  if (lookupTable != nullptr)
  {
    Json::Value lut = this->ToJson(val, lookupTable);
    std::string lutId =
      (newLUTId ? std::to_string(this->UniqueId()) : std::to_string(this->UniqueId(lookupTable)));
    lut["id"] = lutId;
    val["dependencies"].append(lut);
    Json::Value v = Json::arrayValue;
    v.append("setLookupTable");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + lutId + "}");
    v.append(w);
    val["calls"].append(v);
  }
  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(
  Json::Value& parent, Json::ArrayIndex id, svtkGlyph3DMapper* mapper)
{
  Json::Value val = this->ToJson(parent, id, static_cast<svtkMapper*>(mapper));
  val["type"] = "svtkGlyph3DMapper";

  Json::Value& properties = val["properties"];

  properties["address"] = ptrToString(mapper);
  properties["orient"] = mapper->GetOrient();
  properties["orientationMode"] = mapper->GetOrientationMode();
  properties["scaleFactor"] = mapper->GetScaleFactor();
  properties["scaleMode"] = mapper->GetScaleMode();
  properties["scaling"] = mapper->GetScaling();
  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, svtkCamera* camera)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(camera));
  val["type"] = "svtkCamera";

  Json::Value properties;

  properties["address"] = ptrToString(camera);

  for (int i = 0; i < 3; ++i)
  {
    properties["focalPoint"][i] = camera->GetFocalPoint()[i];
    properties["position"][i] = camera->GetPosition()[i];
    properties["viewUp"][i] = camera->GetViewUp()[i];
  }

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, svtkLight* light)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(light));
  val["type"] = "svtkLight";

  Json::Value properties;

  properties["address"] = ptrToString(light);
  properties["intensity"] = light->GetIntensity();
  properties["switch"] = light->GetSwitch();
  properties["positional"] = light->GetPositional();
  properties["exponent"] = light->GetExponent();
  properties["coneAngle"] = light->GetConeAngle();
  std::array<std::string, 4> lightTypes{ "", "HeadLight", "SceneLight", "CameraLight" };
  properties["lightType"] = lightTypes[light->GetLightType()];
  properties["shadowAttenuation"] = light->GetShadowAttenuation();

  for (int i = 0; i < 3; ++i)
  {
    properties["color"][i] = light->GetDiffuseColor()[i];
    properties["focalPoint"][i] = light->GetFocalPoint()[i];
    properties["position"][i] = light->GetPosition()[i];
    properties["attenuationValues"][i] = light->GetAttenuationValues()[i];
  }

  val["properties"] = properties;

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(Json::Value& parent, svtkRenderer* renderer)
{
  Json::Value val;
  val["parent"] = parent["id"];
  val["id"] = std::to_string(this->UniqueId(renderer));
  val["type"] = renderer->GetClassName();

  Json::Value properties;

  properties["address"] = ptrToString(renderer);
  properties["twoSidedLighting"] = renderer->GetTwoSidedLighting();
  properties["lightFollowCamera"] = renderer->GetLightFollowCamera();
  properties["automaticLightCreation"] = renderer->GetAutomaticLightCreation();
  properties["erase"] = renderer->GetErase();
  properties["draw"] = renderer->GetDraw();
  properties["nearClippingPlaneTolerance"] = renderer->GetNearClippingPlaneTolerance();
  properties["clippingRangeExpansion"] = renderer->GetClippingRangeExpansion();
  properties["backingStore"] = renderer->GetBackingStore();
  properties["interactive"] = renderer->GetInteractive();
  properties["layer"] = renderer->GetLayer();
  properties["preserveColorBuffer"] = renderer->GetPreserveColorBuffer();
  properties["preserveDepthBuffer"] = renderer->GetPreserveDepthBuffer();
  properties["useDepthPeeling"] = renderer->GetUseDepthPeeling();
  properties["occlusionRatio"] = renderer->GetOcclusionRatio();
  properties["maximumNumberOfPeels"] = renderer->GetMaximumNumberOfPeels();
  properties["useShadows"] = renderer->GetUseShadows();
  for (int i = 0; i < 3; ++i)
  {
    properties["background"][i] = renderer->GetBackground()[i];
  }
  properties["background"][3] = 1.;

  val["properties"] = properties;
  val["dependencies"] = Json::arrayValue;
  val["calls"] = Json::arrayValue;

  {
    val["dependencies"].append(this->ToJson(val, renderer->GetActiveCamera()));
    Json::Value v = Json::arrayValue;
    v.append("setActiveCamera");
    Json::Value w = Json::arrayValue;
    w.append("instance:${" + std::to_string(this->UniqueId(renderer->GetActiveCamera())) + "}");
    v.append(w);
    val["calls"].append(v);
  }

  svtkLightCollection* lights = renderer->GetLights();

  if (lights->GetNumberOfItems() > 0)
  {
    lights->InitTraversal();

    Json::Value v = Json::arrayValue;
    v.append("addLight");
    Json::Value w = Json::arrayValue;
    for (svtkLight* light = lights->GetNextItem(); light != nullptr; light = lights->GetNextItem())
    {
      val["dependencies"].append(this->ToJson(val, light));
      w.append("instance:${" + std::to_string(this->UniqueId(light)) + "}");
    }
    v.append(w);
    val["calls"].append(v);
  }

  return val;
}

//----------------------------------------------------------------------------
Json::Value svtkVtkJSSceneGraphSerializer::ToJson(svtkRenderWindow* renderWindow)
{
  Json::Value val;
  val["parent"] = "0x0";
  val["id"] = std::to_string(this->UniqueId(renderWindow));
  val["type"] = renderWindow->GetClassName();
  val["mtime"] = static_cast<Json::UInt64>(renderWindow->GetMTime());

  Json::Value properties;
  properties["address"] = ptrToString(renderWindow);
  properties["numberOfLayers"] = renderWindow->GetNumberOfLayers();

  val["properties"] = properties;
  val["dependencies"] = Json::arrayValue;
  val["calls"] = Json::arrayValue;

  return val;
}

//----------------------------------------------------------------------------
Json::ArrayIndex svtkVtkJSSceneGraphSerializer::UniqueId(void* ptr)
{
  return this->Internals->uniqueId(ptr);
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::extractRequiredFields(
  Json::Value& extractedFields, svtkMapper* mapper, svtkDataSet* dataSet)
{
  // FIXME should evolve and support funky mapper which leverage many arrays
  svtkDataArray* pointDataArray = nullptr;
  svtkDataArray* cellDataArray = nullptr;
  if (mapper != nullptr && mapper->IsA("svtkMapper"))
  {
    svtkTypeBool scalarVisibility = mapper->GetScalarVisibility();
    int arrayAccessMode = mapper->GetArrayAccessMode();

    int scalarMode = mapper->GetScalarMode();
    if (scalarVisibility && scalarMode == 3)
    {
      pointDataArray =
        (arrayAccessMode == 1 ? dataSet->GetPointData()->GetArray(mapper->GetArrayName())
                              : dataSet->GetPointData()->GetArray(mapper->GetArrayId()));

      if (pointDataArray != nullptr)
      {
        Json::Value arrayMeta = this->ToJson(pointDataArray);
        arrayMeta["location"] = "pointData";
        extractedFields.append(arrayMeta);
      }
    }

    if (scalarVisibility && scalarMode == 4)
    {
      cellDataArray =
        (arrayAccessMode == 1 ? dataSet->GetCellData()->GetArray(mapper->GetArrayName())
                              : dataSet->GetCellData()->GetArray(mapper->GetArrayId()));
      if (cellDataArray != nullptr)
      {
        Json::Value arrayMeta = this->ToJson(cellDataArray);
        arrayMeta["location"] = "cellData";
        extractedFields.append(arrayMeta);
      }
    }
  }

  if (pointDataArray == nullptr)
  {
    svtkDataArray* array = dataSet->GetPointData()->GetScalars();
    if (array != nullptr)
    {
      Json::Value arrayMeta = this->ToJson(array);
      arrayMeta["location"] = "pointData";
      arrayMeta["registration"] = "setScalars";
      extractedFields.append(arrayMeta);
    }
  }

  if (cellDataArray == nullptr)
  {
    svtkDataArray* array = dataSet->GetCellData()->GetScalars();
    if (array != nullptr)
    {
      Json::Value arrayMeta = this->ToJson(array);
      arrayMeta["location"] = "cellData";
      arrayMeta["registration"] = "setScalars";
      extractedFields.append(arrayMeta);
    }
  }

  // Normal handling
  svtkDataArray* normals = dataSet->GetPointData()->GetNormals();
  if (normals != nullptr)
  {
    Json::Value arrayMeta = this->ToJson(normals);
    arrayMeta["location"] = "pointData";
    arrayMeta["registration"] = "setNormals";
    extractedFields.append(arrayMeta);
  }

  // TCoord handling
  svtkDataArray* tcoords = dataSet->GetPointData()->GetTCoords();
  if (tcoords != nullptr)
  {
    Json::Value arrayMeta = this->ToJson(tcoords);
    arrayMeta["location"] = "pointData";
    arrayMeta["registration"] = "setTCoords";
    extractedFields.append(arrayMeta);
  }
}

//----------------------------------------------------------------------------
void svtkVtkJSSceneGraphSerializer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
