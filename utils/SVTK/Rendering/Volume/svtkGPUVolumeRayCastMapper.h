/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGPUVolumeRayCastMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGPUVolumeRayCastMapper
 * @brief   Ray casting performed on the GPU.
 *
 * svtkGPUVolumeRayCastMapper is a volume mapper that performs ray casting on
 * the GPU using fragment programs.
 *
 * This mapper supports connections in multiple ports of input 0 (port 0 being
 * the only required connection). It is up to the concrete implementation
 * whether additional inputs will be used during rendering. This class maintains
 * a list of the currently active input ports (Ports) as well as a list of the
 * ports that have been disconnected (RemovedPorts). RemovedPorts is used the
 * the concrete implementation to clean up internal structures.
 *
 */
#ifndef svtkGPUVolumeRayCastMapper_h
#define svtkGPUVolumeRayCastMapper_h
#include <unordered_map> // For std::unordered_map
#include <vector>        // For std::vector

#include "svtkVolumeMapper.h"
#include <svtkRenderingVolumeModule.h> // For export macro

class svtkContourValues;
class svtkRenderWindow;
class svtkVolumeProperty;

class SVTKRENDERINGVOLUME_EXPORT svtkGPUVolumeRayCastMapper : public svtkVolumeMapper
{
public:
  static svtkGPUVolumeRayCastMapper* New();
  svtkTypeMacro(svtkGPUVolumeRayCastMapper, svtkVolumeMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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
   * Compute the sample distance from the data spacing.  When the number of
   * voxels is 8, the sample distance will be roughly 1/200 the average voxel
   * size. The distance will grow proportionally to numVoxels^(1/3). Off by default.
   */
  svtkSetClampMacro(LockSampleDistanceToInputSpacing, svtkTypeBool, 0, 1);
  svtkGetMacro(LockSampleDistanceToInputSpacing, svtkTypeBool);
  svtkBooleanMacro(LockSampleDistanceToInputSpacing, svtkTypeBool);
  //@}

  //@{
  /**
   * If UseJittering is on, each ray traversal direction will be
   * perturbed slightly using a noise-texture to get rid of wood-grain
   * effect.
   */
  svtkSetClampMacro(UseJittering, svtkTypeBool, 0, 1);
  svtkGetMacro(UseJittering, svtkTypeBool);
  svtkBooleanMacro(UseJittering, svtkTypeBool);
  //@}

  //@{
  /**
   * If UseDepthPass is on, the mapper will use two passes. In the first
   * pass, an isocontour depth buffer will be utilized as starting point
   * for ray-casting hence eliminating traversal on voxels that are
   * not going to participate in final rendering. UseDepthPass requires
   * reasonable contour values to be set which can be set by calling
   * GetDepthPassContourValues() method and using svtkControurValues API.
   */
  svtkSetClampMacro(UseDepthPass, svtkTypeBool, 0, 1);
  svtkGetMacro(UseDepthPass, svtkTypeBool);
  svtkBooleanMacro(UseDepthPass, svtkTypeBool);
  //@}

  /**
   * Return handle to contour values container so
   * that values can be set by the application. Contour values
   * will be used only when UseDepthPass is on.
   */
  svtkContourValues* GetDepthPassContourValues();

  //@{
  /**
   * Set/Get the distance between samples used for rendering
   * when AutoAdjustSampleDistances is off, or when this mapper
   * has more than 1 second allocated to it for rendering.
   * Initial value is 1.0.
   */
  svtkSetMacro(SampleDistance, float);
  svtkGetMacro(SampleDistance, float);
  //@}

  //@{
  /**
   * Sampling distance in the XY image dimensions. Default value of 1 meaning
   * 1 ray cast per pixel. If set to 0.5, 4 rays will be cast per pixel. If
   * set to 2.0, 1 ray will be cast for every 4 (2 by 2) pixels. This value
   * will be adjusted to meet a desired frame rate when AutoAdjustSampleDistances
   * is on.
   */
  svtkSetClampMacro(ImageSampleDistance, float, 0.1f, 100.0f);
  svtkGetMacro(ImageSampleDistance, float);
  //@}

  //@{
  /**
   * This is the minimum image sample distance allow when the image
   * sample distance is being automatically adjusted.
   */
  svtkSetClampMacro(MinimumImageSampleDistance, float, 0.1f, 100.0f);
  svtkGetMacro(MinimumImageSampleDistance, float);
  //@}

  //@{
  /**
   * This is the maximum image sample distance allow when the image
   * sample distance is being automatically adjusted.
   */
  svtkSetClampMacro(MaximumImageSampleDistance, float, 0.1f, 100.0f);
  svtkGetMacro(MaximumImageSampleDistance, float);
  //@}

  //@{
  /**
   * Set/Get the window / level applied to the final color.
   * This allows brightness / contrast adjustments on the
   * final image.
   * window is the width of the window.
   * level is the center of the window.
   * Initial window value is 1.0
   * Initial level value is 0.5
   * window cannot be null but can be negative, this way
   * values will be reversed.
   * |window| can be larger than 1.0
   * level can be any real value.
   */
  svtkSetMacro(FinalColorWindow, float);
  svtkGetMacro(FinalColorWindow, float);
  svtkSetMacro(FinalColorLevel, float);
  svtkGetMacro(FinalColorLevel, float);
  //@}

  //@{
  /**
   * Maximum size of the 3D texture in GPU memory.
   * Will default to the size computed from the graphics
   * card. Can be adjusted by the user.
   */
  svtkSetMacro(MaxMemoryInBytes, svtkIdType);
  svtkGetMacro(MaxMemoryInBytes, svtkIdType);
  //@}

  //@{
  /**
   * Maximum fraction of the MaxMemoryInBytes that should
   * be used to hold the texture. Valid values are 0.1 to
   * 1.0.
   */
  svtkSetClampMacro(MaxMemoryFraction, float, 0.1f, 1.0f);
  svtkGetMacro(MaxMemoryFraction, float);
  //@}

  //@{
  /**
   * Tells if the mapper will report intermediate progress.
   * Initial value is true. As the progress works with a GL blocking
   * call (glFinish()), this can be useful for huge dataset but can
   * slow down rendering of small dataset. It should be set to true
   * for big dataset or complex shading and streaming but to false for
   * small datasets.
   */
  svtkSetMacro(ReportProgress, bool);
  svtkGetMacro(ReportProgress, bool);
  //@}

  /**
   * Based on hardware and properties, we may or may not be able to
   * render using 3D texture mapping. This indicates if 3D texture
   * mapping is supported by the hardware, and if the other extensions
   * necessary to support the specific properties are available.
   */
  virtual int IsRenderSupported(
    svtkRenderWindow* svtkNotUsed(window), svtkVolumeProperty* svtkNotUsed(property))
  {
    return 0;
  }

  void CreateCanonicalView(svtkRenderer* ren, svtkVolume* volume, svtkImageData* image, int blend_mode,
    double viewDirection[3], double viewUp[3]);

  //@{
  /**
   * Optionally, set a mask input. This mask may be a binary mask or a label
   * map. This must be specified via SetMaskType.

   * If the mask is a binary mask, the volume rendering is confined to regions
   * within the binary mask. The binary mask is assumed to have a datatype of
   * UCHAR and values of 255 (inside) and 0 (outside).

   * The mask may also be a label map. The label map must have a datatype of
   * UCHAR i.e. it can have upto 256 labels. The label 0 is reserved as a
   * special label. In voxels with label value of 0, the default transfer
   * functions supplied by svtkVolumeProperty are used.
   *
   * For voxels with a label values greater than 0, the color transfer functions
   * supplied using svtkVolumeProperty's label API are used.
   *
   * For voxels with a label value greater than 0, the color transfer function
   * is blended with the default color transfer function, with the blending
   * weight determined by MaskBlendFactor.
   */
  void SetMaskInput(svtkImageData* mask);
  svtkGetObjectMacro(MaskInput, svtkImageData);
  //@}

  enum
  {
    BinaryMaskType = 0,
    LabelMapMaskType
  };

  //@{
  /**
   * Set the mask type, if mask is to be used. See documentation for
   * SetMaskInput(). The default is a LabelMapMaskType.
   */
  svtkSetMacro(MaskType, int);
  svtkGetMacro(MaskType, int);
  void SetMaskTypeToBinary();
  void SetMaskTypeToLabelMap();
  //@}

  //@{
  /**
   * Tells how much mask color transfer function is used compared to the
   * standard color transfer function when the mask is true. This is relevant
   * only for the label map mask.
   * 0.0 means only standard color transfer function.
   * 1.0 means only mask color transfer function.
   * The default value is 1.0.
   */
  svtkSetClampMacro(MaskBlendFactor, float, 0.0f, 1.0f);
  svtkGetMacro(MaskBlendFactor, float);
  //@}

  //@{
  /**
   * Enable or disable setting output of volume rendering to be
   * color and depth textures. By default this is set to 0 (off).
   * It should be noted that it is possible that underlying API specific
   * mapper may not supoport RenderToImage mode.
   * \warning
   * \li This method ignores any other volumes / props in the scene.
   * \li This method does not respect the general attributes of the
   * scene i.e. background color, etc. It always produces a color
   * image that has a transparent white background outside the
   * bounds of the volume.

   * \sa GetDepthImage(), GetColorImage()
   */
  svtkSetMacro(RenderToImage, svtkTypeBool);
  svtkGetMacro(RenderToImage, svtkTypeBool);
  svtkBooleanMacro(RenderToImage, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the scalar type of the depth texture in RenderToImage mode.
   * By default, the type if SVTK_FLOAT.
   * \sa SetRenderToImage()
   */
  svtkSetMacro(DepthImageScalarType, int);
  svtkGetMacro(DepthImageScalarType, int);
  void SetDepthImageScalarTypeToUnsignedChar();
  void SetDepthImageScalarTypeToUnsignedShort();
  void SetDepthImageScalarTypeToFloat();
  //@}

  //@{
  /**
   * Enable or disable clamping the depth value of the fully
   * transparent voxel to the depth of the back-face of the
   * volume. This parameter is used when RenderToImage mode is
   * enabled. When ClampDepthToBackFace is false, the fully transparent
   * voxels will have a value of 1.0 in the depth image. When
   * this is true, the fully transparent voxels will have the
   * depth value of the face at which the ray exits the volume.
   * By default, this is set to 0 (off).
   * \sa SetRenderToImage(), GetDepthImage()
   */
  svtkSetMacro(ClampDepthToBackface, svtkTypeBool);
  svtkGetMacro(ClampDepthToBackface, svtkTypeBool);
  svtkBooleanMacro(ClampDepthToBackface, svtkTypeBool);
  //@}

  /**
   * Low level API to export the depth texture as svtkImageData in
   * RenderToImage mode.
   * Should be implemented by the graphics API specific mapper (GL or other).
   * \sa SetRenderToImage()
   */
  virtual void GetDepthImage(svtkImageData*) {}

  /**
   * Low level API to export the color texture as svtkImageData in
   * RenderToImage mode.
   * Should be implemented by the graphics API specific mapper (GL or other).
   * \sa SetRenderToImage()
   */
  virtual void GetColorImage(svtkImageData*) {}

  /**
   * Initialize rendering for this volume.
   * \warning INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   */
  void Render(svtkRenderer*, svtkVolume*) override;

  /**
   * Handled in the subclass - the actual render method
   * \pre input is up-to-date.
   */
  virtual void GPURender(svtkRenderer*, svtkVolume*) {}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   * \warning INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   */
  void ReleaseGraphicsResources(svtkWindow*) override {}

  /**
   * Return how much the dataset has to be reduced in each dimension to
   * fit on the GPU. If the value is 1.0, there is no need to reduce the
   * dataset.
   * \pre the calling thread has a current OpenGL context.
   * \pre mapper_supported: IsRenderSupported(renderer->GetRenderWindow(),0)
   * The computation is based on hardware limits (3D texture indexable size)
   * and MaxMemoryInBytes.
   * \post valid_i_ratio: ratio[0]>0 && ratio[0]<=1.0
   * \post valid_j_ratio: ratio[1]>0 && ratio[1]<=1.0
   * \post valid_k_ratio: ratio[2]>0 && ratio[2]<=1.0
   */
  virtual void GetReductionRatio(double ratio[3]) = 0;

  enum TFRangeType
  {
    SCALAR = 0, // default
    NATIVE
  };

  //@{
  /**
   * Set whether to use the scalar range or the native transfer function range
   * when looking up transfer functions for color and opacity values. When the
   * range is set to TransferFunctionRange::SCALAR, the function is distributed
   * over the entire scalar range. If it is set to
   * TransferFunctionRange::NATIVE, the scalar values outside the native
   * transfer function range will be truncated to native range. By
   * default, the volume scalar range is used.
   *
   * \note The native range of the transfer function is the range returned by
   * svtkColorTransferFunction::GetRange() or svtkPiecewiseFunction::GetRange().
   *
   * \note There is no special API provided for 2D transfer functions
   * considering that they are set as a pre-generated svtkImageData on this
   * class i.e. the range is already encoded.
   */
  svtkSetMacro(ColorRangeType, int);
  svtkGetMacro(ColorRangeType, int);
  svtkSetMacro(ScalarOpacityRangeType, int);
  svtkGetMacro(ScalarOpacityRangeType, int);
  svtkSetMacro(GradientOpacityRangeType, int);
  svtkGetMacro(GradientOpacityRangeType, int);
  //@}

  svtkImageData* GetInput() override { return this->GetInput(0); };

  //@{
  /**
   * Add/Remove input connections. Active and removed ports are cached in
   * Ports and RemovedPorts respectively.
   */
  void RemoveInputConnection(int port, svtkAlgorithmOutput* input) override;
  void RemoveInputConnection(int port, int idx) override;
  void SetInputConnection(int port, svtkAlgorithmOutput* input) override;
  void SetInputConnection(svtkAlgorithmOutput* input) override
  {
    this->SetInputConnection(0, input);
  }
  //@}

  /**
   * Number of currently active ports.
   */
  int GetInputCount();

  svtkImageData* GetTransformedInput(const int port = 0);

  double* GetBoundsFromPort(const int port) SVTK_SIZEHINT(6);

protected:
  svtkGPUVolumeRayCastMapper();
  ~svtkGPUVolumeRayCastMapper() override;

  /**
   * Handle inputs. This mapper provides an interface to support multiple
   * inputs but it is up to the OpenGL implementation use them during rendering.
   * Currently, only VolumeOpenGL2/svtkOpenGLGPUVolumeRayCastMapper makes use
   * of these inputs.
   *
   * \sa svtkOpenGLGPUVolumeRayCastMapper svtkMultiVolume
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * A transformation is applied (translation) to the input.  The resulting
   * data is stored in TransformedInputs. Takes as an argumet the port of an
   * input connection.
   *
   * ///TODO Elaborate on why this is an issue, texture coords (?)
   * @TODO: This is the workaround to deal with GPUVolumeRayCastMapper
   * not able to handle extents starting from non zero values.
   * There is not a easy fix in the GPU volume ray cast mapper hence
   * this fix has been introduced.
   */
  void TransformInput(const int port);

  //@{
  /**
   * This method is used by the Render() method to validate everything before
   * attempting to render. This method returns 0 if something is not right -
   * such as missing input, a null renderer or a null volume, no scalars, etc.
   * In some cases it will produce a svtkErrorMacro message, and in others
   * (for example, in the case of cropping planes that define a region with
   * a volume or 0 or less) it will fail silently. If everything is OK, it will
   * return with a value of 1.
   */
  int ValidateRender(svtkRenderer*, svtkVolume*);
  int ValidateInputs();
  int ValidateInput(svtkVolumeProperty* property, const int port);
  //@}

  //@{
  /**
   * Shallow-copy the inputs into a transform-adjusted clone.
   * \sa svtkGPUVolumeRayCastMapper::TransformInput
   */
  void CloneInputs();
  void CloneInput(svtkImageData* input, const int port);
  //@}

  // Special version of render called during the creation
  // of a canonical view.
  void CanonicalViewRender(svtkRenderer*, svtkVolume*);

  // Methods called by the AMR Volume Mapper.
  virtual void PreRender(svtkRenderer* ren, svtkVolume* vol, double datasetBounds[6],
    double scalarRange[2], int numberOfScalarComponents, unsigned int numberOfLevels) = 0;

  // \pre input is up-to-date
  virtual void RenderBlock(svtkRenderer* ren, svtkVolume* vol, unsigned int level) = 0;

  virtual void PostRender(svtkRenderer* ren, int numberOfScalarComponents) = 0;
  svtkImageData* GetInput(const int port) override;

  /**
   * Called by the AMR Volume Mapper.
   * Set the flag that tells if the scalars are on point data (0) or
   * cell data (1).
   */
  void SetCellFlag(int cellFlag);
  void RemovePortInternal(const int port);

  svtkTypeBool LockSampleDistanceToInputSpacing;
  svtkTypeBool AutoAdjustSampleDistances;
  float ImageSampleDistance;
  float MinimumImageSampleDistance;
  float MaximumImageSampleDistance;

  // Render to texture mode flag
  svtkTypeBool RenderToImage;

  // Depth image scalar type
  int DepthImageScalarType;

  // Clamp depth values to the depth of the face at which the ray
  // exits the volume
  svtkTypeBool ClampDepthToBackface;

  // Enable / disable stochastic jittering
  svtkTypeBool UseJittering;

  // Enable / disable two pass rendering
  svtkTypeBool UseDepthPass;
  svtkContourValues* DepthPassContourValues;

  // The distance between sample points along the ray
  float SampleDistance;

  int SmallVolumeRender;
  double BigTimeToDraw;
  double SmallTimeToDraw;

  float FinalColorWindow;
  float FinalColorLevel;

  // 1 if we are generating the canonical image, 0 otherwise
  int GeneratingCanonicalView;
  svtkImageData* CanonicalViewImageData;

  //@{
  /**
   * Set the mapper in AMR Mode or not. Initial value is false.
   * Called only by the svtkKWAMRVolumeMapper
   */
  svtkSetClampMacro(AMRMode, svtkTypeBool, 0, 1);
  svtkGetMacro(AMRMode, svtkTypeBool);
  svtkBooleanMacro(AMRMode, svtkTypeBool);
  //@}

  svtkImageData* MaskInput;
  float MaskBlendFactor;
  int MaskType;

  svtkTypeBool AMRMode;

  // Transfer function range type
  int ColorRangeType;
  int ScalarOpacityRangeType;
  int GradientOpacityRangeType;

  // Point data or cell data (or field data, not handled) ?
  int CellFlag;

  /**
   * Compute the cropping planes clipped by the bounds of the volume.
   * The result is put into this->ClippedCroppingRegionPlanes.
   * NOTE: IT WILL BE MOVED UP TO svtkVolumeMapper after bullet proof usage
   * in this mapper. Other subclasses will use the ClippedCroppingRegionsPlanes
   * members instead of CroppingRegionPlanes.
   * \pre volume_exists: this->GetInput()!=0
   * \pre valid_cropping: this->Cropping &&
   * this->CroppingRegionPlanes[0]<this->CroppingRegionPlanes[1] &&
   * this->CroppingRegionPlanes[2]<this->CroppingRegionPlanes[3] &&
   * this->CroppingRegionPlanes[4]<this->CroppingRegionPlanes[5])
   */
  virtual void ClipCroppingRegionPlanes();

  using DataMap = std::unordered_map<int, svtkImageData*>;
  void SetTransformedInput(svtkImageData*);
  svtkImageData* FindData(int port, DataMap& container);

  double ClippedCroppingRegionPlanes[6];

  svtkIdType MaxMemoryInBytes;
  float MaxMemoryFraction;

  bool ReportProgress;
  std::vector<int> Ports;
  std::vector<int> RemovedPorts;
  DataMap TransformedInputs;

  /**
   * This is needed only to check if the input data has been changed since the last
   * Render() call.
   */
  DataMap LastInputs;

private:
  svtkGPUVolumeRayCastMapper(const svtkGPUVolumeRayCastMapper&) = delete;
  void operator=(const svtkGPUVolumeRayCastMapper&) = delete;
};

#endif
