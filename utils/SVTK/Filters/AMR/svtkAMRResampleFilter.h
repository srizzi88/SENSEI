/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRResampleFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMRResampleFilter
 *
 *
 *  This filter is a concrete instance of svtkMultiBlockDataSetAlgorithm and
 *  provides functionality for extracting portion of the AMR dataset, specified
 *  by a bounding box, in a uniform grid of the desired level of resolution.
 *  The resulting uniform grid is stored in a svtkMultiBlockDataSet where the
 *  number of blocks correspond to the number of processors utilized for the
 *  operation.
 *
 * @warning
 *  Data of the input AMR dataset is assumed to be cell-centered.
 *
 * @sa
 *  svtkOverlappingAMR, svtkUniformGrid
 */

#ifndef svtkAMRResampleFilter_h
#define svtkAMRResampleFilter_h

#include "svtkFiltersAMRModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"
#include <vector> // For STL vector

class svtkInformation;
class svtkInformationVector;
class svtkUniformGrid;
class svtkOverlappingAMR;
class svtkMultiBlockDataSet;
class svtkMultiProcessController;
class svtkFieldData;
class svtkCellData;
class svtkPointData;
class svtkIndent;

class svtkAMRBox;
class SVTKFILTERSAMR_EXPORT svtkAMRResampleFilter : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkAMRResampleFilter* New();
  svtkTypeMacro(svtkAMRResampleFilter, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& oss, svtkIndent indent) override;

  //@{
  /**
   * Set & Get macro for the number of samples (cells) in each dimension.
   * Nominal value for the number of samples is 10x10x10.
   */
  svtkSetVector3Macro(NumberOfSamples, int);
  svtkGetVector3Macro(NumberOfSamples, int);
  //@}

  //@{
  /**
   * Set & Get macro for the TransferToNodes flag
   */
  svtkSetMacro(TransferToNodes, int);
  svtkGetMacro(TransferToNodes, int);
  //@}

  //@{
  /**
   * Set & Get macro to allow the filter to operate in both demand-driven
   * and standard modes
   */
  svtkSetMacro(DemandDrivenMode, int);
  svtkGetMacro(DemandDrivenMode, int);
  //@}

  //@{
  /**
   * Set & Get macro for the number of subdivisions
   */
  svtkSetMacro(NumberOfPartitions, int);
  svtkGetMacro(NumberOfPartitions, int);
  //@}

  //@{
  /**
   * Set and Get the min corner
   */
  svtkSetVector3Macro(Min, double);
  svtkGetVector3Macro(Min, double);
  //@}

  //@{
  /**
   * Set and Get the max corner
   */
  svtkSetVector3Macro(Max, double);
  svtkGetVector3Macro(Max, double);
  //@}

  //@{
  /**
   * Set & Get macro for the number of subdivisions
   */
  svtkSetMacro(UseBiasVector, bool);
  svtkGetMacro(UseBiasVector, bool);
  //@}

  //@{
  /**
   * Set and Get the bias vector.  If UseBiasVector is true
   * then the largest component of this vector can not have
   * the max number of samples
   */
  svtkSetVector3Macro(BiasVector, double);
  svtkGetVector3Macro(BiasVector, double);
  //@}

  //@{
  /**
   * Set & Get macro for the multi-process controller
   */
  svtkSetMacro(Controller, svtkMultiProcessController*);
  svtkGetMacro(Controller, svtkMultiProcessController*);
  //@}

  // Standard pipeline routines

  /**
   * Gets the metadata from upstream module and determines which blocks
   * should be loaded by this instance.
   */
  int RequestInformation(svtkInformation* rqst, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Performs upstream requests to the reader
   */
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkAMRResampleFilter();
  ~svtkAMRResampleFilter() override;

  svtkOverlappingAMR* AMRMetaData;
  svtkMultiBlockDataSet* ROI; // Pointer to the region of interest.
  int NumberOfSamples[3];
  int GridNumberOfSamples[3];
  double Min[3];
  double Max[3];
  double GridMin[3];
  double GridMax[3];
  int LevelOfResolution;
  int NumberOfPartitions;
  int TransferToNodes;
  int DemandDrivenMode;
  svtkMultiProcessController* Controller;
  bool UseBiasVector;
  double BiasVector[3];

  // Debugging Stuff
  int NumberOfBlocksTestedForLevel;
  int NumberOfBlocksTested;
  int NumberOfBlocksVisSkipped;
  int NumberOfTimesFoundOnDonorLevel;
  int NumberOfTimesLevelUp;
  int NumberOfTimesLevelDown;
  int NumberOfFailedPoints;
  double AverageLevel;

  std::vector<int> BlocksToLoad; // Holds the ids of the blocks to load.

  /**
   * Checks if this filter instance is running on more than one processes
   */
  bool IsParallel();

  /**
   * Given the Region ID this function returns whether or not the region
   * belongs to this process or not.
   */
  bool IsRegionMine(const int regionIdx);

  /**
   * Given the Region ID, this method computes the corresponding process ID
   * that owns the region based on static block-cyclic distribution.
   */
  int GetRegionProcessId(const int regionIdx);

  /**
   * Given a cell index and a grid, this method computes the cell centroid.
   */
  void ComputeCellCentroid(svtkUniformGrid* g, const svtkIdType cellIdx, double c[3]);

  /**
   * Given the source cell data of an AMR grid, this method initializes the
   * field values, i.e., the number of arrays with the prescribed size. Note,
   * the size must correspond to the number of points if node-centered or the
   * the number of cells if cell-centered.
   */
  void InitializeFields(svtkFieldData* f, svtkIdType size, svtkCellData* src);

  /**
   * Copies the data to the target from the given source.
   */
  void CopyData(svtkFieldData* target, svtkIdType targetIdx, svtkCellData* src, svtkIdType srcIdx);

  /**
   * Given a query point q and a candidate donor grid, this method checks for
   * the corresponding donor cell containing the point in the given grid.
   */
  bool FoundDonor(double q[3], svtkUniformGrid*& donorGrid, int& cellIdx);

  /**
   * Given a query point q and a target level, this method finds a suitable
   * grid at the given level that contains the point if one exists. If a grid
   * is not found, donorGrid is set to nullptr.
   */
  bool SearchForDonorGridAtLevel(double q[3], svtkOverlappingAMR* amrds, unsigned int level,
    unsigned int& gridId, int& donorCellIdx);

  /**
   * Finds the AMR grid that contains the point q. If donorGrid points to a
   * valid AMR grid in the hierarchy, the algorithm will search this grid
   * first. The method returns the ID of the cell w.r.t. the donorGrid that
   * contains the probe point q.
   */
  int ProbeGridPointInAMR(double q[3], unsigned int& donorLevel, unsigned int& donorGridId,
    svtkOverlappingAMR* amrds, unsigned int maxLevel, bool useCached);

  /**
   * Finds the AMR grid that contains the point q. If donorGrid points to a
   * valid AMR grid in the hierarchy, the algorithm will search this grid
   * first. The method returns the ID of the cell w.r.t. the donorGrid that
   * contains the probe point q. - Makes use of Parent/Child Info
   */
  int ProbeGridPointInAMRGraph(double q[3], unsigned int& donorLevel, unsigned int& donorGridId,
    svtkOverlappingAMR* amrds, unsigned int maxLevel, bool useCached);

  /**
   * Transfers the solution from the AMR dataset to the cell-centers of
   * the given uniform grid.
   */
  void TransferToCellCenters(svtkUniformGrid* g, svtkOverlappingAMR* amrds);

  /**
   * Transfer the solution from the AMR dataset to the nodes of the
   * given uniform grid.
   */
  void TransferToGridNodes(svtkUniformGrid* g, svtkOverlappingAMR* amrds);

  /**
   * Transfers the solution
   */
  void TransferSolution(svtkUniformGrid* g, svtkOverlappingAMR* amrds);

  /**
   * Extract the region (as a multiblock) from the given AMR dataset.
   */
  void ExtractRegion(
    svtkOverlappingAMR* amrds, svtkMultiBlockDataSet* mbds, svtkOverlappingAMR* metadata);

  /**
   * Checks if the AMR block, described by a uniform grid, is within the
   * bounds of the ROI perscribed by the user.
   */
  bool IsBlockWithinBounds(double* grd);

  /**
   * Given a user-supplied region of interest and the metadata by a module
   * upstream, this method generates the list of linear AMR block indices
   * that need to be loaded.
   */
  void ComputeAMRBlocksToLoad(svtkOverlappingAMR* metadata);

  /**
   * Computes the region parameters
   */
  void ComputeRegionParameters(
    svtkOverlappingAMR* amrds, int N[3], double min[3], double max[3], double h[3]);

  /**
   * This method accesses the domain boundaries
   */
  void GetDomainParameters(svtkOverlappingAMR* amr, double domainMin[3], double domainMax[3],
    double h[3], int dims[3], double& rf);

  /**
   * Checks if the domain and requested region intersect.
   */
  bool RegionIntersectsWithAMR(
    double domainMin[3], double domainMax[3], double regionMin[3], double regionMax[3]);

  /**
   * This method adjust the numbers of samples in the region, N, if the
   * requested region falls outside, but, intersects the domain.
   */
  void AdjustNumberOfSamplesInRegion(const double Rh[3], const bool outside[6], int N[3]);

  /**
   * This method computes the level of resolution based on the number of
   * samples requested, N, the root level spacing h0, the length of the box,
   * L (actual length after snapping) and the refinement ratio.
   */
  void ComputeLevelOfResolution(
    const int N[3], const double h0[3], const double L[3], const double rf);

  /**
   * This method snaps the bounds s.t. they are within the interior of the
   * domain described the root level uniform grid with h0, domainMin and
   * domain Max. The method computes and returns the new min/max bounds and
   * the corresponding ijkmin/ijkmax coordinates w.r.t. the root level.
   */
  void SnapBounds(const double h0[3], const double domainMin[3], const double domainMax[3],
    const int dims[3], bool outside[6]);

  /**
   * This method computes and adjusts the region parameters s.t. the requested
   * region always fall within the AMR region and the number of samples is
   * adjusted if the region of interest moves outsided the domain.
   */
  void ComputeAndAdjustRegionParameters(svtkOverlappingAMR* amrds, double h[3]);

  /**
   * This method gets the region of interest as perscribed by the user.
   */
  void GetRegion(double h[3]);

  /**
   * Checks if two uniform grids intersect.
   */
  bool GridsIntersect(double* g1, double* g2);

  /**
   * Returns a reference grid from the amrdataset.
   */
  svtkUniformGrid* GetReferenceGrid(svtkOverlappingAMR* amrds);

  /**
   * Writes a uniform grid to a file. Used for debugging purposes.
   * void WriteUniformGrid( svtkUniformGrid *g, std::string prefix );
   * void WriteUniformGrid(
   * double origin[3], int dims[3], double h[3],
   * std::string prefix );
   */

  /**
   * Find a descendant of the specified grid that contains the point.
   * If none is found then the original grid information is returned.
   * The search is limited to levels < maxLevel
   */
  void SearchGridDecendants(double q[3], svtkOverlappingAMR* amrds, unsigned int maxLevel,
    unsigned int& level, unsigned int& gridId, int& id);

  /**
   * Find an ancestor of the specified grid that contains the point.
   * If none is found then the original grid information is returned
   */
  bool SearchGridAncestors(
    double q[3], svtkOverlappingAMR* amrds, unsigned int& level, unsigned int& gridId, int& id);

private:
  svtkAMRResampleFilter(const svtkAMRResampleFilter&) = delete;
  void operator=(const svtkAMRResampleFilter&) = delete;
};

#endif /* svtkAMRResampleFilter_h */
