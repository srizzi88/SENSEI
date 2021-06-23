/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkUnstructuredGridVolumeRayCastMapper
 * @brief   A software mapper for unstructured volumes
 *
 * This is a software ray caster for rendering volumes in svtkUnstructuredGrid.
 *
 * @sa
 * svtkVolumeMapper
 */

#ifndef svtkUnstructuredGridVolumeRayCastMapper_h
#define svtkUnstructuredGridVolumeRayCastMapper_h

#include "svtkRenderingVolumeModule.h" // For export macro
#include "svtkUnstructuredGridVolumeMapper.h"

class svtkDoubleArray;
class svtkIdList;
class svtkMultiThreader;
class svtkRayCastImageDisplayHelper;
class svtkRenderer;
class svtkTimerLog;
class svtkUnstructuredGridVolumeRayCastFunction;
class svtkUnstructuredGridVolumeRayCastIterator;
class svtkUnstructuredGridVolumeRayIntegrator;
class svtkVolume;

class SVTKRENDERINGVOLUME_EXPORT svtkUnstructuredGridVolumeRayCastMapper
  : public svtkUnstructuredGridVolumeMapper
{
public:
  static svtkUnstructuredGridVolumeRayCastMapper* New();
  svtkTypeMacro(svtkUnstructuredGridVolumeRayCastMapper, svtkUnstructuredGridVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Sampling distance in the XY image dimensions. Default value of 1 meaning
   * 1 ray cast per pixel. If set to 0.5, 4 rays will be cast per pixel. If
   * set to 2.0, 1 ray will be cast for every 4 (2 by 2) pixels.
   */
  svtkSetClampMacro(ImageSampleDistance, float, 0.1f, 100.0f);
  svtkGetMacro(ImageSampleDistance, float);
  //@}

  //@{
  /**
   * This is the minimum image sample distance allow when the image
   * sample distance is being automatically adjusted
   */
  svtkSetClampMacro(MinimumImageSampleDistance, float, 0.1f, 100.0f);
  svtkGetMacro(MinimumImageSampleDistance, float);
  //@}

  //@{
  /**
   * This is the maximum image sample distance allow when the image
   * sample distance is being automatically adjusted
   */
  svtkSetClampMacro(MaximumImageSampleDistance, float, 0.1f, 100.0f);
  svtkGetMacro(MaximumImageSampleDistance, float);
  //@}

  //@{
  /**
   * If AutoAdjustSampleDistances is on, the ImageSampleDistance
   * will be varied to achieve the allocated render time of this
   * prop (controlled by the desired update rate and any culling in
   * use).
   */
  svtkSetClampMacro(AutoAdjustSampleDistances, svtkTypeBool, 0, 1);
  svtkGetMacro(AutoAdjustSampleDistances, svtkTypeBool);
  svtkBooleanMacro(AutoAdjustSampleDistances, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the number of threads to use. This by default is equal to
   * the number of available processors detected.
   */
  svtkSetMacro(NumberOfThreads, int);
  svtkGetMacro(NumberOfThreads, int);
  //@}

  //@{
  /**
   * If IntermixIntersectingGeometry is turned on, the zbuffer will be
   * captured and used to limit the traversal of the rays.
   */
  svtkSetClampMacro(IntermixIntersectingGeometry, svtkTypeBool, 0, 1);
  svtkGetMacro(IntermixIntersectingGeometry, svtkTypeBool);
  svtkBooleanMacro(IntermixIntersectingGeometry, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the helper class for casting rays.
   */
  virtual void SetRayCastFunction(svtkUnstructuredGridVolumeRayCastFunction* f);
  svtkGetObjectMacro(RayCastFunction, svtkUnstructuredGridVolumeRayCastFunction);
  //@}

  //@{
  /**
   * Set/Get the helper class for integrating rays.  If set to NULL, a
   * default integrator will be assigned.
   */
  virtual void SetRayIntegrator(svtkUnstructuredGridVolumeRayIntegrator* ri);
  svtkGetObjectMacro(RayIntegrator, svtkUnstructuredGridVolumeRayIntegrator);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Initialize rendering for this volume.
   */
  void Render(svtkRenderer*, svtkVolume*) override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  svtkGetVectorMacro(ImageInUseSize, int, 2);
  svtkGetVectorMacro(ImageOrigin, int, 2);
  svtkGetVectorMacro(ImageViewportSize, int, 2);

  void CastRays(int threadID, int threadCount);

protected:
  svtkUnstructuredGridVolumeRayCastMapper();
  ~svtkUnstructuredGridVolumeRayCastMapper() override;

  float ImageSampleDistance;
  float MinimumImageSampleDistance;
  float MaximumImageSampleDistance;
  svtkTypeBool AutoAdjustSampleDistances;

  svtkMultiThreader* Threader;
  int NumberOfThreads;

  svtkRayCastImageDisplayHelper* ImageDisplayHelper;

  // This is how big the image would be if it covered the entire viewport
  int ImageViewportSize[2];

  // This is how big the allocated memory for image is. This may be bigger
  // or smaller than ImageFullSize - it will be bigger if necessary to
  // ensure a power of 2, it will be smaller if the volume only covers a
  // small region of the viewport
  int ImageMemorySize[2];

  // This is the size of subregion in ImageSize image that we are using for
  // the current image. Since ImageSize is a power of 2, there is likely
  // wasted space in it. This number will be used for things such as clearing
  // the image if necessary.
  int ImageInUseSize[2];

  // This is the location in ImageFullSize image where our ImageSize image
  // is located.
  int ImageOrigin[2];

  // This is the allocated image
  unsigned char* Image;

  float* RenderTimeTable;
  svtkVolume** RenderVolumeTable;
  svtkRenderer** RenderRendererTable;
  int RenderTableSize;
  int RenderTableEntries;

  void StoreRenderTime(svtkRenderer* ren, svtkVolume* vol, float t);
  float RetrieveRenderTime(svtkRenderer* ren, svtkVolume* vol);

  svtkTypeBool IntermixIntersectingGeometry;

  float* ZBuffer;
  int ZBufferSize[2];
  int ZBufferOrigin[2];

  // Get the ZBuffer value corresponding to location (x,y) where (x,y)
  // are indexing into the ImageInUse image. This must be converted to
  // the zbuffer image coordinates. Nearest neighbor value is returned.
  double GetZBufferValue(int x, int y);

  double GetMinimumBoundsDepth(svtkRenderer* ren, svtkVolume* vol);

  svtkUnstructuredGridVolumeRayCastFunction* RayCastFunction;
  svtkUnstructuredGridVolumeRayCastIterator** RayCastIterators;
  svtkUnstructuredGridVolumeRayIntegrator* RayIntegrator;
  svtkUnstructuredGridVolumeRayIntegrator* RealRayIntegrator;

  svtkIdList** IntersectedCellsBuffer;
  svtkDoubleArray** IntersectionLengthsBuffer;
  svtkDataArray** NearIntersectionsBuffer;
  svtkDataArray** FarIntersectionsBuffer;

  svtkVolume* CurrentVolume;
  svtkRenderer* CurrentRenderer;

  svtkDataArray* Scalars;
  int CellScalars;

private:
  svtkUnstructuredGridVolumeRayCastMapper(const svtkUnstructuredGridVolumeRayCastMapper&) = delete;
  void operator=(const svtkUnstructuredGridVolumeRayCastMapper&) = delete;
};

#endif
