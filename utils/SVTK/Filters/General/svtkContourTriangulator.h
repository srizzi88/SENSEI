/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourTriangulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkContourTriangulator
 * @brief   Fill all 2D contours to create polygons
 *
 * svtkContourTriangulator will generate triangles to fill all of the 2D
 * contours in its input.  The contours may be concave, and may even
 * contain holes i.e. a contour may contain an internal contour that
 * is wound in the opposite direction to indicate that it is a hole.
 * @warning
 * The triangulation of is done in O(n) time for simple convex
 * inputs, but for non-convex inputs the worst-case time is O(n^2*m^2)
 * where n is the number of points and m is the number of holes.
 * The best triangulation algorithms, in contrast, are O(n log n).
 * The resulting triangles may be quite narrow, the algorithm does
 * not attempt to produce high-quality triangles.
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to SVTK.
 */

#ifndef svtkContourTriangulator_h
#define svtkContourTriangulator_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCellArray;
class svtkIdList;

class SVTKFILTERSGENERAL_EXPORT svtkContourTriangulator : public svtkPolyDataAlgorithm
{
public:
  static svtkContourTriangulator* New();
  svtkTypeMacro(svtkContourTriangulator, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Check if there was a triangulation failure in the last update.
   */
  svtkGetMacro(TriangulationError, int);
  //@}

  //@{
  /**
   * Generate errors when the triangulation fails.
   * Note that triangulation failures are often minor, because they involve
   * tiny triangles that are too small to see.
   */
  svtkSetMacro(TriangulationErrorDisplay, svtkTypeBool);
  svtkBooleanMacro(TriangulationErrorDisplay, svtkTypeBool);
  svtkGetMacro(TriangulationErrorDisplay, svtkTypeBool);
  //@}

  /**
   * A robust method for triangulating a polygon.
   * It cleans up the polygon and then applies the ear-cut triangulation.
   * A zero return value indicates that triangulation failed.
   */
  static int TriangulatePolygon(svtkIdList* polygon, svtkPoints* points, svtkCellArray* triangles);

  /**
   * Given some closed contour lines, create a triangle mesh that
   * fills those lines.  The input lines must be single-segment lines,
   * not polylines.  The input lines do not have to be in order.
   * Only numLines starting from firstLine will be used.
   */
  static int TriangulateContours(svtkPolyData* data, svtkIdType firstLine, svtkIdType numLines,
    svtkCellArray* outputPolys, const double normal[3]);

protected:
  svtkContourTriangulator();
  ~svtkContourTriangulator() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int TriangulationError;
  svtkTypeBool TriangulationErrorDisplay;

private:
  svtkContourTriangulator(const svtkContourTriangulator&) = delete;
  void operator=(const svtkContourTriangulator&) = delete;
};

#endif
