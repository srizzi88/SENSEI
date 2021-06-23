/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataNormals
 * @brief   compute normals for polygonal mesh
 *
 * svtkPolyDataNormals is a filter that computes point and/or cell normals
 * for a polygonal mesh. The user specifies if they would like the point
 * and/or cell normals to be computed by setting the ComputeCellNormals
 * and ComputePointNormals flags.
 *
 * The computed normals (a svtkFloatArray) are set to be the active normals
 * (using SetNormals()) of the PointData and/or the CellData (respectively)
 * of the output PolyData. The name of these arrays is "Normals", so they
 * can be retrieved either with
 * svtkArrayDownCast<svtkFloatArray>(output->GetPointData()->GetNormals())
 * or with
 * svtkArrayDownCast<svtkFloatArray>(output->GetPointData()->GetArray("Normals"))
 *
 * The filter can reorder polygons to insure consistent
 * orientation across polygon neighbors. Sharp edges can be split and points
 * duplicated with separate normals to give crisp (rendered) surface definition.
 * It is also possible to globally flip the normal orientation.
 *
 * The algorithm works by determining normals for each polygon and then
 * averaging them at shared points. When sharp edges are present, the edges
 * are split and new points generated to prevent blurry edges (due to
 * Gouraud shading).
 *
 * @warning
 * Normals are computed only for polygons and triangle strips. Normals are
 * not computed for lines or vertices.
 *
 * @warning
 * Triangle strips are broken up into triangle polygons. You may want to
 * restrip the triangles.
 *
 * @sa
 * For high-performance rendering, you could use svtkTriangleMeshPointNormals
 * if you know that you have a triangle mesh which does not require splitting
 * nor consistency check on the cell orientations.
 *
 */

#ifndef svtkPolyDataNormals_h
#define svtkPolyDataNormals_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkFloatArray;
class svtkIdList;
class svtkPolyData;

class SVTKFILTERSCORE_EXPORT svtkPolyDataNormals : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkPolyDataNormals, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with feature angle=30, splitting and consistency turned on,
   * flipNormals turned off, and non-manifold traversal turned on.
   * ComputePointNormals is on and ComputeCellNormals is off.
   */
  static svtkPolyDataNormals* New();

  //@{
  /**
   * Specify the angle that defines a sharp edge. If the difference in
   * angle across neighboring polygons is greater than this value, the
   * shared edge is considered "sharp".
   */
  svtkSetClampMacro(FeatureAngle, double, 0.0, 180.0);
  svtkGetMacro(FeatureAngle, double);
  //@}

  //@{
  /**
   * Turn on/off the splitting of sharp edges.
   */
  svtkSetMacro(Splitting, svtkTypeBool);
  svtkGetMacro(Splitting, svtkTypeBool);
  svtkBooleanMacro(Splitting, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the enforcement of consistent polygon ordering.
   */
  svtkSetMacro(Consistency, svtkTypeBool);
  svtkGetMacro(Consistency, svtkTypeBool);
  svtkBooleanMacro(Consistency, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the automatic determination of correct normal
   * orientation. NOTE: This assumes a completely closed surface
   * (i.e. no boundary edges) and no non-manifold edges. If these
   * constraints do not hold, all bets are off. This option adds some
   * computational complexity, and is useful if you don't want to have
   * to inspect the rendered image to determine whether to turn on the
   * FlipNormals flag. However, this flag can work with the FlipNormals
   * flag, and if both are set, all the normals in the output will
   * point "inward".
   */
  svtkSetMacro(AutoOrientNormals, svtkTypeBool);
  svtkGetMacro(AutoOrientNormals, svtkTypeBool);
  svtkBooleanMacro(AutoOrientNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the computation of point normals.
   */
  svtkSetMacro(ComputePointNormals, svtkTypeBool);
  svtkGetMacro(ComputePointNormals, svtkTypeBool);
  svtkBooleanMacro(ComputePointNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the computation of cell normals.
   */
  svtkSetMacro(ComputeCellNormals, svtkTypeBool);
  svtkGetMacro(ComputeCellNormals, svtkTypeBool);
  svtkBooleanMacro(ComputeCellNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the global flipping of normal orientation. Flipping
   * reverves the meaning of front and back for Frontface and Backface
   * culling in svtkProperty.  Flipping modifies both the normal
   * direction and the order of a cell's points.
   */
  svtkSetMacro(FlipNormals, svtkTypeBool);
  svtkGetMacro(FlipNormals, svtkTypeBool);
  svtkBooleanMacro(FlipNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off traversal across non-manifold edges. This will prevent
   * problems where the consistency of polygonal ordering is corrupted due
   * to topological loops.
   */
  svtkSetMacro(NonManifoldTraversal, svtkTypeBool);
  svtkGetMacro(NonManifoldTraversal, svtkTypeBool);
  svtkBooleanMacro(NonManifoldTraversal, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkPolyDataNormals();
  ~svtkPolyDataNormals() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  double FeatureAngle;
  svtkTypeBool Splitting;
  svtkTypeBool Consistency;
  svtkTypeBool FlipNormals;
  svtkTypeBool AutoOrientNormals;
  svtkTypeBool NonManifoldTraversal;
  svtkTypeBool ComputePointNormals;
  svtkTypeBool ComputeCellNormals;
  int NumFlips;
  int OutputPointsPrecision;

private:
  svtkIdList* Wave;
  svtkIdList* Wave2;
  svtkIdList* CellIds;
  svtkIdList* CellPoints;
  svtkIdList* NeighborPoints;
  svtkIdList* Map;
  svtkPolyData* OldMesh;
  svtkPolyData* NewMesh;
  int* Visited;
  svtkFloatArray* PolyNormals;
  double CosAngle;

  // Uses the list of cell ids (this->Wave) to propagate a wave of
  // checked and properly ordered polygons.
  void TraverseAndOrder(void);

  // Check the point id give to see whether it lies on a feature
  // edge. If so, split the point (i.e., duplicate it) to topologically
  // separate the mesh.
  void MarkAndSplit(svtkIdType ptId);

private:
  svtkPolyDataNormals(const svtkPolyDataNormals&) = delete;
  void operator=(const svtkPolyDataNormals&) = delete;
};

#endif
