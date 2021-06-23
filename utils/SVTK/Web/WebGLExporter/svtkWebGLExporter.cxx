/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebGLExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkWebGLExporter.h"

#include "svtkAbstractMapper.h"
#include "svtkActor2D.h"
#include "svtkActorCollection.h"
#include "svtkBase64Utilities.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkDiscretizableColorTransferFunction.h"
#include "svtkFollower.h"
#include "svtkGenericCell.h"
#include "svtkMapper.h"
#include "svtkMapper2D.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkScalarBarActor.h"
#include "svtkScalarBarRepresentation.h"
#include "svtkSmartPointer.h"
#include "svtkTriangleFilter.h"
#include "svtkViewport.h"
#include "svtkWidgetRepresentation.h"

#include "svtkWebGLObject.h"
#include "svtkWebGLPolyData.h"
#include "svtkWebGLWidget.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "glMatrix.h"
#include "webglRenderer.h"

#include "svtksys/FStream.hxx"
#include "svtksys/MD5.h"
#include "svtksys/SystemTools.hxx"

//*****************************************************************************
class svtkWebGLExporter::svtkInternal
{
public:
  std::string LastMetaData;
  std::map<svtkProp*, svtkMTimeType> ActorTimestamp;
  std::map<svtkProp*, svtkMTimeType> OldActorTimestamp;
  std::vector<svtkWebGLObject*> Objects;
  std::vector<svtkWebGLObject*> tempObj;
};
//*****************************************************************************

svtkStandardNewMacro(svtkWebGLExporter);

svtkWebGLExporter::svtkWebGLExporter()
{
  this->meshObjMaxSize = 65532 / 3;
  this->lineObjMaxSize = 65534 / 2;
  this->Internal = new svtkInternal();
  this->TriangleFilter = nullptr;
  this->GradientBackground = false;
  this->SetCenterOfRotation(0.0, 0.0, 0.0);
  this->renderersMetaData = "";
  this->SceneSize[0] = 0;
  this->SceneSize[1] = 0;
  this->SceneSize[2] = 0;
  this->hasWidget = false;
}

svtkWebGLExporter::~svtkWebGLExporter()
{
  while (!this->Internal->Objects.empty())
  {
    svtkWebGLObject* obj = this->Internal->Objects.back();
    obj->Delete();
    this->Internal->Objects.pop_back();
  }
  delete this->Internal;
  if (this->TriangleFilter)
  {
    this->TriangleFilter->Delete();
  }
}

void svtkWebGLExporter::SetMaxAllowedSize(int mesh, int lines)
{
  this->meshObjMaxSize = mesh;
  this->lineObjMaxSize = lines;
  if (this->meshObjMaxSize * 3 > 65532)
    this->meshObjMaxSize = 65532 / 3;
  if (this->lineObjMaxSize * 2 > 65534)
    this->lineObjMaxSize = 65534 / 2;
  if (this->meshObjMaxSize < 10)
    this->meshObjMaxSize = 10;
  if (this->lineObjMaxSize < 10)
    this->lineObjMaxSize = 10;
  for (size_t i = 0; i < this->Internal->Objects.size(); i++)
    this->Internal->Objects[i]->GenerateBinaryData();
}

void svtkWebGLExporter::SetMaxAllowedSize(int size)
{
  this->SetMaxAllowedSize(size, size);
}

void svtkWebGLExporter::SetCenterOfRotation(float a1, float a2, float a3)
{
  this->CenterOfRotation[0] = a1;
  this->CenterOfRotation[1] = a2;
  this->CenterOfRotation[2] = a3;
}

