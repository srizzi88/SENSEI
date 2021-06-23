/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageToPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageToPolyDataFilter
 * @brief   generate linear primitives (svtkPolyData) from an image
 *
 * svtkImageToPolyDataFilter converts raster data (i.e., an image) into
 * polygonal data (i.e., quads or n-sided polygons), with each polygon
 * assigned a constant color. This is useful for writers that generate vector
 * formats (i.e., CGM or PostScript). To use this filter, you specify how to
 * quantize the color (or whether to use an image with a lookup table), and
 * what style the output should be. The output is always polygons, but the
 * choice is n x m quads (where n and m define the input image dimensions)
 * "Pixelize" option; arbitrary polygons "Polygonalize" option; or variable
 * number of quads of constant color generated along scan lines "RunLength"
 * option.
 *
 * The algorithm quantizes color in order to create coherent regions that the
 * polygons can represent with good compression. By default, the input image
 * is quantized to 256 colors using a 3-3-2 bits for red-green-blue. However,
 * you can also supply a single component image and a lookup table, with the
 * single component assumed to be an index into the table.  (Note: a quantized
 * image can be generated with the filter svtkImageQuantizeRGBToIndex.) The
 * number of colors on output is equal to the number of colors in the input
 * lookup table (or 256 if the built in linear ramp is used).
 *
 * The output of the filter is polygons with a single color per polygon cell.
 * If the output style is set to "Polygonalize", the polygons may have an
 * large number of points (bounded by something like 2*(n+m)); and the
 * polygon may not be convex which may cause rendering problems on some
 * systems (use svtkTriangleFilter). Otherwise, each polygon will have four
 * vertices. The output also contains scalar data defining RGB color in
 * unsigned char form.
 *
 * @warning
 * The input linear lookup table must
 * be of the form of 3-component unsigned char.
 *
 * @warning
 * This filter defines constant cell colors. If you have a plotting
 * device that supports Gouraud shading (linear interpolation of color), then
 * superior algorithms are available for generating polygons from images.
 *
 * @warning
 * Note that many plotting devices/formats support only a limited number of
 * colors.
 *
 * @sa
 * svtkCGMWriter svtkImageQuantizeRGBToIndex svtkTriangleFilter
 */

#ifndef svtkImageToPolyDataFilter_h
#define svtkImageToPolyDataFilter_h

#include "svtkFiltersHybridModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_STYLE_PIXELIZE 0
#define SVTK_STYLE_POLYGONALIZE 1
#define SVTK_STYLE_RUN_LENGTH 2

#define SVTK_COLOR_MODE_LUT 0
#define SVTK_COLOR_MODE_LINEAR_256 1

class svtkDataArray;
class svtkEdgeTable;
class svtkIdTypeArray;
class svtkIntArray;
class svtkScalarsToColors;
class svtkStructuredPoints;
class svtkTimeStamp;
class svtkUnsignedCharArray;

