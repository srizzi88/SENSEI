/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDiscretizableColorTransferFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDiscretizableColorTransferFunction
 * @brief   a combination of
 * svtkColorTransferFunction and svtkLookupTable.
 *
 * This is a cross between a svtkColorTransferFunction and a svtkLookupTable
 * selectively combining the functionality of both. This class is a
 * svtkColorTransferFunction allowing users to specify the RGB control points
 * that control the color transfer function. At the same time, by setting
 * \a Discretize to 1 (true), one can force the transfer function to only have
 * \a NumberOfValues discrete colors.
 *
 * When \a IndexedLookup is true, this class behaves differently. The annotated
 * values are considered to the be only valid values for which entries in the
 * color table should be returned. The colors for annotated values are those
 * specified using \a AddIndexedColors. Typically, there must be at least as many
 * indexed colors specified as the annotations. For backwards compatibility, if
 * no indexed-colors are specified, the colors in the lookup \a Table are assigned
 * to annotated values by taking the modulus of their index in the list
 * of annotations. If a scalar value is not present in \a AnnotatedValues,
 * then \a NanColor will be used.
 *
 * One can set a scalar opacity function to map scalars to color types handling
 * transparency (SVTK_RGBA, SVTK_LUMINANCE_ALPHA). Opacity mapping is off by
 * default. Call EnableOpacityMappingOn() to handle mapping of alpha values.
 *
 * NOTE: One must call Build() after making any changes to the points
 * in the ColorTransferFunction to ensure that the discrete and non-discrete
 * versions match up.
 */

#ifndef svtkDiscretizableColorTransferFunction_h
#define svtkDiscretizableColorTransferFunction_h

#include "svtkColorTransferFunction.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkSmartPointer.h"        // for svtkSmartPointer

class svtkColorTransferFunction;
class svtkLookupTable;
class svtkPiecewiseFunction;