void svtkWebGLExporter::parseRenderer(
  svtkRenderer* renderer, const char* svtkNotUsed(viewId), bool onlyWidget, void* svtkNotUsed(mapTime))
{
  svtkPropCollection* propCollection = renderer->GetViewProps();
  for (int i = 0; i < propCollection->GetNumberOfItems(); i++)
  {
    svtkProp* prop = (svtkProp*)propCollection->GetItemAsObject(i);
    svtkWidgetRepresentation* trt = svtkWidgetRepresentation::SafeDownCast(prop);
    if (trt != nullptr)
      this->hasWidget = true;
    if ((onlyWidget == false || trt != nullptr) && prop->GetVisibility())
    {
      svtkPropCollection* allactors = svtkPropCollection::New();
      prop->GetActors(allactors);
      for (int j = 0; j < allactors->GetNumberOfItems(); j++)
      {
        svtkActor* actor = svtkActor::SafeDownCast(allactors->GetItemAsObject(j));
        svtkActor* key = actor;
        svtkMTimeType previousValue = this->Internal->OldActorTimestamp[key];
        this->parseActor(
          actor, previousValue, (size_t)renderer, renderer->GetLayer(), trt != nullptr);
      }
      allactors->Delete();
    }
    if (onlyWidget == false && prop->GetVisibility())
    {
      svtkPropCollection* all2dactors = svtkPropCollection::New();
      prop->GetActors2D(all2dactors);
      for (int k = 0; k < all2dactors->GetNumberOfItems(); k++)
      {
        svtkActor2D* actor = svtkActor2D::SafeDownCast(all2dactors->GetItemAsObject(k));
        svtkActor2D* key = actor;
        svtkMTimeType previousValue = this->Internal->OldActorTimestamp[key];
        this->parseActor2D(
          actor, previousValue, (size_t)renderer, renderer->GetLayer(), trt != nullptr);
      }
      all2dactors->Delete();
    }
  }
}

void svtkWebGLExporter::parseActor2D(
  svtkActor2D* actor, svtkMTimeType actorTime, size_t renderId, int layer, bool isWidget)
{
  svtkActor2D* key = actor;
  svtkScalarBarActor* scalarbar = svtkScalarBarActor::SafeDownCast(actor);

  svtkMTimeType dataMTime =
    actor->GetMTime() + actor->GetRedrawMTime() + actor->GetProperty()->GetMTime();
  dataMTime += (svtkMTimeType)actor->GetMapper();
  if (scalarbar)
    dataMTime += scalarbar->GetLookupTable()->GetMTime();
  if (dataMTime != actorTime && actor->GetVisibility())
  {
    this->Internal->ActorTimestamp[key] = dataMTime;

    if (actor->GetMapper())
    {
      std::string name = actor->GetMapper()->GetClassName();
      if (svtkPolyDataMapper2D::SafeDownCast(actor->GetMapper()))
      {
      }
    }
    else
    {
      if (scalarbar)
      {
        svtkWebGLWidget* obj = svtkWebGLWidget::New();
        obj->GetDataFromColorMap(actor);

        std::stringstream ss;
        ss << (size_t)actor;
        obj->SetId(ss.str());
        obj->SetRendererId(static_cast<int>(renderId));
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetIsWidget(isWidget);
        obj->SetInteractAtServer(false);
        obj->GenerateBinaryData();
      }
    }
  }
  else
  {
    this->Internal->ActorTimestamp[key] = dataMTime;
    std::stringstream ss;
    ss << (svtkMTimeType)actor;
    for (size_t i = 0; i < this->Internal->tempObj.size(); i++)
    {
      if (this->Internal->tempObj[i]->GetId().compare(ss.str()) == 0)
      {
        svtkWebGLObject* obj = this->Internal->tempObj[i];
        this->Internal->tempObj.erase(this->Internal->tempObj.begin() + i);
        obj->SetVisibility(actor->GetVisibility() != 0);
        this->Internal->Objects.push_back(obj);
      }
    }
  }
}

