/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class  svtkVolumeTexture
 * @brief  Creates and manages the volume texture rendered by
 * svtkOpenGLGPUVolumeRayCastMapper.
 *
 * Wraps a svtkTextureObject for which it selects the appropriate format
 * (depending on the input svtkDataArray type, number of components, etc.) and
 * loads input data. The class maintains a set of members of interest to the
 * parent mapper, such as:
 *
 * * Active svtkDataArray scalar range.
 * * Volume's scale and bias (pixel transfer functions).
 * * HandleLargeDataType flag.
 * * Texture to data transformations.
 * * Block extents
 * * Block loaded bounds
 *
 * This class supports streaming the volume data in separate blocks to make it
 * fit in graphics memory (sometimes referred to as bricking). The data is split
 * into a user-defined number of blocks in such a way that a single sub-block
 * (brick) fits completely into GPU memory.  A stride is passed to OpenGL so
 * that it can access the underlying svtkDataArray adequately for each of the
 * blocks to be streamed into GPU memory (back-to-front for correct
 * composition).
 *
 * Streaming the volume as separate texture bricks certainly imposes a
 * performance trade-off but acts as a graphics memory expansion scheme for
 * devices that would not be able to render higher resoulution volumes
 * otherwise.
 *
 * @warning There are certain caveats when texture streaming is enabled, given
 * the locality constraint that rendering a single block imposes.
 *
 * - Quality might suffer near the block seams with ShadeOn() (gradient
 * computation at the boundaries needs adjustment).
 *
 * - Not all of the features supported by the mapper currently work correctly.
 * This is a list of known issues:
 *   -# Blending modes such as average and additive might compute a different
 *      value near the edges.
 *
 *  - Future work will extend the API to be able to compute an ideal number of
 *  partitions and extents based on the platform capabilities.
 *
 * @warning This is an internal class of svtkOpenGLGPUVolumeRayCastMapper. It
 * assumes there is an active OpenGL context in methods involving GL calls
 * (MakeCurrent() is expected to be called in the mapper beforehand).
 */

#ifndef svtkVolumeTexture_h
#define svtkVolumeTexture_h
#include <map>    // For ImageDataBlockMap
#include <vector> // For ImageDataBlocks

#include "svtkMatrix4x4.h" // For svtkMatrix4
#include "svtkNew.h"       // For svtkNew
#include "svtkObject.h"
#include "svtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "svtkSmartPointer.h"                 // For SmartPointer
#include "svtkTimeStamp.h"                    // For UploadTime
#include "svtkTuple.h"                        // For Size6 and Size3

class svtkDataArray;
class svtkImageData;
class svtkVolumeProperty;
class svtkRenderer;
class svtkTextureObject;
class svtkWindow;

class SVTKRENDERINGVOLUMEOPENGL2_EXPORT svtkVolumeTexture : public svtkObject
{
  typedef svtkTuple<int, 6> Size6;
  typedef svtkTuple<int, 3> Size3;

public:
  static svtkVolumeTexture* New();

  struct VolumeBlock
  {
    VolumeBlock(svtkImageData* imData, svtkTextureObject* tex, Size3 const& texSize)
    {
      // Block extent is stored in svtkImageData
      ImageData = imData;
      TextureObject = tex;
      TextureSize = texSize;
      TupleIndex = 0;

      this->Extents[0] = SVTK_INT_MAX;
      this->Extents[1] = SVTK_INT_MIN;
      this->Extents[2] = SVTK_INT_MAX;
      this->Extents[3] = SVTK_INT_MIN;
      this->Extents[4] = SVTK_INT_MAX;
      this->Extents[5] = SVTK_INT_MIN;
    }

    svtkImageData* ImageData;
    svtkTextureObject* TextureObject;
    Size3 TextureSize;
    svtkIdType TupleIndex;
    svtkNew<svtkMatrix4x4> TextureToDataset;
    svtkNew<svtkMatrix4x4> TextureToDatasetInv;

    float CellStep[3];
    double DatasetStepSize[3];

    /**
     * LoadedBounds are corrected for cell-data (if that is the case). So they
     * are not equivalent to svtkImageData::GetBounds().
     */
    double LoadedBounds[6];
    double LoadedBoundsAA[6];
    double VolumeGeometry[24];
    int Extents[6];
  };

