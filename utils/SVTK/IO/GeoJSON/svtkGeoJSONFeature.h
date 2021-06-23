/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoJSONFeature.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGeoJSONFeature
 * @brief   Represents GeoJSON feature geometry & properties
 *
 * This class is used by the svtkGeoJSONReader when parsing GeoJSON input.
 * It is not intended to be instantiated by applications directly.
 */

#ifndef svtkGeoJSONFeature_h
#define svtkGeoJSONFeature_h

// SVTK Includes
#include "svtkDataObject.h"
#include "svtkIOGeoJSONModule.h" // For export macro
#include "svtk_jsoncpp.h"        // For json parser

class svtkPolyData;

// Currently implemented geoJSON compatible Geometries
#define GeoJSON_POINT "Point"
#define GeoJSON_MULTI_POINT "MultiPoint"
#define GeoJSON_LINE_STRING "LineString"
#define GeoJSON_MULTI_LINE_STRING "MultiLineString"
#define GeoJSON_POLYGON "Polygon"
#define GeoJSON_MULTI_POLYGON "MultiPolygon"
#define GeoJSON_GEOMETRY_COLLECTION "GeometryCollection"

class SVTKIOGEOJSON_EXPORT svtkGeoJSONFeature : public svtkDataObject
{
public:
  static svtkGeoJSONFeature* New();
  virtual void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkGeoJSONFeature, svtkDataObject);

  //@{
  /**
   * Set/get option to generate the border outlining each polygon,
   * so that resulting cells are svtkPolyLine instead of svtkPolygon.
   * The default is off
   */
  svtkSetMacro(OutlinePolygons, bool);
  svtkGetMacro(OutlinePolygons, bool);
  svtkBooleanMacro(OutlinePolygons, bool);
  //@}

  /**
   * Extract the geometry corresponding to the geoJSON feature stored at root
   * Assign any feature properties passed as cell data
   */
  void ExtractGeoJSONFeature(const Json::Value& root, svtkPolyData* outputData);

protected:
  svtkGeoJSONFeature();
  ~svtkGeoJSONFeature() override;

  /**
   * Json::Value featureRoot corresponds to the root of the geoJSON feature
   * from which the geometry and properties are to be extracted
   */
  Json::Value featureRoot;

  /**
   * Id of current GeoJSON feature being parsed
   */
  char* FeatureId;

  /**
   * Set/get option to generate the border outlining each polygon,
   * so that the output cells are polyine data.
   */
  bool OutlinePolygons;

  /**
   * Extract geoJSON geometry into svtkPolyData *
   */
  void ExtractGeoJSONFeatureGeometry(const Json::Value& root, svtkPolyData* outputData);

  //@{
  /**
   * In extractXXXX() Extract geoJSON geometries XXXX into outputData
   */
  svtkPolyData* ExtractPoint(const Json::Value& coordinates, svtkPolyData* outputData);
  svtkPolyData* ExtractLineString(const Json::Value& coordinates, svtkPolyData* outputData);
  svtkPolyData* ExtractPolygon(const Json::Value& coordinates, svtkPolyData* outputData);
  //@}

  //@{
  /**
   * extractMultiXXXX extracts an array of geometries XXXX into the outputData
   */
  svtkPolyData* ExtractMultiPoint(const Json::Value& coordinates, svtkPolyData* outputData);
  svtkPolyData* ExtractMultiLineString(const Json::Value& coordinates, svtkPolyData* outputData);
  svtkPolyData* ExtractMultiPolygon(const Json::Value& coordinates, svtkPolyData* outputData);
  //@}

  //@{
  /**
   * Check if the root contains corresponding appropriate geometry in the
   * Jsoncpp root
   */
  bool IsPoint(const Json::Value& root);
  bool IsMultiPoint(const Json::Value& root);
  bool IsLineString(const Json::Value& root);      // To Do.
  bool IsMultiLineString(const Json::Value& root); // To Do.
  bool IsPolygon(const Json::Value& root);         // To Do.
  bool IsMultiPolygon(const Json::Value& root);    // To Do.
  //@}

  /**
   * Point[] from its JSON equivalent
   */
  bool CreatePoint(const Json::Value& coordinates, double point[3]);

  void InsertFeatureProperties(svtkPolyData* outputData);

private:
  svtkGeoJSONFeature(const svtkGeoJSONFeature&) = delete;
  void operator=(const svtkGeoJSONFeature&) = delete;
};

#endif // svtkGeoJSONFeature_h
