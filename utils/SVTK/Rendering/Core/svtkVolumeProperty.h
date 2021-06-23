/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkVolumeProperty
 * @brief   represents the common properties for rendering a volume.
 *
 * svtkVolumeProperty is used to represent common properties associated
 * with volume rendering. This includes properties for determining the type
 * of interpolation to use when sampling a volume, the color of a volume,
 * the scalar opacity of a volume, the gradient opacity of a volume, and the
 * shading parameters of a volume.
 *
 * Color, scalar opacity and gradient magnitude opacity transfer functions
 * can be set as either 3 separate 1D functions or as a single 2D transfer
 * function.
 *
 * - 1D Transfer functions (svtkVolumeProperty::TF_1D)
 * Color, scalar opacity and gradient magnitude opacity are defined by 1
 * svtkColorTransferFunction and 2 svtkPiecewiseFunctions respectively.
 * When the scalar opacity or the gradient opacity of a volume is not set,
 * then the function is defined to be a constant value of 1.0. When a
 * scalar and gradient opacity are both set simultaneously, then the opacity
 * is defined to be the product of the scalar opacity and gradient opacity
 * transfer functions. 1D transfer functions is the legacy and default behavior.
 *
 * - 2D Transfer functions (svtkVolumeProperty::TF_2D)
 * Color and scalar/gradient magnitude opacity are defined by a 4-component
 * svtkImageData instance mapping scalar value vs. gradient magnitude on its
 * x and y axis respectively. This mode is only available if a 2D TF has been
 * explicitly set (see SetTransferFunction2D).
 *
 * Most properties can be set per "component" for volume mappers that
 * support multiple independent components. If you are using 2 component
 * data as LV or 4 component data as RGBV (as specified in the mapper)
 * only the first scalar opacity and gradient opacity transfer functions
 * will be used (and all color functions will be ignored). Omitting the
 * index parameter on the Set/Get methods will access index = 0.
 *
 * @sa svtkPiecewiseFunction svtkColorTransferFunction
 */

#ifndef svtkVolumeProperty_h
#define svtkVolumeProperty_h

#include "svtkImplicitFunction.h" // For svtkImplicitFunction
#include "svtkNew.h"              // Needed for svtkNew
#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkSmartPointer.h"        // Needed for svtkSmartPointer

// STL includes
#include <set>           // For labelmap labels set
#include <unordered_map> // For labelmap transfer function maps

class svtkColorTransferFunction;
class svtkContourValues;
class svtkImageData;
class svtkPiecewiseFunction;
class svtkTimeStamp;

