/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGeoJSONReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGeoJSONReader
 * @brief   Convert Geo JSON format to svtkPolyData
 *
 * Outputs a svtkPolyData from the input
 * Geo JSON Data (http://www.geojson.org)
 */

#ifndef svtkGeoJSONReader_h
#define svtkGeoJSONReader_h

// SVTK Includes
#include "svtkIOGeoJSONModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPolyData;

class SVTKIOGEOJSON_EXPORT svtkGeoJSONReader : public svtkPolyDataAlgorithm
{
public:
  static svtkGeoJSONReader* New();
  svtkTypeMacro(svtkGeoJSONReader, svtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Accessor for name of the file that will be opened on WriteData
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * String used as data input (instead of file) when StringInputMode is enabled
   */
  svtkSetStringMacro(StringInput);
  svtkGetStringMacro(StringInput);
  //@}

  //@{
  /**
   * Set/get whether to use StringInput instead of reading input from file
   * The default is off
   */
  svtkSetMacro(StringInputMode, bool);
  svtkGetMacro(StringInputMode, bool);
  svtkBooleanMacro(StringInputMode, bool);
  //@}

  //@{
  /**
   * Set/get whether to convert all output polygons to triangles.
   * Note that if OutinePolygons mode is on, then no output polygons
   * are generated, and in that case, this option is not relevant.
   * The default is off.
   */
  svtkSetMacro(TriangulatePolygons, bool);
  svtkGetMacro(TriangulatePolygons, bool);
  svtkBooleanMacro(TriangulatePolygons, bool);
  //@}

  //@{
  /**
   * Set/get option to generate the border outlining each polygon,
   * so that the output cells for polygons are svtkPolyLine instances.
   * The default is off.
   */
  svtkSetMacro(OutlinePolygons, bool);
  svtkGetMacro(OutlinePolygons, bool);
  svtkBooleanMacro(OutlinePolygons, bool);
  //@}

  //@{
  /**
   * Set/get name of data array for serialized GeoJSON "properties" node.
   * If specified, data will be stored as svtkCellData/svtkStringArray.
   */
  svtkSetStringMacro(SerializedPropertiesArrayName);
  svtkGetStringMacro(SerializedPropertiesArrayName);
  //@}

  /**
   * Specify feature property to read in with geometry objects
   * Note that defaultValue specifies both type & value
   */
  void AddFeatureProperty(const char* name, svtkVariant& typeAndDefaultValue);

protected:
  svtkGeoJSONReader();
  ~svtkGeoJSONReader() override;

  //@{
  /**
   * Core implementation of the
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  char* FileName;
  char* StringInput;
  bool StringInputMode;
  bool TriangulatePolygons;
  bool OutlinePolygons;
  char* SerializedPropertiesArrayName;
  //@}

private:
  class GeoJSONReaderInternal;
  GeoJSONReaderInternal* Internal;

  svtkGeoJSONReader(const svtkGeoJSONReader&) = delete;
  void operator=(const svtkGeoJSONReader&) = delete;
};

#endif // svtkGeoJSONReader_h