void svtkWebGLExporter::parseActor(
  svtkActor* actor, svtkMTimeType actorTime, size_t rendererId, int layer, bool isWidget)
{
  svtkMapper* mapper = actor->GetMapper();
  if (mapper)
  {
    svtkMTimeType dataMTime;
    svtkTriangleFilter* polydata = this->GetPolyData(mapper, dataMTime);
    svtkActor* key = actor;
    dataMTime = actor->GetMTime() + mapper->GetLookupTable()->GetMTime();
    dataMTime += actor->GetProperty()->GetMTime() + mapper->GetMTime() + actor->GetRedrawMTime();
    dataMTime +=
      polydata->GetOutput()->GetNumberOfLines() + polydata->GetOutput()->GetNumberOfPolys();
    dataMTime +=
      actor->GetProperty()->GetRepresentation() + mapper->GetScalarMode() + actor->GetVisibility();
    dataMTime += polydata->GetInput()->GetMTime();
    if (svtkFollower::SafeDownCast(actor))
      dataMTime += svtkFollower::SafeDownCast(actor)->GetCamera()->GetMTime();
    if (dataMTime != actorTime && actor->GetVisibility())
    {
      double bb[6];
      actor->GetBounds(bb);
      double m1 = std::max(bb[1] - bb[0], bb[3] - bb[2]);
      m1 = std::max(m1, bb[5] - bb[4]);
      double m2 = std::max(this->SceneSize[0], this->SceneSize[1]);
      m2 = std::max(m2, this->SceneSize[2]);
      if (m1 > m2)
      {
        this->SceneSize[0] = bb[1] - bb[0];
        this->SceneSize[1] = bb[3] - bb[2];
        this->SceneSize[2] = bb[5] - bb[4];
      }

      this->Internal->ActorTimestamp[key] = dataMTime;
      svtkWebGLObject* obj = nullptr;
      std::stringstream ss;
      ss << (size_t)actor;
      for (size_t i = 0; i < this->Internal->tempObj.size(); i++)
      {
        if (this->Internal->tempObj[i]->GetId().compare(ss.str()) == 0)
        {
          obj = this->Internal->tempObj[i];
          this->Internal->tempObj.erase(this->Internal->tempObj.begin() + i);
        }
      }
      if (obj == nullptr)
        obj = svtkWebGLPolyData::New();

      if (polydata->GetOutput()->GetNumberOfPolys() != 0)
      {
        if (actor->GetProperty()->GetRepresentation() == SVTK_WIREFRAME)
        {
          ((svtkWebGLPolyData*)obj)
            ->GetLinesFromPolygon(mapper, actor, this->lineObjMaxSize, nullptr);
        }
        else
        {

          if (actor->GetProperty()->GetEdgeVisibility())
          {
            svtkWebGLPolyData* newobj = svtkWebGLPolyData::New();
            double ccc[3];
            actor->GetProperty()->GetEdgeColor(&ccc[0]);
            ((svtkWebGLPolyData*)newobj)
              ->GetLinesFromPolygon(mapper, actor, this->lineObjMaxSize, ccc);
            newobj->SetId(ss.str() + "1");
            newobj->SetRendererId(static_cast<int>(rendererId));
            this->Internal->Objects.push_back(newobj);
            newobj->SetLayer(layer);
            newobj->SetTransformationMatrix(actor->GetMatrix());
            newobj->SetVisibility(actor->GetVisibility() != 0);
            newobj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
            newobj->SetIsWidget(isWidget);
            newobj->SetInteractAtServer(isWidget);
            newobj->GenerateBinaryData();
          }

          switch (mapper->GetScalarMode())
          {
            case SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA:
              ((svtkWebGLPolyData*)obj)
                ->GetPolygonsFromPointData(polydata, actor, this->meshObjMaxSize);
              break;
            case SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA:
              ((svtkWebGLPolyData*)obj)
                ->GetPolygonsFromCellData(polydata, actor, this->meshObjMaxSize);
              break;
            default:
              ((svtkWebGLPolyData*)obj)
                ->GetPolygonsFromPointData(polydata, actor, this->meshObjMaxSize);
              break;
          }
        }
        obj->SetId(ss.str());
        obj->SetRendererId(static_cast<int>(rendererId));
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetTransformationMatrix(actor->GetMatrix());
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
        obj->SetIsWidget(isWidget);
        obj->SetInteractAtServer(isWidget);
        obj->GenerateBinaryData();
      }
      else if (polydata->GetOutput()->GetNumberOfLines() != 0)
      {
        ((svtkWebGLPolyData*)obj)->GetLines(polydata, actor, this->lineObjMaxSize);
        obj->SetId(ss.str());
        obj->SetRendererId(static_cast<int>(rendererId));
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetTransformationMatrix(actor->GetMatrix());
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
        obj->SetIsWidget(isWidget);
        obj->SetInteractAtServer(isWidget);
        obj->GenerateBinaryData();
      }
      else if (polydata->GetOutput()->GetNumberOfPoints() != 0)
      {
        ((svtkWebGLPolyData*)obj)->GetPoints(polydata, actor, 65534); // Wendel
        obj->SetId(ss.str());
        obj->SetRendererId(static_cast<int>(rendererId));
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetTransformationMatrix(actor->GetMatrix());
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
        obj->SetIsWidget(false);
        obj->SetInteractAtServer(false);
        obj->GenerateBinaryData();
      }

      if (polydata->GetOutput()->GetNumberOfPolys() != 0 &&
        polydata->GetOutput()->GetNumberOfLines() != 0)
      {
        obj = svtkWebGLPolyData::New();
        ((svtkWebGLPolyData*)obj)->GetLines(polydata, actor, this->lineObjMaxSize);
        ss << "1";
        obj->SetId(ss.str());
        obj->SetRendererId(static_cast<int>(rendererId));
        this->Internal->Objects.push_back(obj);
        obj->SetLayer(layer);
        obj->SetTransformationMatrix(actor->GetMatrix());
        obj->SetVisibility(actor->GetVisibility() != 0);
        obj->SetHasTransparency(actor->HasTranslucentPolygonalGeometry() != 0);
        obj->SetIsWidget(isWidget);
        obj->SetInteractAtServer(isWidget);
        obj->GenerateBinaryData();
      }

      if (polydata->GetOutput()->GetNumberOfLines() == 0 &&
        polydata->GetOutput()->GetNumberOfPolys() == 0 &&
        polydata->GetOutput()->GetNumberOfPoints() == 0)
      {
        obj->Delete();
      }
    }
    else
    {
      this->Internal->ActorTimestamp[key] = actorTime;
      std::stringstream ss;
      ss << (size_t)actor;
      for (size_t i = 0; i < this->Internal->tempObj.size(); i++)
      {
        if (this->Internal->tempObj[i]->GetId().compare(ss.str()) == 0)
        {
          svtkWebGLObject* obj = this->Internal->tempObj[i];
          this->Internal->tempObj.erase(this->Internal->tempObj.begin() + i);
          obj->SetVisibility(actor->GetVisibility() != 0);
          this->Internal->Objects.push_back(obj);
        }
      }
    }
  }
}

