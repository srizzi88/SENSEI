/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlyph3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGlyph3D
 * @brief   copy oriented and scaled glyph geometry to every input point
 *
 * svtkGlyph3D is a filter that copies a geometric representation (called
 * a glyph) to every point in the input dataset. The glyph is defined with
 * polygonal data from a source filter input. The glyph may be oriented
 * along the input vectors or normals, and it may be scaled according to
 * scalar data or vector magnitude. More than one glyph may be used by
 * creating a table of source objects, each defining a different glyph. If a
 * table of glyphs is defined, then the table can be indexed into by using
 * either scalar value or vector magnitude.
 *
 * To use this object you'll have to provide an input dataset and a source
 * to define the glyph. Then decide whether you want to scale the glyph and
 * how to scale the glyph (using scalar value or vector magnitude). Next
 * decide whether you want to orient the glyph, and whether to use the
 * vector data or normal data to orient it. Finally, decide whether to use a
 * table of glyphs, or just a single glyph. If you use a table of glyphs,
 * you'll have to decide whether to index into it with scalar value or with
 * vector magnitude.
 *
 * @warning
 * The scaling of the glyphs is controlled by the ScaleFactor ivar multiplied
 * by the scalar value at each point (if SVTK_SCALE_BY_SCALAR is set), or
 * multiplied by the vector magnitude (if SVTK_SCALE_BY_VECTOR is set),
 * Alternatively (if SVTK_SCALE_BY_VECTORCOMPONENTS is set), the scaling
 * may be specified for x,y,z using the vector components. The
 * scale factor can be further controlled by enabling clamping using the
 * Clamping ivar. If clamping is enabled, the scale is normalized by the
 * Range ivar, and then multiplied by the scale factor. The normalization
 * process includes clamping the scale value between (0,1).
 *
 * @warning
 * Typically this object operates on input data with scalar and/or vector
 * data. However, scalar and/or vector aren't necessary, and it can be used
 * to copy data from a single source to each point. In this case the scale
 * factor can be used to uniformly scale the glyphs.
 *
 * @warning
 * The object uses "vector" data to scale glyphs, orient glyphs, and/or index
 * into a table of glyphs. You can choose to use either the vector or normal
 * data at each input point. Use the method SetVectorModeToUseVector() to use
 * the vector input data, and SetVectorModeToUseNormal() to use the
 * normal input data.
 *
 * @warning
 * If you do use a table of glyphs, make sure to set the Range ivar to make
 * sure the index into the glyph table is computed correctly.
 *
 * @warning
 * You can turn off scaling of the glyphs completely by using the Scaling
 * ivar. You can also turn off scaling due to data (either vector or scalar)
 * by using the SetScaleModeToDataScalingOff() method.
 *
 * @warning
 * You can set what arrays to use for the scalars, vectors, normals, and
 * color scalars by using the SetInputArrayToProcess methods in
 * svtkAlgorithm. The first array is scalars, the next vectors, the next
 * normals and finally color scalars.
 *
 * @sa
 * svtkTensorGlyph
 */

#ifndef svtkGlyph3D_h
#define svtkGlyph3D_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_SCALE_BY_SCALAR 0
#define SVTK_SCALE_BY_VECTOR 1
#define SVTK_SCALE_BY_VECTORCOMPONENTS 2
#define SVTK_DATA_SCALING_OFF 3

#define SVTK_COLOR_BY_SCALE 0
#define SVTK_COLOR_BY_SCALAR 1
#define SVTK_COLOR_BY_VECTOR 2

#define SVTK_USE_VECTOR 0
#define SVTK_USE_NORMAL 1
#define SVTK_VECTOR_ROTATION_OFF 2

#define SVTK_INDEXING_OFF 0
#define SVTK_INDEXING_BY_SCALAR 1
#define SVTK_INDEXING_BY_VECTOR 2

class svtkTransform;

