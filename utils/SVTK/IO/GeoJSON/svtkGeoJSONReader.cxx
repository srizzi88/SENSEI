/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoJSONReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGeoJSONReader.h"

// SVTK Includes
#include "svtkAbstractArray.h"
#include "svtkBitArray.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkGeoJSONFeature.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkStringArray.h"
#include "svtkTriangleFilter.h"
#include "svtk_jsoncpp.h"
#include "svtksys/FStream.hxx"

// C++ includes
#include <fstream>
#include <iostream>
#include <sstream>

svtkStandardNewMacro(svtkGeoJSONReader);

//----------------------------------------------------------------------------
class svtkGeoJSONReader::GeoJSONReaderInternal
{
public:
  typedef struct
  {
    std::string Name;
    svtkVariant Value;
  } GeoJSONProperty;

  // List of property names to read. Property value is used the default
  std::vector<GeoJSONProperty> PropertySpecs;

  // Parse the Json Value corresponding to the root of the geoJSON data from the file
  void ParseRoot(const Json::Value& root, svtkPolyData* output, bool outlinePolygons,
    const char* serializedPropertiesArrayName);

  // Verify if file exists and can be read by the parser
  // If exists, parse into Jsoncpp data structure
  int CanParseFile(const char* filename, Json::Value& root);

  // Verify if string can be read by the parser
  // If exists, parse into Jsoncpp data structure
  int CanParseString(char* input, Json::Value& root);

  // Extract property values from json node
  void ParseFeatureProperties(const Json::Value& propertiesNode,
    std::vector<GeoJSONProperty>& properties, const char* serializedPropertiesArrayName);

  void InsertFeatureProperties(
    svtkPolyData* polyData, const std::vector<GeoJSONProperty>& featureProperties);
};

//----------------------------------------------------------------------------
void svtkGeoJSONReader::GeoJSONReaderInternal::ParseRoot(const Json::Value& root,
  svtkPolyData* output, bool outlinePolygons, const char* serializedPropertiesArrayName)
{
  // Initialize geometry containers
  svtkNew<svtkPoints> points;
  points->SetDataTypeToDouble();
  output->SetPoints(points);
  svtkNew<svtkCellArray> verts;
  output->SetVerts(verts);
  svtkNew<svtkCellArray> lines;
  output->SetLines(lines);
  svtkNew<svtkCellArray> polys;
  output->SetPolys(polys);

  // Initialize feature-id array
  svtkStringArray* featureIdArray = svtkStringArray::New();
  featureIdArray->SetName("feature-id");
  output->GetCellData()->AddArray(featureIdArray);
  featureIdArray->Delete();

  // Initialize properties arrays
  if (serializedPropertiesArrayName)
  {
    svtkStringArray* propertiesArray = svtkStringArray::New();
    propertiesArray->SetName(serializedPropertiesArrayName);
    output->GetCellData()->AddArray(propertiesArray);
    propertiesArray->Delete();
  }

  svtkAbstractArray* array;
  std::vector<GeoJSONProperty>::iterator iter = this->PropertySpecs.begin();
  for (; iter != this->PropertySpecs.end(); ++iter)
  {
    array = nullptr;
    switch (iter->Value.GetType())
    {
      case SVTK_BIT:
        array = svtkBitArray::New();
        break;

      case SVTK_INT:
        array = svtkIntArray::New();
        break;

      case SVTK_DOUBLE:
        array = svtkDoubleArray::New();
        break;

      case SVTK_STRING:
        array = svtkStringArray::New();
        break;

      default:
        svtkGenericWarningMacro("unexpected data type " << iter->Value.GetType());
        break;
    }

    // Skip if array not created for some reason
    if (!array)
    {
      continue;
    }

    array->SetName(iter->Name.c_str());
    output->GetCellData()->AddArray(array);
    array->Delete();
  }

  // Check type
  Json::Value rootType = root["type"];
  if (rootType.isNull())
  {
    svtkGenericWarningMacro(<< "ParseRoot: Missing type node");
    return;
  }

  // Parse features
  Json::Value rootFeatures;
  std::string strRootType = rootType.asString();
  std::vector<GeoJSONProperty> properties;
  if ("FeatureCollection" == strRootType)
  {
    rootFeatures = root["features"];
    if (rootFeatures.isNull())
    {
      svtkGenericWarningMacro(<< "ParseRoot: Missing \"features\" node");
      return;
    }

    if (!rootFeatures.isArray())
    {
      svtkGenericWarningMacro(<< "ParseRoot: features node is not an array");
      return;
    }

    GeoJSONProperty property;
    for (Json::Value::ArrayIndex i = 0; i < rootFeatures.size(); i++)
    {
      // Append extracted geometry to existing outputData
      Json::Value featureNode = rootFeatures[i];
      Json::Value propertiesNode = featureNode["properties"];
      this->ParseFeatureProperties(propertiesNode, properties, serializedPropertiesArrayName);
      svtkNew<svtkGeoJSONFeature> feature;
      feature->SetOutlinePolygons(outlinePolygons);
      feature->ExtractGeoJSONFeature(featureNode, output);
      this->InsertFeatureProperties(output, properties);
    }
  }
  else if ("Feature" == strRootType)
  {
    // Process single feature
    this->ParseFeatureProperties(root, properties, serializedPropertiesArrayName);
    svtkNew<svtkGeoJSONFeature> feature;
    feature->SetOutlinePolygons(outlinePolygons);

    // Next call adds (exactly) one cell to the polydata
    feature->ExtractGeoJSONFeature(root, output);
    // Next call adds (exactly) one tuple to the polydata's cell data
    this->InsertFeatureProperties(output, properties);
  }
  else
  {
    svtkGenericWarningMacro(<< "ParseRoot: do not support root type \"" << strRootType << "\"");
  }
}

