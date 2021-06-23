/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGLTFReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkGLTFReader
 * @brief   Read a GLTF file.
 *
 * svtkGLTFReader is a concrete subclass of svtkMultiBlockDataSetAlgorithm that reads glTF 2.0 files.
 *
 * The GL Transmission Format (glTF) is an API-neutral runtime asset delivery format.
 * A glTF asset is represented by:
 * - A JSON-formatted file (.gltf) containing a full scene description: node hierarchy, materials,
 *   cameras, as well as descriptor information for meshes, animations, and other constructs
 * - Binary files (.bin) containing geometry and animation data, and other buffer-based data
 * - Image files (.jpg, .png) for textures
 *
 * This reader currently outputs a svtkMultiBlockDataSet containing geometry information
 * for the current selected scene, with animations, skins and morph targets applied, unless
 * configured not to (see ApplyDeformationsToGeometry).
 *
 * It is possible to get information about available scenes and animations by using the
 * corresponding accessors.
 * To use animations, first call SetFramerate with a non-zero value,
 * then use EnableAnimation or DisableAnimation to configure which animations you would like to
 * apply to the geometry.
 * Finally, use UPDATE_TIME_STEPS to choose which frame to apply.
 * If ApplyDeformationsToGeometry is set to true, the reader will apply the deformations, otherwise,
 * animation transformation information will be saved to the dataset's FieldData.
 *
 * Materials are currently not supported in this reader. If you would like to display materials,
 * please try using svtkGLTFImporter.
 * You could also use svtkGLTFReader::GetGLTFTexture, to access the image data that was loaded from
 * the glTF 2.0 document.
 *
 * This reader only supports assets that use the 2.x version of the glTF specification.
 *
 * For the full glTF specification, see:
 * https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * Note: array sizes should not exceed INT_MAX
 *
 * @sa
 * svtkMultiBlockDataSetAlgorithm
 * svtkGLTFImporter
 */

#ifndef svtkGLTFReader_h
#define svtkGLTFReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"
#include "svtkSmartPointer.h" // For SmartPointer

#include <string> // For std::string
#include <vector> // For std::vector

class svtkDataArraySelection;
class svtkFieldData;
class svtkGLTFDocumentLoader;
class svtkImageData;
class svtkStringArray;

class SVTKIOGEOMETRY_EXPORT svtkGLTFReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkGLTFReader* New();
  svtkTypeMacro(svtkGLTFReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Materials are not directly applied to this reader's output.
   * Use GetGLTFTexture to access a specific texture's image data, and the indices present in the
   * output dataset's field data to create svtkTextures and apply them to the geometry.
   */
  struct GLTFTexture
  {
    svtkSmartPointer<svtkImageData> Image;
    unsigned short MinFilterValue;
    unsigned short MaxFilterValue;
    unsigned short WrapSValue;
    unsigned short WrapTValue;
  };

  svtkIdType GetNumberOfTextures();
  GLTFTexture GetGLTFTexture(svtkIdType textureIndex);
  //@}

  //@{
  /**
   * Set/Get the name of the file from which to read points.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * The model's skinning transforms are computed and added to the different svtkPolyData objects'
   * field data.
   * If this flag is set to true, the reader will apply those skinning transforms to the model's
   * geometry.
   */
  void SetApplyDeformationsToGeometry(bool flag);
  svtkGetMacro(ApplyDeformationsToGeometry, bool);
  svtkBooleanMacro(ApplyDeformationsToGeometry, bool);
  //@}

  //@{
  /**
   * glTF models can contain multiple animations, with various names and duration. glTF does not
   * specify however any runtime behavior (order of playing, auto-start, loops, mapping of
   * timelines, etc), which is why no animation is enabled by default.
   * These accessors expose metadata information about a model's available animations.
   */
  svtkGetMacro(NumberOfAnimations, svtkIdType);
  std::string GetAnimationName(svtkIdType animationIndex);
  float GetAnimationDuration(svtkIdType animationIndex);
  //@}

  //@{
  /**
   * Enable/Disable an animation. The reader will apply all enabled animations to the model's
   * transformations, at the specified time step. Use UPDATE_TIME_STEP to select which frame should
   * be applied.
   */
  void EnableAnimation(svtkIdType animationIndex);
  void DisableAnimation(svtkIdType animationIndex);
  bool IsAnimationEnabled(svtkIdType animationIndex);
  //@}

  //@{
  /**
   * glTF models can contain multiple scene descriptions.
   * These accessors expose metadata information about a model's available scenes.
   */
  std::string GetSceneName(svtkIdType sceneIndex);
  svtkGetMacro(NumberOfScenes, svtkIdType);
  //@}

  //@{
  /**
   * Get/Set the scene to be used by the reader
   */
  svtkGetMacro(CurrentScene, svtkIdType);
  svtkSetMacro(CurrentScene, svtkIdType);
  void SetScene(const std::string& scene);
  //@}

  //@{
  /**
   * Get/Set the rate at which animations will be sampled:
   * the glTF format does not have the concept of static timesteps.
   * TimeSteps are generated, during the REQUEST_INFORMATION pass,
   * as linearly interpolated time values between 0s and the animations' maximum durations,
   * sampled at the specified frame rate.
   * Use the TIME_STEPS information key to obtain integer indices to each of these steps.
   */
  svtkGetMacro(FrameRate, unsigned int);
  svtkSetMacro(FrameRate, unsigned int);
  //@}

  /**
   * Get a list all scenes names as a svtkStringArray, with duplicate names numbered and empty names
   * replaced by a generic name. All names are guaranteed to be unique, and their index in the array
   * matches the glTF document's scene indices.
   */
  svtkStringArray* GetAllSceneNames();

  /**
   * Get the svtkDataArraySelection object to enable/disable animations.
   */
  svtkDataArraySelection* GetAnimationSelection();

protected:
  svtkGLTFReader();
  ~svtkGLTFReader() override;

  svtkSmartPointer<svtkGLTFDocumentLoader> Loader;

  svtkSmartPointer<svtkMultiBlockDataSet> OutputDataSet;

  std::vector<GLTFTexture> Textures;

  /**
   * Create and store GLTFTexture struct for each image present in the model.
   */
  void StoreTextureData();

  char* FileName = nullptr;

  svtkIdType CurrentScene = 0;
  unsigned int FrameRate = 60;
  svtkIdType NumberOfAnimations = 0;
  svtkIdType NumberOfScenes = 0;

  bool IsModelLoaded = false;
  bool IsMetaDataLoaded = false;

  bool ApplyDeformationsToGeometry = true;

  svtkSmartPointer<svtkStringArray> SceneNames;

  svtkSmartPointer<svtkDataArraySelection> PreviousAnimationSelection;
  svtkSmartPointer<svtkDataArraySelection> AnimationSelection;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Create the SceneNames array, generate unique identifiers for each scene based on their glTF
   * name, then fill the SceneNames array with the generated identifiers.
   */
  void CreateSceneNamesArray();

  /**
   * Fill the AnimationSelection svtkDataArraySelection with animation names. Names are adapted from
   * the glTF document to ensure that they are unique and non-empty.
   */
  void CreateAnimationSelection();

private:
  svtkGLTFReader(const svtkGLTFReader&) = delete;
  void operator=(const svtkGLTFReader&) = delete;
};

#endif