void svtkWebGLExporter::parseScene(
  svtkRendererCollection* renderers, const char* viewId, int parseType)
{
  if (!renderers)
    return;

  bool onlyWidget = parseType == SVTK_ONLYWIDGET;
  bool cameraOnly = onlyWidget && !this->hasWidget;

  this->SceneId = viewId ? viewId : "";
  if (cameraOnly)
  {
    this->generateRendererData(renderers, viewId);
    return;
  }

  if (onlyWidget)
  {
    for (int i = static_cast<int>(this->Internal->Objects.size()) - 1; i >= 0; i--)
    {
      svtkWebGLObject* obj = this->Internal->Objects[i];
      if (obj->InteractAtServer())
      {
        this->Internal->tempObj.push_back(obj);
        this->Internal->Objects.erase(this->Internal->Objects.begin() + i);
      }
    }
  }
  else
  {
    while (!this->Internal->Objects.empty())
    {
      this->Internal->tempObj.push_back(this->Internal->Objects.back());
      this->Internal->Objects.pop_back();
    }
  }

  this->Internal->OldActorTimestamp = this->Internal->ActorTimestamp;
  if (!onlyWidget)
    this->Internal->ActorTimestamp.clear();
  this->hasWidget = false;
  for (int i = 0; i < renderers->GetNumberOfItems(); i++)
  {
    svtkRenderer* renderer = svtkRenderer::SafeDownCast(renderers->GetItemAsObject(i));
    if (renderer->GetDraw())
      this->parseRenderer(renderer, viewId, onlyWidget, nullptr);
  }
  while (!this->Internal->tempObj.empty())
  {
    svtkWebGLObject* obj = this->Internal->tempObj.back();
    this->Internal->tempObj.pop_back();
    obj->Delete();
  }

  this->generateRendererData(renderers, viewId);
}

bool sortLayer(svtkRenderer* i, svtkRenderer* j)
{
  return (i->GetLayer() < j->GetLayer());
}

