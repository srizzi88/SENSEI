/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGDALVectorReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGDALVectorReader
 * @brief   Read vector file formats using GDAL.
 *
 * svtkGDALVectorReader is a source object that reads vector files and uses
 * GDAL as the underlying library for the task. GDAL is required for this
 * reader. The output of the reader is a svtkMultiBlockDataSet
 *
 * This filter uses the ActiveLayer member to only load entries from the
 * specified layer (when ActiveLayer >= 0).
 *
 * @sa
 * svtkMultiBlockDataSet
 */

#ifndef svtkGDALVectorReader_h
#define svtkGDALVectorReader_h

#include "svtkIOGDALModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

#include <map>    // STL required.
#include <string> // for ivars

class SVTKIOGDAL_EXPORT svtkGDALVectorReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkGDALVectorReader* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkGDALVectorReader, svtkMultiBlockDataSetAlgorithm);

  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

  /**
   * Return number of layers.
   */
  int GetNumberOfLayers();

  /**
   * Given a index return layer type (eg point, line, polygon).
   */
  int GetLayerType(int layerIndex = 0);

  /**
   * Given a layer index return number of features (shapes).
   */
  int GetFeatureCount(int layerIndex = 0);

  /**
   * Return the active layer type (eg point, line, polygon).
   */
  int GetActiveLayerType();

  /**
   * Return the number of features in the active layer (shapes).
   */
  int GetActiveLayerFeatureCount();

  //@{
  /**
   * Set and Get the active layer.
   * If ActiveLayer is less than 0 (the default is -1), then all
   * layers are read. Otherwise, only the specified layer is read.
   */
  svtkSetMacro(ActiveLayer, int);
  svtkGetMacro(ActiveLayer, int);
  //@}

  //@{
  /**
   * Set and Get whether features are appended to a single
   * svtkPolyData. Turning the option on is useful when a shapefile has
   * a number of features which could otherwise lead to a huge
   * multiblock structure.
   */
  svtkSetMacro(AppendFeatures, int);
  svtkGetMacro(AppendFeatures, int);
  svtkBooleanMacro(AppendFeatures, int);
  //@}

  /**
   * Return projection string belonging to each layer in WKT format.
   */
  std::map<int, std::string> GetLayersProjection();

  /**
   * Return projection string belonging to a layer in WKT format.
   */
  const char* GetLayerProjection(int layerIndex);

  /**
   * Return projection string belonging to a layer in PROJ.4 format
   *
   * \note The returned string has to be deleted (via delete[]) by the
   * calling program.
   */
  const char* GetLayerProjectionAsProj4(int layerIndex);

  //@{
  /**
   * Set/get whether feature IDs should be generated.
   * Some GDAL primitives (e.g., a polygon with a hole
   * in its interior) are represented by multiple SVTK
   * cells. If you wish to identify the primitive
   * responsible for a SVTK cell, turn this on. It is
   * off by default for backwards compatibility.
   * The array of feature IDs will be the active
   * cell-data pedigree IDs.
   */
  svtkSetMacro(AddFeatureIds, int);
  svtkGetMacro(AddFeatureIds, int);
  svtkBooleanMacro(AddFeatureIds, int);
  //@}

protected:
  svtkGDALVectorReader();
  ~svtkGDALVectorReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int InitializeInternal();

  /// The name of the file that will be opened on the next call to RequestData()
  char* FileName;

  int ActiveLayer;
  int AppendFeatures;
  int AddFeatureIds;

  class Internal;

  /// Private per-file metadata
  svtkGDALVectorReader::Internal* Implementation;

  /// Global variable indicating whether the OGR library has been registered yet or not.
  static int OGRRegistered;

  /// Mapping of layer to projection.
  std::map<int, std::string> LayersProjection;

private:
  svtkGDALVectorReader(const svtkGDALVectorReader&) = delete;
  void operator=(const svtkGDALVectorReader&) = delete;
};

#endif // svtkGDALVectorReader_h
