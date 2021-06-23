/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebGLPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkWebGLPolyData.h"

#include "svtkActor.h"
#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataSet.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkMapper.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkScalarsToColors.h"
#include "svtkSmartPointer.h"
#include "svtkTriangleFilter.h"
#include "svtkUnsignedCharArray.h"
#include "svtkWebGLDataSet.h"
#include "svtkWebGLExporter.h"
#include "svtkWebGLObject.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

svtkStandardNewMacro(svtkWebGLPolyData);
//*****************************************************************************
class svtkWebGLPolyData::svtkInternal
{
public:
  std::vector<svtkWebGLDataSet*> Parts;
  std::map<long int, short> IndexMap;
};
//*****************************************************************************

svtkWebGLPolyData::svtkWebGLPolyData()
{
  this->webGlType = wTRIANGLES;
  this->iswidget = false;
  this->Internal = new svtkInternal();
}

svtkWebGLPolyData::~svtkWebGLPolyData()
{
  svtkWebGLDataSet* obj;
  while (!this->Internal->Parts.empty())
  {
    obj = this->Internal->Parts.back();
    this->Internal->Parts.pop_back();
    obj->Delete();
  }
  delete this->Internal;
}

void svtkWebGLPolyData::SetMesh(float* _vertices, int _numberOfVertices, int* _index,
  int _numberOfIndexes, float* _normals, unsigned char* _colors, float* _tcoords, int maxSize)
{
  this->webGlType = wTRIANGLES;

  svtkWebGLDataSet* obj;
  while (!this->Internal->Parts.empty())
  {
    obj = this->Internal->Parts.back();
    this->Internal->Parts.pop_back();
    obj->Delete();
  }

  short* index;
  int div = maxSize * 3;
  if (_numberOfVertices < div)
  {
    index = new short[_numberOfIndexes];
    for (int i = 0; i < _numberOfIndexes; i++)
      index[i] = (short)_index[i];

    obj = svtkWebGLDataSet::New();
    obj->SetVertices(_vertices, _numberOfVertices);
    obj->SetIndexes(index, _numberOfIndexes);
    obj->SetNormals(_normals);
    obj->SetColors(_colors);
    obj->SetMatrix(this->Matrix);
    this->Internal->Parts.push_back(obj);
  }
  else
  {
    int total = _numberOfIndexes;
    int curr = 0;
    int size = 0;

    while (curr < total)
    {
      if (div + curr > total)
        size = total - curr;
      else
        size = div;

      float* vertices = new float[size * 3];
      float* normals = new float[size * 3];
      unsigned char* colors = new unsigned char[size * 4];
      short* indexes = new short[size];
      float* tcoord = nullptr;
      if (_tcoords)
        tcoord = new float[size * 2];

      this->Internal->IndexMap.clear();
      int count = 0;
      for (int j = 0; j < size; j++)
      {
        int ind = _index[curr + j];
        if (this->Internal->IndexMap.find(ind) == this->Internal->IndexMap.end())
        {
          vertices[count * 3 + 0] = _vertices[ind * 3 + 0];
          vertices[count * 3 + 1] = _vertices[ind * 3 + 1];
          vertices[count * 3 + 2] = _vertices[ind * 3 + 2];

          normals[count * 3 + 0] = _normals[ind * 3 + 0];
          normals[count * 3 + 1] = _normals[ind * 3 + 1];
          normals[count * 3 + 2] = _normals[ind * 3 + 2];

          colors[count * 4 + 0] = _colors[ind * 4 + 0];
          colors[count * 4 + 1] = _colors[ind * 4 + 1];
          colors[count * 4 + 2] = _colors[ind * 4 + 2];
          colors[count * 4 + 3] = _colors[ind * 4 + 3];

          if (_tcoords)
          {
            tcoord[count * 2 + 0] = _tcoords[ind * 2 + 0];
            tcoord[count * 2 + 1] = _tcoords[ind * 2 + 1];
          }
          this->Internal->IndexMap[ind] = count;
          indexes[j] = count++;
        }
        else
        {
          indexes[j] = this->Internal->IndexMap[ind];
        }
      }
      curr += size;
      float* v = new float[count * 3];
      memcpy(v, vertices, count * 3 * sizeof(float));
      delete[] vertices;
      float* n = new float[count * 3];
      memcpy(n, normals, count * 3 * sizeof(float));
      delete[] normals;
      unsigned char* c = new unsigned char[count * 4];
      memcpy(c, colors, count * 4);
      delete[] colors;
      obj = svtkWebGLDataSet::New();
      obj->SetVertices(v, count);
      obj->SetIndexes(indexes, size);
      obj->SetNormals(n);
      obj->SetColors(c);
      if (_tcoords)
      {
        float* tc = new float[count * 2];
        memcpy(tc, tcoord, count * 2 * sizeof(float));
        delete[] tcoord;
        obj->SetTCoords(tc);
      }
      obj->SetMatrix(this->Matrix);
      this->Internal->Parts.push_back(obj);
    }

    delete[] _vertices;
    delete[] _index;
    delete[] _normals;
    delete[] _colors;
    delete[] _tcoords;
  }
}

