/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQuadricDecimation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQuadricDecimation
 * @brief   reduce the number of triangles in a mesh
 *
 * svtkQuadricDecimation is a filter to reduce the number of triangles in
 * a triangle mesh, forming a good approximation to the original geometry.
 * The input to svtkQuadricDecimation is a svtkPolyData object, and only
 * triangles are treated. If you desire to decimate polygonal meshes, first
 * triangulate the polygons with svtkTriangleFilter.
 *
 * The algorithm is based on repeated edge collapses until the requested mesh
 * reduction is achieved. Edges are placed in a priority queue based on the
 * "cost" to delete the edge. The cost is an approximate measure of error
 * (distance to the original surface)--described by the so-called quadric
 * error measure. The quadric error measure is associated with each vertex of
 * the mesh and represents a matrix of planes incident on that vertex. The
 * distance of the planes to the vertex is the error in the position of the
 * vertex (originally the vertex error iz zero). As edges are deleted, the
 * quadric error measure associated with the two end points of the edge are
 * summed (this combines the plane equations) and an optimal collapse point
 * can be computed. Edges connected to the collapse point are then reinserted
 * into the queue after computing the new cost to delete them. The process
 * continues until the desired reduction level is reached or topological
 * constraints prevent further reduction. Note that this basic algorithm can
 * be extended to higher dimensions by
 * taking into account variation in attributes (i.e., scalars, vectors, and
 * so on).
 *
 * This paper is based on the work of Garland and Heckbert who first
 * presented the quadric error measure at Siggraph '97 "Surface
 * Simplification Using Quadric Error Metrics". For details of the algorithm
 * Michael Garland's Ph.D. thesis is also recommended. Hughues Hoppe's Vis
 * '99 paper, "New Quadric Metric for Simplifying Meshes with Appearance
 * Attributes" is also a good take on the subject especially as it pertains
 * to the error metric applied to attributes.
 *
 * @par Thanks:
 * Thanks to Bradley Lowekamp of the National Library of Medicine/NIH for
 * contributing this class.
 */

#ifndef svtkQuadricDecimation_h
#define svtkQuadricDecimation_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkEdgeTable;
class svtkIdList;
class svtkPointData;
class svtkPriorityQueue;
class svtkDoubleArray;

