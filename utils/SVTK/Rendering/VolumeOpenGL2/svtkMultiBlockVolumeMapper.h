/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * \class svtkMultiBlockVolumeMapper
 * \brief Mapper to render volumes defined as svtkMultiBlockDataSet.
 *
 * svtkMultiBlockVolumeMapper renders svtkMultiBlockDataSet instances containing
 * svtkImageData blocks (all of the blocks are expected to be svtkImageData). Bounds
 * containing the full set of blocks are computed so that svtkRenderer can adjust the
 * clipping planes appropriately.
 *
 * This mapper creates an instance of svtkSmartVolumeMapper per block to which
 * it defers the actual rendering.  At render time, blocks (mappers) are sorted
 * back-to-front and each block is rendered independently.  It attempts to load all
 * of the blocks at the same time but tries to catch allocation errors in which case
 * it falls back to using a single mapper instance and reloading data for each block.
 *
 * Jittering is used to alleviate seam artifacts at the block edges due to the
 * discontinuous resolution between blocks.  Jittering is enabled by default.
 * Jittering is only supported in GPURenderMode.
 *
 */
#ifndef svtkMultiBlockVolumeMapper_h
#define svtkMultiBlockVolumeMapper_h

#include <vector> // For DataBlocks

#include "svtkRenderingVolumeOpenGL2Module.h" // For export macro
#include "svtkVolumeMapper.h"

class svtkDataObjectTree;
class svtkDataSet;
class svtkImageData;
class svtkMultiBlockDataSet;
class svtkSmartVolumeMapper;

class SVTKRENDERINGVOLUMEOPENGL2_EXPORT svtkMultiBlockVolumeMapper : public svtkVolumeMapper
{
public:
  static svtkMultiBlockVolumeMapper* New();
  svtkTypeMacro(svtkMultiBlockVolumeMapper, svtkVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   *  \brief API Superclass
   *  \sa svtkAbstractVolumeMapper
   */
  double* GetBounds() override;
  using svtkAbstractVolumeMapper::GetBounds;

  void SelectScalarArray(int arrayNum) override;
  void SelectScalarArray(char const* arrayName) override;
  void SetScalarMode(int ScalarMode) override;
  void SetArrayAccessMode(int accessMode) override;

  /**
   * Render the current dataset.
   *
   * \warning Internal method - not intended for general use, do
   * NOT use this method outside of the rendering process.
   */
  void Render(svtkRenderer* ren, svtkVolume* vol) override;

  /**
   * \warning Internal method - not intended for general use, do
   * NOT use this method outside of the rendering process.
   */
  void ReleaseGraphicsResources(svtkWindow* window) override;
  //@}

  //@{
  /**
   * VectorMode interface exposed from svtkSmartVolumeMapper.
   */
  void SetVectorMode(int mode);
  svtkGetMacro(VectorMode, int);
  void SetVectorComponent(int component);
  svtkGetMacro(VectorComponent, int);
  //@}

  //@{
  /**
   * Blending mode API from svtkVolumeMapper
   * \sa svtkVolumeMapper::SetBlendMode
   */
  void SetBlendMode(int mode) override;
  //@}

  //@{
  /**
   * Cropping API from svtkVolumeMapper
   * \sa svtkVolumeMapper::SetCropping
   */
  void SetCropping(svtkTypeBool mode) override;

  /**
   * \sa svtkVolumeMapper::SetCroppingRegionPlanes
   */
  void SetCroppingRegionPlanes(
    double arg1, double arg2, double arg3, double arg4, double arg5, double arg6) override;
  void SetCroppingRegionPlanes(const double* planes) override;

  /**
   * \sa svtkVolumeMapper::SetCroppingRegionFlags
   */
  void SetCroppingRegionFlags(int mode) override;
  //@}

  //@{
  /**
   * Forwarded to internal svtkSmartVolumeMappers used.
   * @sa svtkSmartVolumeMapper::SetRequestedRenderMode.
   */
  void SetRequestedRenderMode(int);
  //@}

protected:
  svtkMultiBlockVolumeMapper();
  ~svtkMultiBlockVolumeMapper() override;

  /**
   * Specify the type of data this mapper can handle. This mapper requires
   * svtkDataObjectTree, internally checks whether all the blocks of the data
   * set are svtkImageData.
   *
   * \sa svtkAlgorithm::FillInputPortInformation
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  /**
   * Traverse the svtkMultiBlockDataSet and create shallow copies to its valid blocks
   * (svtkImageData blocks). References are kept in a vector which is sorted back-to-front
   * on every render call.
   */
  void LoadDataSet(svtkRenderer* ren, svtkVolume* vol);

  /**
   * Creates a mapper per data block and tries to load the data. If allocating
   * fails in any of the mappers, an additional mapper instance is created
   * (FallBackMapper) and used for rendering (single mapper). The FallBackMapper
   * instance is created and used in single-mapper-mode for convenience, just to
   * keep using the Mappers vector for sorting without having to manage their
   * data.
   */
  void CreateMappers(svtkDataObjectTree* input, svtkRenderer* ren, svtkVolume* vol);

  svtkDataObjectTree* GetDataObjectTreeInput();

  /**
   * Compute the bounds enclosing all of the blocks in the dataset.
   */
  void ComputeBounds();

  /**
   * Sort loaded svtkImageData blocks back-to-front.
   */
  void SortMappers(svtkRenderer* ren, svtkMatrix4x4* volumeMat);

  void ClearMappers();

  /**
   * Create and setup a proxy rendering-mapper with the current flags.
   */
  svtkSmartVolumeMapper* CreateMapper();

  svtkMultiBlockVolumeMapper(const svtkMultiBlockVolumeMapper&) = delete;
  void operator=(const svtkMultiBlockVolumeMapper&) = delete;

  /////////////////////////////////////////////////////////////////////////////

  typedef std::vector<svtkSmartVolumeMapper*> MapperVec;
  MapperVec Mappers;
  svtkSmartVolumeMapper* FallBackMapper;

  svtkMTimeType BlockLoadingTime;
  svtkMTimeType BoundsComputeTime;

  int VectorMode;
  int VectorComponent;
  int RequestedRenderMode;
};
#endif
