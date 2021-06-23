/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTubeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTubeFilter
 * @brief   filter that generates tubes around lines
 *
 * svtkTubeFilter is a filter that generates a tube around each input line.
 * The tubes are made up of triangle strips and rotate around the tube with
 * the rotation of the line normals. (If no normals are present, they are
 * computed automatically.) The radius of the tube can be set to vary with
 * scalar or vector value. If the radius varies with scalar value the radius
 * is linearly adjusted. If the radius varies with vector value, a mass
 * flux preserving variation is used. The number of sides for the tube also
 * can be specified. You can also specify which of the sides are visible. This
 * is useful for generating interesting striping effects. Other options
 * include the ability to cap the tube and generate texture coordinates.
 * Texture coordinates can be used with an associated texture map to create
 * interesting effects such as marking the tube with stripes corresponding
 * to length or time.
 *
 * This filter is typically used to create thick or dramatic lines. Another
 * common use is to combine this filter with svtkStreamTracer to generate
 * streamtubes.
 *
 * @warning
 * The number of tube sides must be greater than 3. If you wish to use fewer
 * sides (i.e., a ribbon), use svtkRibbonFilter.
 *
 * @warning
 * The input line must not have duplicate points, or normals at points that
 * are parallel to the incoming/outgoing line segments. (Duplicate points
 * can be removed with svtkCleanPolyData.) If a line does not meet this
 * criteria, then that line is not tubed.
 *
 * @sa
 * svtkRibbonFilter svtkStreamTracer
 *
 * @par Thanks:
 * Michael Finch for absolute scalar radius
 */

#ifndef svtkTubeFilter_h
#define svtkTubeFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_VARY_RADIUS_OFF 0
#define SVTK_VARY_RADIUS_BY_SCALAR 1
#define SVTK_VARY_RADIUS_BY_VECTOR 2
#define SVTK_VARY_RADIUS_BY_ABSOLUTE_SCALAR 3

#define SVTK_TCOORDS_OFF 0
#define SVTK_TCOORDS_FROM_NORMALIZED_LENGTH 1
#define SVTK_TCOORDS_FROM_LENGTH 2
#define SVTK_TCOORDS_FROM_SCALARS 3

class svtkCellArray;
class svtkCellData;
class svtkDataArray;
class svtkFloatArray;
class svtkPointData;
class svtkPoints;

