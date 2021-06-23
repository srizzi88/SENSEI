/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataPointSampler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataPointSampler
 * @brief   generate points from svtkPolyData
 *
 * svtkPolyDataPointSampler generates points from input svtkPolyData. The
 * points are placed approximately a specified distance apart. Optionally,
 * the points attributes can be interpolated from the generating vertices,
 * edges, and polygons.
 *
 * This filter functions as follows. First, it regurgitates all input points,
 * then it samples all lines, plus edges associated with the input polygons
 * and triangle strips to produce edge points. Finally, the interiors of
 * polygons and triangle strips are subsampled to produce points. All of
 * these operations can be enabled or disabled separately. Note that this
 * algorithm only approximately generates points the specified distance
 * apart. Generally the point density is finer than requested.
 *
 * @warning
 * While this algorithm processes general polygons. it does so by performing
 * a fan triangulation. This may produce poor results, especially for convave
 * polygons. For better results, use a triangle filter to pre-tesselate
 * polygons.
 *
 * @warning
 * Point generation can be useful in a variety of applications. For example,
 * generating seed points for glyphing or streamline generation. Another
 * useful application is generating points for implicit modeling. In many
 * cases implicit models can be more efficiently generated from points than
 * from polygons or other primitives.
 *
 * @warning
 * When sampling polygons of 5 sides or more, the polygon is triangulated.
 * This can result in variations in point density near tesselation boudaries.
 *
 * @sa
 * svtkTriangleFilter svtkImplicitModeller
 */

#ifndef svtkPolyDataPointSampler_h
#define svtkPolyDataPointSampler_h

#include "svtkEdgeTable.h"             // for sampling edges
#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkNew.h"                   // for data members
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSMODELING_EXPORT svtkPolyDataPointSampler : public svtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate this class.
   */
  static svtkPolyDataPointSampler* New();

  //@{
  /**
   * Standard macros for type information and printing.
   */
  svtkTypeMacro(svtkPolyDataPointSampler, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the approximate distance between points. This is an absolute
   * distance measure. The default is 0.01.
   */
  svtkSetClampMacro(Distance, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Distance, double);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell vertex points should
   * be output.
   */
  svtkGetMacro(GenerateVertexPoints, bool);
  svtkSetMacro(GenerateVertexPoints, bool);
  svtkBooleanMacro(GenerateVertexPoints, bool);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell edges should
   * be sampled to produce output points. The default is true.
   */
  svtkGetMacro(GenerateEdgePoints, bool);
  svtkSetMacro(GenerateEdgePoints, bool);
  svtkBooleanMacro(GenerateEdgePoints, bool);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell interiors should
   * be sampled to produce output points. The default is true.
   */
  svtkGetMacro(GenerateInteriorPoints, bool);
  svtkSetMacro(GenerateInteriorPoints, bool);
  svtkBooleanMacro(GenerateInteriorPoints, bool);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether cell vertices should
   * be generated. Cell vertices are useful if you actually want to display
   * the points (that is, for each point generated, a vertex is generated).
   * Recall that SVTK only renders vertices and not points.  The default is
   * true.
   */
  svtkGetMacro(GenerateVertices, bool);
  svtkSetMacro(GenerateVertices, bool);
  svtkBooleanMacro(GenerateVertices, bool);
  //@}

  //@{
  /**
   * Specify/retrieve a boolean flag indicating whether point data should be
   * interpolated onto the newly generated points. If enabled, points
   * generated from existing vertices will carry the vertex point data;
   * points generated from edges will interpolate point data along each edge;
   * and interior point data (inside triangles, polygons cells) will be
   * interpolated from the cell vertices. By default this is off.
   */
  svtkGetMacro(InterpolatePointData, bool);
  svtkSetMacro(InterpolatePointData, bool);
  svtkBooleanMacro(InterpolatePointData, bool);
  //@}

protected:
  svtkPolyDataPointSampler();
  ~svtkPolyDataPointSampler() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Distance;
  double Distance2;

  bool GenerateVertexPoints;
  bool GenerateEdgePoints;
  bool GenerateInteriorPoints;
  bool GenerateVertices;

  bool InterpolatePointData;

  // Internal scratch arrays supporting point data interpolation, and
  // sampling edges.
  svtkNew<svtkEdgeTable> EdgeTable;
  double TriWeights[3];
  svtkNew<svtkIdList> TriIds;
  double QuadWeights[4];
  svtkNew<svtkIdList> QuadIds;

  // Internal methods for sampling edges, triangles, and polygons
  void SampleEdge(
    svtkPoints* pts, svtkIdType p0, svtkIdType p1, svtkPointData* inPD, svtkPointData* outPD);
  void SampleTriangle(svtkPoints* newPts, svtkPoints* inPts, const svtkIdType* pts, svtkPointData* inPD,
    svtkPointData* outPD);
  void SamplePolygon(svtkPoints* newPts, svtkPoints* inPts, svtkIdType npts, const svtkIdType* pts,
    svtkPointData* inPD, svtkPointData* outPD);

private:
  svtkPolyDataPointSampler(const svtkPolyDataPointSampler&) = delete;
  void operator=(const svtkPolyDataPointSampler&) = delete;
};

#endif
