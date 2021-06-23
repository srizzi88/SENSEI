/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGlyphSource2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGlyphSource2D
 * @brief   create 2D glyphs represented by svtkPolyData
 *
 * svtkGlyphSource2D can generate a family of 2D glyphs each of which lies
 * in the x-y plane (i.e., the z-coordinate is zero). The class is a helper
 * class to be used with svtkGlyph2D and svtkXYPlotActor.
 *
 * To use this class, specify the glyph type to use and its
 * attributes. Attributes include its position (i.e., center point), scale,
 * color, and whether the symbol is filled or not (a polygon or closed line
 * sequence). You can also put a short line through the glyph running from -x
 * to +x (the glyph looks like it's on a line), or a cross.
 */

#ifndef svtkGlyphSource2D_h
#define svtkGlyphSource2D_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_NO_GLYPH 0
#define SVTK_VERTEX_GLYPH 1
#define SVTK_DASH_GLYPH 2
#define SVTK_CROSS_GLYPH 3
#define SVTK_THICKCROSS_GLYPH 4
#define SVTK_TRIANGLE_GLYPH 5
#define SVTK_SQUARE_GLYPH 6
#define SVTK_CIRCLE_GLYPH 7
#define SVTK_DIAMOND_GLYPH 8
#define SVTK_ARROW_GLYPH 9
#define SVTK_THICKARROW_GLYPH 10
#define SVTK_HOOKEDARROW_GLYPH 11
#define SVTK_EDGEARROW_GLYPH 12

#define SVTK_MAX_CIRCLE_RESOLUTION 1024

class svtkPoints;
class svtkUnsignedCharArray;
class svtkCellArray;

class SVTKFILTERSSOURCES_EXPORT svtkGlyphSource2D : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkGlyphSource2D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a vertex glyph centered at the origin, scale 1.0, white in
   * color, filled, with line segment passing through the point.
   */
  static svtkGlyphSource2D* New();

  //@{
  /**
   * Set the center of the glyph. By default the center is (0,0,0).
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set the scale of the glyph. Note that the glyphs are designed
   * to fit in the (1,1) rectangle.
   */
  svtkSetClampMacro(Scale, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Scale, double);
  //@}

  //@{
  /**
   * Set the scale of optional portions of the glyph (e.g., the
   * dash and cross is DashOn() and CrossOn()).
   */
  svtkSetClampMacro(Scale2, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Scale2, double);
  //@}

  //@{
  /**
   * Set the color of the glyph. The default color is white.
   */
  svtkSetVector3Macro(Color, double);
  svtkGetVectorMacro(Color, double, 3);
  //@}

  //@{
  /**
   * Specify whether the glyph is filled (a polygon) or not (a
   * closed polygon defined by line segments). This only applies
   * to 2D closed glyphs.
   */
  svtkSetMacro(Filled, svtkTypeBool);
  svtkGetMacro(Filled, svtkTypeBool);
  svtkBooleanMacro(Filled, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether a short line segment is drawn through the
   * glyph. (This is in addition to the glyph. If the glyph type
   * is set to "Dash" there is no need to enable this flag.)
   */
  svtkSetMacro(Dash, svtkTypeBool);
  svtkGetMacro(Dash, svtkTypeBool);
  svtkBooleanMacro(Dash, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether a cross is drawn as part of the glyph. (This
   * is in addition to the glyph. If the glyph type is set to
   * "Cross" there is no need to enable this flag.)
   */
  svtkSetMacro(Cross, svtkTypeBool);
  svtkGetMacro(Cross, svtkTypeBool);
  svtkBooleanMacro(Cross, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify an angle (in degrees) to rotate the glyph around
   * the z-axis. Using this ivar, it is possible to generate
   * rotated glyphs (e.g., crosses, arrows, etc.)
   */
  svtkSetMacro(RotationAngle, double);
  svtkGetMacro(RotationAngle, double);
  //@}

  //@{
  /**
   * Specify the number of points that form the circular glyph.
   */
  svtkSetClampMacro(Resolution, int, 3, SVTK_MAX_CIRCLE_RESOLUTION);
  svtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Specify the type of glyph to generate.
   */
  svtkSetClampMacro(GlyphType, int, SVTK_NO_GLYPH, SVTK_EDGEARROW_GLYPH);
  svtkGetMacro(GlyphType, int);
  void SetGlyphTypeToNone() { this->SetGlyphType(SVTK_NO_GLYPH); }
  void SetGlyphTypeToVertex() { this->SetGlyphType(SVTK_VERTEX_GLYPH); }
  void SetGlyphTypeToDash() { this->SetGlyphType(SVTK_DASH_GLYPH); }
  void SetGlyphTypeToCross() { this->SetGlyphType(SVTK_CROSS_GLYPH); }
  void SetGlyphTypeToThickCross() { this->SetGlyphType(SVTK_THICKCROSS_GLYPH); }
  void SetGlyphTypeToTriangle() { this->SetGlyphType(SVTK_TRIANGLE_GLYPH); }
  void SetGlyphTypeToSquare() { this->SetGlyphType(SVTK_SQUARE_GLYPH); }
  void SetGlyphTypeToCircle() { this->SetGlyphType(SVTK_CIRCLE_GLYPH); }
  void SetGlyphTypeToDiamond() { this->SetGlyphType(SVTK_DIAMOND_GLYPH); }
  void SetGlyphTypeToArrow() { this->SetGlyphType(SVTK_ARROW_GLYPH); }
  void SetGlyphTypeToThickArrow() { this->SetGlyphType(SVTK_THICKARROW_GLYPH); }
  void SetGlyphTypeToHookedArrow() { this->SetGlyphType(SVTK_HOOKEDARROW_GLYPH); }
  void SetGlyphTypeToEdgeArrow() { this->SetGlyphType(SVTK_EDGEARROW_GLYPH); }
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * svtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * svtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkGlyphSource2D();
  ~svtkGlyphSource2D() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Center[3];
  double Scale;
  double Scale2;
  double Color[3];
  svtkTypeBool Filled;
  svtkTypeBool Dash;
  svtkTypeBool Cross;
  int GlyphType;
  double RotationAngle;
  int Resolution;
  int OutputPointsPrecision;

  void TransformGlyph(svtkPoints* pts);
  void ConvertColor();
  unsigned char RGB[3];

  void CreateVertex(svtkPoints* pts, svtkCellArray* verts, svtkUnsignedCharArray* colors);
  void CreateDash(svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys,
    svtkUnsignedCharArray* colors, double scale);
  void CreateCross(svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys,
    svtkUnsignedCharArray* colors, double scale);
  void CreateThickCross(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);
  void CreateTriangle(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);
  void CreateSquare(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);
  void CreateCircle(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);
  void CreateDiamond(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);
  void CreateArrow(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);
  void CreateThickArrow(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);
  void CreateHookedArrow(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);
  void CreateEdgeArrow(
    svtkPoints* pts, svtkCellArray* lines, svtkCellArray* polys, svtkUnsignedCharArray* colors);

private:
  svtkGlyphSource2D(const svtkGlyphSource2D&) = delete;
  void operator=(const svtkGlyphSource2D&) = delete;
};

#endif
