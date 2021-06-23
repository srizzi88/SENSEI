/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEuclideanClusterExtraction.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEuclideanClusterExtraction
 * @brief   perform segmentation based on geometric
 * proximity and optional scalar threshold
 *
 * svtkEuclideanClusterExtraction is a filter that extracts points that are in
 * close geometric proximity, and optionally satisfies a scalar threshold
 * criterion. (Points extracted in this way are referred to as clusters.)
 * The filter works in one of five ways: 1) extract the largest cluster in the
 * dataset; 2) extract specified cluster number(s); 3) extract all clusters
 * containing specified point ids; 4) extract the cluster closest to a specified
 * point; or 5) extract all clusters (which can be used for coloring the clusters).
 *
 * Note that geometric proximity is defined by setting the Radius instance
 * variable. This variable defines a local sphere around each point; other
 * points contained in this sphere are considered "connected" to the
 * point. Setting this number too large will connect clusters that should not
 * be; setting it too small will fragment the point cloud into myriad
 * clusters. To accelerate the geometric proximity operations, a point
 * locator may be specified. By default, a svtkStaticPointLocator is used, but
 * any svtkAbstractPointLocator may be specified.
 *
 * The behavior of svtkEuclideanClusterExtraction can be modified by turning
 * on the boolean ivar ScalarConnectivity. If this flag is on, the clustering
 * algorithm is modified so that points are considered part of a cluster if
 * they satisfy both the geometric proximity measure, and the points scalar
 * values falls into the scalar range specified. This use of
 * ScalarConnectivity is particularly useful for data with intensity or color
 * information, serving as a simple "connected segmentation" algorithm. For
 * example, by using a seed point in a known cluster, clustering will pull
 * out all points "representing" the local structure.
 *
 * @sa
 * svtkConnectivityFilter svtkPolyDataConnectivityFilter
 */

#ifndef svtkEuclideanClusterExtraction_h
#define svtkEuclideanClusterExtraction_h

#include "svtkFiltersPointsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_EXTRACT_POINT_SEEDED_CLUSTERS 1
#define SVTK_EXTRACT_SPECIFIED_CLUSTERS 2
#define SVTK_EXTRACT_LARGEST_CLUSTER 3
#define SVTK_EXTRACT_ALL_CLUSTERS 4
#define SVTK_EXTRACT_CLOSEST_POINT_CLUSTER 5

class svtkDataArray;
class svtkFloatArray;
class svtkIdList;
class svtkIdTypeArray;
class svtkAbstractPointLocator;

