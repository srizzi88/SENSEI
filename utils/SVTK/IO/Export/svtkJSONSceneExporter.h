/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkJSONSceneExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkJSONSceneExporter
 * @brief   Export the content of a svtkRenderWindow into a directory with
 *          a JSON meta file describing the scene along with the http datasets
 *
 * @warning
 * This writer assume LittleEndian by default. Additional work should be done to properly
 * handle endianness.
 */

#ifndef svtkJSONSceneExporter_h
#define svtkJSONSceneExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro
#include "svtkSmartPointer.h"   // For svtkSmartPointer

#include <map>    // For member variables
#include <string> // For string parameter
#include <vector> // For member variables

class svtkActor;
class svtkDataObject;
class svtkDataSet;
class svtkPolyData;
class svtkScalarsToColors;
class svtkTexture;

class SVTKIOEXPORT_EXPORT svtkJSONSceneExporter : public svtkExporter
{
public:
  static svtkJSONSceneExporter* New();
  svtkTypeMacro(svtkJSONSceneExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of svtk data file to write.
   * This correspond to the root directory of the data to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Whether or not to write textures.
   * Textures will be written in JPEG format.
   * Default is false.
   */
  svtkSetMacro(WriteTextures, bool);
  svtkGetMacro(WriteTextures, bool);
  //@}

  //@{
  /**
   * Whether or not to write texture LODs.
   * This will write out the textures in a series of decreasing
   * resolution JPEG files, which are intended to be uploaded to the
   * web. Each file will be 1/4 the size of the previous one. The files
   * will stop being written out when one is smaller than the
   * TextureLODsBaseSize.
   * Default is false.
   */
  svtkSetMacro(WriteTextureLODs, bool);
  svtkGetMacro(WriteTextureLODs, bool);
  //@}

  //@{
  /**
   * The base size to be used for texture LODs. The texture LODs will
   * stop being written out when one is smaller than this size.
   * Default is 100 KB. Units are in bytes.
   */
  svtkSetMacro(TextureLODsBaseSize, size_t);
  svtkGetMacro(TextureLODsBaseSize, size_t);
  //@}

  //@{
  /**
   * The base URL to be used for texture LODs.
   * Default is nullptr.
   */
  svtkSetStringMacro(TextureLODsBaseUrl);
  svtkGetStringMacro(TextureLODsBaseUrl);
  //@}

  //@{
  /**
   * Whether or not to write poly LODs.
   * This will write out the poly LOD sources in a series of decreasing
   * resolution data sets, which are intended to be uploaded to the
   * web. svtkQuadricCluster is used to decrease the resolution of the
   * poly data. Each will be approximately 1/4 the size of the previous
   * one (unless certain errors occur, and then the defaults for quadric
   * clustering will be used, which will produce an unknown size). The
   * files will stop being written out when one is smaller than the
   * PolyLODsBaseSize, or if the difference in the sizes of the two
   * most recent LODs is less than 5%. The smallest LOD will be written
   * into the svtkjs file, rather than with the rest of the LODs.
   * Default is false.
   */
  svtkSetMacro(WritePolyLODs, bool);
  svtkGetMacro(WritePolyLODs, bool);
  //@}

  //@{
  /**
   * The base size to be used for poly LODs. The poly LODs will stop
   * being written out when one is smaller than this size, or if the
   * difference in the sizes of the two most recent LODs is less
   * than 5%.
   * Default is 100 KB. Units are in bytes.
   */
  svtkSetMacro(PolyLODsBaseSize, size_t);
  svtkGetMacro(PolyLODsBaseSize, size_t);
  //@}

  //@{
  /**
   * The base URL to be used for poly LODs.
   * Default is nullptr.
   */
  svtkSetStringMacro(PolyLODsBaseUrl);
  svtkGetStringMacro(PolyLODsBaseUrl);
  //@}

protected:
  svtkJSONSceneExporter();
  ~svtkJSONSceneExporter() override;

  void WriteDataObject(ostream& os, svtkDataObject* dataObject, svtkActor* actor);
  std::string ExtractRenderingSetup(svtkActor* actor);
  std::string WriteDataSet(svtkDataSet* dataset, const char* addOnMeta);
  void WriteLookupTable(const char* name, svtkScalarsToColors* lookupTable);

  void WriteData() override;

  std::string CurrentDataSetPath() const;

  std::string WriteTexture(svtkTexture* texture);
  std::string WriteTextureLODSeries(svtkTexture* texture);

  // The returned pointer is the smallest poly LOD, intended to be
  // written out in the svtkjs file.
  svtkSmartPointer<svtkPolyData> WritePolyLODSeries(svtkPolyData* polys, std::string& config);

  char* FileName;
  bool WriteTextures;
  bool WriteTextureLODs;
  size_t TextureLODsBaseSize;
  char* TextureLODsBaseUrl;
  bool WritePolyLODs;
  size_t PolyLODsBaseSize;
  char* PolyLODsBaseUrl;
  int DatasetCount;
  std::map<std::string, std::string> LookupTables;
  std::map<svtkTexture*, std::string> TextureStrings;
  std::map<svtkTexture*, std::string> TextureLODStrings;

  // Files that subclasses should zip
  std::vector<std::string> FilesToZip;

private:
  svtkJSONSceneExporter(const svtkJSONSceneExporter&) = delete;
  void operator=(const svtkJSONSceneExporter&) = delete;
};

#endif