class SVTKFILTERSHYBRID_EXPORT svtkImageToPolyDataFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkImageToPolyDataFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Instantiate object with initial number of colors 256.
   */
  static svtkImageToPolyDataFilter* New();

  //@{
  /**
   * Specify how to create the output. Pixelize means converting the image
   * to quad polygons with a constant color per quad. Polygonalize means
   * merging colors together into polygonal regions, and then smoothing
   * the regions (if smoothing is turned on). RunLength means creating
   * quad polygons that may encompass several pixels on a scan line. The
   * default behavior is Polygonalize.
   */
  svtkSetClampMacro(OutputStyle, int, SVTK_STYLE_PIXELIZE, SVTK_STYLE_RUN_LENGTH);
  svtkGetMacro(OutputStyle, int);
  void SetOutputStyleToPixelize() { this->SetOutputStyle(SVTK_STYLE_PIXELIZE); }
  void SetOutputStyleToPolygonalize() { this->SetOutputStyle(SVTK_STYLE_POLYGONALIZE); }
  void SetOutputStyleToRunLength() { this->SetOutputStyle(SVTK_STYLE_RUN_LENGTH); }
  //@}

  //@{
  /**
   * Specify how to quantize color.
   */
  svtkSetClampMacro(ColorMode, int, SVTK_COLOR_MODE_LUT, SVTK_COLOR_MODE_LINEAR_256);
  svtkGetMacro(ColorMode, int);
  void SetColorModeToLUT() { this->SetColorMode(SVTK_COLOR_MODE_LUT); }
  void SetColorModeToLinear256() { this->SetColorMode(SVTK_COLOR_MODE_LINEAR_256); }
  //@}

  //@{
  /**
   * Set/Get the svtkLookupTable to use. The lookup table is used when the
   * color mode is set to LUT and a single component scalar is input.
   */
  virtual void SetLookupTable(svtkScalarsToColors*);
  svtkGetObjectMacro(LookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * If the output style is set to polygonalize, then you can control
   * whether to smooth boundaries.
   */
  svtkSetMacro(Smoothing, svtkTypeBool);
  svtkGetMacro(Smoothing, svtkTypeBool);
  svtkBooleanMacro(Smoothing, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the number of smoothing iterations to smooth polygons. (Only
   * in effect if output style is Polygonalize and smoothing is on.)
   */
  svtkSetClampMacro(NumberOfSmoothingIterations, int, 0, SVTK_INT_MAX);
  svtkGetMacro(NumberOfSmoothingIterations, int);
  //@}

  //@{
  /**
   * Turn on/off whether the final polygons should be decimated.
   * whether to smooth boundaries.
   */
  svtkSetMacro(Decimation, svtkTypeBool);
  svtkGetMacro(Decimation, svtkTypeBool);
  svtkBooleanMacro(Decimation, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the error to use for decimation (if decimation is on).
   * The error is an absolute number--the image spacing and
   * dimensions are used to create points so the error should be
   * consistent with the image size.
   */
  svtkSetClampMacro(DecimationError, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(DecimationError, double);
  //@}

  //@{
  /**
   * Specify the error value between two colors where the colors are
   * considered the same. Only use this if the color mode uses the
   * default 256 table.
   */
  svtkSetClampMacro(Error, int, 0, SVTK_INT_MAX);
  svtkGetMacro(Error, int);
  //@}

  //@{
  /**
   * Specify the size (n by n pixels) of the largest region to
   * polygonalize. When the OutputStyle is set to SVTK_STYLE_POLYGONALIZE,
   * large amounts of memory are used. In order to process large images,
   * the image is broken into pieces that are at most Size pixels in
   * width and height.
   */
  svtkSetClampMacro(SubImageSize, int, 10, SVTK_INT_MAX);
  svtkGetMacro(SubImageSize, int);
  //@}

protected:
  svtkImageToPolyDataFilter();
  ~svtkImageToPolyDataFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int OutputStyle;
  int ColorMode;
  svtkTypeBool Smoothing;
  int NumberOfSmoothingIterations;
  svtkTypeBool Decimation;
  double DecimationError;
  int Error;
  int SubImageSize;
  svtkScalarsToColors* LookupTable;

  virtual void PixelizeImage(svtkUnsignedCharArray* pixels, int dims[3], double origin[3],
    double spacing[3], svtkPolyData* output);
  virtual void PolygonalizeImage(svtkUnsignedCharArray* pixels, int dims[3], double origin[3],
    double spacing[3], svtkPolyData* output);
  virtual void RunLengthImage(svtkUnsignedCharArray* pixels, int dims[3], double origin[3],
    double spacing[3], svtkPolyData* output);

private:
  svtkUnsignedCharArray* Table; // color table used to quantize points
  svtkTimeStamp TableMTime;
  int* Visited;                     // traverse & mark connected regions
  svtkUnsignedCharArray* PolyColors; // the colors of each region -> polygon
  svtkEdgeTable* EdgeTable;          // keep track of intersection points
  svtkEdgeTable* EdgeUseTable;       // keep track of polygons use of edges
  svtkIntArray* EdgeUses;            // the two polygons that use an edge
                                    // and point id associated with edge (if any)

  void BuildTable(unsigned char* inPixels);
  svtkUnsignedCharArray* QuantizeImage(
    svtkDataArray* inScalars, int numComp, int type, int dims[3], int ext[4]);
  int ProcessImage(svtkUnsignedCharArray* pixels, int dims[2]);
  int BuildEdges(svtkUnsignedCharArray* pixels, int dims[3], double origin[3], double spacing[3],
    svtkUnsignedCharArray* pointDescr, svtkPolyData* edges);
  void BuildPolygons(svtkUnsignedCharArray* pointDescr, svtkPolyData* edges, int numPolys,
    svtkUnsignedCharArray* polyColors);
  void SmoothEdges(svtkUnsignedCharArray* pointDescr, svtkPolyData* edges);
  void DecimateEdges(svtkPolyData* edges, svtkUnsignedCharArray* pointDescr, double tol2);
  void GeneratePolygons(svtkPolyData* edges, int numPolys, svtkPolyData* output,
    svtkUnsignedCharArray* polyColors, svtkUnsignedCharArray* pointDescr);

  int GetNeighbors(
    unsigned char* ptr, int& i, int& j, int dims[3], unsigned char* neighbors[4], int mode);

  void GetIJ(int id, int& i, int& j, int dims[2]);
  unsigned char* GetColor(unsigned char* rgb);
  int IsSameColor(unsigned char* p1, unsigned char* p2);

private:
  svtkImageToPolyDataFilter(const svtkImageToPolyDataFilter&) = delete;
  void operator=(const svtkImageToPolyDataFilter&) = delete;
};

#endif
