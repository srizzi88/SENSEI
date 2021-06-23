/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataConnectivityFilter
 * @brief   extract polygonal data based on geometric connectivity
 *
 * svtkPolyDataConnectivityFilter is a filter that extracts cells that
 * share common points and/or satisfy a scalar threshold
 * criterion. (Such a group of cells is called a region.) The filter
 * works in one of six ways: 1) extract the largest (most points) connected region
 * in the dataset; 2) extract specified region numbers; 3) extract all
 * regions sharing specified point ids; 4) extract all regions sharing
 * specified cell ids; 5) extract the region closest to the specified
 * point; or 6) extract all regions (used to color regions).
 *
 * This filter is specialized for polygonal data. This means it runs a bit
 * faster and is easier to construct visualization networks that process
 * polygonal data.
 *
 * The behavior of svtkPolyDataConnectivityFilter can be modified by turning
 * on the boolean ivar ScalarConnectivity. If this flag is on, the
 * connectivity algorithm is modified so that cells are considered connected
 * only if 1) they are geometrically connected (share a point) and 2) the
 * scalar values of the cell's points falls in the scalar range specified.
 * If ScalarConnectivity and FullScalarConnectivity is ON, all the cell's
 * points must lie in the scalar range specified for the cell to qualify as
 * being connected. If FullScalarConnectivity is OFF, any one of the cell's
 * points may lie in the user specified scalar range for the cell to qualify
 * as being connected.
 *
 * This use of ScalarConnectivity is particularly useful for selecting cells
 * for later processing.
 *
 * @sa
 * svtkConnectivityFilter
 */

#ifndef svtkPolyDataConnectivityFilter_h
#define svtkPolyDataConnectivityFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define SVTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define SVTK_EXTRACT_SPECIFIED_REGIONS 3
#define SVTK_EXTRACT_LARGEST_REGION 4
#define SVTK_EXTRACT_ALL_REGIONS 5
#define SVTK_EXTRACT_CLOSEST_POINT_REGION 6

class svtkDataArray;
class svtkIdList;
class svtkIdTypeArray;