//----------------------------------------------------------------------------
int svtkGeoJSONReader::GeoJSONReaderInternal::CanParseFile(const char* filename, Json::Value& root)
{
  if (!filename)
  {
    svtkGenericWarningMacro(<< "Input filename not specified");
    return SVTK_ERROR;
  }

  svtksys::ifstream file;
  file.open(filename);

  if (!file.is_open())
  {
    svtkGenericWarningMacro(<< "Unable to Open File " << filename);
    return SVTK_ERROR;
  }

  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;

  std::string formattedErrors;

  // parse the entire geoJSON data into the Json::Value root
  bool parsedSuccess = parseFromStream(builder, file, &root, &formattedErrors);

  if (!parsedSuccess)
  {
    // Report failures and their locations in the document
    svtkGenericWarningMacro(<< "Failed to parse JSON" << endl << formattedErrors);
    return SVTK_ERROR;
  }

  return SVTK_OK;
}

//----------------------------------------------------------------------------
int svtkGeoJSONReader::GeoJSONReaderInternal::CanParseString(char* input, Json::Value& root)
{
  if (!input)
  {
    svtkGenericWarningMacro(<< "Input string is empty");
    return SVTK_ERROR;
  }

  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;

  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

  std::string formattedErrors;

  // parse the entire geoJSON data into the Json::Value root
  bool parsedSuccess = reader->parse(input, input + strlen(input), &root, &formattedErrors);

  if (!parsedSuccess)
  {
    // Report failures and their locations in the document
    svtkGenericWarningMacro(<< "Failed to parse JSON" << endl << formattedErrors);
    return SVTK_ERROR;
  }

  return SVTK_OK;
}

//----------------------------------------------------------------------------
void svtkGeoJSONReader::GeoJSONReaderInternal::ParseFeatureProperties(
  const Json::Value& propertiesNode, std::vector<GeoJSONProperty>& featureProperties,
  const char* serializedPropertiesArrayName)
{
  featureProperties.clear();

  GeoJSONProperty spec;
  GeoJSONProperty property;
  std::vector<GeoJSONProperty>::iterator iter = this->PropertySpecs.begin();
  for (; iter != this->PropertySpecs.end(); ++iter)
  {
    spec = *iter;
    property.Name = spec.Name;

    Json::Value propertyNode = propertiesNode[spec.Name];
    if (propertyNode.isNull())
    {
      property.Value = spec.Value;
      featureProperties.push_back(property);
      continue;
    }

    // (else)
    switch (spec.Value.GetType())
    {
      case SVTK_BIT:
        property.Value = svtkVariant(propertyNode.asBool());
        break;

      case SVTK_DOUBLE:
        property.Value = svtkVariant(propertyNode.asDouble());
        break;

      case SVTK_INT:
        property.Value = svtkVariant(propertyNode.asInt());
        break;

      case SVTK_STRING:
        property.Value = svtkVariant(propertyNode.asString());
        break;
    }

    featureProperties.push_back(property);
  }

  // Add GeoJSON string if enabled
  if (serializedPropertiesArrayName)
  {
    property.Name = serializedPropertiesArrayName;

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    std::stringstream stream;
    writer->write(propertiesNode, &stream);
    std::string propString = stream.str();

    if (!propString.empty() && *propString.rbegin() == '\n')
    {
      propString.resize(propString.size() - 1);
    }
    property.Value = svtkVariant(propString);
    featureProperties.push_back(property);
  }
}