class SVTKRENDERINGCORE_EXPORT svtkDiscretizableColorTransferFunction
  : public svtkColorTransferFunction
{
public:
  static svtkDiscretizableColorTransferFunction* New();
  svtkTypeMacro(svtkDiscretizableColorTransferFunction, svtkColorTransferFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Returns the negation of \a EnableOpacityMapping.
   */
  int IsOpaque() override;
  int IsOpaque(svtkAbstractArray* scalars, int colorMode, int component) override;
  //@}

  /**
   * Add colors to use when \a IndexedLookup is true.
   * \a SetIndexedColor() will automatically call
   * SetNumberOfIndexedColors(index+1) if the current number of indexed colors
   * is not sufficient for the specified index and all will be initialized to
   * the RGBA/RGB values passed to this call.
   */
  void SetIndexedColorRGB(unsigned int index, const double rgb[3])
  {
    this->SetIndexedColor(index, rgb[0], rgb[1], rgb[2]);
  }
  void SetIndexedColorRGBA(unsigned int index, const double rgba[4])
  {
    this->SetIndexedColor(index, rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  void SetIndexedColor(unsigned int index, double r, double g, double b, double a = 1.0);

  /**
   * Get the "indexed color" assigned to an index.

   * The index is used in \a IndexedLookup mode to assign colors to annotations (in the order
   * the annotations were set).
   * Subclasses must implement this and interpret how to treat the index.
   * svtkLookupTable simply returns GetTableValue(\a index % \a this->GetNumberOfTableValues()).
   * svtkColorTransferFunction returns the color associated with node \a index % \a this->GetSize().

   * Note that implementations *must* set the opacity (alpha) component of the color, even if they
   * do not provide opacity values in their colormaps. In that case, alpha = 1 should be used.
   */
  void GetIndexedColor(svtkIdType i, double rgba[4]) override;

  //@{
  /**
   * Set the number of indexed colors. These are used when IndexedLookup is
   * true. If no indexed colors are specified, for backwards compatibility,
   * this class reverts to using the RGBPoints for colors.
   */
  void SetNumberOfIndexedColors(unsigned int count);
  unsigned int GetNumberOfIndexedColors();
  //@}

  /**
   * Generate discretized lookup table, if applicable.
   * This method must be called after changes to the ColorTransferFunction
   * otherwise the discretized version will be inconsistent with the
   * non-discretized one.
   */
  void Build() override;

  //@{
  /**
   * Set if the values are to be mapped after discretization. The
   * number of discrete values is set by using SetNumberOfValues().
   * Not set by default, i.e. color value is determined by
   * interpolating at the scalar value.
   */
  svtkSetMacro(Discretize, svtkTypeBool);
  svtkGetMacro(Discretize, svtkTypeBool);
  svtkBooleanMacro(Discretize, svtkTypeBool);
  //@}

  //@{
  /**
   * Get/Set if log scale must be used while mapping scalars
   * to colors. The default is 0.
   */
  virtual void SetUseLogScale(int useLogScale);
  svtkGetMacro(UseLogScale, int);
  //@}

  //@{
  /**
   * Set the number of values i.e. colors to be generated in the
   * discrete lookup table. This has no effect if Discretize is off.
   * The default is 256.
   */
  svtkSetMacro(NumberOfValues, svtkIdType);
  svtkGetMacro(NumberOfValues, svtkIdType);
  //@}

  /**
   * Map one value through the lookup table and return a color defined
   * as a RGBA unsigned char tuple (4 bytes).
   */
  const unsigned char* MapValue(double v) override;

  /**
   * Map one value through the lookup table and return the color as
   * an RGB array of doubles between 0 and 1.
   */
  void GetColor(double v, double rgb[3]) override;

  /**
   * Return the opacity of a given scalar.
   */
  double GetOpacity(double v) override;

  /**
   * Map a set of scalars through the lookup table.
   * Overridden to map the opacity value. This internal method is inherited
   * from svtkScalarsToColors and should never be called directly.
   */
  void MapScalarsThroughTable2(void* input, unsigned char* output, int inputDataType,
    int numberOfValues, int inputIncrement, int outputFormat) override;

  /**
   * Specify an additional opacity (alpha) value to blend with. Values
   * != 1 modify the resulting color consistent with the requested
   * form of the output. This is typically used by an actor in order to
   * blend its opacity.
   * Overridden to pass the alpha to the internal svtkLookupTable.
   */
  void SetAlpha(double alpha) override;

  //@{
  /**
   * Set the color to use when a NaN (not a number) is encountered.  This is an
   * RGB 3-tuple color of doubles in the range [0, 1].
   * Overridden to pass the NanColor to the internal svtkLookupTable.
   */
  void SetNanColor(double r, double g, double b) override;
  void SetNanColor(const double rgb[3]) override { this->SetNanColor(rgb[0], rgb[1], rgb[2]); }
  //@}

  /**
   * Set the opacity to use when a NaN (not a number) is encountered.  This is an
   * double in the range [0, 1].
   * Overridden to pass the NanOpacity to the internal svtkLookupTable.
   */
  void SetNanOpacity(double a) override;

  /**
   * This should return 1 if the subclass is using log scale for
   * mapping scalars to colors.
   */
  int UsingLogScale() override { return this->UseLogScale; }

  /**
   * Get the number of available colors for mapping to.
   */
  svtkIdType GetNumberOfAvailableColors() override;

  //@{
  /**
   * Set/get the opacity function to use.
   */
  virtual void SetScalarOpacityFunction(svtkPiecewiseFunction* function);
  virtual svtkPiecewiseFunction* GetScalarOpacityFunction() const;
  //@}

  //@{
  /**
   * Enable/disable the usage of the scalar opacity function.
   */
  svtkSetMacro(EnableOpacityMapping, bool);
  svtkGetMacro(EnableOpacityMapping, bool);
  svtkBooleanMacro(EnableOpacityMapping, bool);
  //@}

  /**
   * Overridden to include the ScalarOpacityFunction's MTime.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkDiscretizableColorTransferFunction();
  ~svtkDiscretizableColorTransferFunction() override;

  /**
   * Flag indicating whether transfer function is discretized.
   */
  svtkTypeBool Discretize;

  /**
   * Flag indicating whether log scaling is to be used.
   */
  int UseLogScale;

  /**
   * Number of values to use in discretized color map.
   */
  svtkIdType NumberOfValues;

  /**
   * Internal lookup table used for some aspects of the color mapping
   */
  svtkLookupTable* LookupTable;

  svtkTimeStamp LookupTableUpdateTime;

  bool EnableOpacityMapping;
  svtkSmartPointer<svtkPiecewiseFunction> ScalarOpacityFunction;

  void MapDataArrayToOpacity(svtkDataArray* scalars, int component, svtkUnsignedCharArray* colors);

private:
  svtkDiscretizableColorTransferFunction(const svtkDiscretizableColorTransferFunction&) = delete;
  void operator=(const svtkDiscretizableColorTransferFunction&) = delete;

  template <typename T, typename VectorGetter>
  void MapVectorToOpacity(VectorGetter getter, T* scalars, int component, int numberOfComponents,
    svtkIdType numberOfTuples, unsigned char* colors);

  template <template <class> class VectorGetter>
  void AllTypesMapVectorToOpacity(int scalarType, void* scalarsPtr, int component,
    int numberOfComponents, svtkIdType numberOfTuples, unsigned char* colors);

  class svtkInternals;
  svtkInternals* Internals;
};

#endif