class SVTKRENDERINGCORE_EXPORT svtkVolumeProperty : public svtkObject
{
public:
  static svtkVolumeProperty* New();
  svtkTypeMacro(svtkVolumeProperty, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  void DeepCopy(svtkVolumeProperty* p);

  /**
   * Get the modified time for this object (or the properties registered
   * with this object).
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Does the data have independent components, or do some define color
   * only? If IndependentComponents is On (the default) then each component
   * will be independently passed through a lookup table to determine RGBA,
   * shaded. Some volume Mappers can handle 1 to 4 component
   * unsigned char or unsigned short data (see each mapper header file to
   * determine functionality). If IndependentComponents is Off, then you
   * must have either 2 or 4 component data. For 2 component data, the
   * first is passed through the first color transfer function and the
   * second component is passed through the first scalar opacity (and
   * gradient opacity) transfer function.
   * Normals will be generated off of the second component. When using gradient
   * based opacity modulation, the gradients are computed off of the
   * second component. For 4 component
   * data, the first three will directly represent RGB (no lookup table).
   * The fourth component will be passed through the first scalar opacity
   * transfer function for opacity and first gradient opacity transfer function
   * for gradient based opacity modulation. Normals will be generated from the
   * fourth component. When using gradient based opacity modulation, the
   * gradients are computed off of the fourth component.
   */
  svtkSetClampMacro(IndependentComponents, svtkTypeBool, 0, 1);
  svtkGetMacro(IndependentComponents, svtkTypeBool);
  svtkBooleanMacro(IndependentComponents, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the interpolation type for sampling a volume. Initial value is
   * SVTK_NEAREST_INTERPOLATION.
   */
  svtkSetClampMacro(InterpolationType, int, SVTK_NEAREST_INTERPOLATION, SVTK_LINEAR_INTERPOLATION);
  svtkGetMacro(InterpolationType, int);
  void SetInterpolationTypeToNearest() { this->SetInterpolationType(SVTK_NEAREST_INTERPOLATION); }
  void SetInterpolationTypeToLinear() { this->SetInterpolationType(SVTK_LINEAR_INTERPOLATION); }
  const char* GetInterpolationTypeAsString(void);
  //@}

  //@{
  /**
   * Set/Get the scalar component weights.
   * Clamped between the range of (0.0, 1.0)
   */
  virtual void SetComponentWeight(int index, double value);
  virtual double GetComponentWeight(int index);
  //@}

  /**
   * Set the color of a volume to a gray level transfer function
   * for the component indicated by index. This will set the
   * color channels for this component to 1.
   */
  void SetColor(int index, svtkPiecewiseFunction* function);
  void SetColor(svtkPiecewiseFunction* function) { this->SetColor(0, function); }

  /**
   * Set the color of a volume to an RGB transfer function
   * for the component indicated by index. This will set the
   * color channels for this component to 3.
   * This will also recompute the color channels
   */
  void SetColor(int index, svtkColorTransferFunction* function);
  void SetColor(svtkColorTransferFunction* function) { this->SetColor(0, function); }

  /**
   * Get the number of color channels in the transfer function
   * for the given component.
   */
  int GetColorChannels(int index);
  int GetColorChannels() { return this->GetColorChannels(0); }

  /**
   * Get the gray transfer function.
   * If no transfer function has been set for this component, a default one
   * is created and returned.
   */
  svtkPiecewiseFunction* GetGrayTransferFunction(int index);
  svtkPiecewiseFunction* GetGrayTransferFunction() { return this->GetGrayTransferFunction(0); }

  /**
   * Get the RGB transfer function for the given component.
   * If no transfer function has been set for this component, a default one
   * is created and returned.
   */
  svtkColorTransferFunction* GetRGBTransferFunction(int index);
  svtkColorTransferFunction* GetRGBTransferFunction() { return this->GetRGBTransferFunction(0); }

  /**
   * Set the opacity of a volume to an opacity transfer function based
   * on scalar value for the component indicated by index.
   */
  void SetScalarOpacity(int index, svtkPiecewiseFunction* function);
  void SetScalarOpacity(svtkPiecewiseFunction* function) { this->SetScalarOpacity(0, function); }

  /**
   * Get the scalar opacity transfer function for the given component.
   * If no transfer function has been set for this component, a default one
   * is created and returned.
   */
  svtkPiecewiseFunction* GetScalarOpacity(int index);
  svtkPiecewiseFunction* GetScalarOpacity() { return this->GetScalarOpacity(0); }

  //@{
  /**
   * Set/Get the unit distance on which the scalar opacity transfer function
   * is defined. By default this is 1.0, meaning that over a distance of
   * 1.0 units, a given opacity (from the transfer function) is accumulated.
   * This is adjusted for the actual sampling distance during rendering.
   */
  void SetScalarOpacityUnitDistance(int index, double distance);
  void SetScalarOpacityUnitDistance(double distance)
  {
    this->SetScalarOpacityUnitDistance(0, distance);
  }
  double GetScalarOpacityUnitDistance(int index);
  double GetScalarOpacityUnitDistance() { return this->GetScalarOpacityUnitDistance(0); }
  //@}

  /**
   * Set the opacity of a volume to an opacity transfer function based
   * on gradient magnitude for the given component.
   */
  void SetGradientOpacity(int index, svtkPiecewiseFunction* function);
  void SetGradientOpacity(svtkPiecewiseFunction* function) { this->SetGradientOpacity(0, function); }

  //@{
  /**
   * Set/Get a 2D transfer function. Volume mappers interpret the x-axis of
   * of this transfer function as scalar value and the y-axis as gradient
   * magnitude. The value at (X, Y) corresponds to the color and opacity
   * for a salar value of X and a gradient magnitude of Y.
   */
  void SetTransferFunction2D(int index, svtkImageData* function);
  void SetTransferFunction2D(svtkImageData* function) { this->SetTransferFunction2D(0, function); };

  svtkImageData* GetTransferFunction2D(int index);
  svtkImageData* GetTransferFunction2D() { return this->GetTransferFunction2D(0); };

  /**
   * Color-opacity transfer function mode. TF_1D is its default value.
   *  - TF_1D Mappers will use 3 separate 1D functions for color, scalar opacity
   *  and gradient mag. opacity.
   *  - TF_2D Mappers will use a single 2D function for color and scalar/gradient mag.
   *  opacity.
   */
  enum TransferMode
  {
    TF_1D = 0,
    TF_2D
  };

  svtkSetClampMacro(TransferFunctionMode, int, 0, 1);
  svtkGetMacro(TransferFunctionMode, int);
  //@}

  /**
   * Get the gradient magnitude opacity transfer function for
   * the given component.
   * If no transfer function has been set for this component, a default one
   * is created and returned.
   * This default function is always returned if DisableGradientOpacity is On
   * for that component.
   */
  svtkPiecewiseFunction* GetGradientOpacity(int index);
  svtkPiecewiseFunction* GetGradientOpacity() { return this->GetGradientOpacity(0); }

  //@{
  /**
   * Enable/Disable the gradient opacity function for the given component.
   * If set to true, any call to GetGradientOpacity() will return a default
   * function for this component. Note that the gradient opacity function is
   * still stored, it is not set or reset and can be retrieved using
   * GetStoredGradientOpacity().
   */
  virtual void SetDisableGradientOpacity(int index, int value);
  virtual void SetDisableGradientOpacity(int value) { this->SetDisableGradientOpacity(0, value); }
  virtual void DisableGradientOpacityOn(int index) { this->SetDisableGradientOpacity(index, 1); }
  virtual void DisableGradientOpacityOn() { this->DisableGradientOpacityOn(0); }
  virtual void DisableGradientOpacityOff(int index) { this->SetDisableGradientOpacity(index, 0); }
  virtual void DisableGradientOpacityOff() { this->DisableGradientOpacityOff(0); }
  virtual int GetDisableGradientOpacity(int index);
  virtual int GetDisableGradientOpacity() { return this->GetDisableGradientOpacity(0); }
  svtkPiecewiseFunction* GetStoredGradientOpacity(int index);
  svtkPiecewiseFunction* GetStoredGradientOpacity() { return this->GetStoredGradientOpacity(0); }
  //@}

  /**
   * Check whether or not we have the gradient opacity. Checking
   * gradient opacity via GetDisableGradientOpacity or GetGradientOpacity
   * will not work as in the former case,  GetDisableGradientOpacity returns
   * false by default and in the later case, a default gradient opacity will be created.
   */
  bool HasGradientOpacity(int index = 0)
  {
    switch (this->TransferFunctionMode)
    {
      case TF_1D:
        return (this->GradientOpacity[index] != nullptr);
      case TF_2D:
        return true;
    }
    return false;
  }

  /*
   * Check whether or not we have label map gradient opacity functions.
   */
  bool HasLabelGradientOpacity() { return !this->LabelGradientOpacity.empty(); }

  //@{
  /**
   * Set/Get the shading of a volume. If shading is turned off, then
   * the mapper for the volume will not perform shading calculations.
   * If shading is turned on, the mapper may perform shading
   * calculations - in some cases shading does not apply (for example,
   * in a maximum intensity projection) and therefore shading will
   * not be performed even if this flag is on. For a compositing type
   * of mapper, turning shading off is generally the same as setting
   * ambient=1, diffuse=0, specular=0. Shading can be independently
   * turned on/off per component.
   *
   * \note Shading is \b only supported for svtkVolumeMapper::COMPOSITE_BLEND.
   * For minimum and maximum intensity blend modes, there is not necessarily one
   * unique location along the ray through the volume where that minimum or
   * maximum occurs. For average and additive blend modes, the value being
   * visualized does not represent a location in the volume but rather a
   * statistical measurement along the ray traversing through the volume, and
   * hence shading is not applicable.
   * \sa svtkVolumeMapper::BlendModes
   */
  void SetShade(int index, int value);
  void SetShade(int value) { this->SetShade(0, value); }
  int GetShade(int index);
  int GetShade() { return this->GetShade(0); }
  void ShadeOn(int index);
  void ShadeOn() { this->ShadeOn(0); }
  void ShadeOff(int index);
  void ShadeOff() { this->ShadeOff(0); }
  //@}

  //@{
  /**
   * Set/Get the ambient lighting coefficient.
   */
  void SetAmbient(int index, double value);
  void SetAmbient(double value) { this->SetAmbient(0, value); }
  double GetAmbient(int index);
  double GetAmbient() { return this->GetAmbient(0); }
  //@}

  //@{
  /**
   * Set/Get the diffuse lighting coefficient.
   */
  void SetDiffuse(int index, double value);
  void SetDiffuse(double value) { this->SetDiffuse(0, value); }
  double GetDiffuse(int index);
  double GetDiffuse() { return this->GetDiffuse(0); }
  //@}

  //@{
  /**
   * Set/Get the specular lighting coefficient.
   */
  void SetSpecular(int index, double value);
  void SetSpecular(double value) { this->SetSpecular(0, value); }
  double GetSpecular(int index);
  double GetSpecular() { return this->GetSpecular(0); }
  //@}

  //@{
  /**
   * Set/Get the specular power.
   */
  void SetSpecularPower(int index, double value);
  void SetSpecularPower(double value) { this->SetSpecularPower(0, value); }
  double GetSpecularPower(int index);
  double GetSpecularPower() { return this->GetSpecularPower(0); }
  //@}

  /**
   * Get contour values for isosurface blending mode.
   * Do not affect other blending modes.
   */
  svtkContourValues* GetIsoSurfaceValues();

  //@{
  /**
   * Get/Set the function used for slicing.
   * Currently, only svtkPlane is supported.
   */
  svtkSetSmartPointerMacro(SliceFunction, svtkImplicitFunction);
  svtkGetSmartPointerMacro(SliceFunction, svtkImplicitFunction);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * UpdateMTimes performs a Modified() on all TimeStamps.
   * This is used by svtkVolume when the property is set, so
   * that any other object that might have been caching
   * information for the property will rebuild.
   */
  void UpdateMTimes();

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get the time that the gradient opacity transfer function was set
   */
  svtkTimeStamp GetGradientOpacityMTime(int index);
  svtkTimeStamp GetGradientOpacityMTime() { return this->GetGradientOpacityMTime(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get the time that the scalar opacity transfer function was set.
   */
  svtkTimeStamp GetScalarOpacityMTime(int index);
  svtkTimeStamp GetScalarOpacityMTime() { return this->GetScalarOpacityMTime(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get the time that the RGBTransferFunction was set
   */
  svtkTimeStamp GetRGBTransferFunctionMTime(int index);
  svtkTimeStamp GetRGBTransferFunctionMTime() { return this->GetRGBTransferFunctionMTime(0); }

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get the time that the GrayTransferFunction was set
   */
  svtkTimeStamp GetGrayTransferFunctionMTime(int index);
  svtkTimeStamp GetGrayTransferFunctionMTime() { return this->GetGrayTransferFunctionMTime(0); }

  //@{
  /**
   * Set/Get whether to use a fixed intensity value for voxels in the clipped
   * space for gradient calculations. When UseClippedVoxelIntensity is
   * enabled, the ClippedVoxelIntensity value will be used as intensity of
   * clipped voxels. By default, this is false.
   *
   * \note This property is only used by the svtkGPUVolumeRayCastMapper for now.
   * \sa SetClippedVoxelIntensity
   */
  svtkSetMacro(UseClippedVoxelIntensity, int);
  svtkGetMacro(UseClippedVoxelIntensity, int);
  svtkBooleanMacro(UseClippedVoxelIntensity, int);
  //@}

  //@{
  /**
   * Set/Get the intensity value for voxels in the clipped space for gradient
   * computations (for shading and gradient based opacity modulation).
   * By default, this is set to SVTK_DOUBLE_MIN.
   *
   * \note This value is only used when UseClippedVoxelIntensity is true.
   * \note This property is only used by the svtkGPUVolumeRayCastMapper for now.
   * \sa SetUseClippedVoxelIntensity
   */
  svtkSetMacro(ClippedVoxelIntensity, double);
  svtkGetMacro(ClippedVoxelIntensity, double);
  //@}

  //@{
  /**
   * Set/Get the color transfer function for a label in the label map.
   */
  void SetLabelColor(int label, svtkColorTransferFunction* function);
  svtkColorTransferFunction* GetLabelColor(int label);
  //@}

  //@{
  /**
   * Set/Get the opacity transfer function for a label in the label map.
   */
  void SetLabelScalarOpacity(int label, svtkPiecewiseFunction* function);
  svtkPiecewiseFunction* GetLabelScalarOpacity(int label);
  //@}

  //@{
  /**
   * Set/Get the gradient opacity function for a label in the label map.
   */
  void SetLabelGradientOpacity(int label, svtkPiecewiseFunction* function);
  svtkPiecewiseFunction* GetLabelGradientOpacity(int label);
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get the time that label color transfer functions were set
   */
  svtkGetMacro(LabelColorMTime, svtkTimeStamp);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get the time that label scalar opacity transfer functions were set
   */
  svtkGetMacro(LabelScalarOpacityMTime, svtkTimeStamp);

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get the time that label gradient opacity transfer functions were set
   */
  svtkGetMacro(LabelGradientOpacityMTime, svtkTimeStamp);

  /**
   * Get the number of labels that are provided with transfer functions using
   * either SetLabelColor, SetLabelScalarOpacity or SetLabelGradientOpacity.
   */
  std::size_t GetNumberOfLabels();

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get access to the internal set that keeps track of labels
   */
  std::set<int> GetLabelMapLabels();

protected:
  svtkVolumeProperty();
  ~svtkVolumeProperty() override;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Get the time when the TransferFunction2D was set.
   */
  svtkTimeStamp GetTransferFunction2DMTime(int index);
  svtkTimeStamp GetTransferFunction2DMTime() { return this->GetTransferFunction2DMTime(0); }

  virtual void CreateDefaultGradientOpacity(int index);

  svtkTypeBool IndependentComponents;
  double ComponentWeight[SVTK_MAX_VRCOMP];

  int InterpolationType;

  int ColorChannels[SVTK_MAX_VRCOMP];

  svtkPiecewiseFunction* GrayTransferFunction[SVTK_MAX_VRCOMP];
  svtkTimeStamp GrayTransferFunctionMTime[SVTK_MAX_VRCOMP];

  svtkColorTransferFunction* RGBTransferFunction[SVTK_MAX_VRCOMP];
  svtkTimeStamp RGBTransferFunctionMTime[SVTK_MAX_VRCOMP];

  svtkPiecewiseFunction* ScalarOpacity[SVTK_MAX_VRCOMP];
  svtkTimeStamp ScalarOpacityMTime[SVTK_MAX_VRCOMP];
  double ScalarOpacityUnitDistance[SVTK_MAX_VRCOMP];

  svtkPiecewiseFunction* GradientOpacity[SVTK_MAX_VRCOMP];
  svtkTimeStamp GradientOpacityMTime[SVTK_MAX_VRCOMP];

  svtkPiecewiseFunction* DefaultGradientOpacity[SVTK_MAX_VRCOMP];
  int DisableGradientOpacity[SVTK_MAX_VRCOMP];

  int TransferFunctionMode;
  svtkImageData* TransferFunction2D[SVTK_MAX_VRCOMP];
  svtkTimeStamp TransferFunction2DMTime[SVTK_MAX_VRCOMP];

  svtkTimeStamp LabelColorMTime;
  svtkTimeStamp LabelScalarOpacityMTime;
  svtkTimeStamp LabelGradientOpacityMTime;

  int Shade[SVTK_MAX_VRCOMP];
  double Ambient[SVTK_MAX_VRCOMP];
  double Diffuse[SVTK_MAX_VRCOMP];
  double Specular[SVTK_MAX_VRCOMP];
  double SpecularPower[SVTK_MAX_VRCOMP];

  double ClippedVoxelIntensity;
  int UseClippedVoxelIntensity;

  /**
   * Contour values for isosurface blend mode
   */
  svtkNew<svtkContourValues> IsoSurfaceValues;

  /**
   * Function used for slice
   */
  svtkSmartPointer<svtkImplicitFunction> SliceFunction;

  /**
   * Label map transfer functions
   */
  std::unordered_map<int, svtkColorTransferFunction*> LabelColor;
  std::unordered_map<int, svtkPiecewiseFunction*> LabelScalarOpacity;
  std::unordered_map<int, svtkPiecewiseFunction*> LabelGradientOpacity;
  std::set<int> LabelMapLabels;

private:
  svtkVolumeProperty(const svtkVolumeProperty&) = delete;
  void operator=(const svtkVolumeProperty&) = delete;
};

//@{
/**
 * Return the interpolation type as a descriptive character string.
 */
inline const char* svtkVolumeProperty::GetInterpolationTypeAsString(void)
{
  if (this->InterpolationType == SVTK_NEAREST_INTERPOLATION)
  {
    return "Nearest Neighbor";
  }
  if (this->InterpolationType == SVTK_LINEAR_INTERPOLATION)
  {
    return "Linear";
  }
  return "Unknown";
}
//@}

#endif