class SVTKFILTERSPOINTS_EXPORT svtkEuclideanClusterExtraction : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkEuclideanClusterExtraction, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with default extraction mode to extract largest clusters.
   */
  static svtkEuclideanClusterExtraction* New();

  //@{
  /**
   * Specify the local search radius.
   */
  svtkSetClampMacro(Radius, double, 0.0, SVTK_FLOAT_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Turn on/off connectivity based on scalar value. If on, points are
   * connected only if the are proximal AND the scalar value of a candidate
   * point falls in the scalar range specified. Of course input point scalar
   * data must be provided.
   */
  svtkSetMacro(ScalarConnectivity, bool);
  svtkGetMacro(ScalarConnectivity, bool);
  svtkBooleanMacro(ScalarConnectivity, bool);
  //@}

  //@{
  /**
   * Set the scalar range used to extract points based on scalar connectivity.
   */
  svtkSetVector2Macro(ScalarRange, double);
  svtkGetVector2Macro(ScalarRange, double);
  //@}

  //@{
  /**
   * Control the extraction of connected surfaces.
   */
  svtkSetClampMacro(
    ExtractionMode, int, SVTK_EXTRACT_POINT_SEEDED_CLUSTERS, SVTK_EXTRACT_CLOSEST_POINT_CLUSTER);
  svtkGetMacro(ExtractionMode, int);
  void SetExtractionModeToPointSeededClusters()
  {
    this->SetExtractionMode(SVTK_EXTRACT_POINT_SEEDED_CLUSTERS);
  }
  void SetExtractionModeToLargestCluster() { this->SetExtractionMode(SVTK_EXTRACT_LARGEST_CLUSTER); }
  void SetExtractionModeToSpecifiedClusters()
  {
    this->SetExtractionMode(SVTK_EXTRACT_SPECIFIED_CLUSTERS);
  }
  void SetExtractionModeToClosestPointCluster()
  {
    this->SetExtractionMode(SVTK_EXTRACT_CLOSEST_POINT_CLUSTER);
  }
  void SetExtractionModeToAllClusters() { this->SetExtractionMode(SVTK_EXTRACT_ALL_CLUSTERS); }
  const char* GetExtractionModeAsString();
  //@}

  /**
   * Initialize the list of point ids used to seed clusters.
   */
  void InitializeSeedList();

  /**
   * Add a seed id (point id). Note: ids are 0-offset.
   */
  void AddSeed(svtkIdType id);

  /**
   * Delete a seed id.a
   */
  void DeleteSeed(svtkIdType id);

  /**
   * Initialize the list of cluster ids to extract.
   */
  void InitializeSpecifiedClusterList();

  /**
   * Add a cluster id to extract. Note: ids are 0-offset.
   */
  void AddSpecifiedCluster(int id);

  /**
   * Delete a cluster id to extract.
   */
  void DeleteSpecifiedCluster(int id);

  //@{
  /**
   * Used to specify the x-y-z point coordinates when extracting the cluster
   * closest to a specified point.
   */
  svtkSetVector3Macro(ClosestPoint, double);
  svtkGetVectorMacro(ClosestPoint, double, 3);
  //@}

  /**
   * Obtain the number of connected clusters. This value is valid only after filter execution.
   */
  int GetNumberOfExtractedClusters();

  //@{
  /**
   * Turn on/off the coloring of connected clusters.
   */
  svtkSetMacro(ColorClusters, bool);
  svtkGetMacro(ColorClusters, bool);
  svtkBooleanMacro(ColorClusters, bool);
  //@}

  //@{
  /**
   * Specify a point locator. By default a svtkStaticPointLocator is
   * used. The locator performs efficient proximity searches near a
   * specified interpolation position.
   */
  void SetLocator(svtkAbstractPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkAbstractPointLocator);
  //@}

protected:
  svtkEuclideanClusterExtraction();
  ~svtkEuclideanClusterExtraction() override;

  double Radius;                  // connection radius
  bool ColorClusters;             // boolean turns on/off scalar gen for separate clusters
  int ExtractionMode;             // how to extract clusters
  svtkIdList* Seeds;               // id's of points or cells used to seed clusters
  svtkIdList* SpecifiedClusterIds; // clusters specified for extraction
  svtkIdTypeArray* ClusterSizes;   // size (in cells) of each cluster extracted

  double ClosestPoint[3];

  bool ScalarConnectivity;
  double ScalarRange[2];

  svtkAbstractPointLocator* Locator;

  // Configure the pipeline
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Internal method for propagating connected waves.
  void InsertIntoWave(svtkIdList* wave, svtkIdType ptId);
  void TraverseAndMark(svtkPoints* pts);

private:
  svtkEuclideanClusterExtraction(const svtkEuclideanClusterExtraction&) = delete;
  void operator=(const svtkEuclideanClusterExtraction&) = delete;

  // used to support algorithm execution
  svtkFloatArray* NeighborScalars;
  svtkIdList* NeighborPointIds;
  char* Visited;
  svtkIdType* PointMap;
  svtkIdTypeArray* NewScalars;
  svtkIdType ClusterNumber;
  svtkIdType PointNumber;
  svtkIdType NumPointsInCluster;
  svtkDataArray* InScalars;
  svtkIdList* Wave;
  svtkIdList* Wave2;
  svtkIdList* PointIds;
};

//@{
/**
 * Return the method of extraction as a string.
 */
inline const char* svtkEuclideanClusterExtraction::GetExtractionModeAsString(void)
{
  if (this->ExtractionMode == SVTK_EXTRACT_POINT_SEEDED_CLUSTERS)
  {
    return "ExtractPointSeededClusters";
  }
  else if (this->ExtractionMode == SVTK_EXTRACT_SPECIFIED_CLUSTERS)
  {
    return "ExtractSpecifiedClusters";
  }
  else if (this->ExtractionMode == SVTK_EXTRACT_ALL_CLUSTERS)
  {
    return "ExtractAllClusters";
  }
  else if (this->ExtractionMode == SVTK_EXTRACT_CLOSEST_POINT_CLUSTER)
  {
    return "ExtractClosestPointCluster";
  }
  else
  {
    return "ExtractLargestCluster";
  }
}
//@}

#endif
