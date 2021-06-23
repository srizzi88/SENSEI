/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOTDensityMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOTDensityMap
 * @brief
 * A SVTK Filter to compute density map on any pair of numeric data arrays
 * with the same number of tuples, using OpenTURNS.
 * The Output will be a MultiBlock of table, each table containing
 * X and Y coordinate of a density map line.
 *
 */

#ifndef svtkOTDensityMap_h
#define svtkOTDensityMap_h

#include "svtkContourValues.h"          // For Contour Values
#include "svtkFiltersOpenTURNSModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"
#include "svtkSmartPointer.h" // For Smart Pointer

#include <map> // For map

class svtkIdList;
class svtkInformationDoubleKey;
class svtkPolyData;
class svtkTable;

class SVTKFILTERSOPENTURNS_EXPORT svtkOTDensityMap : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkOTDensityMap* New();
  svtkTypeMacro(svtkOTDensityMap, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Check contour values to return actual mtime
   */
  virtual svtkMTimeType GetMTime() override;

  //@{
  /**
   * Methods to set / get density lines values.
   * Values are expected to be between 0 and 1.
   * Modifying these parameters does not trigger a pdf
   * computation, thus will be very fast to compute,
   * empty by default
   */
  /**
   * Set a particular contour value at contour number i. The index i ranges
   * between 0<=i<NumberOfContours.
   */
  void SetValue(int i, double value);

  /**
   * Get the ith contour value.
   */
  double GetValue(int i);

  /**
   * Get a pointer to an array of contour values. There will be
   * GetNumberOfContours() values in the list.
   */
  double* GetValues();

  /**
   * Fill a supplied list with contour values. There will be
   * GetNumberOfContours() values in the list. Make sure you allocate
   * enough memory to hold the list.
   */
  void GetValues(double* contourValues);

  /**
   * Set the number of contours to place into the list. You only really
   * need to use this method to reduce list size. The method SetValue()
   * will automatically increase list size as needed.
   */
  void SetNumberOfContours(int number);

  /**
   * Get the number of contours in the list of contour values.
   */
  int GetNumberOfContours();
  //@}

  //@{
  /**
   * Methods to set / get grid subdivisions,
   * aka the number of point in each dimensions of the grid
   * for computing the PDF.
   * Modifying this parameter will trigger a recomputation of a PDF
   * and LogPDF, 50 by default
   */
  void SetGridSubdivisions(int gridSubdivisions);
  svtkGetMacro(GridSubdivisions, int);
  //@}

  //@{
  /**
   * Methods to set / get number of points to compute
   * the contour values approximations using a LogPDF.
   * It ensure the DensityLogPDFSampleCache time is modified.
   * Modyfying the parameter will trigger a recomputation of the
   * LogPDF only, 600 by default
   */
  svtkGetMacro(ContourApproximationNumberOfPoints, int);
  virtual void SetContourApproximationNumberOfPoints(int val);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Key to recover density in output metadata
   */
  static svtkInformationDoubleKey* DENSITY();

protected:
  svtkOTDensityMap();
  ~svtkOTDensityMap() override;

  /**
   * Protected method to find the next cellid in specified direction on
   * a polydata containing only lines or polylines.
   * pd is the polydata input containing only poly/lines
   * cellId is the current cellId to start from
   * previousCellId is an optional parameter allowing to ensure the direction
   * on the line we advance. If > 0, the result of this method is ensured to be !=
   * previousCellId.
   * invertedPoints is an output, informing that the current cellId has inverted points
   * up is an input, allowing to specify the direction to follow. In any case, if the
   * next cell id cannot be found in this direction, the other direction will be tried, this is the
   * case where we consider the cell points are inverted.
   * currentPointIndices is an optional output, if != nullptr, the current cell points will be
   * stored in.
   */
  virtual svtkIdType FindNextCellId(svtkPolyData* pd, svtkIdType cellId, svtkIdType previousCellId,
    bool& invertedPoints, bool up = true, svtkIdList* currentPointIndices = nullptr);

  void ClearCache();
  void BuildContours(svtkPolyData* contourPd, int numContours, const double* contourValues,
    const double* densityPDFContourValues, const char* xArrayName, const char* yArrayName,
    std::multimap<double, svtkSmartPointer<svtkTable> >& contoursMap);

  // Cache
  class OTDensityCache;
  class OTDistributionCache;
  OTDensityCache* DensityPDFCache;
  OTDensityCache* DensityLogPDFSampleCache;
  OTDistributionCache* DistributionCache;

  svtkTimeStamp BuildTime;                // Keep track of last build time
  svtkTimeStamp DensityLogPDFSampleMTime; // Keep track of DensityLogPDFSample Parameters mtime
  svtkTimeStamp DensityPDFMTime;          // Keep track of DensityPDF Parameters modification time

  svtkContourValues* ContourValues;
  int GridSubdivisions;
  int ContourApproximationNumberOfPoints;

private:
  void operator=(const svtkOTDensityMap&) = delete;
  svtkOTDensityMap(const svtkOTDensityMap&) = delete;
};
#endif
