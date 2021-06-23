/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDijkstraGraphGeodesicPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDijkstraGraphGeodesicPath
 * @brief   Dijkstra algorithm to compute the graph geodesic.
 *
 * Takes as input a polygonal mesh and performs a single source shortest
 * path calculation. Dijkstra's algorithm is used. The implementation is
 * similar to the one described in Introduction to Algorithms (Second Edition)
 * by Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, and
 * Cliff Stein, published by MIT Press and McGraw-Hill. Some minor
 * enhancement are added though. All vertices are not pushed on the heap
 * at start, instead a front set is maintained. The heap is implemented as
 * a binary heap. The output of the filter is a set of lines describing
 * the shortest path from StartVertex to EndVertex. If a path cannot be found
 * the output will have no lines or points.
 *
 * @warning
 * The input polydata must have only triangle cells.
 *
 * @par Thanks:
 * The class was contributed by Rasmus Paulsen.
 * www.imm.dtu.dk/~rrp/SVTK . Also thanks to Alexandre Gouaillard and Shoaib
 * Ghias for bug fixes and enhancements.
 */

#ifndef svtkDijkstraGraphGeodesicPath_h
#define svtkDijkstraGraphGeodesicPath_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkGraphGeodesicPath.h"

class svtkDijkstraGraphInternals;
class svtkIdList;

class SVTKFILTERSMODELING_EXPORT svtkDijkstraGraphGeodesicPath : public svtkGraphGeodesicPath
{
public:
  /**
   * Instantiate the class
   */
  static svtkDijkstraGraphGeodesicPath* New();

  //@{
  /**
   * Standard methods for printing and determining type information.
   */
  svtkTypeMacro(svtkDijkstraGraphGeodesicPath, svtkGraphGeodesicPath);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * The vertex ids (of the input polydata) on the shortest path
   */
  svtkGetObjectMacro(IdList, svtkIdList);
  //@}

  //@{
  /**
   * Stop when the end vertex is reached
   * or calculate shortest path to all vertices
   */
  svtkSetMacro(StopWhenEndReached, svtkTypeBool);
  svtkGetMacro(StopWhenEndReached, svtkTypeBool);
  svtkBooleanMacro(StopWhenEndReached, svtkTypeBool);
  //@}

  //@{
  /**
   * Use scalar values in the edge weight (experimental)
   */
  svtkSetMacro(UseScalarWeights, svtkTypeBool);
  svtkGetMacro(UseScalarWeights, svtkTypeBool);
  svtkBooleanMacro(UseScalarWeights, svtkTypeBool);
  //@}

  //@{
  /**
   * Use the input point to repel the path by assigning high costs.
   */
  svtkSetMacro(RepelPathFromVertices, svtkTypeBool);
  svtkGetMacro(RepelPathFromVertices, svtkTypeBool);
  svtkBooleanMacro(RepelPathFromVertices, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify svtkPoints to use to repel the path from.
   */
  virtual void SetRepelVertices(svtkPoints*);
  svtkGetObjectMacro(RepelVertices, svtkPoints);
  //@}

  /**
   * Fill the array with the cumulative weights.
   */
  virtual void GetCumulativeWeights(svtkDoubleArray* weights);

protected:
  svtkDijkstraGraphGeodesicPath();
  ~svtkDijkstraGraphGeodesicPath() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // Build a graph description of the input.
  virtual void BuildAdjacency(svtkDataSet* inData);

  svtkTimeStamp AdjacencyBuildTime;

  // The fixed cost going from vertex u to v.
  virtual double CalculateStaticEdgeCost(svtkDataSet* inData, svtkIdType u, svtkIdType v);

  // The cost going from vertex u to v that may depend on one or more vertices
  // that precede u.
  virtual double CalculateDynamicEdgeCost(svtkDataSet*, svtkIdType, svtkIdType) { return 0.0; }

  void Initialize(svtkDataSet* inData);

  void Reset();

  // Calculate shortest path from vertex startv to vertex endv.
  virtual void ShortestPath(svtkDataSet* inData, int startv, int endv);

  // Relax edge u,v with weight w.
  void Relax(const int& u, const int& v, const double& w);

  // Backtrace the shortest path
  void TraceShortestPath(
    svtkDataSet* inData, svtkPolyData* outPoly, svtkIdType startv, svtkIdType endv);

  // The number of vertices.
  int NumberOfVertices;

  // The vertex ids on the shortest path.
  svtkIdList* IdList;

  // Internalized STL containers.
  svtkDijkstraGraphInternals* Internals;

  svtkTypeBool StopWhenEndReached;
  svtkTypeBool UseScalarWeights;
  svtkTypeBool RepelPathFromVertices;

  svtkPoints* RepelVertices;

private:
  svtkDijkstraGraphGeodesicPath(const svtkDijkstraGraphGeodesicPath&) = delete;
  void operator=(const svtkDijkstraGraphGeodesicPath&) = delete;
};

#endif