  svtkTypeMacro(svtkVolumeTexture, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   *  Set a number of blocks per axis.
   */
  void SetPartitions(int const i, int const j, int const k);
  const Size3& GetPartitions();

  /**
   * Loads the data array into the texture in the case only a single block is
   * is defined. Does not load when the input data is divided in multiple blocks
   * (in which case they will be loaded into GPU memory by GetNextBlock()).
   * Requires an active OpenGL context.
   */
  bool LoadVolume(svtkRenderer* ren, svtkImageData* data, svtkDataArray* scalars, int const isCell,
    int const interpolation);

  /**
   * It currently only calls SetInterpolation internally. Requires an active OpenGL
   * context.
   */
  void UpdateVolume(svtkVolumeProperty* property);

  /**
   * If streaming the data array as separate blocks, sort them back to front.
   * This method does nothing if there is a single block.
   */
  void SortBlocksBackToFront(svtkRenderer* ren, svtkMatrix4x4* volumeMat);

  /**
   * Return the next volume block to be rendered and load its data.  If the
   * current block is the last one, it will return nullptr.
   */
  VolumeBlock* GetNextBlock();
  /**
   * Return the currently loaded block.
   */
  VolumeBlock* GetCurrentBlock();

  /**
   * Clean-up acquired graphics resources.
   */
  void ReleaseGraphicsResources(svtkWindow* win);

  /**
   * Get the scale and bias values given a SVTK scalar type and a finite range.
   * The scale and bias values computed using this method can be useful for
   * custom shader code. For example, when looking up color values through the
   * transfer function texture, the scalar value must be scaled and offset.
   */
  static void GetScaleAndBias(const int scalarType, float* scalarRange, float& scale, float& bias);
  svtkDataArray* GetLoadedScalars();

  bool HandleLargeDataTypes;
  float Scale[4];
  float Bias[4];
  float ScalarRange[4][2];
  float CellSpacing[3];
  int InterpolationType;
  svtkTimeStamp UploadTime;

  int IsCellData = 0;
  svtkNew<svtkMatrix4x4> CellToPointMatrix;
  float AdjustedTexMin[4];
  float AdjustedTexMax[4];

protected:
  svtkVolumeTexture();
  ~svtkVolumeTexture() override;

private:
  svtkVolumeTexture(const svtkVolumeTexture&) = delete;
  void operator=(const svtkVolumeTexture&) = delete;

  /**
   * Load an image block as defined in volBlock into GPU memory.
   * Requires an active OpenGL context.
   */
  bool LoadTexture(int const interpolation, VolumeBlock* volBlock);

  /**
   * Divide the image data in NxMxO user-defined blocks.
   */
  void SplitVolume(svtkImageData* imageData, Size3 const& part);

  void CreateBlocks(unsigned int const format, unsigned int const internalFormat, int const type);

  void AdjustExtentForCell(Size6& extent);
  Size3 ComputeBlockSize(int* extent);

  /**
   * Defines OpenGL's texture type, format and internal format based on the
   * svtkDataArray type (scalarType) and the number of array components.
   */
  void SelectTextureFormat(unsigned int& format, unsigned int& internalFormat, int& type,
    int const scalarType, int const noOfComponents);

  /**
   * Clean-up any acquired host side resources (image blocks, etc.).
   */
  void ClearBlocks();

  /**
   * Computes loaded bounds in data-coordinates.
   */
  // can be combined into one call
  void ComputeBounds(VolumeBlock* block);
  void UpdateTextureToDataMatrix(VolumeBlock* block);

  /**
   *  Compute transformation from cell texture-coordinates to point texture-coords
   *  (CTP). Cell data maps correctly to OpenGL cells, point data does not (SVTK
   *  defines points at the cell corners). To set the point data in the center of the
   *  OpenGL texels, a translation of 0.5 texels is applied, and the range is rescaled
   *  to the point range.
   *
   *  delta = TextureExtentsMax - TextureExtentsMin;
   *  min   = vec3(0.5) / delta;
   *  max   = (delta - vec3(0.5)) / delta;
   *  range = max - min
   *
   *  CTP = translation * Scale
   *  CTP = range.x,        0,        0,  min.x
   *              0,  range.y,        0,  min.y
   *              0,        0,  range.z,  min.z
   *              0,        0,        0,    1.0
   */
  void ComputeCellToPointMatrix(int extents[6]);

  //@{
  /**
   * @brief Helper functions to catch potential issues when doing GPU
   * texture allocations.
   *
   * They make use of the available OpenGL mechanisms to try to detect whether
   * a volume would not fit in the GPU (due to MAX_TEXTURE_SIZE limitations,
   * memory availability, etc.).
   */
  bool AreDimensionsValid(
    svtkTextureObject* texture, int const width, int const height, int const depth);

  bool SafeLoadTexture(svtkTextureObject* texture, int const width, int const height,
    int const depth, int numComps, int dataType, void* dataPtr);
  //@}

  void UpdateInterpolationType(int const interpolation);
  void SetInterpolation(int const interpolation);

  //----------------------------------------------------------------------------
  svtkTimeStamp UpdateTime;

  svtkSmartPointer<svtkTextureObject> Texture;
  std::vector<svtkImageData*> ImageDataBlocks;
  std::map<svtkImageData*, VolumeBlock*> ImageDataBlockMap;
  std::vector<VolumeBlock*> SortedVolumeBlocks;
  size_t CurrentBlockIdx;
  bool StreamBlocks;

  std::vector<Size3> TextureSizes;
  Size6 FullExtent;
  Size3 FullSize;
  Size3 Partitions;

  svtkDataArray* Scalars;
};

#endif // svtkVolumeTexture_h
