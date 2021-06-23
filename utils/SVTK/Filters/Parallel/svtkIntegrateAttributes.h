/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIntegrateAttributes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkIntegrateAttributes
 * @brief   Integrates lines, surfaces and volume.
 *
 * Integrates all point and cell data attributes while computing
 * length, area or volume.  Works for 1D, 2D or 3D.  Only one dimensionality
 * at a time.  For volume, this filter ignores all but 3D cells.  It
 * will not compute the volume contained in a closed surface.
 * The output of this filter is a single point and vertex.  The attributes
 * for this point and cell will contain the integration results
 * for the corresponding input attributes.
 */

#ifndef svtkIntegrateAttributes_h
#define svtkIntegrateAttributes_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkDataSet;
class svtkIdList;
class svtkInformation;
class svtkInformationVector;
class svtkDataSetAttributes;
class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkIntegrateAttributes : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkIntegrateAttributes* New();
  svtkTypeMacro(svtkIntegrateAttributes, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the parallel controller to use. By default, set to.
   * `svtkMultiProcessController::GlobalController`.
   */
  void SetController(svtkMultiProcessController* controller);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * If set to true then the filter will divide all output cell data arrays (the integrated values)
   * by the computed volume/area of the dataset.  Defaults to false.
   */
  svtkSetMacro(DivideAllCellDataByVolume, bool);
  svtkGetMacro(DivideAllCellDataByVolume, bool);
  //@}

protected:
  svtkIntegrateAttributes();
  ~svtkIntegrateAttributes() override;

  svtkMultiProcessController* Controller;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // Create a default executive.
  svtkExecutive* CreateDefaultExecutive() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int CompareIntegrationDimension(svtkDataSet* output, int dim);
  int IntegrationDimension;

  // The length, area or volume of the data set.  Computed by Execute;
  double Sum;
  // ToCompute the location of the output point.
  double SumCenter[3];

  bool DivideAllCellDataByVolume;

  void IntegratePolyLine(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds);
  void IntegratePolygon(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds);
  void IntegrateTriangleStrip(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds);
  void IntegrateTriangle(svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId,
    svtkIdType pt1Id, svtkIdType pt2Id, svtkIdType pt3Id);
  void IntegrateTetrahedron(svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId,
    svtkIdType pt1Id, svtkIdType pt2Id, svtkIdType pt3Id, svtkIdType pt4Id);
  void IntegratePixel(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds);
  void IntegrateVoxel(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds);
  void IntegrateGeneral1DCell(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds);
  void IntegrateGeneral2DCell(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds);
  void IntegrateGeneral3DCell(
    svtkDataSet* input, svtkUnstructuredGrid* output, svtkIdType cellId, svtkIdList* cellPtIds);
  void IntegrateSatelliteData(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda);
  void ZeroAttributes(svtkDataSetAttributes* outda);
  int PieceNodeMinToNode0(svtkUnstructuredGrid* data);
  void SendPiece(svtkUnstructuredGrid* src);
  void ReceivePiece(svtkUnstructuredGrid* mergeTo, int fromId);

  // This function assumes the data is in the format of the output of this filter with one
  // point/cell having the value computed as its only tuple.  It divides each value by sum,
  // skipping the last data array if requested (so the volume doesn't get divided by itself
  // and set to 1).
  static void DivideDataArraysByConstant(
    svtkDataSetAttributes* data, bool skipLastArray, double sum);

private:
  svtkIntegrateAttributes(const svtkIntegrateAttributes&) = delete;
  void operator=(const svtkIntegrateAttributes&) = delete;

  class svtkFieldList;
  svtkFieldList* CellFieldList;
  svtkFieldList* PointFieldList;
  int FieldListIndex;

  void AllocateAttributes(svtkFieldList& fieldList, svtkDataSetAttributes* outda);
  void ExecuteBlock(svtkDataSet* input, svtkUnstructuredGrid* output, int fieldset_index,
    svtkFieldList& pdList, svtkFieldList& cdList);

  void IntegrateData1(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda, svtkIdType pt1Id,
    double k, svtkFieldList& fieldlist, int fieldlist_index);
  void IntegrateData2(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda, svtkIdType pt1Id,
    svtkIdType pt2Id, double k, svtkFieldList& fieldlist, int fieldlist_index);
  void IntegrateData3(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda, svtkIdType pt1Id,
    svtkIdType pt2Id, svtkIdType pt3Id, double k, svtkFieldList& fieldlist, int fieldlist_index);
  void IntegrateData4(svtkDataSetAttributes* inda, svtkDataSetAttributes* outda, svtkIdType pt1Id,
    svtkIdType pt2Id, svtkIdType pt3Id, svtkIdType pt4Id, double k, svtkFieldList& fieldlist,
    int fieldlist_index);

public:
  enum CommunicationIds
  {
    IntegrateAttrInfo = 2000,
    IntegrateAttrData
  };
};

#endif
