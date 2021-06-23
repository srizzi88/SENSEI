/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridVolumeZSweepMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnstructuredGridVolumeZSweepMapper
 * @brief   Unstructured grid volume mapper based the ZSweep Algorithm
 *
 *
 * This is a volume mapper for unstructured grid implemented with the ZSweep
 * algorithm. This is a software projective method.
 *
 * @sa
 * svtkVolumetMapper
 *
 * @par Background:
 * The algorithm is described in the following paper:
 * Ricardo Farias, Joseph S. B. Mitchell and Claudio T. Silva.
 * ZSWEEP: An Efficient and Exact Projection Algorithm for Unstructured Volume
 * Rendering. In 2000 Volume Visualization Symposium, pages 91--99.
 * October 2000.
 * http://www.cse.ogi.edu/~csilva/papers/volvis2000.pdf
 */

#ifndef svtkUnstructuredGridVolumeZSweepMapper_h
#define svtkUnstructuredGridVolumeZSweepMapper_h

#include "svtkRenderingVolumeModule.h" // For export macro
#include "svtkUnstructuredGridVolumeMapper.h"

class svtkRenderer;
class svtkVolume;
class svtkRayCastImageDisplayHelper;
class svtkCell;
class svtkGenericCell;
class svtkIdList;
class svtkPriorityQueue;
class svtkTransform;
class svtkMatrix4x4;
class svtkVolumeProperty;
class svtkDoubleArray;
class svtkUnstructuredGridVolumeRayIntegrator;
class svtkRenderWindow;

// Internal classes
namespace svtkUnstructuredGridVolumeZSweepMapperNamespace
{
class svtkScreenEdge;
class svtkSpan;
class svtkPixelListFrame;
class svtkUseSet;
class svtkVertices;
class svtkSimpleScreenEdge;
class svtkDoubleScreenEdge;
class svtkVertexEntry;
class svtkPixelListEntryMemory;
};

