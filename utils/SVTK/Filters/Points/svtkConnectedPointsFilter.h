/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConnectedPointsFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkConnectedPointsFilter
 * @brief   extract / segment points based on geometric connectivity
 *
 * svtkConnectedPointsFilter is a filter that extracts and/or segments points
 * from a point cloud based on geometric distance measures (e.g., proximity,
 * normal alignments, etc.) and optional measures such as scalar range. The
 * default operation is to segment the points into "connected" regions where
 * the connection is determined by an appropriate distance measure. Each
 * region is given a region id. Optionally, the filter can output the largest
 * connected region of points; a particular region (via id specification);
 * those regions that are seeded using a list of input point ids; or the
 * region of points closest to a specified position.
 *
 * The key parameter of this filter is the radius defining a sphere around
 * each point which defines a local neighborhood: any other points in the
 * local neighborhood are assumed connected to the point. Note that the
 * radius is defined in absolute terms.
 *
 * Other parameters are used to further qualify what it means to be a
 * neighboring point. For example, scalar range and/or point normals can be
 * used to further constrain the neighborhood. Also the extraction mode
 * defines how the filter operates. By default, all regions are extracted but
 * it is possible to extract particular regions; the region closest to a seed
 * point; seeded regions; or the largest region found while processing. By
 * default, all regions are extracted.
 *
 * On output, all points are labeled with a region number. However note that
 * the number of input and output points may not be the same: if not
 * extracting all regions then the output size may be less than the input
 * size.
 *
 * @sa
 * svtkPolyDataConnectivityFilter svtkConnectivityFilter
 */

#ifndef svtkConnectedPointsFilter_h
#define svtkConnectedPointsFilter_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

// Make these consistent with the other connectivity filters
#define SVTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define SVTK_EXTRACT_SPECIFIED_REGIONS 3
#define SVTK_EXTRACT_LARGEST_REGION 4
#define SVTK_EXTRACT_ALL_REGIONS 5
#define SVTK_EXTRACT_CLOSEST_POINT_REGION 6

class svtkAbstractPointLocator;
class svtkDataArray;
class svtkFloatArray;
class svtkIdList;
class svtkIdTypeArray;
class svtkIntArray;

class SVTKFILTERSPOINTS_EXPORT svtkConnectedPointsFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkConnectedPointsFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with default extraction mode to extract the largest region.
   */
  static svtkConnectedPointsFilter* New();

  //@{
  /**
   * Set / get the radius variable specifying a local sphere used to define
   * local point neighborhood.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Control the extraction of connected regions.
   */
  svtkSetClampMacro(
    ExtractionMode, int, SVTK_EXTRACT_POINT_SEEDED_REGIONS, SVTK_EXTRACT_CLOSEST_POINT_REGION);
  svtkGetMacro(ExtractionMode, int);
  void SetExtractionModeToPointSeededRegions()
  {
    this->SetExtractionMode(SVTK_EXTRACT_POINT_SEEDED_REGIONS);
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

  //@{
  /**
   * Use to specify x-y-z point coordinates when extracting the region
   * closest to a specified point.
   */
  svtkSetVector3Macro(ClosestPoint, double);
  svtkGetVectorMacro(ClosestPoint, double, 3);
  //@}

  /**
   * Initialize list of point ids ids used to seed regions.
   */
  void InitializeSeedList();

  /**
   * Add a non-negative point seed id. Note: ids are 0-offset.
   */
  void AddSeed(svtkIdType id);

  /**
   * Delete a point seed id. Note: ids are 0-offset.
   */
  void DeleteSeed(svtkIdType id);

  /**
   * Initialize list of region ids to extract.
   */
  void InitializeSpecifiedRegionList();

  /**
   * Add a non-negative region id to extract. Note: ids are 0-offset.
   */
  void AddSpecifiedRegion(svtkIdType id);

  /**
   * Delete a region id to extract. Note: ids are 0-offset.
   */
  void DeleteSpecifiedRegion(svtkIdType id);

  //@{
  /**
   * Turn on/off connectivity based on point normal consistency. If on, and
   * point normals are defined, points are connected only if they satisfy
   * other criterion (e.g., geometric proximity, scalar connectivity, etc.)
   * AND the angle between normals is no greater than NormalAngle;
   */
  svtkSetMacro(AlignedNormals, int);
  svtkGetMacro(AlignedNormals, int);
  svtkBooleanMacro(AlignedNormals, int);
  //@}

  //@{
  /**
   * Specify a threshold for normal angles. If AlignedNormalsOn is set, then
   * points are connected if the angle between their normals is within this
   * angle threshold (expressed in degress).
   */
  svtkSetClampMacro(NormalAngle, double, 0.0001, 90.0);
  svtkGetMacro(NormalAngle, double);
  //@}

  //@{
  /**
   * Turn on/off connectivity based on scalar value. If on, points are
   * connected only if they satisfy the various geometric criterion AND one
   * of the points scalar values falls in the scalar range specified.
   */
  svtkSetMacro(ScalarConnectivity, int);
  svtkGetMacro(ScalarConnectivity, int);
  svtkBooleanMacro(ScalarConnectivity, int);
  //@}

  //@{
  /**
   * Set the scalar range to use to extract points based on scalar connectivity.
   */
  svtkSetVector2Macro(ScalarRange, double);
  svtkGetVector2Macro(ScalarRange, double);
  //@}

  /**
   * Obtain the number of connected regions. The return value is valid only
   * after the filter has executed.
   */
  int GetNumberOfExtractedRegions();

  //@{
  /**
   * Specify a point locator. By default a svtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * around a sample point.
   */
  void SetLocator(svtkAbstractPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkAbstractPointLocator);
  //@}

protected:
  svtkConnectedPointsFilter();
  ~svtkConnectedPointsFilter() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // The radius defines the proximal neighborhood of points
  double Radius;

  // indicate how to extract regions
  int ExtractionMode;

  // id's of points used to seed regions
  svtkIdList* Seeds;

  // regions specified for extraction
  svtkIdList* SpecifiedRegionIds;

  // Seed with a closest point
  double ClosestPoint[3];

  // Segment based on nearly aligned normals
  int AlignedNormals;
  double NormalAngle;
  double NormalThreshold;

  // Support segmentation based on scalar connectivity
  int ScalarConnectivity;
  double ScalarRange[2];

  // accelerate searching
  svtkAbstractPointLocator* Locator;

  // Wave propagation used to segment points
  void TraverseAndMark(
    svtkPoints* inPts, svtkDataArray* inScalars, float* normals, svtkIdType* labels);

private:
  // used to support algorithm execution
  svtkIdType CurrentRegionNumber;
  svtkIdTypeArray* RegionLabels;
  svtkIdType NumPointsInRegion;
  svtkIdTypeArray* RegionSizes;
  svtkIdList* NeighborPointIds; // avoid repetitive new/delete
  svtkIdList* Wave;
  svtkIdList* Wave2;

private:
  svtkConnectedPointsFilter(const svtkConnectedPointsFilter&) = delete;
  void operator=(const svtkConnectedPointsFilter&) = delete;
};

//@{
/**
 * Return the method of extraction as a string.
 */
inline const char* svtkConnectedPointsFilter::GetExtractionModeAsString(void)
{
  if (this->ExtractionMode == SVTK_EXTRACT_POINT_SEEDED_REGIONS)
  {
    return "ExtractPointSeededRegions";
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