//----------------------------------------------------------------------------
void svtkGeoJSONReader::GeoJSONReaderInternal::InsertFeatureProperties(
  svtkPolyData* polyData, const std::vector<GeoJSONProperty>& featureProperties)
{
  std::vector<GeoJSONProperty>::const_iterator iter = featureProperties.begin();
  for (; iter != featureProperties.end(); ++iter)
  {
    std::string name = iter->Name;
    svtkVariant value = iter->Value;

    svtkAbstractArray* array = polyData->GetCellData()->GetAbstractArray(name.c_str());
    switch (array->GetDataType())
    {
      case SVTK_BIT:
        svtkArrayDownCast<svtkBitArray>(array)->InsertNextValue(value.ToChar());
        break;

      case SVTK_DOUBLE:
        svtkArrayDownCast<svtkDoubleArray>(array)->InsertNextValue(value.ToDouble());
        break;

      case SVTK_INT:
        svtkArrayDownCast<svtkIntArray>(array)->InsertNextValue(value.ToInt());
        break;

      case SVTK_STRING:
        svtkArrayDownCast<svtkStringArray>(array)->InsertNextValue(value.ToString());
        break;
    }
  }
}

//----------------------------------------------------------------------------
svtkGeoJSONReader::svtkGeoJSONReader()
{
  this->FileName = nullptr;
  this->StringInput = nullptr;
  this->StringInputMode = false;
  this->TriangulatePolygons = false;
  this->OutlinePolygons = false;
  this->SerializedPropertiesArrayName = nullptr;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Internal = new GeoJSONReaderInternal;
}

//----------------------------------------------------------------------------
svtkGeoJSONReader::~svtkGeoJSONReader()
{
  delete[] FileName;
  delete[] StringInput;
  delete Internal;
}

//----------------------------------------------------------------------------
void svtkGeoJSONReader::AddFeatureProperty(const char* name, svtkVariant& typeAndDefaultValue)
{
  GeoJSONReaderInternal::GeoJSONProperty property;

  // Traverse internal list checking if name already used
  std::vector<GeoJSONReaderInternal::GeoJSONProperty>::iterator iter =
    this->Internal->PropertySpecs.begin();
  for (; iter != this->Internal->PropertySpecs.end(); ++iter)
  {
    if (iter->Name == name)
    {
      svtkGenericWarningMacro(<< "Overwriting property spec for name " << name);
      property.Name = name;
      property.Value = typeAndDefaultValue;
      *iter = property;
      break;
    }
  }

  // If not found, add to list
  if (iter == this->Internal->PropertySpecs.end())
  {
    property.Name = name;
    property.Value = typeAndDefaultValue;
    this->Internal->PropertySpecs.push_back(property);
    svtkDebugMacro(<< "Added feature property " << property.Name);
  }
}

//----------------------------------------------------------------------------
int svtkGeoJSONReader::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(request), svtkInformationVector* outputVector)
{
  // Get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the output
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Parse either string input of file, depending on mode
  Json::Value root;
  int parseResult = 0;
  if (this->StringInputMode)
  {
    parseResult = this->Internal->CanParseString(this->StringInput, root);
  }
  else
  {
    parseResult = this->Internal->CanParseFile(this->FileName, root);
  }

  if (parseResult != SVTK_OK)
  {
    return SVTK_ERROR;
  }

  // If parsed successfully into Json, then convert it
  // into appropriate svtkPolyData
  if (root.isObject())
  {
    this->Internal->ParseRoot(
      root, output, this->OutlinePolygons, this->SerializedPropertiesArrayName);

    // Convert Concave Polygons to convex polygons using triangulation
    if (output->GetNumberOfPolys() && this->TriangulatePolygons)
    {
      svtkNew<svtkTriangleFilter> filter;
      filter->SetInputData(output);
      filter->Update();

      output->ShallowCopy(filter->GetOutput());
    }
  }
  return SVTK_OK;
}

//----------------------------------------------------------------------------
void svtkGeoJSONReader::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "svtkGeoJSONReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
