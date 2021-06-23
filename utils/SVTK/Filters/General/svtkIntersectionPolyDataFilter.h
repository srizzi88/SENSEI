/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIntersectionPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkIntersectionPolyDataFilter
 *
 *
 * svtkIntersectionPolyDataFilter computes the intersection between two
 * svtkPolyData objects. The first output is a set of lines that marks
 * the intersection of the input svtkPolyData objects. This contains five
 * different attached data arrays:
 *
 * SurfaceID: Point data array that contains information about the origin
 * surface of each point
 *
 * Input0CellID: Cell data array that contains the original
 * cell ID number on the first input mesh
 *
 * Input1CellID: Cell data array that contains the original
 * cell ID number on the second input mesh
 *
 * NewCell0ID: Cell data array that contains information about which cells
 * of the remeshed first input surface it touches (If split)
 *
 * NewCell1ID: Cell data array that contains information about which cells
 * on the remeshed second input surface it touches (If split)
 *
 * The second and third outputs are the first and second input svtkPolyData,
 * respectively. Optionally, the two output svtkPolyData can be split
 * along the intersection lines by remeshing. Optionally, the surface
 * can be cleaned and checked at the end of the remeshing.
 * If the meshes are split, The output svtkPolyDatas contain three possible
 * data arrays:
 *
 * IntersectionPoint: This is a boolean indicating whether or not the point is
 * on the boundary of the two input objects
 *
 * BadTriangle: If the surface is cleaned and checked, this is a cell data array
 * indicating whether or not the cell has edges with multiple neighbors
 * A manifold surface will have 0 everywhere for this array!
 *
 * FreeEdge: If the surface is cleaned and checked, this is a cell data array
 * indicating if the cell has any free edges. A watertight surface will have
 * 0 everywhere for this array!
 *
 * @author Adam Updegrove updega2@gmail.com
 *
 * @warning This filter is not designed to perform 2D boolean operations,
 * and in fact relies on the inputs having no co-planar, overlapping cells.
 */

#ifndef svtkIntersectionPolyDataFilter_h
#define svtkIntersectionPolyDataFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkIntersectionPolyDataFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkIntersectionPolyDataFilter* New();
  svtkTypeMacro(svtkIntersectionPolyDataFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Integer describing the number of intersection points and lines
   */
  svtkGetMacro(NumberOfIntersectionPoints, int);
  svtkGetMacro(NumberOfIntersectionLines, int);
  //@}

  //@{
  /**
   * If on, the second output will be the first input mesh split by the
   * intersection with the second input mesh. Defaults to on.
   */
  svtkGetMacro(SplitFirstOutput, svtkTypeBool);
  svtkSetMacro(SplitFirstOutput, svtkTypeBool);
  svtkBooleanMacro(SplitFirstOutput, svtkTypeBool);
  //@}

  //@{
  /**
   * If on, the third output will be the second input mesh split by the
   * intersection with the first input mesh. Defaults to on.
   */
  svtkGetMacro(SplitSecondOutput, svtkTypeBool);
  svtkSetMacro(SplitSecondOutput, svtkTypeBool);
  svtkBooleanMacro(SplitSecondOutput, svtkTypeBool);
  //@}

  //@{
  /**
   * If on, the output split surfaces will contain information about which
   * points are on the intersection of the two inputs. Default: ON
   */
  svtkGetMacro(ComputeIntersectionPointArray, svtkTypeBool);
  svtkSetMacro(ComputeIntersectionPointArray, svtkTypeBool);
  svtkBooleanMacro(ComputeIntersectionPointArray, svtkTypeBool);
  //@}

  //@{
  /**
   * If on, the normals of the input will be checked. Default: OFF
   */
  svtkGetMacro(CheckInput, svtkTypeBool);
  svtkSetMacro(CheckInput, svtkTypeBool);
  svtkBooleanMacro(CheckInput, svtkTypeBool);
  //@}

  //@{
  /**
   * If on, the output remeshed surfaces will be checked for bad cells and
   * free edges. Default: ON
   */
  svtkGetMacro(CheckMesh, svtkTypeBool);
  svtkSetMacro(CheckMesh, svtkTypeBool);
  svtkBooleanMacro(CheckMesh, svtkTypeBool);
  //@}

  //@{
  /**
   * Check the status of the filter after update. If the status is zero,
   * there was an error in the operation. If status is one, everything
   * went smoothly
   */
  svtkGetMacro(Status, int);
  //@}

  //@{
  /**
   * The tolerance for geometric tests in the filter
   */
  svtkGetMacro(Tolerance, double);
  svtkSetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * When discretizing polygons, the minimum ratio of the smallest acceptable
   * triangle area w.r.t. the area of the polygon
   *
   */
  svtkGetMacro(RelativeSubtriangleArea, double);
  svtkSetMacro(RelativeSubtriangleArea, double);
  //@}

  /**
   * Given two triangles defined by points (p1, q1, r1) and (p2, q2,
   * r2), returns whether the two triangles intersect. If they do,
   * the endpoints of the line forming the intersection are returned
   * in pt1 and pt2. The parameter coplanar is set to 1 if the
   * triangles are coplanar and 0 otherwise. The surfaceid array
   * is filled with the surface on which the first and second
   * intersection points resides, respectively. A geometric tolerance
   * can be specified in the last argument.
   */
  static int TriangleTriangleIntersection(double p1[3], double q1[3], double r1[3], double p2[3],
    double q2[3], double r2[3], int& coplanar, double pt1[3], double pt2[3], double surfaceid[2],
    double tolerance);

  /**
   * Function to clean and check the output surfaces for bad triangles and
   * free edges
   */
  static void CleanAndCheckSurface(svtkPolyData* pd, double stats[2], double tolerance);

  /**
   * Function to clean and check the inputs
   */
  static void CleanAndCheckInput(svtkPolyData* pd, double tolerance);

protected:
  svtkIntersectionPolyDataFilter();           // Constructor
  ~svtkIntersectionPolyDataFilter() override; // Destructor

  int RequestData(svtkInformation*, svtkInformationVector**,
    svtkInformationVector*) override;                           // Update
  int FillInputPortInformation(int, svtkInformation*) override; // Input,Output

private:
  svtkIntersectionPolyDataFilter(const svtkIntersectionPolyDataFilter&) = delete;
  void operator=(const svtkIntersectionPolyDataFilter&) = delete;

  int NumberOfIntersectionPoints;
  int NumberOfIntersectionLines;
  svtkTypeBool SplitFirstOutput;
  svtkTypeBool SplitSecondOutput;
  svtkTypeBool ComputeIntersectionPointArray;
  svtkTypeBool CheckMesh;
  svtkTypeBool CheckInput;
  int Status;
  double Tolerance;
  double RelativeSubtriangleArea;

  class Impl; // Implementation class
};

#endif // svtkIntersectionPolyDataFilter_h
