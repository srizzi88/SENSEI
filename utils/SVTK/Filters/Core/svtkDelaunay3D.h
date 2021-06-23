/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDelaunay3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDelaunay3D
 * @brief   create 3D Delaunay triangulation of input points
 *
 * svtkDelaunay3D is a filter that constructs a 3D Delaunay
 * triangulation from a list of input points. These points may be
 * represented by any dataset of type svtkPointSet and subclasses. The
 * output of the filter is an unstructured grid dataset. Usually the
 * output is a tetrahedral mesh, but if a non-zero alpha distance
 * value is specified (called the "alpha" value), then only tetrahedra,
 * triangles, edges, and vertices laying within the alpha radius are
 * output. In other words, non-zero alpha values may result in arbitrary
 * combinations of tetrahedra, triangles, lines, and vertices. (The notion
 * of alpha value is derived from Edelsbrunner's work on "alpha shapes".)
 * Note that a modification to alpha shapes enables output of combinations
 * of tetrahedra, triangles, lines, and/or verts (see the boolean ivars
 * AlphaTets, AlphaTris, AlphaLines, AlphaVerts).
 *
 * The 3D Delaunay triangulation is defined as the triangulation that
 * satisfies the Delaunay criterion for n-dimensional simplexes (in
 * this case n=3 and the simplexes are tetrahedra). This criterion
 * states that a circumsphere of each simplex in a triangulation
 * contains only the n+1 defining points of the simplex. (See text for
 * more information.) While in two dimensions this translates into an
 * "optimal" triangulation, this is not true in 3D, since a measurement
 * for optimality in 3D is not agreed on.
 *
 * Delaunay triangulations are used to build topological structures
 * from unorganized (or unstructured) points. The input to this filter
 * is a list of points specified in 3D. (If you wish to create 2D
 * triangulations see svtkDelaunay2D.) The output is an unstructured grid.
 *
 * The Delaunay triangulation can be numerically sensitive. To prevent
 * problems, try to avoid injecting points that will result in
 * triangles with bad aspect ratios (1000:1 or greater). In practice
 * this means inserting points that are "widely dispersed", and
 * enables smooth transition of triangle sizes throughout the
 * mesh. (You may even want to add extra points to create a better
 * point distribution.) If numerical problems are present, you will
 * see a warning message to this effect at the end of the
 * triangulation process.
 *
 * @warning
 * Points arranged on a regular lattice (termed degenerate cases) can be
 * triangulated in more than one way (at least according to the Delaunay
 * criterion). The choice of triangulation (as implemented by
 * this algorithm) depends on the order of the input points. The first four
 * points will form a tetrahedron; other degenerate points (relative to this
 * initial tetrahedron) will not break it.
 *
 * @warning
 * Points that are coincident (or nearly so) may be discarded by the
 * algorithm.  This is because the Delaunay triangulation requires
 * unique input points.  You can control the definition of coincidence
 * with the "Tolerance" instance variable.
 *
 * @warning
 * The output of the Delaunay triangulation is supposedly a convex hull. In
 * certain cases this implementation may not generate the convex hull. This
 * behavior can be controlled by the Offset instance variable. Offset is a
 * multiplier used to control the size of the initial triangulation. The
 * larger the offset value, the more likely you will generate a convex hull;
 * and the more likely you are to see numerical problems.
 *
 * @warning
 * The implementation of this algorithm varies from the 2D Delaunay
 * algorithm (i.e., svtkDelaunay2D) in an important way. When points are
 * injected into the triangulation, the search for the enclosing tetrahedron
 * is quite different. In the 3D case, the closest previously inserted point
 * point is found, and then the connected tetrahedra are searched to find
 * the containing one. (In 2D, a "walk" towards the enclosing triangle is
 * performed.) If the triangulation is Delaunay, then an enclosing tetrahedron
 * will be found. However, in degenerate cases an enclosing tetrahedron may
 * not be found and the point will be rejected.
 *
 * @sa
 * svtkDelaunay2D svtkGaussianSplatter svtkUnstructuredGrid
 */

#ifndef svtkDelaunay3D_h
#define svtkDelaunay3D_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkIdList;
class svtkPointLocator;
class svtkPointSet;
class svtkPoints;
class svtkTetraArray;
class svtkIncrementalPointLocator;

class SVTKFILTERSCORE_EXPORT svtkDelaunay3D : public svtkUnstructuredGridAlgorithm
{
public:
  svtkTypeMacro(svtkDelaunay3D, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with Alpha = 0.0; Tolerance = 0.001; Offset = 2.5;
   * BoundingTriangulation turned off.
   */
  static svtkDelaunay3D* New();

  //@{
  /**
   * Specify alpha (or distance) value to control output of this filter.  For
   * a non-zero alpha value, only verts, edges, faces, or tetra contained
   * within the circumsphere (of radius alpha) will be output. Otherwise,
   * only tetrahedra will be output. Note that the flags AlphaTets, AlphaTris,
   * AlphaLines, and AlphaVerts control whether these primitives are output
   * when Alpha is non-zero. (By default all tets, triangles, lines and verts
   * satisfying the alpha shape criterion are output.)
   */
  svtkSetClampMacro(Alpha, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Alpha, double);
  //@}

  //@{
  /**
   * Boolean controls whether tetrahedra are output for non-zero alpha values.
   */
  svtkSetMacro(AlphaTets, svtkTypeBool);
  svtkGetMacro(AlphaTets, svtkTypeBool);
  svtkBooleanMacro(AlphaTets, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean controls whether triangles are output for non-zero alpha values.
   */
  svtkSetMacro(AlphaTris, svtkTypeBool);
  svtkGetMacro(AlphaTris, svtkTypeBool);
  svtkBooleanMacro(AlphaTris, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean controls whether lines are output for non-zero alpha values.
   */
  svtkSetMacro(AlphaLines, svtkTypeBool);
  svtkGetMacro(AlphaLines, svtkTypeBool);
  svtkBooleanMacro(AlphaLines, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean controls whether vertices are output for non-zero alpha values.
   */
  svtkSetMacro(AlphaVerts, svtkTypeBool);
  svtkGetMacro(AlphaVerts, svtkTypeBool);
  svtkBooleanMacro(AlphaVerts, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify a tolerance to control discarding of closely spaced points.
   * This tolerance is specified as a fraction of the diagonal length of
   * the bounding box of the points.
   */
  svtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Specify a multiplier to control the size of the initial, bounding
   * Delaunay triangulation.
   */
  svtkSetClampMacro(Offset, double, 2.5, SVTK_DOUBLE_MAX);
  svtkGetMacro(Offset, double);
  //@}

  //@{
  /**
   * Boolean controls whether bounding triangulation points (and associated
   * triangles) are included in the output. (These are introduced as an
   * initial triangulation to begin the triangulation process. This feature
   * is nice for debugging output.)
   */
  svtkSetMacro(BoundingTriangulation, svtkTypeBool);
  svtkGetMacro(BoundingTriangulation, svtkTypeBool);
  svtkBooleanMacro(BoundingTriangulation, svtkTypeBool);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of svtkPointLocator is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to eliminate "coincident" points.
   */
  void CreateDefaultLocator();

  /**
   * This is a helper method used with InsertPoint() to create
   * tetrahedronalizations of points. Its purpose is construct an initial
   * Delaunay triangulation into which to inject other points. You must
   * specify the center of a cubical bounding box and its length, as well
   * as the number of points to insert. The method returns a pointer to
   * an unstructured grid. Use this pointer to manipulate the mesh as
   * necessary. You must delete (with Delete()) the mesh when done.
   * Note: This initialization method places points forming bounding octahedron
   * at the end of the Mesh's point list. That is, InsertPoint() assumes that
   * you will be inserting points between (0,numPtsToInsert-1).
   */
  svtkUnstructuredGrid* InitPointInsertion(
    double center[3], double length, svtkIdType numPts, svtkPoints*& pts);

  /**
   * This is a helper method used with InitPointInsertion() to create
   * tetrahedronalizations of points. Its purpose is to inject point at
   * coordinates specified into tetrahedronalization. The point id is an index
   * into the list of points in the mesh structure.  (See
   * svtkDelaunay3D::InitPointInsertion() for more information.)  When you have
   * completed inserting points, traverse the mesh structure to extract desired
   * tetrahedra (or tetra faces and edges).The holeTetras id list lists all the
   * tetrahedra that are deleted (invalid) in the mesh structure.
   */
  void InsertPoint(
    svtkUnstructuredGrid* Mesh, svtkPoints* points, svtkIdType id, double x[3], svtkIdList* holeTetras);

  /**
   * Invoke this method after all points have been inserted. The purpose of
   * the method is to clean up internal data structures. Note that the
   * (svtkUnstructuredGrid *)Mesh returned from InitPointInsertion() is NOT
   * deleted, you still are responsible for cleaning that up.
   */
  void EndPointInsertion();

  /**
   * Return the MTime also considering the locator.
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
  svtkDelaunay3D();
  ~svtkDelaunay3D() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double Alpha;
  svtkTypeBool AlphaTets;
  svtkTypeBool AlphaTris;
  svtkTypeBool AlphaLines;
  svtkTypeBool AlphaVerts;
  double Tolerance;
  svtkTypeBool BoundingTriangulation;
  double Offset;
  int OutputPointsPrecision;

  svtkIncrementalPointLocator* Locator; // help locate points faster

  svtkTetraArray* TetraArray; // used to keep track of circumspheres/neighbors
  int FindTetra(svtkUnstructuredGrid* Mesh, double x[3], svtkIdType tetId, int depth);
  int InSphere(double x[3], svtkIdType tetraId);
  void InsertTetra(svtkUnstructuredGrid* Mesh, svtkPoints* pts, svtkIdType tetraId);

  int NumberOfDuplicatePoints; // keep track of bad data
  int NumberOfDegeneracies;

  // Keep track of number of references to points to avoid new/delete calls
  int* References;

  svtkIdType FindEnclosingFaces(double x[3], svtkUnstructuredGrid* Mesh, svtkIdList* tetras,
    svtkIdList* faces, svtkIncrementalPointLocator* Locator);

  int FillInputPortInformation(int, svtkInformation*) override;

private:                    // members added for performance
  svtkIdList* Tetras;        // used in InsertPoint
  svtkIdList* Faces;         // used in InsertPoint
  svtkIdList* CheckedTetras; // used by InsertPoint

private:
  svtkDelaunay3D(const svtkDelaunay3D&) = delete;
  void operator=(const svtkDelaunay3D&) = delete;
};

#endif
