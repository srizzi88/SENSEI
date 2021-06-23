/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCityGMLReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCityGMLReader
 * @brief   read CityGML data file
 *
 */

#ifndef svtkCityGMLReader_h
#define svtkCityGMLReader_h

#include "svtkIOCityGMLModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

/**
 * @class   svtkCityGMLReader
 * @brief   reads CityGML files
 *
 * svtkCityGMLReader is a reader for CityGML .gml files. The output is
 * a multiblock dataset. We read objects at level of detail (LOD)
 * specified (default is 3).

 * The leafs of the multiblock dataset (which are polygonal datasets)
 * have a field array with one element called "gml_id" which
 * corresponds to the gml:id for gml:TriangulatedSurface,
 * gml:MultiSurface or gml:CompositeSurface in the CityGML file. If
 * the poly dataset has a texture, we specify this with a point array
 * called "tcoords" and a field array with one element called
 * "texture_uri" containing the path to the texture file. If the poly
 * dataset has a app::X3DMaterial we store two fields arrays with 3
 * components and 1 tuple: "diffuse_color" and "specular_color" and
 * one field array with 1 component and 1 tuple: "transparency".

 * Top level children of the multiblock dataset have a field array
 * with one element called "element" which contains the CityGML
 * element name for example: dem:ReliefFeature, wtr:WaterBody,
 * grp::CityObjectGroup (forest), veg:SolitaryVegetationObject,
 * brid:Bridge, run:Tunel, tran:Railway, tran:Road, bldg:Building,
 * gen:GenericCityObject, luse:LandUse. These nodes also have a gml_id field array.
*/
class SVTKIOCITYGML_EXPORT svtkCityGMLReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkCityGMLReader* New();
  svtkTypeMacro(svtkCityGMLReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of the CityGML data file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the level of detail (LOD) to read. Valid values are from 0 (least detailed)
   * through 4 (most detailed), default value is 3.
   */
  svtkSetClampMacro(LOD, int, 0, 4);
  svtkGetMacro(LOD, int);
  //@}

  //@{
  /**
   * Certain input files use app:transparency as opacity. Set this field true
   * to show that correctly. The default is false.
   */
  svtkSetMacro(UseTransparencyAsOpacity, int);
  svtkGetMacro(UseTransparencyAsOpacity, int);
  svtkBooleanMacro(UseTransparencyAsOpacity, int);
  //@}

  //@{
  /**
   * Number of buildings read from the file.
   * Default is numeric_limits<int>::max().
   */
  svtkSetMacro(NumberOfBuildings, int);
  svtkGetMacro(NumberOfBuildings, int);
  //@}

protected:
  svtkCityGMLReader();
  ~svtkCityGMLReader() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;
  int LOD;
  int UseTransparencyAsOpacity;
  int NumberOfBuildings;

private:
  svtkCityGMLReader(const svtkCityGMLReader&) = delete;
  void operator=(const svtkCityGMLReader&) = delete;

  class Implementation;
  Implementation* Impl;
};

#endif