void svtkWebGLExporter::generateRendererData(
  svtkRendererCollection* renderers, const char* svtkNotUsed(viewId))
{
  std::stringstream ss;
  ss << "\"Renderers\": [";

  std::vector<svtkRenderer*> orderedList;
  orderedList.reserve(renderers->GetNumberOfItems());
  for (int i = 0; i < renderers->GetNumberOfItems(); i++)
    orderedList.push_back(svtkRenderer::SafeDownCast(renderers->GetItemAsObject(i)));
  std::sort(orderedList.begin(), orderedList.begin() + orderedList.size(), sortLayer);

  int* fullSize = nullptr;
  for (size_t i = 0; i < orderedList.size(); i++)
  {
    svtkRenderer* renderer = orderedList[i];

    if (i == 0)
    {
      fullSize = renderer->GetSize();
    }

    double cam[10];
    cam[0] = renderer->GetActiveCamera()->GetViewAngle();
    renderer->GetActiveCamera()->GetFocalPoint(&cam[1]);
    renderer->GetActiveCamera()->GetViewUp(&cam[4]);
    renderer->GetActiveCamera()->GetPosition(&cam[7]);
    int *s, *o;
    s = renderer->GetSize();
    o = renderer->GetOrigin();
    ss << "{\"layer\":" << renderer->GetLayer() << ","; // Render Layer
    if (renderer->GetLayer() == 0)                      // Render Background
    {
      double back[3];
      renderer->GetBackground(back);
      ss << "\"Background1\":[" << back[0] << "," << back[1] << "," << back[2] << "],";
      if (renderer->GetGradientBackground())
      {
        renderer->GetBackground2(back);
        ss << "\"Background2\":[" << back[0] << "," << back[1] << "," << back[2] << "],";
      }
    }
    ss << "\"LookAt\":["; // Render Camera
    for (int j = 0; j < 9; j++)
      ss << cam[j] << ",";
    ss << cam[9] << "], ";
    ss << "\"size\": [" << (float)(s[0] / (float)fullSize[0]) << ","
       << (float)(s[1] / (float)fullSize[1]) << "],"; // Render Size
    ss << "\"origin\": [" << (float)(o[0] / (float)fullSize[0]) << ","
       << (float)(o[1] / (float)fullSize[1]) << "]"; // Render Position
    ss << "}";
    if (static_cast<int>(i + 1) != renderers->GetNumberOfItems())
      ss << ", ";
  }
  ss << "]";
  this->renderersMetaData = ss.str();
}

svtkTriangleFilter* svtkWebGLExporter::GetPolyData(svtkMapper* mapper, svtkMTimeType& dataMTime)
{
  svtkDataSet* dataset = nullptr;
  svtkSmartPointer<svtkDataSet> tempDS;
  svtkDataObject* dObj = mapper->GetInputDataObject(0, 0);
  svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(dObj);
  if (cd)
  {
    dataMTime = cd->GetMTime();
    svtkCompositeDataGeometryFilter* gf = svtkCompositeDataGeometryFilter::New();
    gf->SetInputData(cd);
    gf->Update();
    tempDS = gf->GetOutput();
    gf->Delete();
    dataset = tempDS;
  }
  else
  {
    dataset = mapper->GetInput();
    dataMTime = dataset->GetMTime();
  }

  // Converting to triangles. WebGL only support triangles.
  if (this->TriangleFilter)
    this->TriangleFilter->Delete();
  this->TriangleFilter = svtkTriangleFilter::New();
  this->TriangleFilter->SetInputData(dataset);
  this->TriangleFilter->Update();
  return this->TriangleFilter;
}

/*
  Function: GenerateMetaData
  Description:
  - Generates the metadata of the scene in JSON format
  Ex.:
    { "id": ,"LookAt": ,"Background1": ,"Background2":
    "Objects": [{"id": ,"md5": ,"parts": },  {"id": ,"md5": ,"parts": }] }
*/
const char* svtkWebGLExporter::GenerateMetadata()
{
  double max = std::max(this->SceneSize[0], this->SceneSize[1]);
  max = std::max(max, this->SceneSize[2]);
  std::stringstream ss;

  ss << "{\"id\":" << this->SceneId.c_str() << ",";
  ss << "\"MaxSize\":" << max << ",";
  ss << "\"Center\":[";
  for (int i = 0; i < 2; i++)
    ss << this->CenterOfRotation[i] << ", ";
  ss << this->CenterOfRotation[2] << "],";

  ss << this->renderersMetaData << ",";

  ss << " \"Objects\":[";
  bool first = true;
  for (size_t i = 0; i < this->Internal->Objects.size(); i++)
  {
    svtkWebGLObject* obj = this->Internal->Objects[i];
    if (obj->isVisible())
    {
      if (first)
        first = false;
      else
        ss << ", ";
      ss << "{\"id\":" << obj->GetId() << ", \"md5\":\"" << obj->GetMD5() << "\""
         << ", \"parts\":" << obj->GetNumberOfParts()
         << ", \"interactAtServer\":" << obj->InteractAtServer()
         << ", \"transparency\":" << obj->HasTransparency() << ", \"layer\":" << obj->GetLayer()
         << ", \"wireframe\":" << obj->isWireframeMode() << "}";
    }
  }
  ss << "]}";

  this->Internal->LastMetaData = ss.str();
  return this->Internal->LastMetaData.c_str();
}

