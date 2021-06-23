/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRibbonFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRibbonFilter
 * @brief   create oriented ribbons from lines defined in polygonal dataset
 *
 * svtkRibbonFilter is a filter to create oriented ribbons from lines defined
 * in polygonal dataset. The orientation of the ribbon is along the line
 * segments and perpendicular to "projected" line normals. Projected line
 * normals are the original line normals projected to be perpendicular to
 * the local line segment. An offset angle can be specified to rotate the
 * ribbon with respect to the normal.
 *
 * @warning
 * The input line must not have duplicate points, or normals at points that
 * are parallel to the incoming/outgoing line segments. (Duplicate points
 * can be removed with svtkCleanPolyData.) If a line does not meet this
 * criteria, then that line is not tubed.
 *
 * @sa
 * svtkTubeFilter
 */

#ifndef svtkRibbonFilter_h
#define svtkRibbonFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

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

class SVTKFILTERSMODELING_EXPORT svtkRibbonFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkRibbonFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct ribbon so that width is 0.1, the width does
   * not vary with scalar values, and the width factor is 2.0.
   */
  static svtkRibbonFilter* New();

  //@{
  /**
   * Set the "half" width of the ribbon. If the width is allowed to vary,
   * this is the minimum width. The default is 0.5
   */
  svtkSetClampMacro(Width, double, 0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Width, double);
  //@}

  //@{
  /**
   * Set the offset angle of the ribbon from the line normal. (The angle
   * is expressed in degrees.) The default is 0.0
   */
  svtkSetClampMacro(Angle, double, 0, 360);
  svtkGetMacro(Angle, double);
  //@}

  //@{
  /**
   * Turn on/off the variation of ribbon width with scalar value.
   * The default is Off
   */
  svtkSetMacro(VaryWidth, svtkTypeBool);
  svtkGetMacro(VaryWidth, svtkTypeBool);
  svtkBooleanMacro(VaryWidth, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the maximum ribbon width in terms of a multiple of the minimum width.
   * The default is 2.0
   */
  svtkSetMacro(WidthFactor, double);
  svtkGetMacro(WidthFactor, double);
  //@}

  //@{
  /**
   * Set the default normal to use if no normals are supplied, and
   * DefaultNormalOn is set. The default is (0,0,1)
   */
  svtkSetVector3Macro(DefaultNormal, double);
  svtkGetVectorMacro(DefaultNormal, double, 3);
  //@}

  //@{
  /**
   * Set a boolean to control whether to use default normals.
   * The default is Off
   */
  svtkSetMacro(UseDefaultNormal, svtkTypeBool);
  svtkGetMacro(UseDefaultNormal, svtkTypeBool);
  svtkBooleanMacro(UseDefaultNormal, svtkTypeBool);
  //@}

  //@{
  /**
   * Control whether and how texture coordinates are produced. This is
   * useful for striping the ribbon with time textures, etc.
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
   * texture space. The default is 1.0
   */
  svtkSetClampMacro(TextureLength, double, 0.000001, SVTK_INT_MAX);
  svtkGetMacro(TextureLength, double);
  //@}

protected:
  svtkRibbonFilter();
  ~svtkRibbonFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double Width;
  double Angle;
  svtkTypeBool VaryWidth; // controls whether width varies with scalar data
  double WidthFactor;
  double DefaultNormal[3];
  svtkTypeBool UseDefaultNormal;
  int GenerateTCoords;  // control texture coordinate generation
  double TextureLength; // this length is mapped to [0,1) texture space

  // Helper methods
  int GeneratePoints(svtkIdType offset, svtkIdType npts, const svtkIdType* pts, svtkPoints* inPts,
    svtkPoints* newPts, svtkPointData* pd, svtkPointData* outPD, svtkFloatArray* newNormals,
    svtkDataArray* inScalars, double range[2], svtkDataArray* inNormals);
  void GenerateStrip(svtkIdType offset, svtkIdType npts, const svtkIdType* pts, svtkIdType inCellId,
    svtkCellData* cd, svtkCellData* outCD, svtkCellArray* newStrips);
  void GenerateTextureCoords(svtkIdType offset, svtkIdType npts, const svtkIdType* pts,
    svtkPoints* inPts, svtkDataArray* inScalars, svtkFloatArray* newTCoords);
  svtkIdType ComputeOffset(svtkIdType offset, svtkIdType npts);

  // Helper data members
  double Theta;

private:
  svtkRibbonFilter(const svtkRibbonFilter&) = delete;
  void operator=(const svtkRibbonFilter&) = delete;
};

#endif