class SVTKFILTERSCORE_EXPORT svtkPolyDataConnectivityFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkPolyDataConnectivityFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Obtain the array containing the region sizes of the extracted
   * regions
   */
  svtkGetObjectMacro(RegionSizes, svtkIdTypeArray);
  //@}

  /**
   * Construct with default extraction mode to extract largest regions.
   */
  static svtkPolyDataConnectivityFilter* New();

  //@{
  /**
   * Turn on/off connectivity based on scalar value. If on, cells are connected
   * only if they share points AND one of the cells scalar values falls in the
   * scalar range specified.
   */
  svtkSetMacro(ScalarConnectivity, svtkTypeBool);
  svtkGetMacro(ScalarConnectivity, svtkTypeBool);
  svtkBooleanMacro(ScalarConnectivity, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the use of Fully connected scalar connectivity. This is off
   * by default. The flag is used only if ScalarConnectivity is on. If
   * FullScalarConnectivity is ON, all the cell's points must lie in the
   * scalar range specified for the cell to qualify as being connected. If
   * FullScalarConnectivity is OFF, any one of the cell's points may lie in
   * the user specified scalar range for the cell to qualify as being
   * connected.
   */
  svtkSetMacro(FullScalarConnectivity, svtkTypeBool);
  svtkGetMacro(FullScalarConnectivity, svtkTypeBool);
  svtkBooleanMacro(FullScalarConnectivity, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the scalar range to use to extract cells based on scalar connectivity.
   */
  svtkSetVector2Macro(ScalarRange, double);
  svtkGetVector2Macro(ScalarRange, double);
  //@}

  //@{
  /**
   * Control the extraction of connected surfaces.
   */
  svtkSetClampMacro(
    ExtractionMode, int, SVTK_EXTRACT_POINT_SEEDED_REGIONS, SVTK_EXTRACT_CLOSEST_POINT_REGION);
  svtkGetMacro(ExtractionMode, int);
  void SetExtractionModeToPointSeededRegions()
  {
    this->SetExtractionMode(SVTK_EXTRACT_POINT_SEEDED_REGIONS);
  }
  void SetExtractionModeToCellSeededRegions()
  {
    this->SetExtractionMode(SVTK_EXTRACT_CELL_SEEDED_REGIONS);
  }
  void SetExtractionModeToLargestRegion() { this->SetExtractionMode(SVTK_EXTRACT_LARGEST_REGION); }
  void SetExtractionModeToSpecifiedRegions()
  {
    this->SetExtractionMode(SVTK_EXTRACT_SPECIFIED_REGIONS);
  }
  void SetExtractionModeToClosestPointRegion()
  {
    this->SetExtractionMode(SVTK_EXTRACT_CLOSEST_POINT_REGION);
  }
  void SetExtractionModeToAllRegions() { this->SetExtractionMode(SVTK_EXTRACT_ALL_REGIONS); }
  const char* GetExtractionModeAsString();
  //@}

  /**
   * Initialize list of point ids/cell ids used to seed regions.
   */
  void InitializeSeedList();

  /**
   * Add a seed id (point or cell id). Note: ids are 0-offset.
   */
  void AddSeed(int id);

  /**
   * Delete a seed id (point or cell id). Note: ids are 0-offset.
   */
  void DeleteSeed(int id);

  /**
   * Initialize list of region ids to extract.
   */
  void InitializeSpecifiedRegionList();

  /**
   * Add a region id to extract. Note: ids are 0-offset.
   */
  void AddSpecifiedRegion(int id);

  /**
   * Delete a region id to extract. Note: ids are 0-offset.
   */
  void DeleteSpecifiedRegion(int id);

  //@{
  /**
   * Use to specify x-y-z point coordinates when extracting the region
   * closest to a specified point.
   */
  svtkSetVector3Macro(ClosestPoint, double);
  svtkGetVectorMacro(ClosestPoint, double, 3);
  //@}

  /**
   * Obtain the number of connected regions.
   */
  int GetNumberOfExtractedRegions();

  //@{
  /**
   * Turn on/off the coloring of connected regions.
   */
  svtkSetMacro(ColorRegions, svtkTypeBool);
  svtkGetMacro(ColorRegions, svtkTypeBool);
  svtkBooleanMacro(ColorRegions, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify whether to record input point ids that appear in the output connected
   * components. It may be useful to extract the visited point ids for use by a
   * downstream filter. Default is OFF.
   */
  svtkSetMacro(MarkVisitedPointIds, svtkTypeBool);
  svtkGetMacro(MarkVisitedPointIds, svtkTypeBool);
  svtkBooleanMacro(MarkVisitedPointIds, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the input point ids that appear in the output connected components. This is
   * non-empty only if MarkVisitedPointIds has been set.
   */
  svtkGetObjectMacro(VisitedPointIds, svtkIdList);
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
  svtkPolyDataConnectivityFilter();
  ~svtkPolyDataConnectivityFilter() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool ColorRegions;      // boolean turns on/off scalar gen for separate regions
  int ExtractionMode;            // how to extract regions
  svtkIdList* Seeds;              // id's of points or cells used to seed regions
  svtkIdList* SpecifiedRegionIds; // regions specified for extraction
  svtkIdTypeArray* RegionSizes;   // size (in cells) of each region extracted

  double ClosestPoint[3];

  svtkTypeBool ScalarConnectivity;
  svtkTypeBool FullScalarConnectivity;

  // Does this cell qualify as being scalar connected ?
  int IsScalarConnected(svtkIdType cellId);

  double ScalarRange[2];

  void TraverseAndMark();

  // used to support algorithm execution
  svtkDataArray* CellScalars;
  svtkIdList* NeighborCellPointIds;
  svtkIdType* Visited;
  svtkIdType* PointMap;
  svtkDataArray* NewScalars;
  svtkIdType RegionNumber;
  svtkIdType PointNumber;
  svtkIdType NumCellsInRegion;
  svtkDataArray* InScalars;
  svtkPolyData* Mesh;
  std::vector<svtkIdType> Wave;
  std::vector<svtkIdType> Wave2;
  svtkIdList* PointIds;
  svtkIdList* CellIds;
  svtkIdList* VisitedPointIds;

  svtkTypeBool MarkVisitedPointIds;
  int OutputPointsPrecision;

private:
  svtkPolyDataConnectivityFilter(const svtkPolyDataConnectivityFilter&) = delete;
  void operator=(const svtkPolyDataConnectivityFilter&) = delete;
};

//@{
/**
 * Return the method of extraction as a string.
 */
inline const char* svtkPolyDataConnectivityFilter::GetExtractionModeAsString(void)
{
  if (this->ExtractionMode == SVTK_EXTRACT_POINT_SEEDED_REGIONS)
  {
    return "ExtractPointSeededRegions";
  }
  else if (this->ExtractionMode == SVTK_EXTRACT_CELL_SEEDED_REGIONS)
  {
    return "ExtractCellSeededRegions";
  }
  else if (this->ExtractionMode == SVTK_EXTRACT_SPECIFIED_REGIONS)
  {
    return "ExtractSpecifiedRegions";
  }
  else if (this->ExtractionMode == SVTK_EXTRACT_ALL_REGIONS)
  {
    return "ExtractAllRegions";
  }
  else if (this->ExtractionMode == SVTK_EXTRACT_CLOSEST_POINT_REGION)
  {
    return "ExtractClosestPointRegion";
  }
  else
  {
    return "ExtractLargestRegion";
  }
}
//@}

#endif