const char* svtkWebGLExporter::GenerateExportMetadata()
{
  double max = std::max(this->SceneSize[0], this->SceneSize[1]);
  max = std::max(max, this->SceneSize[2]);
  std::stringstream ss;

  ss << "{\"id\":" << this->SceneId << ",";
  ss << "\"MaxSize\":" << max << ",";
  ss << "\"Center\":[";
  for (int i = 0; i < 2; i++)
    ss << this->CenterOfRotation[i] << ", ";
  ss << this->CenterOfRotation[2] << "],";

  ss << this->renderersMetaData << ",";

  ss << " \"Objects\":[";
  bool first = true;
  for (size_t i = 0; i < this->Internal->Objects.size(); i++)
  {
    svtkWebGLObject* obj = this->Internal->Objects[i];
    if (obj->isVisible())
    {
      for (int j = 0; j < obj->GetNumberOfParts(); j++)
      {
        if (first)
          first = false;
        else
          ss << ", ";
        ss << "{\"id\":" << obj->GetId() << ", \"md5\":\"" << obj->GetMD5() << "\""
           << ", \"parts\":" << 1 << ", \"interactAtServer\":" << obj->InteractAtServer()
           << ", \"transparency\":" << obj->HasTransparency() << ", \"layer\":" << obj->GetLayer()
           << ", \"wireframe\":" << obj->isWireframeMode() << "}";
      }
    }
  }
  ss << "]}";

  this->Internal->LastMetaData = ss.str();
  return this->Internal->LastMetaData.c_str();
}

svtkWebGLObject* svtkWebGLExporter::GetWebGLObject(int index)
{
  return this->Internal->Objects[index];
}

int svtkWebGLExporter::GetNumberOfObjects()
{
  return static_cast<int>(this->Internal->Objects.size());
}

void svtkWebGLExporter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

const char* svtkWebGLExporter::GetId()
{
  return this->SceneId.c_str();
}

bool svtkWebGLExporter::hasChanged()
{
  for (size_t i = 0; i < this->Internal->Objects.size(); i++)
    if (this->Internal->Objects[i]->HasChanged())
      return true;
  return false;
}

