/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtk3DSImporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtk3DSImporter
 * @brief   imports 3D Studio files.
 *
 * svtk3DSImporter imports 3D Studio files into svtk.
 *
 * @sa
 * svtkImporter
 */

#ifndef svtk3DSImporter_h
#define svtk3DSImporter_h

#include "svtk3DS.h"            // Needed for all the 3DS structures
#include "svtkIOImportModule.h" // For export macro
#include "svtkImporter.h"

class svtkPolyData;

class SVTKIOIMPORT_EXPORT svtk3DSImporter : public svtkImporter
{
public:
  static svtk3DSImporter* New();

  svtkTypeMacro(svtk3DSImporter, svtkImporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set/Get the computation of normals. If on, imported geometry will
   * be run through svtkPolyDataNormals.
   */
  svtkSetMacro(ComputeNormals, svtkTypeBool);
  svtkGetMacro(ComputeNormals, svtkTypeBool);
  svtkBooleanMacro(ComputeNormals, svtkTypeBool);
  //@}

  /**
   * Get a printable string describing the outputs
   */
  std::string GetOutputsDescription() override;

  /**
   * Return the file pointer to the open file.
   */
  FILE* GetFileFD() { return this->FileFD; }

  svtk3DSOmniLight* OmniList;
  svtk3DSSpotLight* SpotLightList;
  svtk3DSCamera* CameraList;
  svtk3DSMesh* MeshList;
  svtk3DSMaterial* MaterialList;
  svtk3DSMatProp* MatPropList;

protected:
  svtk3DSImporter();
  ~svtk3DSImporter() override;

  int ImportBegin() override;
  void ImportEnd() override;
  void ImportActors(svtkRenderer* renderer) override;
  void ImportCameras(svtkRenderer* renderer) override;
  void ImportLights(svtkRenderer* renderer) override;
  void ImportProperties(svtkRenderer* renderer) override;
  svtkPolyData* GeneratePolyData(svtk3DSMesh* meshPtr);
  int Read3DS();

  char* FileName;
  FILE* FileFD;
  svtkTypeBool ComputeNormals;

private:
  svtk3DSImporter(const svtk3DSImporter&) = delete;
  void operator=(const svtk3DSImporter&) = delete;
};

#endif
