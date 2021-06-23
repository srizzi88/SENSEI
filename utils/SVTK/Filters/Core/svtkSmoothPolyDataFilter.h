/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSmoothPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSmoothPolyDataFilter
 * @brief   adjust point positions using Laplacian smoothing
 *
 * svtkSmoothPolyDataFilter is a filter that adjusts point coordinates using
 * Laplacian smoothing. The effect is to "relax" the mesh, making the cells
 * better shaped and the vertices more evenly distributed. Note that this
 * filter operates on the lines, polygons, and triangle strips composing an
 * instance of svtkPolyData. Vertex or poly-vertex cells are never modified.
 *
 * The algorithm proceeds as follows. For each vertex v, a topological and
 * geometric analysis is performed to determine which vertices are connected
 * to v, and which cells are connected to v. Then, a connectivity array is
 * constructed for each vertex. (The connectivity array is a list of lists
 * of vertices that directly attach to each vertex.) Next, an iteration
 * phase begins over all vertices. For each vertex v, the coordinates of v
 * are modified according to an average of the connected vertices.  (A
 * relaxation factor is available to control the amount of displacement of
 * v).  The process repeats for each vertex. This pass over the list of
 * vertices is a single iteration. Many iterations (generally around 20 or
 * so) are repeated until the desired result is obtained.
 *
 * There are some special instance variables used to control the execution
 * of this filter. (These ivars basically control what vertices can be
 * smoothed, and the creation of the connectivity array.) The
 * BoundarySmoothing ivar enables/disables the smoothing operation on
 * vertices that are on the "boundary" of the mesh. A boundary vertex is one
 * that is surrounded by a semi-cycle of polygons (or used by a single
 * line).
 *
 * Another important ivar is FeatureEdgeSmoothing. If this ivar is
 * enabled, then interior vertices are classified as either "simple",
 * "interior edge", or "fixed", and smoothed differently. (Interior
 * vertices are manifold vertices surrounded by a cycle of polygons; or used
 * by two line cells.) The classification is based on the number of feature
 * edges attached to v. A feature edge occurs when the angle between the two
 * surface normals of a polygon sharing an edge is greater than the
 * FeatureAngle ivar. Then, vertices used by no feature edges are classified
 * "simple", vertices used by exactly two feature edges are classified
 * "interior edge", and all others are "fixed" vertices.
 *
 * Once the classification is known, the vertices are smoothed
 * differently. Corner (i.e., fixed) vertices are not smoothed at all.
 * Simple vertices are smoothed as before (i.e., average of connected
 * vertex coordinates). Interior edge vertices are smoothed only along
 * their two connected edges, and only if the angle between the edges
 * is less than the EdgeAngle ivar.
 *
 * The total smoothing can be controlled by using two ivars. The
 * NumberOfIterations is a cap on the maximum number of smoothing passes.
 * The Convergence ivar is a limit on the maximum point motion. If the
 * maximum motion during an iteration is less than Convergence, then the
 * smoothing process terminates. (Convergence is expressed as a fraction of
 * the diagonal of the bounding box.)
 *
 * There are two instance variables that control the generation of error
 * data. If the ivar GenerateErrorScalars is on, then a scalar value indicating
 * the distance of each vertex from its original position is computed. If the
 * ivar GenerateErrorVectors is on, then a vector representing change in
 * position is computed.
 *
 * Optionally you can further control the smoothing process by defining a
 * second input: the Source. If defined, the input mesh is constrained to
 * lie on the surface defined by the Source ivar.
 *
 *
 * @warning
 * The Laplacian operation reduces high frequency information in the geometry
 * of the mesh. With excessive smoothing important details may be lost, and
 * the surface may shrink towards the centroid. Enabling FeatureEdgeSmoothing
 * helps reduce this effect, but cannot entirely eliminate it. You may also
 * wish to try svtkWindowedSincPolyDataFilter. It does a better job of
 * minimizing shrinkage.
 *
 * @sa
 * svtkWindowedSincPolyDataFilter svtkDecimate svtkDecimatePro
 */