void svtkWebGLExporter::exportStaticScene(
  svtkRendererCollection* renderers, int width, int height, std::string path)
{
  std::stringstream ss;
  ss << width << "," << height;
  std::string resultHTML =
    "<html><head></head><body onload='loadStaticScene();' style='margin: 0px; padding: 0px; "
    "position: absolute; overflow: hidden; top:0px; left:0px;'>";
  resultHTML += "<div id='container' onclick='consumeEvent(event);' style='margin: 0px; padding: "
                "0px; position: absolute; overflow: hidden; top:0px; left:0px;'></div></body>\n";
  resultHTML += "<script type='text/javascript'> var rendererWebGL = null;";
  resultHTML += "function reresize(event){ if (rendererWebGL != null) "
                "rendererWebGL.setSize(window.innerWidth, window.innerHeight); }";
  resultHTML += "function loadStaticScene(){ ";
  resultHTML += "  var objs=[];";
  resultHTML += "  for(i=0; i<object.length; i++){";
  resultHTML += "  objs[i] = decode64(object[i]);";
  resultHTML += "  }\n object = [];";
  resultHTML += "  rendererWebGL = new WebGLRenderer('webglRenderer-1', '');";
  resultHTML += "  rendererWebGL.init('', '');";
  resultHTML += "  rendererWebGL.bindToElementId('container');";
  resultHTML += "  //rendererWebGL.setSize(" + ss.str() + ");\n";
  resultHTML += "  rendererWebGL.setSize(window.innerWidth, window.innerHeight);";
  resultHTML += "  rendererWebGL.start(metadata, objs);";
  resultHTML += "  window.onresize = reresize;";
  resultHTML += "}\n";
  resultHTML += "function consumeEvent(event) { if (event.preventDefault) { "
                "event.preventDefault();} else { event.returnValue= false;} return false;}";

  resultHTML += "function ntos(n){ n=n.toString(16); if (n.length == 1) n='0'+n; n='%'+n; return "
                "unescape(n); }";
  resultHTML += "var END_OF_INPUT = -1; var base64Chars = new Array(";
  resultHTML += "'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','"
                "U','V','W','X',";
  resultHTML += "'Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','"
                "s','t','u','v',";
  resultHTML += "'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/');";
  resultHTML += "var base64Str; var base64Count;";
  resultHTML += "var reverseBase64Chars = new Array();";
  resultHTML +=
    "for (var i=0; i < base64Chars.length; i++){ reverseBase64Chars[base64Chars[i]] = i; }";
  resultHTML += "function readReverseBase64(){ if (!base64Str) return END_OF_INPUT;";
  resultHTML += "while (true){ if (base64Count >= base64Str.length) return END_OF_INPUT;";
  resultHTML += "var nextCharacter = base64Str.charAt(base64Count); base64Count++;";
  resultHTML +=
    "if (reverseBase64Chars[nextCharacter]){ return reverseBase64Chars[nextCharacter]; }";
  resultHTML += "if (nextCharacter == 'A') return 0; } return END_OF_INPUT; }";
  resultHTML += "function decode64(str){";
  resultHTML += "base64Str = str; base64Count = 0; var result = ''; var inBuffer = new Array(4); "
                "var done = false;";
  resultHTML += "while (!done && (inBuffer[0] = readReverseBase64()) != END_OF_INPUT";
  resultHTML += "&& (inBuffer[1] = readReverseBase64()) != END_OF_INPUT){";
  resultHTML += "inBuffer[2] = readReverseBase64();";
  resultHTML += "inBuffer[3] = readReverseBase64();";
  resultHTML += "result += ntos((((inBuffer[0] << 2) & 0xff)| inBuffer[1] >> 4));";
  resultHTML += "if (inBuffer[2] != END_OF_INPUT){";
  resultHTML += "result +=  ntos((((inBuffer[1] << 4) & 0xff)| inBuffer[2] >> 2));";
  resultHTML += "if (inBuffer[3] != END_OF_INPUT){";
  resultHTML += "result +=  ntos((((inBuffer[2] << 6)  & 0xff) | inBuffer[3]));";
  resultHTML += "} else { done = true; }";
  resultHTML += "} else { done = true; } }";
  resultHTML += "return result; }";

  svtkBase64Utilities* base64 = svtkBase64Utilities::New();

  this->parseScene(renderers, "1234567890", SVTK_PARSEALL);
  const char* metadata = this->GenerateExportMetadata();
  resultHTML += "var metadata = '" + std::string(metadata) + "';";
  resultHTML += "var object = [";
  for (int i = 0; i < this->GetNumberOfObjects(); i++)
  {
    std::string test;
    int size = 0;

    svtkWebGLObject* obj = this->GetWebGLObject(i);
    if (obj->isVisible())
    {
      for (int j = 0; j < obj->GetNumberOfParts(); j++)
      {
        unsigned char* output = new unsigned char[obj->GetBinarySize(j) * 2];
        size = base64->Encode(obj->GetBinaryData(j), obj->GetBinarySize(j), output, false);
        test = std::string((const char*)output, size);
        resultHTML += "'" + test + "',\n";
        delete[] output;
      }
    }
  }
  resultHTML += "''];";

  resultHTML += webglRenderer;
  resultHTML += glMatrix;

  resultHTML += "</script></html>";

  svtksys::ofstream file;
  file.open(path.c_str());
  file << resultHTML;
  file.close();

  base64->Delete();
}

//-----------------------------------------------------------------------------
void svtkWebGLExporter::ComputeMD5(const unsigned char* content, int size, std::string& hash)
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
