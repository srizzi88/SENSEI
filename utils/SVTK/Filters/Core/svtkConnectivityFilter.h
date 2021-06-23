/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkConnectivityFilter
 * @brief   extract data based on geometric connectivity
 *
 * svtkConnectivityFilter is a filter that extracts cells that share common
 * points and/or meet other connectivity criterion. (Cells that share
 * vertices and meet other connectivity criterion such as scalar range are
 * known as a region.)  The filter works in one of six ways: 1) extract the
 * largest connected region in the dataset; 2) extract specified region
 * numbers; 3) extract all regions sharing specified point ids; 4) extract
 * all regions sharing specified cell ids; 5) extract the region closest to
 * the specified point; or 6) extract all regions (used to color the data by
 * region).
 *
 * svtkConnectivityFilter is generalized to handle any type of input dataset.
 * If the input to this filter is a svtkPolyData, the output will be a svtkPolyData.
 * For all other input types, it generates output data of type svtkUnstructuredGrid.
 * Note that the only Get*Output() methods that will return a non-null pointer
 * are GetUnstructuredGridOutput() and GetPolyDataOutput() when the output of the
 * filter is a svtkUnstructuredGrid or svtkPolyData, respectively.
 *
 * The behavior of svtkConnectivityFilter can be modified by turning on the
 * boolean ivar ScalarConnectivity. If this flag is on, the connectivity
 * algorithm is modified so that cells are considered connected only if 1)
 * they are geometrically connected (share a point) and 2) the scalar values
 * of one of the cell's points falls in the scalar range specified. This use
 * of ScalarConnectivity is particularly useful for volume datasets: it can
 * be used as a simple "connected segmentation" algorithm. For example, by
 * using a seed voxel (i.e., cell) on a known anatomical structure,
 * connectivity will pull out all voxels "containing" the anatomical
 * structure. These voxels can then be contoured or processed by other
 * visualization filters.
 *
 * If the extraction mode is set to all regions and ColorRegions is enabled,
 * The RegionIds are assigned to each region by the order in which the region
 * was processed and has no other significance with respect to the size of
 * or number of cells.
 *
 * @sa
 * svtkPolyDataConnectivityFilter
 */

#ifndef svtkConnectivityFilter_h
#define svtkConnectivityFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

#define SVTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define SVTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define SVTK_EXTRACT_SPECIFIED_REGIONS 3
#define SVTK_EXTRACT_LARGEST_REGION 4
#define SVTK_EXTRACT_ALL_REGIONS 5
#define SVTK_EXTRACT_CLOSEST_POINT_REGION 6

class svtkDataArray;
class svtkDataSet;
class svtkFloatArray;
class svtkIdList;
class svtkIdTypeArray;
class svtkIntArray;
class svtkPolyData;

class SVTKFILTERSCORE_EXPORT svtkConnectivityFilter : public svtkPointSetAlgorithm
{
public:
  svtkTypeMacro(svtkConnectivityFilter, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with default extraction mode to extract largest regions.
   */
  static svtkConnectivityFilter* New();

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
  void AddSeed(svtkIdType id);

  /**
   * Delete a seed id (point or cell id). Note: ids are 0-offset.
   */
  void DeleteSeed(svtkIdType id);

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

  /**
   * Enumeration of the various ways to assign RegionIds when
   * the ColorRegions option is on.
   */
  enum RegionIdAssignment
  {
    UNSPECIFIED,
    CELL_COUNT_DESCENDING,
    CELL_COUNT_ASCENDING
  };

  //@{
  /**
   * Set/get mode controlling how RegionIds are assigned.
   */
  //@}
  svtkSetMacro(RegionIdAssignmentMode, int);
  svtkGetMacro(RegionIdAssignmentMode, int);

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
  svtkConnectivityFilter();
  ~svtkConnectivityFilter() override;

  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // Usual data generation method
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info) override;

  svtkTypeBool ColorRegions; // boolean turns on/off scalar gen for separate regions
  int ExtractionMode;       // how to extract regions
  int OutputPointsPrecision;
  svtkIdList* Seeds;              // id's of points or cells used to seed regions
  svtkIdList* SpecifiedRegionIds; // regions specified for extraction
  svtkIdTypeArray* RegionSizes;   // size (in cells) of each region extracted

  double ClosestPoint[3];

  svtkTypeBool ScalarConnectivity;
  double ScalarRange[2];

  int RegionIdAssignmentMode;

  void TraverseAndMark(svtkDataSet* input);

  void OrderRegionIds(svtkIdTypeArray* pointRegionIds, svtkIdTypeArray* cellRegionIds);

private:
  // used to support algorithm execution
  svtkFloatArray* CellScalars;
  svtkIdList* NeighborCellPointIds;
  svtkIdType* Visited;
  svtkIdType* PointMap;
  svtkIdTypeArray* NewScalars;
  svtkIdTypeArray* NewCellScalars;
  svtkIdType RegionNumber;
  svtkIdType PointNumber;
  svtkIdType NumCellsInRegion;
  svtkDataArray* InScalars;
  svtkIdList* Wave;
  svtkIdList* Wave2;
  svtkIdList* PointIds;
  svtkIdList* CellIds;

private:
  svtkConnectivityFilter(const svtkConnectivityFilter&) = delete;
  void operator=(const svtkConnectivityFilter&) = delete;
};

//@{
/**
 * Return the method of extraction as a string.
 */
inline const char* svtkConnectivityFilter::GetExtractionModeAsString(void)
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