void svtkWebGLPolyData::SetLine(float* _points, int _numberOfPoints, int* _index, int _numberOfIndex,
  unsigned char* _colors, int maxSize)
{
  this->webGlType = wLINES;

  svtkWebGLDataSet* obj;
  while (!this->Internal->Parts.empty())
  {
    obj = this->Internal->Parts.back();
    this->Internal->Parts.pop_back();
    obj->Delete();
  }

  short* index;
  int div = maxSize * 2;
  if (_numberOfPoints < div)
  {
    index = new short[_numberOfIndex];
    for (int i = 0; i < _numberOfIndex; i++)
      index[i] = (short)((unsigned int)_index[i]);
    obj = svtkWebGLDataSet::New();
    obj->SetPoints(_points, _numberOfPoints);
    obj->SetIndexes(index, _numberOfIndex);
    obj->SetColors(_colors);
    obj->SetMatrix(this->Matrix);
    this->Internal->Parts.push_back(obj);
  }
  else
  {
    int total = _numberOfIndex;
    int curr = 0;
    int size = 0;

    while (curr < total)
    {
      if (div + curr > total)
        size = total - curr;
      else
        size = div;

      float* points = new float[size * 3];
      unsigned char* colors = new unsigned char[size * 4];
      short* indexes = new short[size];

      for (int j = 0; j < size; j++)
      {
        indexes[j] = j;

        points[j * 3 + 0] = _points[_index[curr + j] * 3 + 0];
        points[j * 3 + 1] = _points[_index[curr + j] * 3 + 1];
        points[j * 3 + 2] = _points[_index[curr + j] * 3 + 2];

        colors[j * 4 + 0] = _colors[_index[curr + j] * 4 + 0];
        colors[j * 4 + 1] = _colors[_index[curr + j] * 4 + 1];
        colors[j * 4 + 2] = _colors[_index[curr + j] * 4 + 2];
        colors[j * 4 + 3] = _colors[_index[curr + j] * 4 + 3];
      }
      curr += size;
      obj = svtkWebGLDataSet::New();
      obj->SetPoints(points, size);
      obj->SetIndexes(indexes, size);
      obj->SetColors(colors);
      obj->SetMatrix(this->Matrix);
      this->Internal->Parts.push_back(obj);
    }
    delete[] _points;
    delete[] _index;
    delete[] _colors;
  }
}

void svtkWebGLPolyData::SetTransformationMatrix(svtkMatrix4x4* m)
{
  this->Superclass::SetTransformationMatrix(m);
  for (size_t i = 0; i < this->Internal->Parts.size(); i++)
  {
    this->Internal->Parts[i]->SetMatrix(this->Matrix);
  }
}

unsigned char* svtkWebGLPolyData::GetBinaryData(int part)
{
  this->hasChanged = false;
  svtkWebGLDataSet* obj = this->Internal->Parts[part];
  return obj->GetBinaryData();
}

int svtkWebGLPolyData::GetBinarySize(int part)
{
  svtkWebGLDataSet* obj = this->Internal->Parts[part];
  return obj->GetBinarySize();
}

