/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGLTFImporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkGLTFImporter
 * @brief   Import a GLTF file.
 *
 * svtkGLTFImporter is a concrete subclass of svtkImporter that reads glTF 2.0
 * files.
 *
 * The GL Transmission Format (glTF) is an API-neutral runtime asset delivery format.
 * A glTF asset is represented by:
 * - A JSON-formatted file (.gltf) containing a full scene description: node hierarchy, materials,
 *   cameras, as well as descriptor information for meshes, animations, and other constructs
 * - Binary files (.bin) containing geometry and animation data, and other buffer-based data
 * - Image files (.jpg, .png) for textures
 *
 * This importer supports all physically-based rendering material features, with the exception of
 * alpha masking and mirrored texture wrapping, which are not supported.
 *
 *
 * This importer does not support materials that use multiple
 * sets of texture coordinates. Only the first set will be used in this case.
 *
 * This importer does not support animations, morphing and skinning. If you would like to use
 * animations, morphing or skinning, please consider using svtkGLTFReader.
 *
 * This importer only supports assets that use the 2.x version of the glTF specification.
 *
 * For the full glTF specification, see:
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * Note: array sizes should not exceed INT_MAX
 *
 * Supported extensions:
 * - KHR_lights_punctual :
 *   The importer supports the KHR_lights_punctual extension except for this feature:
 *   - SVTK does not support changing the falloff of the cone with innerConeAngle and outerConeAngle.
 *     The importer uses outerConeAngle and ignores innerConeAngle as specified for this situation.
 *
 * @sa
 * svtkImporter
 * svtkGLTFReader
 */

#ifndef svtkGLTFImporter_h
#define svtkGLTFImporter_h

#include "svtkIOImportModule.h" // For export macro
#include "svtkImporter.h"
#include "svtkSmartPointer.h" // For SmartPointer

#include <map>    // For map
#include <vector> // For vector

class svtkCamera;
class svtkGLTFDocumentLoader;
class svtkTexture;

class SVTKIOIMPORT_EXPORT svtkGLTFImporter : public svtkImporter
{
public:
  static svtkGLTFImporter* New();

  svtkTypeMacro(svtkGLTFImporter, svtkImporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the name of the file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  /**
   * glTF defines multiple camera objects, but no default behavior for which camera should be
   * used. The importer will by default apply the asset's first camera. This accessor lets you use
   * the asset's other cameras.
   */
  svtkSmartPointer<svtkCamera> GetCamera(unsigned int id);

  /**
   * Get the total number of cameras
   */
  size_t GetNumberOfCameras();

  /**
   * Get a printable string describing all outputs
   */
  std::string GetOutputsDescription() override { return this->OutputsDescription; };

protected:
  svtkGLTFImporter() = default;
  ~svtkGLTFImporter() override;

  int ImportBegin() override;
  void ImportActors(svtkRenderer* renderer) override;
  void ImportCameras(svtkRenderer* renderer) override;
  void ImportLights(svtkRenderer* renderer) override;

  char* FileName = nullptr;

  std::vector<svtkSmartPointer<svtkCamera> > Cameras;
  std::map<int, svtkSmartPointer<svtkTexture> > Textures;
  svtkSmartPointer<svtkGLTFDocumentLoader> Loader;
  std::string OutputsDescription;

private:
  svtkGLTFImporter(const svtkGLTFImporter&) = delete;
  void operator=(const svtkGLTFImporter&) = delete;
};

#endif