class SVTKRENDERINGVOLUME_EXPORT svtkUnstructuredGridVolumeZSweepMapper
  : public svtkUnstructuredGridVolumeMapper
{
public:
  svtkTypeMacro(svtkUnstructuredGridVolumeZSweepMapper, svtkUnstructuredGridVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set MaxPixelListSize to 32.
   */
  static svtkUnstructuredGridVolumeZSweepMapper* New();

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
   * If IntermixIntersectingGeometry is turned on, the zbuffer will be
   * captured and used to limit the traversal of the rays.
   */
  svtkSetClampMacro(IntermixIntersectingGeometry, svtkTypeBool, 0, 1);
  svtkGetMacro(IntermixIntersectingGeometry, svtkTypeBool);
  svtkBooleanMacro(IntermixIntersectingGeometry, svtkTypeBool);
  //@}

  /**
   * Maximum size allowed for a pixel list. Default is 32.
   * During the rendering, if a list of pixel is full, incremental compositing
   * is performed. Even if it is a user setting, it is an advanced parameter.
   * You have to understand how the algorithm works to change this value.
   */
  int GetMaxPixelListSize();

  /**
   * Change the maximum size allowed for a pixel list. It is an advanced
   * parameter.
   * \pre positive_size: size>1
   */
  void SetMaxPixelListSize(int size);

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
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  void Render(svtkRenderer* ren, svtkVolume* vol) override;

  svtkGetVectorMacro(ImageInUseSize, int, 2);
  svtkGetVectorMacro(ImageOrigin, int, 2);
  svtkGetVectorMacro(ImageViewportSize, int, 2);

protected:
  svtkUnstructuredGridVolumeZSweepMapper();
  ~svtkUnstructuredGridVolumeZSweepMapper() override;

  /**
   * For each vertex, find the list of incident faces.
   */
  void BuildUseSets();

  /**
   * Reorder vertices `v' in increasing order in `w'. Return if the orientation
   * has changed.
   */
  int ReorderTriangle(svtkIdType v[3], svtkIdType w[3]);

  /**
   * Project and sort the vertices by z-coordinates in view space in the
   * "event list" (an heap).
   * \pre empty_list: this->EventList->GetNumberOfItems()==0
   */
  void ProjectAndSortVertices(svtkRenderer* ren, svtkVolume* vol);

  /**
   * Create an empty "pixel list" for each pixel of the screen.
   */
  void CreateAndCleanPixelList();

  /**
   * MainLoop of the Zsweep algorithm.
   * \post empty_list: this->EventList->GetNumberOfItems()==0
   */
  void MainLoop(svtkRenderWindow* renWin);

  /**
   * Do delayed compositing from back to front, stopping at zTarget for each
   * pixel inside the bounding box.
   */
  void CompositeFunction(double zTarget);

  /**
   * Convert and clamp a float color component into a unsigned char.
   */
  unsigned char ColorComponentRealToByte(float color);

  /**
   * Perform scan conversion of a triangle face.
   */
  void RasterizeFace(svtkIdType faceIds[3], int externalSide);

  /**
   * Perform scan conversion of a triangle defined by its vertices.
   * \pre ve0_exists: ve0!=0
   * \pre ve1_exists: ve1!=0
   * \pre ve2_exists: ve2!=0
   */
  void RasterizeTriangle(svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkVertexEntry* ve0,
    svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkVertexEntry* ve1,
    svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkVertexEntry* ve2, bool exitFace);

  /**
   * Perform scan conversion of an horizontal span from left ro right at line
   * y.
   * \pre left_exists: left!=0
   * \pre right_exists: right!=0
   */
  void RasterizeSpan(int y, svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkScreenEdge* left,
    svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkScreenEdge* right, bool exitFace);

  /**
   * Scan conversion of a straight line defined by endpoints v0 and v1.
   * \pre v0_exists: v0!=0
   * \pre v1_exists: v1!=0
   * \pre y_ordered v0->GetScreenY()<=v1->GetScreenY()
   */
  void RasterizeLine(svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkVertexEntry* v0,
    svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkVertexEntry* v1, bool exitFace);

  void StoreRenderTime(svtkRenderer* ren, svtkVolume* vol, float t);

  float RetrieveRenderTime(svtkRenderer* ren, svtkVolume* vol);

  /**
   * Return the value of the z-buffer at screen coordinates (x,y).
   */
  double GetZBufferValue(int x, int y);

  double GetMinimumBoundsDepth(svtkRenderer* ren, svtkVolume* vol);

  /**
   * Allocate an array of usesets of size `size' only if the current one is not
   * large enough. Otherwise clear each use set of each vertex.
   */
  void AllocateUseSet(svtkIdType size);

  /**
   * Allocate a vertex array of size `size' only if the current one is not
   * large enough.
   */
  void AllocateVertices(svtkIdType size);

  /**
   * For debugging purpose, save the pixel list frame as a dataset.
   */
  void SavePixelListFrame();

  int MaxPixelListSize;

  float ImageSampleDistance;
  float MinimumImageSampleDistance;
  float MaximumImageSampleDistance;
  svtkTypeBool AutoAdjustSampleDistances;

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

  // This is the accumulating double RGBA image
  float* RealRGBAImage;

  float* RenderTimeTable;
  svtkVolume** RenderVolumeTable;
  svtkRenderer** RenderRendererTable;
  int RenderTableSize;
  int RenderTableEntries;

  svtkTypeBool IntermixIntersectingGeometry;

  float* ZBuffer;
  int ZBufferSize[2];
  int ZBufferOrigin[2];

  svtkDataArray* Scalars;
  int CellScalars;

  // if use CellScalars, we need to keep track of the
  // values on each side of the face and figure out
  // if the face is used by two cells (twosided) or one cell.
  double FaceScalars[2];
  int FaceSide;

  svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkSpan* Span;
  svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkPixelListFrame* PixelListFrame;

  // Used by BuildUseSets().
  svtkGenericCell* Cell;

  svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkUseSet* UseSet;

  svtkPriorityQueue* EventList;
  svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkVertices* Vertices;

  svtkTransform* PerspectiveTransform;
  svtkMatrix4x4* PerspectiveMatrix;

  // Used by the main loop
  int MaxPixelListSizeReached;
  int XBounds[2];
  int YBounds[2];

  svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkSimpleScreenEdge* SimpleEdge;
  svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkDoubleScreenEdge* DoubleEdge;

  svtkUnstructuredGridVolumeRayIntegrator* RayIntegrator;
  svtkUnstructuredGridVolumeRayIntegrator* RealRayIntegrator;

  svtkTimeStamp SavedTriangleListMTime;

  // Used during compositing
  svtkDoubleArray* IntersectionLengths;
  svtkDoubleArray* NearIntersections;
  svtkDoubleArray* FarIntersections;

  // Benchmark
  svtkIdType MaxRecordedPixelListSize;

  svtkUnstructuredGridVolumeZSweepMapperNamespace::svtkPixelListEntryMemory* MemoryManager;

private:
  svtkUnstructuredGridVolumeZSweepMapper(const svtkUnstructuredGridVolumeZSweepMapper&) = delete;
  void operator=(const svtkUnstructuredGridVolumeZSweepMapper&) = delete;
};

#endif