void svtkWebGLPolyData::GenerateBinaryData()
{
  svtkWebGLDataSet* obj;
  this->hasChanged = false;
  std::stringstream ss;
  for (size_t i = 0; i < this->Internal->Parts.size(); i++)
  {
    obj = this->Internal->Parts[i];
    obj->GenerateBinaryData();
    ss << obj->GetMD5();
  }
  if (!this->Internal->Parts.empty())
  {
    std::string localMD5;
    svtkWebGLExporter::ComputeMD5(
      (const unsigned char*)ss.str().c_str(), static_cast<int>(ss.str().size()), localMD5);
    this->hasChanged = this->MD5.compare(localMD5) != 0;
    this->MD5 = localMD5;
  }
  else
    cout << "Warning: GenerateBinaryData() @ svtkWebGLObject: This isn\'t supposed to happen.";
}

int svtkWebGLPolyData::GetNumberOfParts()
{
  return static_cast<int>(this->Internal->Parts.size());
}

void svtkWebGLPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkWebGLPolyData::GetLinesFromPolygon(
  svtkMapper* mapper, svtkActor* actor, int lineMaxSize, double* edgeColor)
{
  svtkWebGLPolyData* object = this;
  svtkDataSet* dataset = nullptr;
  svtkSmartPointer<svtkDataSet> tempDS;
  svtkDataObject* dObj = mapper->GetInputDataObject(0, 0);
  svtkCompositeDataSet* cd = svtkCompositeDataSet::SafeDownCast(dObj);
  if (cd)
  {
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
  }

  int np = 0;
  int size = 0;
  for (int i = 0; i < dataset->GetNumberOfCells(); i++)
    size += dataset->GetCell(i)->GetNumberOfPoints();

  float* points = new float[size * 3];
  unsigned char* color = new unsigned char[size * 4];
  int* index = new int[size * 2];
  double* point;
  int pos = 0;

  svtkScalarsToColors* table = mapper->GetLookupTable();
  svtkDataArray* array;
  if (mapper->GetScalarMode() == SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
  {
    svtkCellData* celldata = dataset->GetCellData();
    if (actor->GetMapper()->GetArrayAccessMode() == SVTK_GET_ARRAY_BY_ID)
      array = celldata->GetArray(actor->GetMapper()->GetArrayId());
    else
      array = celldata->GetArray(actor->GetMapper()->GetArrayName());
  }
  else
  {
    svtkPointData* pointdata = dataset->GetPointData();
    if (actor->GetMapper()->GetArrayAccessMode() == SVTK_GET_ARRAY_BY_ID)
      array = pointdata->GetArray(actor->GetMapper()->GetArrayId());
    else
      array = pointdata->GetArray(actor->GetMapper()->GetArrayName());
  }

  int colorComponent = table->GetVectorComponent();
  int numberOfComponents = 0;
  if (array != nullptr)
    numberOfComponents = array->GetNumberOfComponents();
  int mode = table->GetVectorMode();
  double mag = 0, rgb[3];
  int curr = 0;
  for (int i = 0; i < dataset->GetNumberOfCells(); i++)
  {
    svtkCell* cell = dataset->GetCell(i);
    int b = pos;
    np = dataset->GetCell(i)->GetNumberOfPoints();
    for (int j = 0; j < np; j++)
    {
      point = cell->GetPoints()->GetPoint(j);
      points[curr * 3 + j * 3 + 0] = point[0];
      points[curr * 3 + j * 3 + 1] = point[1];
      points[curr * 3 + j * 3 + 2] = point[2];

      index[curr * 2 + j * 2 + 0] = pos++;
      index[curr * 2 + j * 2 + 1] = pos;
      if (j == np - 1)
        index[curr * 2 + j * 2 + 1] = b;

      svtkIdType pointId = cell->GetPointIds()->GetId(j);
      if (numberOfComponents == 0)
      {
        actor->GetProperty()->GetColor(rgb);
      }
      else
      {
        switch (mode)
        {
          case svtkScalarsToColors::MAGNITUDE:
            mag = 0;
            for (int w = 0; w < numberOfComponents; w++)
              mag +=
                (double)array->GetComponent(pointId, w) * (double)array->GetComponent(pointId, w);
            mag = sqrt(mag);
            table->GetColor(mag, &rgb[0]);
            break;
          case svtkScalarsToColors::COMPONENT:
            mag = array->GetComponent(pointId, colorComponent);
            table->GetColor(mag, &rgb[0]);
            break;
          case svtkScalarsToColors::RGBCOLORS:
            array->GetTuple(pointId, &rgb[0]);
            break;
        }
      }
      if (edgeColor != nullptr)
        memcpy(rgb, edgeColor, sizeof(double) * 3);
      color[curr * 4 + j * 4 + 0] = (unsigned char)((int)(rgb[0] * 255));
      color[curr * 4 + j * 4 + 1] = (unsigned char)((int)(rgb[1] * 255));
      color[curr * 4 + j * 4 + 2] = (unsigned char)((int)(rgb[2] * 255));
      color[curr * 4 + j * 4 + 3] = (unsigned char)255;
    }
    curr += np;
  }
  object->SetLine(points, size, index, size * 2, color, lineMaxSize);
}

void svtkWebGLPolyData::GetLines(svtkTriangleFilter* polydata, svtkActor* actor, int lineMaxSize)
{
  svtkWebGLPolyData* object = this;
  svtkCellArray* lines = polydata->GetOutput(0)->GetLines();

  // Index
  // Array of 3 Values. [#number of index, i1, i2]
  // Discarting the first value
  svtkDataArray* conn = lines->GetConnectivityArray();
  const svtkIdType connSize = conn->GetNumberOfValues();
  int* index = new int[static_cast<size_t>(connSize)];
  for (svtkIdType i = 0; i < connSize; ++i)
  {
    index[i] = static_cast<int>(conn->GetComponent(i, 0));
  }
  // Point
  double point[3];
  float* points = new float[polydata->GetOutput(0)->GetNumberOfPoints() * 3];
  for (int i = 0; i < polydata->GetOutput(0)->GetNumberOfPoints(); i++)
  {
    polydata->GetOutput(0)->GetPoint(i, point);
    points[i * 3 + 0] = point[0];
    points[i * 3 + 1] = point[1];
    points[i * 3 + 2] = point[2];
  }
  // Colors
  unsigned char* color = new unsigned char[polydata->GetOutput(0)->GetNumberOfPoints() * 4];
  this->GetColorsFromPolyData(color, polydata->GetOutput(0), actor);

  object->SetLine(points, polydata->GetOutput(0)->GetNumberOfPoints(), index,
    static_cast<int>(connSize), color, lineMaxSize);
}

void svtkWebGLPolyData::SetPoints(
  float* points, int numberOfPoints, unsigned char* colors, int maxSize)
{
  this->webGlType = wPOINTS;

  // Delete Old Objects
  svtkWebGLDataSet* obj;
  while (!this->Internal->Parts.empty())
  {
    obj = this->Internal->Parts.back();
    this->Internal->Parts.pop_back();
    obj->Delete();
  }

  // Create new objs
  int numObjs = (numberOfPoints / maxSize) + 1;
  int offset = 0;
  int size = 0;
  for (int i = 0; i < numObjs; i++)
  {
    size = numberOfPoints - offset;
    if (size > maxSize)
      size = maxSize;

    float* _points = new float[size * 3];
    unsigned char* _colors = new unsigned char[size * 4];
    memcpy(_points, &points[offset * 3], size * 3 * sizeof(float));
    memcpy(_colors, &colors[offset * 4], size * 4 * sizeof(unsigned char));

    obj = svtkWebGLDataSet::New();
    obj->SetPoints(_points, size);
    obj->SetColors(_colors);
    obj->SetType(wPOINTS);
    obj->SetMatrix(this->Matrix);
    this->Internal->Parts.push_back(obj);

    offset += size;
  }

  delete[] points;
  delete[] colors;
}

void svtkWebGLPolyData::GetPoints(svtkTriangleFilter* polydata, svtkActor* actor, int maxSize)
{
  svtkWebGLPolyData* object = this;

  // Points
  double point[3];
  float* points = new float[polydata->GetOutput(0)->GetNumberOfPoints() * 3];
  for (int i = 0; i < polydata->GetOutput(0)->GetNumberOfPoints(); i++)
  {
    polydata->GetOutput(0)->GetPoint(i, point);
    points[i * 3 + 0] = point[0];
    points[i * 3 + 1] = point[1];
    points[i * 3 + 2] = point[2];
  }
  // Colors
  unsigned char* colors = new unsigned char[polydata->GetOutput(0)->GetNumberOfPoints() * 4];
  this->GetColorsFromPolyData(colors, polydata->GetOutput(0), actor);

  object->SetPoints(points, polydata->GetOutput(0)->GetNumberOfPoints(), colors, maxSize);
}

void svtkWebGLPolyData::GetColorsFromPolyData(
  unsigned char* color, svtkPolyData* polydata, svtkActor* actor)
{
  int celldata;
  svtkDataArray* array = svtkAbstractMapper::GetScalars(polydata, actor->GetMapper()->GetScalarMode(),
    actor->GetMapper()->GetArrayAccessMode(), actor->GetMapper()->GetArrayId(),
    actor->GetMapper()->GetArrayName(), celldata);
  if (actor->GetMapper()->GetScalarVisibility() && array != nullptr)
  {
    svtkScalarsToColors* table = actor->GetMapper()->GetLookupTable();

    svtkUnsignedCharArray* cor =
      table->MapScalars(array, table->GetVectorMode(), table->GetVectorComponent());
    memcpy(color, cor->GetPointer(0), polydata->GetNumberOfPoints() * 4);
    cor->Delete();
  }
  else
  {
    for (int i = 0; i < polydata->GetNumberOfPoints(); i++)
    {
      color[i * 4 + 0] = (unsigned char)255;
      color[i * 4 + 1] = (unsigned char)255;
      color[i * 4 + 2] = (unsigned char)255;
      color[i * 4 + 3] = (unsigned char)255;
    }
  }
}

void svtkWebGLPolyData::GetPolygonsFromPointData(
  svtkTriangleFilter* polydata, svtkActor* actor, int maxSize)
{
  svtkWebGLPolyData* object = this;

  svtkPolyDataNormals* polynormals = svtkPolyDataNormals::New();
  polynormals->SetInputConnection(polydata->GetOutputPort(0));
  polynormals->Update();

  svtkPolyData* data = polynormals->GetOutput();

  svtkCellArray* poly = data->GetPolys();
  svtkPointData* point = data->GetPointData();
  svtkNew<svtkIdTypeArray> ndata;
  poly->ExportLegacyFormat(ndata);
  svtkDataSetAttributes* attr = (svtkDataSetAttributes*)point;

  // Vertices
  float* vertices = new float[data->GetNumberOfPoints() * 3];
  for (int i = 0; i < data->GetNumberOfPoints() * 3; i++)
    vertices[i] = data->GetPoint(i / 3)[i % 3];
  // Index
  // ndata contain 4 values for the normal: [number of values per index, index[3]]
  // We don't need the first value
  int* indexes = new int[ndata->GetSize() * 3 / 4];
  for (int i = 0; i < ndata->GetSize(); i++)
    if (i % 4 != 0)
      indexes[i * 3 / 4] = ndata->GetValue(i);
  // Normal
  float* normal = new float[attr->GetNormals()->GetSize()];
  for (int i = 0; i < attr->GetNormals()->GetSize(); i++)
    normal[i] = attr->GetNormals()->GetComponent(0, i);
  // Colors
  unsigned char* color = new unsigned char[data->GetNumberOfPoints() * 4];
  this->GetColorsFromPointData(color, point, data, actor);
  // TCoord
  float* tcoord = nullptr;
  if (attr->GetTCoords())
  {
    tcoord = new float[attr->GetTCoords()->GetSize()];
    for (int i = 0; i < attr->GetTCoords()->GetSize(); i++)
      tcoord[i] = attr->GetTCoords()->GetComponent(0, i);
  }

  object->SetMesh(vertices, data->GetNumberOfPoints(), indexes, ndata->GetSize() * 3 / 4, normal,
    color, tcoord, maxSize);
  polynormals->Delete();
}

void svtkWebGLPolyData::GetPolygonsFromCellData(
  svtkTriangleFilter* polydata, svtkActor* actor, int maxSize)
{
  svtkWebGLPolyData* object = this;

  svtkPolyDataNormals* polynormals = svtkPolyDataNormals::New();
  polynormals->SetInputConnection(polydata->GetOutputPort(0));
  polynormals->Update();

  svtkPolyData* data = polynormals->GetOutput();
  svtkCellData* celldata = data->GetCellData();

  svtkDataArray* array;
  if (actor->GetMapper()->GetArrayAccessMode() == SVTK_GET_ARRAY_BY_ID)
    array = celldata->GetArray(actor->GetMapper()->GetArrayId());
  else
    array = celldata->GetArray(actor->GetMapper()->GetArrayName());
  svtkScalarsToColors* table = actor->GetMapper()->GetLookupTable();
  int colorComponent = table->GetVectorComponent();
  int mode = table->GetVectorMode();

  float* vertices = new float[data->GetNumberOfCells() * 3 * 3];
  float* normals = new float[data->GetNumberOfCells() * 3 * 3];
  unsigned char* colors = new unsigned char[data->GetNumberOfCells() * 3 * 4];
  int* indexes = new int[data->GetNumberOfCells() * 3 * 3];

  svtkGenericCell* cell = svtkGenericCell::New();
  double tuple[3], normal[3], color[3];
  color[0] = 1.0;
  color[1] = 1.0;
  color[2] = 1.0;
  svtkPoints* points;
  int aux;
  double mag, alpha = 1.0;
  int numberOfComponents = 0;
  if (array)
    numberOfComponents = array->GetNumberOfComponents();
  else
    mode = -1;
  for (int i = 0; i < data->GetNumberOfCells(); i++)
  {
    data->GetCell(i, cell);
    points = cell->GetPoints();

    // getColors
    alpha = 1.0;
    switch (mode)
    {
      case -1:
        actor->GetProperty()->GetColor(color);
        alpha = actor->GetProperty()->GetOpacity();
        break;
      case svtkScalarsToColors::MAGNITUDE:
        mag = 0;
        for (int w = 0; w < numberOfComponents; w++)
          mag += (double)array->GetComponent(i, w) * (double)array->GetComponent(i, w);
        mag = sqrt(mag);
        table->GetColor(mag, &color[0]);
        alpha = table->GetOpacity(mag);
        break;
      case svtkScalarsToColors::COMPONENT:
        mag = array->GetComponent(i, colorComponent);
        table->GetColor(mag, &color[0]);
        alpha = table->GetOpacity(mag);
        break;
      case svtkScalarsToColors::RGBCOLORS:
        array->GetTuple(i, &color[0]);
        break;
    }
    // getNormals
    celldata->GetNormals()->GetTuple(i, &normal[0]);
    for (int j = 0; j < 3; j++)
    {
      aux = i * 9 + j * 3;
      // Normals
      normals[aux + 0] = normal[0];
      normals[aux + 1] = normal[1];
      normals[aux + 2] = normal[2];
      // getVertices
      points->GetPoint(j, &tuple[0]);
      vertices[aux + 0] = tuple[0];
      vertices[aux + 1] = tuple[1];
      vertices[aux + 2] = tuple[2];
      // Colors
      colors[4 * (3 * i + j) + 0] = (unsigned char)((int)(color[0] * 255));
      colors[4 * (3 * i + j) + 1] = (unsigned char)((int)(color[1] * 255));
      colors[4 * (3 * i + j) + 2] = (unsigned char)((int)(color[2] * 255));
      colors[4 * (3 * i + j) + 3] = (unsigned char)((int)(alpha * 255));
      // getIndexes
      indexes[aux + 0] = aux + 0;
      indexes[aux + 1] = aux + 1;
      indexes[aux + 2] = aux + 2;
    }
  }
  object->SetMesh(vertices, data->GetNumberOfCells() * 3, indexes, data->GetNumberOfCells() * 3,
    normals, colors, nullptr, maxSize);
  cell->Delete();
  polynormals->Delete();
}

void svtkWebGLPolyData::GetColorsFromPointData(
  unsigned char* color, svtkPointData* pointdata, svtkPolyData* polydata, svtkActor* actor)
{
  svtkDataSetAttributes* attr = (svtkDataSetAttributes*)pointdata;

  int colorSize = attr->GetNormals()->GetSize() * 4 / 3;

  svtkDataArray* array;
  if (actor->GetMapper()->GetArrayAccessMode() == SVTK_GET_ARRAY_BY_ID)
    array = pointdata->GetArray(actor->GetMapper()->GetArrayId());
  else
    array = pointdata->GetArray(actor->GetMapper()->GetArrayName());

  if (array && actor->GetMapper()->GetScalarVisibility() &&
    actor->GetMapper()->GetArrayName() != nullptr && actor->GetMapper()->GetArrayName()[0] != '\0')
  {
    svtkScalarsToColors* table = actor->GetMapper()->GetLookupTable();
    int colorComponent = table->GetVectorComponent(),
        numberOfComponents = array->GetNumberOfComponents();
    int mode = table->GetVectorMode();
    double mag = 0, rgb[3];
    double alpha = 1.0;

    if (numberOfComponents == 1 && mode == svtkScalarsToColors::MAGNITUDE)
    {
      mode = svtkScalarsToColors::COMPONENT;
      colorComponent = 0;
    }
    for (int i = 0; i < colorSize / 4; i++)
    {
      switch (mode)
      {
        case svtkScalarsToColors::MAGNITUDE:
          mag = 0;
          for (int w = 0; w < numberOfComponents; w++)
            mag += (double)array->GetComponent(i, w) * (double)array->GetComponent(i, w);
          mag = sqrt(mag);
          table->GetColor(mag, &rgb[0]);
          alpha = table->GetOpacity(mag);
          break;
        case svtkScalarsToColors::COMPONENT:
          mag = array->GetComponent(i, colorComponent);
          table->GetColor(mag, &rgb[0]);
          alpha = table->GetOpacity(mag);
          break;
        case svtkScalarsToColors::RGBCOLORS:
          array->GetTuple(i, &rgb[0]);
          alpha = actor->GetProperty()->GetOpacity();
          break;
      }
      color[i * 4 + 0] = (unsigned char)((int)(rgb[0] * 255));
      color[i * 4 + 1] = (unsigned char)((int)(rgb[1] * 255));
      color[i * 4 + 2] = (unsigned char)((int)(rgb[2] * 255));
      color[i * 4 + 3] = (unsigned char)((int)(alpha * 255));
    }
  }
  else
  {
    double rgb[3];
    double alpha = 0;
    int celldata;
    array = svtkAbstractMapper::GetScalars(polydata, actor->GetMapper()->GetScalarMode(),
      actor->GetMapper()->GetArrayAccessMode(), actor->GetMapper()->GetArrayId(),
      actor->GetMapper()->GetArrayName(), celldata);
    if (actor->GetMapper()->GetScalarVisibility() &&
      (actor->GetMapper()->GetColorMode() == SVTK_COLOR_MODE_DEFAULT ||
        actor->GetMapper()->GetColorMode() == SVTK_COLOR_MODE_DIRECT_SCALARS) &&
      array)
    {
      svtkScalarsToColors* table = actor->GetMapper()->GetLookupTable();
      svtkUnsignedCharArray* cor =
        table->MapScalars(array, actor->GetMapper()->GetColorMode(), table->GetVectorComponent());
      memcpy(color, cor->GetPointer(0), polydata->GetNumberOfPoints() * 4);
      cor->Delete();
    }
    else
    {
      actor->GetProperty()->GetColor(rgb);
      alpha = actor->GetProperty()->GetOpacity();
      for (int i = 0; i < colorSize / 4; i++)
      {
        color[i * 4 + 0] = (unsigned char)((int)(rgb[0] * 255));
        color[i * 4 + 1] = (unsigned char)((int)(rgb[1] * 255));
        color[i * 4 + 2] = (unsigned char)((int)(rgb[2] * 255));
        color[i * 4 + 3] = (unsigned char)((int)(alpha * 255));
      }
    }
  }
}
