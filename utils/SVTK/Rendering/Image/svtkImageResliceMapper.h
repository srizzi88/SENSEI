/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageResliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageResliceMapper
 * @brief   map a slice of a svtkImageData to the screen
 *
 * svtkImageResliceMapper will cut a 3D image with an abitrary slice plane
 * and draw the results on the screen.  The slice can be set to automatically
 * follow the camera, so that the camera controls the slicing.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * svtkImageSlice svtkImageProperty svtkImageSliceMapper
 */

#ifndef svtkImageResliceMapper_h
#define svtkImageResliceMapper_h

#include "svtkImageMapper3D.h"
#include "svtkRenderingImageModule.h" // For export macro

class svtkImageSliceMapper;
class svtkRenderer;
class svtkRenderWindow;
class svtkCamera;
class svtkLookupTable;
class svtkImageSlice;
class svtkImageData;
class svtkImageResliceToColors;
class svtkMatrix4x4;
class svtkAbstractImageInterpolator;

class SVTKRENDERINGIMAGE_EXPORT svtkImageResliceMapper : public svtkImageMapper3D
{
public:
  static svtkImageResliceMapper* New();
  svtkTypeMacro(svtkImageResliceMapper, svtkImageMapper3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the slice that will be used to cut through the image.
   * This slice should be in world coordinates, rather than
   * data coordinates.  Use SliceFacesCamera and SliceAtFocalPoint
   * if you want the slice to automatically follow the camera.
   */
  virtual void SetSlicePlane(svtkPlane* plane);

  //@{
  /**
   * When using SliceAtFocalPoint, this causes the slicing to occur at
   * the closest slice to the focal point, instead of the default behavior
   * where a new slice is interpolated between the original slices.  This
   * flag is ignored if the slicing is oblique to the original slices.
   */
  svtkSetMacro(JumpToNearestSlice, svtkTypeBool);
  svtkBooleanMacro(JumpToNearestSlice, svtkTypeBool);
  svtkGetMacro(JumpToNearestSlice, svtkTypeBool);
  //@}

  //@{
  /**
   * The slab thickness, for thick slicing (default: zero)
   */
  svtkSetMacro(SlabThickness, double);
  svtkGetMacro(SlabThickness, double);
  //@}

  //@{
  /**
   * The slab type, for thick slicing (default: Mean).
   * The resulting view is a parallel projection through the volume.  This
   * method can be used to generate a facsimile of a digitally-reconstructed
   * radiograph or a minimum-intensity projection as long as perspective
   * geometry is not required.  Note that the Sum mode provides an output
   * with units of intensity times distance, while all other modes provide
   * an output with units of intensity.
   */
  svtkSetClampMacro(SlabType, int, SVTK_IMAGE_SLAB_MIN, SVTK_IMAGE_SLAB_SUM);
  svtkGetMacro(SlabType, int);
  void SetSlabTypeToMin() { this->SetSlabType(SVTK_IMAGE_SLAB_MIN); }
  void SetSlabTypeToMax() { this->SetSlabType(SVTK_IMAGE_SLAB_MAX); }
  void SetSlabTypeToMean() { this->SetSlabType(SVTK_IMAGE_SLAB_MEAN); }
  void SetSlabTypeToSum() { this->SetSlabType(SVTK_IMAGE_SLAB_SUM); }
  virtual const char* GetSlabTypeAsString();
  //@}

  //@{
  /**
   * Set the number of slab samples to use as a factor of the number
   * of input slices within the slab thickness.  The default value
   * is 2, but 1 will increase speed with very little loss of quality.
   */
  svtkSetClampMacro(SlabSampleFactor, int, 1, 2);
  svtkGetMacro(SlabSampleFactor, int);
  //@}

  //@{
  /**
   * Set the reslice sample frequency as in relation to the input image
   * sample frequency.  The default value is 1, but higher values can be
   * used to improve the results.  This is cheaper than turning on
   * ResampleToScreenPixels.
   */
  svtkSetClampMacro(ImageSampleFactor, int, 1, 16);
  svtkGetMacro(ImageSampleFactor, int);
  //@}

  //@{
  /**
   * Automatically reduce the rendering quality for greater speed
   * when doing an interactive render.  This is on by default.
   */
  svtkSetMacro(AutoAdjustImageQuality, svtkTypeBool);
  svtkBooleanMacro(AutoAdjustImageQuality, svtkTypeBool);
  svtkGetMacro(AutoAdjustImageQuality, svtkTypeBool);
  //@}

  //@{
  /**
   * Resample the image directly to the screen pixels, instead of
   * using a texture to scale the image after resampling.  This is
   * slower and uses more memory, but provides high-quality results.
   * It is On by default.
   */
  svtkSetMacro(ResampleToScreenPixels, svtkTypeBool);
  svtkBooleanMacro(ResampleToScreenPixels, svtkTypeBool);
  svtkGetMacro(ResampleToScreenPixels, svtkTypeBool);
  //@}

  //@{
  /**
   * Keep the color mapping stage distinct from the reslicing stage.
   * This will improve the quality and possibly the speed of interactive
   * window/level operations, but it uses more memory and might slow down
   * interactive slicing operations.  On by default.
   */
  svtkSetMacro(SeparateWindowLevelOperation, svtkTypeBool);
  svtkBooleanMacro(SeparateWindowLevelOperation, svtkTypeBool);
  svtkGetMacro(SeparateWindowLevelOperation, svtkTypeBool);
  //@}

  //@{
  /**
   * Set a custom interpolator.  This will only be used if the
   * ResampleToScreenPixels option is on.
   */
  virtual void SetInterpolator(svtkAbstractImageInterpolator* sampler);
  virtual svtkAbstractImageInterpolator* GetInterpolator();
  //@}

  /**
   * This should only be called by the renderer.
   */
  void Render(svtkRenderer* renderer, svtkImageSlice* prop) override;

  /**
   * Release any graphics resources that are being consumed by
   * this mapper.  The parameter window is used to determine
   * which graphic resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * Get the mtime for the mapper.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * The bounding box (array of six doubles) of the data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() override;
  void GetBounds(double bounds[6]) override { this->svtkAbstractMapper3D::GetBounds(bounds); }
  //@}

  /**
   * Handle requests from the pipeline executive.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  // return the bounds in index space
  void GetIndexBounds(double extent[6]) override;

protected:
  svtkImageResliceMapper();
  ~svtkImageResliceMapper() override;

  /**
   * Do a checkerboard pattern to the alpha of an RGBA image
   */
  void CheckerboardImage(svtkImageData* input, svtkCamera* camera, svtkImageProperty* property);

  /**
   * Update the slice-to-world matrix from the camera.
   */
  void UpdateSliceToWorldMatrix(svtkCamera* camera);

  /**
   * Check if the svtkProp3D matrix has changed, and if so, set
   * the WorldToDataMatrix to its inverse.
   */
  void UpdateWorldToDataMatrix(svtkImageSlice* prop);

  /**
   * Update the reslice matrix, which is the slice-to-data matrix.
   */
  void UpdateResliceMatrix(svtkRenderer* ren, svtkImageSlice* prop);

  /**
   * Set all of the reslicing parameters.  This requires that
   * the SliceToWorld and WorldToData matrices are up-to-date.
   */
  void UpdateResliceInformation(svtkRenderer* ren);

  /**
   * Set the interpolation.
   */
  void UpdateResliceInterpolation(svtkImageProperty* property);

  /**
   * Update anything related to the image coloring.
   */
  void UpdateColorInformation(svtkImageProperty* property);

  /**
   * Make a polygon by cutting the data bounds with a plane.
   */
  void UpdatePolygonCoords(svtkRenderer* ren);

  //@{
  /**
   * Override Update to handle some tricky details.
   */
  void Update(int port) override;
  void Update() override;
  svtkTypeBool Update(int port, svtkInformationVector* requests) override;
  svtkTypeBool Update(svtkInformation* requests) override;
  //@}

  /**
   * Garbage collection for reference loops.
   */
  void ReportReferences(svtkGarbageCollector*) override;

  svtkImageSliceMapper* SliceMapper; // Does the OpenGL rendering

  svtkTypeBool JumpToNearestSlice;           // Adjust SliceAtFocalPoint
  svtkTypeBool AutoAdjustImageQuality;       // LOD-style behavior
  svtkTypeBool SeparateWindowLevelOperation; // Do window/level as a separate step
  double SlabThickness;                     // Current slab thickness
  int SlabType;                             // Current slab mode
  int SlabSampleFactor;                     // Sampling factor for slab mode
  int ImageSampleFactor;                    // Sampling factor for image pixels
  svtkTypeBool ResampleToScreenPixels;       // Use software interpolation only
  int InternalResampleToScreenPixels;       // Use software interpolation only
  int ResliceNeedUpdate;                    // Execute reslice on next render
  svtkImageResliceToColors* ImageReslice;    // For software interpolation
  svtkMatrix4x4* ResliceMatrix;              // Cached reslice matrix
  svtkMatrix4x4* WorldToDataMatrix;          // World to Data transform matrix
  svtkMatrix4x4* SliceToWorldMatrix;         // Slice to World transform matrix
  svtkTimeStamp UpdateTime;

private:
  svtkImageResliceMapper(const svtkImageResliceMapper&) = delete;
  void operator=(const svtkImageResliceMapper&) = delete;
};

#endif