#ifndef svtkSmoothPolyDataFilter_h
#define svtkSmoothPolyDataFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkSmoothPoints;

class SVTKFILTERSCORE_EXPORT svtkSmoothPolyDataFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkSmoothPolyDataFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with number of iterations 20; relaxation factor .01;
   * feature edge smoothing turned off; feature
   * angle 45 degrees; edge angle 15 degrees; and boundary smoothing turned
   * on. Error scalars and vectors are not generated (by default). The
   * convergence criterion is 0.0 of the bounding box diagonal.
   */
  static svtkSmoothPolyDataFilter* New();

  //@{
  /**
   * Specify a convergence criterion for the iteration
   * process. Smaller numbers result in more smoothing iterations.
   */
  svtkSetClampMacro(Convergence, double, 0.0, 1.0);
  svtkGetMacro(Convergence, double);
  //@}

  //@{
  /**
   * Specify the number of iterations for Laplacian smoothing,
   */
  svtkSetClampMacro(NumberOfIterations, int, 0, SVTK_INT_MAX);
  svtkGetMacro(NumberOfIterations, int);
  //@}

  //@{
  /**
   * Specify the relaxation factor for Laplacian smoothing. As in all
   * iterative methods, the stability of the process is sensitive to
   * this parameter. In general, small relaxation factors and large
   * numbers of iterations are more stable than larger relaxation
   * factors and smaller numbers of iterations.
   */
  svtkSetMacro(RelaxationFactor, double);
  svtkGetMacro(RelaxationFactor, double);
  //@}

  //@{
  /**
   * Turn on/off smoothing along sharp interior edges.
   */
  svtkSetMacro(FeatureEdgeSmoothing, svtkTypeBool);
  svtkGetMacro(FeatureEdgeSmoothing, svtkTypeBool);
  svtkBooleanMacro(FeatureEdgeSmoothing, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the feature angle for sharp edge identification.
   */
  svtkSetClampMacro(FeatureAngle, double, 0.0, 180.0);
  svtkGetMacro(FeatureAngle, double);
  //@}

  //@{
  /**
   * Specify the edge angle to control smoothing along edges (either interior
   * or boundary).
   */
  svtkSetClampMacro(EdgeAngle, double, 0.0, 180.0);
  svtkGetMacro(EdgeAngle, double);
  //@}

  //@{
  /**
   * Turn on/off the smoothing of vertices on the boundary of the mesh.
   */
  svtkSetMacro(BoundarySmoothing, svtkTypeBool);
  svtkGetMacro(BoundarySmoothing, svtkTypeBool);
  svtkBooleanMacro(BoundarySmoothing, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the generation of scalar distance values.
   */
  svtkSetMacro(GenerateErrorScalars, svtkTypeBool);
  svtkGetMacro(GenerateErrorScalars, svtkTypeBool);
  svtkBooleanMacro(GenerateErrorScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the generation of error vectors.
   */
  svtkSetMacro(GenerateErrorVectors, svtkTypeBool);
  svtkGetMacro(GenerateErrorVectors, svtkTypeBool);
  svtkBooleanMacro(GenerateErrorVectors, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the source object which is used to constrain smoothing. The
   * source defines a surface that the input (as it is smoothed) is
   * constrained to lie upon.
   */
  void SetSourceData(svtkPolyData* source);
  svtkPolyData* GetSource();
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
  svtkSmoothPolyDataFilter();
  ~svtkSmoothPolyDataFilter() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  double Convergence;
  int NumberOfIterations;
  double RelaxationFactor;
  svtkTypeBool FeatureEdgeSmoothing;
  double FeatureAngle;
  double EdgeAngle;
  svtkTypeBool BoundarySmoothing;
  svtkTypeBool GenerateErrorScalars;
  svtkTypeBool GenerateErrorVectors;
  int OutputPointsPrecision;

  svtkSmoothPoints* SmoothPoints;

private:
  svtkSmoothPolyDataFilter(const svtkSmoothPolyDataFilter&) = delete;
  void operator=(const svtkSmoothPolyDataFilter&) = delete;
};

#endif
