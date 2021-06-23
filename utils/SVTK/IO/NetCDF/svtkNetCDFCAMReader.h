/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNetCDFCAMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkNetCDFCAMReader
 * @brief   Read unstructured NetCDF CAM files.
 *
 * Reads in a NetCDF CAM (Community Atmospheric Model) file and
 * produces and unstructured grid.  The grid is actually unstructured
 * in the X and Y directions and rectilinear in the Z direction. If we
 * read one layer we produce quad cells otherwise we produce hex
 * cells.  The reader requires 2 NetCDF files: the main file has all
 * attributes, the connectivity file has point positions and cell
 * connectivity information.
 */

#ifndef svtkNetCDFCAMReader_h
#define svtkNetCDFCAMReader_h

#include "svtkIONetCDFModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkCallbackCommand;
class svtkDataArraySelection;

class SVTKIONETCDF_EXPORT svtkNetCDFCAMReader : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkNetCDFCAMReader* New();
  svtkTypeMacro(svtkNetCDFCAMReader, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Returns 1 if this file can be read and 0 if the file cannot be read.
   * Because NetCDF CAM files come in pairs and we only check one of the
   * files, the result is not definitive.  Invalid files may still return 1
   * although a valid file will never return 0.
   */
  static int CanReadFile(const char* fileName);

  void SetFileName(const char* fileName);
  svtkGetStringMacro(FileName);

  void SetConnectivityFileName(const char* fileName);
  svtkGetStringMacro(ConnectivityFileName);

  //@{
  /**
   * Set whether to read a single layer, midpoint layers or interface layers.
   * VERTICAL_DIMENSION_SINGLE_LAYER (0) indicates that only a single
   * layer will be read in. The NetCDF variables loaded will be the
   * ones with dimensions (time, ncol).
   * VERTICAL_DIMENSION_MIDPOINT_LAYERS (1) indicates that variables defined
   * on midpoint layers will be read in. These are variables with dimensions
   * (time, lev, ncol).
   * VERTICAL_DIMENSION_INTERFACE_LAYERS (2) indicates that variables
   * defined on interface layers will be read in. These are variables with
   * dimensions (time, ilev, ncol).
   */
  enum VerticalDimension
  {
    VERTICAL_DIMENSION_SINGLE_LAYER,
    VERTICAL_DIMENSION_MIDPOINT_LAYERS,
    VERTICAL_DIMENSION_INTERFACE_LAYERS,
    VERTICAL_DIMENSION_COUNT
  };
  svtkSetClampMacro(VerticalDimension, int, 0, 2);
  svtkGetMacro(VerticalDimension, int);
  //@}

  //@{
  /**
   * If SingleXXXLayer is 1, we'll load only the layer specified by
   * XXXLayerIndex.  Otherwise, we load all layers. We do that for
   * midpoint layer variables ( which have dimension 'lev') or for
   * interface layer variables (which have dimension 'ilev').
   */
  svtkBooleanMacro(SingleMidpointLayer, svtkTypeBool);
  svtkSetMacro(SingleMidpointLayer, svtkTypeBool);
  svtkGetMacro(SingleMidpointLayer, svtkTypeBool);
  svtkSetMacro(MidpointLayerIndex, int);
  svtkGetMacro(MidpointLayerIndex, int);
  svtkGetVector2Macro(MidpointLayersRange, int);

  svtkBooleanMacro(SingleInterfaceLayer, svtkTypeBool);
  svtkSetMacro(SingleInterfaceLayer, svtkTypeBool);
  svtkGetMacro(SingleInterfaceLayer, svtkTypeBool);
  svtkSetMacro(InterfaceLayerIndex, int);
  svtkGetMacro(InterfaceLayerIndex, int);
  svtkGetVector2Macro(InterfaceLayersRange, int);
  //@}

  //@{
  /**
   * The following methods allow selective reading of variables.
   * By default, ALL data variables on the nodes are read.
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAllPointArrays();
  void EnableAllPointArrays();
  //@}

protected:
  svtkNetCDFCAMReader();
  ~svtkNetCDFCAMReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Returns true for success.  Based on the piece, number of pieces,
   * number of levels of cells, and the number of cells per level, gives
   * a partitioned space of levels and cells.
   */
  bool GetPartitioning(size_t piece, size_t numPieces, size_t numCellLevels,
    size_t numCellsPerLevel, size_t& beginCellLevel, size_t& endCellLevel, size_t& beginCell,
    size_t& endCell);

  void BuildVarArray();
  static void SelectionCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

private:
  svtkNetCDFCAMReader(const svtkNetCDFCAMReader&) = delete;
  void operator=(const svtkNetCDFCAMReader&) = delete;

  //@{
  /**
   * The file name of the file that contains all of the point
   * data (coordinates and fields).
   */
  char* FileName;
  char* CurrentFileName;
  svtkSetStringMacro(CurrentFileName);
  //@}

  //@{
  /**
   * The file name that contains the cell connectivity information.
   */
  char* ConnectivityFileName;
  char* CurrentConnectivityFileName;
  svtkSetStringMacro(CurrentConnectivityFileName);
  //@}

  int VerticalDimension;
  double* TimeSteps;
  size_t NumberOfTimeSteps;
  svtkDataArraySelection* PointDataArraySelection;
  svtkCallbackCommand* SelectionObserver;

  svtkTypeBool SingleMidpointLayer;
  int MidpointLayerIndex;
  int MidpointLayersRange[2];

  svtkTypeBool SingleInterfaceLayer;
  int InterfaceLayerIndex;
  int InterfaceLayersRange[2];

  class Internal;
  Internal* Internals;
};

#endif