class SVTKFILTERSCORE_EXPORT svtkGlyph3D : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkGlyph3D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with scaling on, scaling mode is by scalar value,
   * scale factor = 1.0, the range is (0,1), orient geometry is on, and
   * orientation is by vector. Clamping and indexing are turned off. No
   * initial sources are defined.
   */
  static svtkGlyph3D* New();

  /**
   * Set the source to use for the glyph.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(svtkPolyData* pd) { this->SetSourceData(0, pd); }

  /**
   * Specify a source object at a specified table location.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(int id, svtkPolyData* pd);

  //@{
  /**
   * Specify a source object at a specified table location. New style.
   * Source connection is stored in port 1. This method is equivalent
   * to SetInputConnection(1, id, outputPort).
   */
  void SetSourceConnection(int id, svtkAlgorithmOutput* algOutput);
  void SetSourceConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetSourceConnection(0, algOutput);
  }
  //@}

  /**
   * Get a pointer to a source object at a specified table location.
   */
  svtkPolyData* GetSource(int id = 0);

  //@{
  /**
   * Turn on/off scaling of source geometry.
   */
  svtkSetMacro(Scaling, svtkTypeBool);
  svtkBooleanMacro(Scaling, svtkTypeBool);
  svtkGetMacro(Scaling, svtkTypeBool);
  //@}

  //@{
  /**
   * Either scale by scalar or by vector/normal magnitude.
   */
  svtkSetMacro(ScaleMode, int);
  svtkGetMacro(ScaleMode, int);
  void SetScaleModeToScaleByScalar() { this->SetScaleMode(SVTK_SCALE_BY_SCALAR); }
  void SetScaleModeToScaleByVector() { this->SetScaleMode(SVTK_SCALE_BY_VECTOR); }
  void SetScaleModeToScaleByVectorComponents()
  {
    this->SetScaleMode(SVTK_SCALE_BY_VECTORCOMPONENTS);
  }
  void SetScaleModeToDataScalingOff() { this->SetScaleMode(SVTK_DATA_SCALING_OFF); }
  const char* GetScaleModeAsString();
  //@}

  //@{
  /**
   * Either color by scale, scalar or by vector/normal magnitude.
   */
  svtkSetMacro(ColorMode, int);
  svtkGetMacro(ColorMode, int);
  void SetColorModeToColorByScale() { this->SetColorMode(SVTK_COLOR_BY_SCALE); }
  void SetColorModeToColorByScalar() { this->SetColorMode(SVTK_COLOR_BY_SCALAR); }
  void SetColorModeToColorByVector() { this->SetColorMode(SVTK_COLOR_BY_VECTOR); }
  const char* GetColorModeAsString();
  //@}

  //@{
  /**
   * Specify scale factor to scale object by.
   */
  svtkSetMacro(ScaleFactor, double);
  svtkGetMacro(ScaleFactor, double);
  //@}

  //@{
  /**
   * Specify range to map scalar values into.
   */
  svtkSetVector2Macro(Range, double);
  svtkGetVectorMacro(Range, double, 2);
  //@}

  //@{
  /**
   * Turn on/off orienting of input geometry along vector/normal.
   */
  svtkSetMacro(Orient, svtkTypeBool);
  svtkBooleanMacro(Orient, svtkTypeBool);
  svtkGetMacro(Orient, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off clamping of "scalar" values to range. (Scalar value may be
   * vector magnitude if ScaleByVector() is enabled.)
   */
  svtkSetMacro(Clamping, svtkTypeBool);
  svtkBooleanMacro(Clamping, svtkTypeBool);
  svtkGetMacro(Clamping, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether to use vector or normal to perform vector operations.
   */
  svtkSetMacro(VectorMode, int);
  svtkGetMacro(VectorMode, int);
  void SetVectorModeToUseVector() { this->SetVectorMode(SVTK_USE_VECTOR); }
  void SetVectorModeToUseNormal() { this->SetVectorMode(SVTK_USE_NORMAL); }
  void SetVectorModeToVectorRotationOff() { this->SetVectorMode(SVTK_VECTOR_ROTATION_OFF); }
  const char* GetVectorModeAsString();
  //@}

  //@{
  /**
   * Index into table of sources by scalar, by vector/normal magnitude, or
   * no indexing. If indexing is turned off, then the first source glyph in
   * the table of glyphs is used. Note that indexing mode will only use the
   * InputScalarsSelection array and not the InputColorScalarsSelection
   * as the scalar source if an array is specified.
   */
  svtkSetMacro(IndexMode, int);
  svtkGetMacro(IndexMode, int);
  void SetIndexModeToScalar() { this->SetIndexMode(SVTK_INDEXING_BY_SCALAR); }
  void SetIndexModeToVector() { this->SetIndexMode(SVTK_INDEXING_BY_VECTOR); }
  void SetIndexModeToOff() { this->SetIndexMode(SVTK_INDEXING_OFF); }
  const char* GetIndexModeAsString();
  //@}

  //@{
  /**
   * Enable/disable the generation of point ids as part of the output. The
   * point ids are the id of the input generating point. The point ids are
   * stored in the output point field data and named "InputPointIds". Point
   * generation is useful for debugging and pick operations.
   */
  svtkSetMacro(GeneratePointIds, svtkTypeBool);
  svtkGetMacro(GeneratePointIds, svtkTypeBool);
  svtkBooleanMacro(GeneratePointIds, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the name of the PointIds array if generated. By default the Ids
   * are named "InputPointIds", but this can be changed with this function.
   */
  svtkSetStringMacro(PointIdsName);
  svtkGetStringMacro(PointIdsName);
  //@}

  //@{
  /**
   * Enable/disable the generation of cell data as part of the output.
   * The cell data at each cell will match the point data of the input
   * at the glyphed point.
   */
  svtkSetMacro(FillCellData, svtkTypeBool);
  svtkGetMacro(FillCellData, svtkTypeBool);
  svtkBooleanMacro(FillCellData, svtkTypeBool);
  //@}

  /**
   * This can be overwritten by subclass to return 0 when a point is
   * blanked. Default implementation is to always return 1;
   */
  virtual int IsPointVisible(svtkDataSet*, svtkIdType) { return 1; }

  //@{
  /**
   * When set, this is use to transform the source polydata before using it to
   * generate the glyph. This is useful if one wanted to reorient the source,
   * for example.
   */
  void SetSourceTransform(svtkTransform*);
  svtkGetObjectMacro(SourceTransform, svtkTransform);
  //@}

  /**
   * Overridden to include SourceTransform's MTime.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkGlyph3D();
  ~svtkGlyph3D() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  svtkPolyData* GetSource(int idx, svtkInformationVector* sourceInfo);

  //@{
  /**
   * Method called in RequestData() to do the actual data processing. This will
   * glyph the \c input, filling up the \c output based on the filter
   * parameters.
   */
  virtual bool Execute(svtkDataSet* input, svtkInformationVector* sourceVector, svtkPolyData* output);
  virtual bool Execute(svtkDataSet* input, svtkInformationVector* sourceVector, svtkPolyData* output,
    svtkDataArray* inSScalars, svtkDataArray* inVectors);
  //@}

  svtkPolyData** Source;         // Geometry to copy to each point
  svtkTypeBool Scaling;          // Determine whether scaling of geometry is performed
  int ScaleMode;                // Scale by scalar value or vector magnitude
  int ColorMode;                // new scalars based on scale, scalar or vector
  double ScaleFactor;           // Scale factor to use to scale geometry
  double Range[2];              // Range to use to perform scalar scaling
  int Orient;                   // boolean controls whether to "orient" data
  int VectorMode;               // Orient/scale via normal or via vector data
  svtkTypeBool Clamping;         // whether to clamp scale factor
  int IndexMode;                // what to use to index into glyph table
  svtkTypeBool GeneratePointIds; // produce input points ids for each output point
  svtkTypeBool FillCellData;     // whether to fill output cell data
  char* PointIdsName;
  svtkTransform* SourceTransform;
  int OutputPointsPrecision;

private:
  svtkGlyph3D(const svtkGlyph3D&) = delete;
  void operator=(const svtkGlyph3D&) = delete;
};

//@{
/**
 * Return the method of scaling as a descriptive character string.
 */
inline const char* svtkGlyph3D::GetScaleModeAsString(void)
{
  if (this->ScaleMode == SVTK_SCALE_BY_SCALAR)
  {
    return "ScaleByScalar";
  }
  else if (this->ScaleMode == SVTK_SCALE_BY_VECTOR)
  {
    return "ScaleByVector";
  }
  else
  {
    return "DataScalingOff";
  }
}
//@}

//@{
/**
 * Return the method of coloring as a descriptive character string.
 */
inline const char* svtkGlyph3D::GetColorModeAsString(void)
{
  if (this->ColorMode == SVTK_COLOR_BY_SCALAR)
  {
    return "ColorByScalar";
  }
  else if (this->ColorMode == SVTK_COLOR_BY_VECTOR)
  {
    return "ColorByVector";
  }
  else
  {
    return "ColorByScale";
  }
}
//@}

//@{
/**
 * Return the vector mode as a character string.
 */
inline const char* svtkGlyph3D::GetVectorModeAsString(void)
{
  if (this->VectorMode == SVTK_USE_VECTOR)
  {
    return "UseVector";
  }
  else if (this->VectorMode == SVTK_USE_NORMAL)
  {
    return "UseNormal";
  }
  else
  {
    return "VectorRotationOff";
  }
}
//@}

//@{
/**
 * Return the index mode as a character string.
 */
inline const char* svtkGlyph3D::GetIndexModeAsString(void)
{
  if (this->IndexMode == SVTK_INDEXING_OFF)
  {
    return "IndexingOff";
  }
  else if (this->IndexMode == SVTK_INDEXING_BY_SCALAR)
  {
    return "IndexingByScalar";
  }
  else
  {
    return "IndexingByVector";
  }
}
//@}

#endif