class SVTKFILTERSCORE_EXPORT svtkTubeFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkTubeFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with radius 0.5, radius variation turned off, the
   * number of sides set to 3, and radius factor of 10.
   */
  static svtkTubeFilter* New();

  //@{
  /**
   * Set the minimum tube radius (minimum because the tube radius may vary).
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Turn on/off the variation of tube radius with scalar value.
   */
  svtkSetClampMacro(VaryRadius, int, SVTK_VARY_RADIUS_OFF, SVTK_VARY_RADIUS_BY_ABSOLUTE_SCALAR);
  svtkGetMacro(VaryRadius, int);
  void SetVaryRadiusToVaryRadiusOff() { this->SetVaryRadius(SVTK_VARY_RADIUS_OFF); }
  void SetVaryRadiusToVaryRadiusByScalar() { this->SetVaryRadius(SVTK_VARY_RADIUS_BY_SCALAR); }
  void SetVaryRadiusToVaryRadiusByVector() { this->SetVaryRadius(SVTK_VARY_RADIUS_BY_VECTOR); }
  void SetVaryRadiusToVaryRadiusByAbsoluteScalar()
  {
    this->SetVaryRadius(SVTK_VARY_RADIUS_BY_ABSOLUTE_SCALAR);
  }
  const char* GetVaryRadiusAsString();
  //@}

  //@{
  /**
   * Set the number of sides for the tube. At a minimum, number of sides is 3.
   */
  svtkSetClampMacro(NumberOfSides, int, 3, SVTK_INT_MAX);
  svtkGetMacro(NumberOfSides, int);
  //@}

  //@{
  /**
   * Set the maximum tube radius in terms of a multiple of the minimum radius.
   */
  svtkSetMacro(RadiusFactor, double);
  svtkGetMacro(RadiusFactor, double);
  //@}

  //@{
  /**
   * Set the default normal to use if no normals are supplied, and the
   * DefaultNormalOn is set.
   */
  svtkSetVector3Macro(DefaultNormal, double);
  svtkGetVectorMacro(DefaultNormal, double, 3);
  //@}

  //@{
  /**
   * Set a boolean to control whether to use default normals.
   * DefaultNormalOn is set.
   */
  svtkSetMacro(UseDefaultNormal, svtkTypeBool);
  svtkGetMacro(UseDefaultNormal, svtkTypeBool);
  svtkBooleanMacro(UseDefaultNormal, svtkTypeBool);
  //@}

  //@{
  /**
   * Set a boolean to control whether tube sides should share vertices.
   * This creates independent strips, with constant normals so the
   * tube is always faceted in appearance.
   */
  svtkSetMacro(SidesShareVertices, svtkTypeBool);
  svtkGetMacro(SidesShareVertices, svtkTypeBool);
  svtkBooleanMacro(SidesShareVertices, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off whether to cap the ends with polygons. Initial value is off.
   */
  svtkSetMacro(Capping, svtkTypeBool);
  svtkGetMacro(Capping, svtkTypeBool);
  svtkBooleanMacro(Capping, svtkTypeBool);
  //@}

  //@{
  /**
   * Control the striping of the tubes. If OnRatio is greater than 1,
   * then every nth tube side is turned on, beginning with the Offset
   * side.
   */
  svtkSetClampMacro(OnRatio, int, 1, SVTK_INT_MAX);
  svtkGetMacro(OnRatio, int);
  //@}

  //@{
  /**
   * Control the striping of the tubes. The offset sets the
   * first tube side that is visible. Offset is generally used with
   * OnRatio to create nifty striping effects.
   */
  svtkSetClampMacro(Offset, int, 0, SVTK_INT_MAX);
  svtkGetMacro(Offset, int);
  //@}

  //@{
  /**
   * Control whether and how texture coordinates are produced. This is
   * useful for striping the tube with length textures, etc. If you
   * use scalars to create the texture, the scalars are assumed to be
   * monotonically increasing (or decreasing).
   */
  svtkSetClampMacro(GenerateTCoords, int, SVTK_TCOORDS_OFF, SVTK_TCOORDS_FROM_SCALARS);
  svtkGetMacro(GenerateTCoords, int);
  void SetGenerateTCoordsToOff() { this->SetGenerateTCoords(SVTK_TCOORDS_OFF); }
  void SetGenerateTCoordsToNormalizedLength()
  {
    this->SetGenerateTCoords(SVTK_TCOORDS_FROM_NORMALIZED_LENGTH);
  }
  void SetGenerateTCoordsToUseLength() { this->SetGenerateTCoords(SVTK_TCOORDS_FROM_LENGTH); }
  void SetGenerateTCoordsToUseScalars() { this->SetGenerateTCoords(SVTK_TCOORDS_FROM_SCALARS); }
  const char* GetGenerateTCoordsAsString();
  //@}

  //@{
  /**
   * Control the conversion of units during the texture coordinates
   * calculation. The TextureLength indicates what length (whether
   * calculated from scalars or length) is mapped to the [0,1)
   * texture space.
   */
  svtkSetClampMacro(TextureLength, double, 0.000001, SVTK_INT_MAX);
  svtkGetMacro(TextureLength, double);
  //@}

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
  svtkTubeFilter();
  ~svtkTubeFilter() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Radius;       // minimum radius of tube
  int VaryRadius;      // controls radius variation
  int NumberOfSides;   // number of sides to create tube
  double RadiusFactor; // maximum allowable radius
  double DefaultNormal[3];
  svtkTypeBool UseDefaultNormal;
  svtkTypeBool SidesShareVertices;
  svtkTypeBool Capping; // control whether tubes are capped
  int OnRatio;         // control the generation of the sides of the tube
  int Offset;          // control the generation of the sides
  int GenerateTCoords; // control texture coordinate generation
  int OutputPointsPrecision;
  double TextureLength; // this length is mapped to [0,1) texture space

  // Helper methods
  int GeneratePoints(svtkIdType offset, svtkIdType npts, const svtkIdType* pts, svtkPoints* inPts,
    svtkPoints* newPts, svtkPointData* pd, svtkPointData* outPD, svtkFloatArray* newNormals,
    svtkDataArray* inScalars, double range[2], svtkDataArray* inVectors, double maxNorm,
    svtkDataArray* inNormals);
  void GenerateStrips(svtkIdType offset, svtkIdType npts, const svtkIdType* pts, svtkIdType inCellId,
    svtkCellData* cd, svtkCellData* outCD, svtkCellArray* newStrips);
  void GenerateTextureCoords(svtkIdType offset, svtkIdType npts, const svtkIdType* pts,
    svtkPoints* inPts, svtkDataArray* inScalars, svtkFloatArray* newTCoords);
  svtkIdType ComputeOffset(svtkIdType offset, svtkIdType npts);

  // Helper data members
  double Theta;

private:
  svtkTubeFilter(const svtkTubeFilter&) = delete;
  void operator=(const svtkTubeFilter&) = delete;
};

#endif
