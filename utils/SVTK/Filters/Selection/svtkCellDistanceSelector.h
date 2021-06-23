/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkCellDistanceSelector

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCellDistanceSelector
 * @brief   select neighbor cells up to a distance
 *
 *
 * This filter grows an input selection by iteratively selecting neighbor
 * cells (a neighbor cell is a cell that shares a vertex/edge/face), up to
 * a given topological distance to the selected neighborhood (number of times
 * we add neighbor cells).
 * This filter takes a svtkSelection and a svtkCompositeDataSet as inputs.
 * It outputs a svtkSelection identifying all the selected cells.
 *
 * @par Thanks:
 * This file has been initially developed in the frame of CEA's Love visualization software
 * development <br> CEA/DIF - Commissariat a l'Energie Atomique, Centre DAM Ile-De-France <br> BP12,
 * F-91297 Arpajon, France. <br> Modified and integrated into SVTK, Kitware SAS 2012 Implementation
 * by Thierry Carrard and Philippe Pebay
 */

#ifndef svtkCellDistanceSelector_h
#define svtkCellDistanceSelector_h

#include "svtkFiltersSelectionModule.h" // For export macro
#include "svtkSelectionAlgorithm.h"
#include "svtkSmartPointer.h" // For smart pointers

class svtkDataSet;
class svtkSelection;
class svtkAlgorithmOutput;
class svtkDataArray;

//@{
/**
 * Grows a selection, selecting neighbor cells, up to a user defined topological distance
 */
class SVTKFILTERSSELECTION_EXPORT svtkCellDistanceSelector : public svtkSelectionAlgorithm
{
public:
  svtkTypeMacro(svtkCellDistanceSelector, svtkSelectionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  static svtkCellDistanceSelector* New();

  /**
   * enumeration values to specify input port types
   */
  enum InputPorts
  {
    INPUT_MESH = 0,     //!< Port 0 is for input mesh
    INPUT_SELECTION = 1 //!< Port 1 is for input selection
  };

  /**
   * A convenience method to set the data object input connection to the producer output
   */
  void SetInputMeshConnection(svtkAlgorithmOutput* in) { this->SetInputConnection(INPUT_MESH, in); }

  /**
   * A convenience method to set the input data object
   */
  void SetInputMesh(svtkDataObject* obj) { this->SetInputData(INPUT_MESH, obj); }

  /**
   * A convenience method to set the selection input connection to the producer output
   */
  void SetInputSelectionConnection(svtkAlgorithmOutput* in)
  {
    this->SetInputConnection(INPUT_SELECTION, in);
  }

  /**
   * A convenience method to set the input selection
   */
  void SetInputSelection(svtkSelection* obj) { this->SetInputData(INPUT_SELECTION, obj); }

  //@{
  /**
   * Tells how far (in term of topological distance) away from seed cells to expand the selection
   */
  svtkSetMacro(Distance, int);
  svtkGetMacro(Distance, int);
  //@}

  //@{
  /**
   * If set, seed cells passed with SetSeedCells will be included in the final selection
   */
  svtkSetMacro(IncludeSeed, svtkTypeBool);
  svtkGetMacro(IncludeSeed, svtkTypeBool);
  svtkBooleanMacro(IncludeSeed, svtkTypeBool);
  //@}

  //@{
  /**
   * If set, intermediate cells (between seed cells and the selection boundary) will be included in
   * the final selection
   */
  svtkSetMacro(AddIntermediate, svtkTypeBool);
  svtkGetMacro(AddIntermediate, svtkTypeBool);
  svtkBooleanMacro(AddIntermediate, svtkTypeBool);
  //@}

protected:
  svtkCellDistanceSelector();
  ~svtkCellDistanceSelector() override;

  void AddSelectionNode(
    svtkSelection* output, svtkSmartPointer<svtkDataArray> outIndices, int partNumber, int d);

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Tological radius from seed cells to be used to select cells
   * Default: 1
   */
  int Distance;

  /**
   * Decide whether seed cells are included in selection
   * Default: 1
   */
  svtkTypeBool IncludeSeed;

  /**
   * Decide whether at distance between 1 and Distance-1 are included in selection
   * Default: 1
   */
  svtkTypeBool AddIntermediate;

private:
  svtkCellDistanceSelector(const svtkCellDistanceSelector&) = delete;
  void operator=(const svtkCellDistanceSelector&) = delete;
};

#endif /* svtkCellDistanceSelector_h */
