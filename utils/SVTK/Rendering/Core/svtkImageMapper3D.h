/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMapper3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMapper3D
 * @brief   abstract class for mapping images to the screen
 *
 * svtkImageMapper3D is a mapper that will draw a 2D image, or a slice
 * of a 3D image.  The slice plane can be set automatically follow the
 * camera, so that it slices through the focal point and faces the camera.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * svtkImage svtkImageProperty svtkImageResliceMapper svtkImageSliceMapper
 */

#ifndef svtkImageMapper3D_h
#define svtkImageMapper3D_h

#include "svtkAbstractMapper3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkProp3D;
class svtkPoints;
class svtkMatrix4x4;
class svtkLookupTable;
class svtkScalarsToColors;
class svtkImageSlice;
class svtkImageProperty;
class svtkImageData;
class svtkMultiThreader;
class svtkImageToImageMapper3DFriendship;

class SVTKRENDERINGCORE_EXPORT svtkImageMapper3D : public svtkAbstractMapper3D
{
public:
  svtkTypeMacro(svtkImageMapper3D, svtkAbstractMapper3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This should only be called by the renderer.
   */
  virtual void Render(svtkRenderer* renderer, svtkImageSlice* prop) = 0;

  /**
   * Release any graphics resources that are being consumed by
   * this mapper.  The parameter window is used to determine
   * which graphic resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override = 0;

  //@{
  /**
   * The input data for this mapper.
   */
  void SetInputData(svtkImageData* input);
  svtkImageData* GetInput();
  svtkDataSet* GetDataSetInput();
  svtkDataObject* GetDataObjectInput();
  //@}

  //@{
  /**
   * Instead of displaying the image only out to the image
   * bounds, include a half-voxel border around the image.
   * Within this border, the image values will be extrapolated
   * rather than interpolated.
   */
  svtkSetMacro(Border, svtkTypeBool);
  svtkBooleanMacro(Border, svtkTypeBool);
  svtkGetMacro(Border, svtkTypeBool);
  //@}

  //@{
  /**
   * Instead of rendering only to the image border, render out
   * to the viewport boundary with the background color.  The
   * background color will be the lowest color on the lookup
   * table that is being used for the image.
   */
  svtkSetMacro(Background, svtkTypeBool);
  svtkBooleanMacro(Background, svtkTypeBool);
  svtkGetMacro(Background, svtkTypeBool);
  //@}

  //@{
  /**
   * Automatically set the slice position to the camera focal point.
   * This provides a convenient way to interact with the image, since
   * most Interactors directly control the camera.
   */
  svtkSetMacro(SliceAtFocalPoint, svtkTypeBool);
  svtkBooleanMacro(SliceAtFocalPoint, svtkTypeBool);
  svtkGetMacro(SliceAtFocalPoint, svtkTypeBool);
  //@}

  //@{
  /**
   * Automatically set the slice orientation so that it faces the camera.
   * This provides a convenient way to interact with the image, since
   * most Interactors directly control the camera.
   */
  svtkSetMacro(SliceFacesCamera, svtkTypeBool);
  svtkBooleanMacro(SliceFacesCamera, svtkTypeBool);
  svtkGetMacro(SliceFacesCamera, svtkTypeBool);
  //@}

  //@{
  /**
   * A plane that describes what slice of the input is being
   * rendered by the mapper.  This plane is in world coordinates,
   * not data coordinates.  Before using this plane, call Update
   * or UpdateInformation to make sure the plane is up-to-date.
   * These methods are automatically called by Render.
   */
  svtkGetObjectMacro(SlicePlane, svtkPlane);
  //@}

  /**
   * Get the plane as a homogeneous 4-vector that gives the plane
   * equation coefficients.  The prop3D matrix must be provided so
   * that the plane can be converted to data coords.
   */
  virtual void GetSlicePlaneInDataCoords(svtkMatrix4x4* propMatrix, double plane[4]);

  //@{
  /**
   * The number of threads to create when rendering.
   */
  svtkSetClampMacro(NumberOfThreads, int, 1, SVTK_MAX_THREADS);
  svtkGetMacro(NumberOfThreads, int);
  //@}

  //@{
  /**
   * Turn on streaming, to pull the minimum amount of data from the input.
   * Streaming decreases the memory required to display large images, since
   * only one slice will be pulled through the input pipeline if only
   * one slice is mapped to the screen.  The default behavior is to pull
   * the full 3D input extent through the input pipeline, but to do this
   * only when the input data changes.  The default behavior results in
   * much faster follow-up renders when the input data is static.
   */
  svtkSetMacro(Streaming, svtkTypeBool);
  svtkGetMacro(Streaming, svtkTypeBool);
  svtkBooleanMacro(Streaming, svtkTypeBool);
  //@}

  // return the bounds in index space
  virtual void GetIndexBounds(double extent[6]) = 0;

protected:
  svtkImageMapper3D();
  ~svtkImageMapper3D() override;

  //@{
  /**
   * See algorithm for more info
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  //@}

  /**
   * Handle requests from the pipeline executive.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  /**
   * Checkerboard the alpha component of an RGBA image.  The origin and
   * spacing are in pixel units.
   */
  static void CheckerboardRGBA(unsigned char* data, int xsize, int ysize, double originx,
    double originy, double spacingx, double spacingy);

  /**
   * Perform window/level and color mapping operations to produce
   * unsigned char data that can be used as a texture.  See the
   * source file for more information.
   */
  unsigned char* MakeTextureData(svtkImageProperty* property, svtkImageData* input, int extent[6],
    int& xsize, int& ysize, int& bytesPerPixel, bool& reuseTexture, bool& reuseData);

  /**
   * Compute the coordinates and texture coordinates for the image, given
   * an extent that describes a single slice.
   */
  void MakeTextureGeometry(const int extent[6], double coords[12], double tcoords[8]);

  /**
   * Given an extent that describes a slice (it must have unit thickness
   * in one of the three directions), return the dimension indices that
   * correspond to the texture "x" and "y", provide the x, y image size,
   * and provide the texture size (padded to a power of two if the hardware
   * requires).
   */
  virtual void ComputeTextureSize(
    const int extent[6], int& xdim, int& ydim, int imageSize[2], int textureSize[2]);

  /**
   * Get the renderer associated with this mapper, or zero if none.
   * This will raise an error if multiple renderers are found.
   */
  svtkRenderer* GetCurrentRenderer();

  /**
   * Get the svtkImage prop associated with this mapper, or zero if none.
   */
  svtkImageSlice* GetCurrentProp() { return this->CurrentProp; }

  /**
   * Get the data-to-world matrix for this mapper, according to the
   * assembly path for its prop.
   */
  svtkMatrix4x4* GetDataToWorldMatrix();

  /**
   * Get the background color, by using the first color in the
   * supplied lookup table, or black if there is no lookup table.
   */
  void GetBackgroundColor(svtkImageProperty* property, double color[4]);

  svtkTypeBool Border;
  svtkTypeBool Background;
  svtkScalarsToColors* DefaultLookupTable;
  svtkMultiThreader* Threader;
  int NumberOfThreads;
  svtkTypeBool Streaming;

  // The slice.
  svtkPlane* SlicePlane;
  svtkTypeBool SliceAtFocalPoint;
  svtkTypeBool SliceFacesCamera;

  // Information about the image, updated by UpdateInformation
  double DataSpacing[3];
  double DataOrigin[3];
  double DataDirection[9];
  int DataWholeExtent[6];

  // Set by svtkImageStack when doing multi-pass rendering
  bool MatteEnable;
  bool ColorEnable;
  bool DepthEnable;

private:
  // The prop this mapper is attached to, or zero if none.
  svtkImageSlice* CurrentProp;
  svtkRenderer* CurrentRenderer;

  // The cached data-to-world matrix
  svtkMatrix4x4* DataToWorldMatrix;

  svtkImageMapper3D(const svtkImageMapper3D&) = delete;
  void operator=(const svtkImageMapper3D&) = delete;

  friend class svtkImageToImageMapper3DFriendship;
};

#endif
