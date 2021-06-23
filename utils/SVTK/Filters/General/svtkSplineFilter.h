/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSplineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSplineFilter
 * @brief   generate uniformly subdivided polylines from a set of input polyline using a svtkSpline
 *
 * svtkSplineFilter is a filter that generates an output polylines from an
 * input set of polylines. The polylines are uniformly subdivided and produced
 * with the help of a svtkSpline class that the user can specify (by default a
 * svtkCardinalSpline is used). The number of subdivisions of the line can be
 * controlled in several ways. The user can either specify the number of
 * subdivisions or a length of each subdivision can be provided (and the
 * class will figure out how many subdivisions is required over the whole
 * polyline). The maximum number of subdivisions can also be set.
 *
 * The output of this filter is a polyline per input polyline (or line). New
 * points and texture coordinates are created. Point data is interpolated and
 * cell data passed on. Any polylines with less than two points, or who have
 * coincident points, are ignored.
 *
 * @sa
 * svtkRibbonFilter svtkTubeFilter
 */

#ifndef svtkSplineFilter_h
#define svtkSplineFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_SUBDIVIDE_SPECIFIED 0
#define SVTK_SUBDIVIDE_LENGTH 1

#define SVTK_TCOORDS_OFF 0
#define SVTK_TCOORDS_FROM_NORMALIZED_LENGTH 1
#define SVTK_TCOORDS_FROM_LENGTH 2
#define SVTK_TCOORDS_FROM_SCALARS 3

class svtkCellArray;
class svtkCellData;
class svtkFloatArray;
class svtkPointData;
class svtkPoints;
class svtkSpline;

class SVTKFILTERSGENERAL_EXPORT svtkSplineFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkSplineFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct the class with no limit on the number of subdivisions
   * and using an instance of svtkCardinalSpline to perform interpolation.
   */
  static svtkSplineFilter* New();

  //@{
  /**
   * Set the maximum number of subdivisions that are created for each
   * polyline.
   */
  svtkSetClampMacro(MaximumNumberOfSubdivisions, int, 1, SVTK_INT_MAX);
  svtkGetMacro(MaximumNumberOfSubdivisions, int);
  //@}

  //@{
  /**
   * Specify how the number of subdivisions is determined.
   */
  svtkSetClampMacro(Subdivide, int, SVTK_SUBDIVIDE_SPECIFIED, SVTK_SUBDIVIDE_LENGTH);
  svtkGetMacro(Subdivide, int);
  void SetSubdivideToSpecified() { this->SetSubdivide(SVTK_SUBDIVIDE_SPECIFIED); }
  void SetSubdivideToLength() { this->SetSubdivide(SVTK_SUBDIVIDE_LENGTH); }
  const char* GetSubdivideAsString();
  //@}

  //@{
  /**
   * Set the number of subdivisions that are created for the
   * polyline. This method only has effect if Subdivisions is set
   * to SetSubdivisionsToSpecify().
   */
  svtkSetClampMacro(NumberOfSubdivisions, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfSubdivisions, int);
  //@}

  //@{
  /**
   * Control the number of subdivisions that are created for the
   * polyline based on an absolute length. The length of the spline
   * is divided by this length to determine the number of subdivisions.
   */
  svtkSetClampMacro(Length, double, 0.0000001, SVTK_DOUBLE_MAX);
  svtkGetMacro(Length, double);
  //@}

  //@{
  /**
   * Specify an instance of svtkSpline to use to perform the interpolation.
   */
  virtual void SetSpline(svtkSpline*);
  svtkGetObjectMacro(Spline, svtkSpline);
  //@}

  //@{
  /**
   * Control whether and how texture coordinates are produced. This is
   * useful for striping the output polyline. The texture coordinates
   * can be generated in three ways: a normalized (0,1) generation;
   * based on the length (divided by the texture length); and by using
   * the input scalar values.
   */
  svtkSetClampMacro(GenerateTCoords, int, SVTK_TCOORDS_OFF, SVTK_TCOORDS_FROM_SCALARS);
  svtkGetMacro(GenerateTCoords, int);
  void SetGenerateTCoordsToOff() { this->SetGenerateTCoords(SVTK_TCOORDS_OFF); }
  void SetGenerateTCoordsToNormalizedLength()
  {
    this->SetGenerateTCoords(SVTK_TCOORDS_FROM_NORMALIZED_LENGTH);
  }
  void SetGenerateTCoordsToUseLength() { this->SetGenerateTCoords(SVTK_TCOORDS_FROM_LENGTH); }
  void SetGenerateTCoordsToUseScalars() { this->SetGenerateTCoords(SVTK_TCOORDS_FROM_SCALARS); }
  const char* GetGenerateTCoordsAsString();
  //@}

  //@{
  /**
   * Control the conversion of units during the texture coordinates
   * calculation. The TextureLength indicates what length (whether
   * calculated from scalars or length) is mapped to the [0,1)
   * texture space.
   */
  svtkSetClampMacro(TextureLength, double, 0.000001, SVTK_INT_MAX);
  svtkGetMacro(TextureLength, double);
  //@}

protected:
  svtkSplineFilter();
  ~svtkSplineFilter() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int MaximumNumberOfSubdivisions;
  int Subdivide;
  int NumberOfSubdivisions;
  double Length;
  svtkSpline* Spline;
  svtkSpline* XSpline;
  svtkSpline* YSpline;
  svtkSpline* ZSpline;
  int GenerateTCoords;
  double TextureLength; // this length is mapped to [0,1) texture space

  // helper methods
  int GeneratePoints(svtkIdType offset, svtkIdType npts, const svtkIdType* pts, svtkPoints* inPts,
    svtkPoints* newPts, svtkPointData* pd, svtkPointData* outPD, int genTCoords,
    svtkFloatArray* newTCoords);

  void GenerateLine(svtkIdType offset, svtkIdType numGenPts, svtkIdType inCellId, svtkCellData* cd,
    svtkCellData* outCD, svtkCellArray* newLines);

  // helper members
  svtkFloatArray* TCoordMap;

private:
  svtkSplineFilter(const svtkSplineFilter&) = delete;
  void operator=(const svtkSplineFilter&) = delete;
};

#endif