class SVTKFILTERSCORE_EXPORT svtkQuadricDecimation : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkQuadricDecimation, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkQuadricDecimation* New();

  //@{
  /**
   * Set/Get the desired reduction (expressed as a fraction of the original
   * number of triangles). The actual reduction may be less depending on
   * triangulation and topological constraints.
   */
  svtkSetClampMacro(TargetReduction, double, 0.0, 1.0);
  svtkGetMacro(TargetReduction, double);
  //@}

  //@{
  /**
   * Decide whether to include data attributes in the error metric. If off,
   * then only geometric error is used to control the decimation. By default
   * the attribute errors are off.
   */
  svtkSetMacro(AttributeErrorMetric, svtkTypeBool);
  svtkGetMacro(AttributeErrorMetric, svtkTypeBool);
  svtkBooleanMacro(AttributeErrorMetric, svtkTypeBool);
  //@}

  //@{
  /**
   * Decide whether to activate volume preservation which greatly reduces errors
   * in triangle normal direction. If off, volume preservation is disabled and
   * if AttributeErrorMetric is active, these errors can be large.
   * By default VolumePreservation is off
   * the attribute errors are off.
   */
  svtkSetMacro(VolumePreservation, svtkTypeBool);
  svtkGetMacro(VolumePreservation, svtkTypeBool);
  svtkBooleanMacro(VolumePreservation, svtkTypeBool);
  //@}

  //@{
  /**
   * If attribute errors are to be included in the metric (i.e.,
   * AttributeErrorMetric is on), then the following flags control which
   * attributes are to be included in the error calculation. By default all
   * of these are on.
   */
  svtkSetMacro(ScalarsAttribute, svtkTypeBool);
  svtkGetMacro(ScalarsAttribute, svtkTypeBool);
  svtkBooleanMacro(ScalarsAttribute, svtkTypeBool);
  svtkSetMacro(VectorsAttribute, svtkTypeBool);
  svtkGetMacro(VectorsAttribute, svtkTypeBool);
  svtkBooleanMacro(VectorsAttribute, svtkTypeBool);
  svtkSetMacro(NormalsAttribute, svtkTypeBool);
  svtkGetMacro(NormalsAttribute, svtkTypeBool);
  svtkBooleanMacro(NormalsAttribute, svtkTypeBool);
  svtkSetMacro(TCoordsAttribute, svtkTypeBool);
  svtkGetMacro(TCoordsAttribute, svtkTypeBool);
  svtkBooleanMacro(TCoordsAttribute, svtkTypeBool);
  svtkSetMacro(TensorsAttribute, svtkTypeBool);
  svtkGetMacro(TensorsAttribute, svtkTypeBool);
  svtkBooleanMacro(TensorsAttribute, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the scaling weight contribution of the attribute. These
   * values are used to weight the contribution of the attributes
   * towards the error metric.
   */
  svtkSetMacro(ScalarsWeight, double);
  svtkSetMacro(VectorsWeight, double);
  svtkSetMacro(NormalsWeight, double);
  svtkSetMacro(TCoordsWeight, double);
  svtkSetMacro(TensorsWeight, double);
  svtkGetMacro(ScalarsWeight, double);
  svtkGetMacro(VectorsWeight, double);
  svtkGetMacro(NormalsWeight, double);
  svtkGetMacro(TCoordsWeight, double);
  svtkGetMacro(TensorsWeight, double);
  //@}

  //@{
  /**
   * Get the actual reduction. This value is only valid after the
   * filter has executed.
   */
  svtkGetMacro(ActualReduction, double);
  //@}

protected:
  svtkQuadricDecimation();
  ~svtkQuadricDecimation() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Do the dirty work of eliminating the edge; return the number of
   * triangles deleted.
   */
  int CollapseEdge(svtkIdType pt0Id, svtkIdType pt1Id);

  /**
   * Compute quadric for all vertices
   */
  void InitializeQuadrics(svtkIdType numPts);

  /**
   * Free boundary edges are weighted
   */
  void AddBoundaryConstraints(void);

  /**
   * Compute quadric for this vertex.
   */
  void ComputeQuadric(svtkIdType pointId);

  /**
   * Add the quadrics for these 2 points since the edge between them has
   * been collapsed.
   */
  void AddQuadric(svtkIdType oldPtId, svtkIdType newPtId);

  //@{
  /**
   * Compute cost for contracting this edge and the point that gives us this
   * cost.
   */
  double ComputeCost(svtkIdType edgeId, double* x);
  double ComputeCost2(svtkIdType edgeId, double* x);
  //@}

  /**
   * Find all edges that will have an endpoint change ids because of an edge
   * collapse.  p1Id and p2Id are the endpoints of the edge.  p2Id is the
   * pointId being removed.
   */
  void FindAffectedEdges(svtkIdType p1Id, svtkIdType p2Id, svtkIdList* edges);

  /**
   * Find a cell that uses this edge.
   */
  svtkIdType GetEdgeCellId(svtkIdType p1Id, svtkIdType p2Id);

  int IsGoodPlacement(svtkIdType pt0Id, svtkIdType pt1Id, const double* x);
  int TrianglePlaneCheck(
    const double t0[3], const double t1[3], const double t2[3], const double* x);
  void ComputeNumberOfComponents(void);
  void UpdateEdgeData(svtkIdType ptoId, svtkIdType pt1Id);

  //@{
  /**
   * Helper function to set and get the point and it's attributes as an array
   */
  void SetPointAttributeArray(svtkIdType ptId, const double* x);
  void GetPointAttributeArray(svtkIdType ptId, double* x);
  //@}

  /**
   * Find out how many components there are for each attribute for this
   * poly data.
   */
  void GetAttributeComponents();

  double TargetReduction;
  double ActualReduction;
  svtkTypeBool AttributeErrorMetric;
  svtkTypeBool VolumePreservation;

  svtkTypeBool ScalarsAttribute;
  svtkTypeBool VectorsAttribute;
  svtkTypeBool NormalsAttribute;
  svtkTypeBool TCoordsAttribute;
  svtkTypeBool TensorsAttribute;

  double ScalarsWeight;
  double VectorsWeight;
  double NormalsWeight;
  double TCoordsWeight;
  double TensorsWeight;

  int NumberOfEdgeCollapses;
  svtkEdgeTable* Edges;
  svtkIdList* EndPoint1List;
  svtkIdList* EndPoint2List;
  svtkPriorityQueue* EdgeCosts;
  svtkDoubleArray* TargetPoints;
  int NumberOfComponents;
  svtkPolyData* Mesh;

  struct ErrorQuadric
  {
    double* Quadric;
  };

  // One ErrorQuadric per point
  ErrorQuadric* ErrorQuadrics;

  // Contains 4 doubles per point. Length = nPoints * 4
  double* VolumeConstraints;
  int AttributeComponents[6];
  double AttributeScale[6];

  // Temporary variables for performance
  svtkIdList* CollapseCellIds;
  double* TempX;
  double* TempQuad;
  double* TempB;
  double** TempA;
  double* TempData;

private:
  svtkQuadricDecimation(const svtkQuadricDecimation&) = delete;
  void operator=(const svtkQuadricDecimation&) = delete;
};

#endif
