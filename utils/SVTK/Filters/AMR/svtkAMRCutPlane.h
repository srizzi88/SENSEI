/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRCutPlane.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMRCutPlane
 *
 *
 *  A concrete instance of svtkMultiBlockDataSet that provides functionality for
 * cutting an AMR dataset (an instance of svtkOverlappingAMR) with user supplied
 * implicit plane function defined by a normal and center.
 */

#ifndef svtkAMRCutPlane_h
#define svtkAMRCutPlane_h

#include "svtkFiltersAMRModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

#include <map>    // For STL map
#include <vector> // For STL vector

class svtkMultiBlockDataSet;
class svtkOverlappingAMR;
class svtkMultiProcessController;
class svtkInformation;
class svtkInformationVector;
class svtkIndent;
class svtkPlane;
class svtkUniformGrid;
class svtkCell;
class svtkPoints;
class svtkCellArray;
class svtkPointData;
class svtkCellData;

class SVTKFILTERSAMR_EXPORT svtkAMRCutPlane : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkAMRCutPlane* New();
  svtkTypeMacro(svtkAMRCutPlane, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& oss, svtkIndent indent) override;

  //@{
  /**
   * Sets the center
   */
  svtkSetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Sets the normal
   */
  svtkSetVector3Macro(Normal, double);
  //@}

  //@{
  /**
   * Sets the level of resolution
   */
  svtkSetMacro(LevelOfResolution, int);
  svtkGetMacro(LevelOfResolution, int);
  //@}

  //@{
  /**

   */
  svtkSetMacro(UseNativeCutter, bool);
  svtkGetMacro(UseNativeCutter, bool);
  svtkBooleanMacro(UseNativeCutter, bool);
  //@}

  //@{
  /**
   * Set/Get a multiprocess controller for parallel processing.
   * By default this parameter is set to nullptr by the constructor.
   */
  svtkSetMacro(Controller, svtkMultiProcessController*);
  svtkGetMacro(Controller, svtkMultiProcessController*);
  //@}

  // Standard pipeline routines

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Gets the metadata from upstream module and determines which blocks
   * should be loaded by this instance.
   */
  int RequestInformation(svtkInformation* rqst, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Performs upstream requests to the reader
   */
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkAMRCutPlane();
  ~svtkAMRCutPlane() override;

  /**
   * Returns the cut-plane defined by a svtkCutPlane instance based on the
   * user-supplied center and normal.
   */
  svtkPlane* GetCutPlane(svtkOverlappingAMR* metadata);

  /**
   * Extracts cell
   */
  void ExtractCellFromGrid(svtkUniformGrid* grid, svtkCell* cell,
    std::map<svtkIdType, svtkIdType>& gridPntMapping, svtkPoints* nodes, svtkCellArray* cells);

  /**
   * Given the grid and a subset ID pair, grid IDs mapping to the extracted
   * grid IDs, extract the point data.
   */
  void ExtractPointDataFromGrid(svtkUniformGrid* grid,
    std::map<svtkIdType, svtkIdType>& gridPntMapping, svtkIdType NumNodes, svtkPointData* PD);

  /**
   * Given the grid and the list of cells that are extracted, extract the
   * corresponding cell data.
   */
  void ExtractCellDataFromGrid(
    svtkUniformGrid* grid, std::vector<svtkIdType>& cellIdxList, svtkCellData* CD);

  /**
   * Given a cut-plane, p, and the metadata, m, this method computes which
   * blocks need to be loaded. The corresponding block IDs are stored in
   * the internal STL vector, blocksToLoad, which is then propagated upstream
   * in the RequestUpdateExtent.
   */
  void ComputeAMRBlocksToLoad(svtkPlane* p, svtkOverlappingAMR* m);

  // Descriription:
  // Initializes the cut-plane center given the min/max bounds.
  void InitializeCenter(double min[3], double max[3]);

  //@{
  /**
   * Determines if a plane intersects with an AMR box
   */
  bool PlaneIntersectsAMRBox(svtkPlane* pl, double bounds[6]);
  bool PlaneIntersectsAMRBox(double plane[4], double bounds[6]);
  //@}

  /**
   * Determines if a plane intersects with a grid cell
   */
  bool PlaneIntersectsCell(svtkPlane* pl, svtkCell* cell);

  /**
   * A utility function that checks if the input AMR data is 2-D.
   */
  bool IsAMRData2D(svtkOverlappingAMR* input);

  /**
   * Applies cutting to an AMR block
   */
  void CutAMRBlock(
    svtkPlane* cutPlane, unsigned int blockIdx, svtkUniformGrid* grid, svtkMultiBlockDataSet* dataSet);

  int LevelOfResolution;
  double Center[3];
  double Normal[3];
  bool initialRequest;
  bool UseNativeCutter;
  svtkMultiProcessController* Controller;

  std::vector<int> BlocksToLoad;

private:
  svtkAMRCutPlane(const svtkAMRCutPlane&) = delete;
  void operator=(const svtkAMRCutPlane&) = delete;
};

#endif /* svtkAMRCutPlane_h */
