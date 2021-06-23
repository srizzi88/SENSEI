/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTecplotReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*****************************************************************************
 *
 * Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
 * Produced at the Lawrence Livermore National Laboratory
 * LLNL-CODE-400124
 * All rights reserved.
 *
 * This file was adapted from the ASCII Tecplot reader of VisIt. For details,
 * see https://visit.llnl.gov/.  The full copyright notice is contained in the
 * file COPYRIGHT located at the root of the VisIt distribution or at
 * http://www.llnl.gov/visit/copyright.html.
 *
 *****************************************************************************/

/**
 * @class   svtkTecplotReader
 * @brief   A concrete class to read an ASCII Tecplot file.
 *
 *
 *  svtkTecplotReader parses an ASCII Tecplot file to get a svtkMultiBlockDataSet
 *  object made up of several svtkDataSet objects, of which each is of type
 *  either svtkStructuredGrid or svtkUnstructuredGrid. Each svtkDataSet object
 *  maintains the geometry, topology, and some associated attributes describing
 *  physical properties.
 *
 *  Tecplot treats 3D coordinates (only one or two coordinates might be
 *  explicitly specified in a file) as variables too, whose names (e.g.,
 *  'X' / 'x' / 'I', 'Y' / 'y' / 'J', 'Z' / 'z' / 'K') are provided in the
 *  variables list (the 'VARIABLES' section). These names are then followed
 *  in the list by those of other traditional variables or attributes (node-
 *  based and / or cell-based data with the mode specified via token 'VAR
 *  LOCATION', to be extracted to create svtkPointData and / or svtkCellData).
 *  Each zone described afterwards (in the 'ZONE's section) provides the
 *  specific values of the aforementioned variables (including 3D coordinates),
 *  in the same order as indicated by the variable-names list, through either
 *  POINT-packing (i.e., tuple-based storage) or BLOCK-packing (component-based
 *  storage). In particular, the first / description line of each zone tells
 *  the type of all the constituent cells as the connectivity / topology
 *  information. In other words, the entire dataset is made up of multiple zones
 *  (blocks), of which each maintains a set of cells of the same type ('BRICK',
 *  'TRIANGLE', 'QUADRILATERAL', 'TETRAHEDRON', and 'POINT' in Tecplot terms).
 *  In addition, the description line of each zone specifies the zone name,
 *  dimensionality information (size of each dimension for a structured zone),
 *  number of nodes, and number of cells. Information about the file format is
 *  available at http://download.tecplot.com/360/dataformat.pdf.
 *
 * @warning
 *  svtkTecplotReader is currently a simplified ASCII Tecplot reader and some
 *  functionalities (e.g., extraction of sections 'GEOMETRY', 'TEXT', and 'DATA
 *  SETAUXDATA', access to multiple time steps, in addition to the construction
 *  of svtkRectilinearGrid and svtkImageData objects) are not supported.
 *
 * @par Thanks:
 *  This class is a SVTK implementation of VisIt's ASCII Tecplot reader.
 *
 * @sa
 *  svtkPoints svtkStructuredGrid svtkUnstructuredGrid svtkPointData svtkCellData
 *  svtkDataSet svtkMultiBlockDataSet
 */

#ifndef svtkTecplotReader_h
#define svtkTecplotReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

#include <string> // STL Header; Required for string
#include <vector> // STL Header; Required for vector

class svtkPoints;
class svtkCellData;
class svtkPointData;
class svtkCallbackCommand;
class svtkUnstructuredGrid;
class svtkMultiBlockDataSet;
class svtkDataArraySelection;
class svtkTecplotReaderInternal;

class SVTKIOGEOMETRY_EXPORT svtkTecplotReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkTecplotReader* New();
  svtkTypeMacro(svtkTecplotReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the number of all variables (including 3D coordinates).
   */
  svtkGetMacro(NumberOfVariables, int);
  //@}

  /**
   * Specify a Tecplot ASCII file for data loading.
   */
  void SetFileName(const char* fileName);

  /**
   * Get the Tecplot data title.
   */
  const char* GetDataTitle();

  /**
   * Get the number of blocks (i.e., zones in Tecplot terms).
   */
  int GetNumberOfBlocks();

  /**
   * Get the name of a block specified by a zero-based index. nullptr is returned
   * for an invalid block index.
   */
  const char* GetBlockName(int blockIdx);

  /**
   * Get the number of standard data attributes (node-based and cell-based),
   * excluding 3D coordinates.
   */
  int GetNumberOfDataAttributes();

  /**
   * Get the name of a zero-based data attribute (not 3D coordinates). nullptr is
   * returned for an invalid attribute index.
   */
  const char* GetDataAttributeName(int attrIndx);

  /**
   * Get the type (0 for node-based and 1 for cell-based) of a specified data
   * attribute (not 3D coordinates). -1 is returned for an invalid attribute
   * name.
   */
  int IsDataAttributeCellBased(const char* attrName);

  /**
   * Get the type (0 for node-based and 1 for cell-based) of a specified data
   * attribute (not 3D coordinates). -1 is returned for an invalid attribute
   * index.
   */
  int IsDataAttributeCellBased(int attrIndx);

  /**
   * Get the number of all data attributes (point data and cell data).
   */
  int GetNumberOfDataArrays();

  /**
   * Get the name of a data array specified by the zero-based index (arrayIdx).
   */
  const char* GetDataArrayName(int arrayIdx);

  /**
   * Get the status of a specific data array (0: un-selected; 1: selected).
   */
  int GetDataArrayStatus(const char* arayName);

  /**
   * Set the status of a specific data array (0: de-select; 1: select) specified
   * by the name.
   */
  void SetDataArrayStatus(const char* arayName, int bChecked);

protected:
  svtkTecplotReader();
  ~svtkTecplotReader() override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * A callback function registered with the selection observer.
   */
  static void SelectionModifiedCallback(svtkObject*, unsigned long, void* tpReader, void*);

  /**
   * This function initializes the context. Note that the Tecplot file name
   * must NOT be addressed (either specified or inited) in this function. It
   * is addressed in constructor, destructor, and SetTecplotFile() only.
   */
  void Init();

  /**
   * Get the data arrays list from the tecplot file header.
   */
  void GetDataArraysList();

  /**
   * This function, the data loading engine, parses the Tecplot file to fill
   * a svtkMultiBlockDataSet object.
   */
  void ReadFile(svtkMultiBlockDataSet* multZone);

  /**
   * This function extracts each variable array from a block-packing (component-
   * based) zone and collects the 3D point coordinates in addition to data
   * attributes (node-based and / or cell-based). Note that Tecplot treats 3D
   * coordinates as variables too, though three special ones.
   */
  void GetArraysFromBlockPackingZone(
    int numNodes, int numCells, svtkPoints* theNodes, svtkPointData* nodeData, svtkCellData* cellData);

  /**
   * This function extracts each variable array from a point-packing (tuple-
   * based) zone and collects the 3D point coordbinates in addition to data
   * attributes (node-based and / or cell-based). Note that Tecplot treats 3D
   * coordinates as variables too, though three special ones. A point-packing
   * zone does not contain any cell data at all, instead it is supposed to
   * contain point data only, if any.
   */
  void GetArraysFromPointPackingZone(int numNodes, svtkPoints* theNodes, svtkPointData* nodeData);

  /**
   * This function creates a svtkStructuredGrid object made up of a set of
   * points and the associated attributes (node-based and / or cell-based)
   * extracted from a block-packing (i.e., component-based) zone. This
   * svtkStructuredGrid is then inserted, with a specified zone name, to a
   * svtkMultiBlockDataSet object.
   */
  void GetStructuredGridFromBlockPackingZone(int iDimSize, int jDimSize, int kDimSize, int zoneIndx,
    const char* zoneName, svtkMultiBlockDataSet* multZone);

  /**
   * This function creates a svtkStructuredGrid object made up of a set of
   * points and the associated attributes (node-based and / or cell-based)
   * extracted from a point-packing (i.e., tuple-based) zone. This
   * svtkStructuredGrid is then inserted, with a specified zone name, to a
   * svtkMultiBlockDataSet object.
   */
  void GetStructuredGridFromPointPackingZone(int iDimSize, int jDimSize, int kDimSize, int zoneIndx,
    const char* zoneName, svtkMultiBlockDataSet* multZone);

  /**
   * This function creates a svtkUnstructuredGrid object made up of a set of
   * points and the associated attributes (node-based and / or cell-based)
   * extracted from a block-packing (i.e., component-based) zone. This
   * svtkUnstructuredGrid is then inserted, with a specified zone name, to a
   * svtkMultiBlockDataSet object.
   */
  void GetUnstructuredGridFromBlockPackingZone(int numNodes, int numCells, const char* cellType,
    int zoneIndx, const char* zoneName, svtkMultiBlockDataSet* multZone);

  /**
   * This function creates a polyhedral svtkUnstructuredGrid object made up of a set of
   * points and the associated attributes (node-based and / or cell-based)
   * extracted from a block-packing (i.e., component-based) zone. This
   * svtkUnstructuredGrid is then inserted, with a specified zone name, to a
   * svtkMultiBlockDataSet object.
   */
  void GetPolyhedralGridFromBlockPackingZone(int numNodes, int numElements, int numFaces,
    int zoneIndex, const char* zoneName, svtkMultiBlockDataSet* multZone);

  /**
   * This function creates a polygonal svtkUnstructuredGrid object made up of a set of
   * points and the associated attributes (node-based and / or cell-based)
   * extracted from a block-packing (i.e., component-based) zone. This
   * svtkUnstructuredGrid is then inserted, with a specified zone name, to a
   * svtkMultiBlockDataSet object.
   */
  void GetPolygonalGridFromBlockPackingZone(int numNodes, int numElements, int numFaces,
    int zoneIndex, const char* zoneName, svtkMultiBlockDataSet* multZone);

  /**
   * This function fills an allocated svtkUnstructuredGrid object with numberCells
   * polyhedral cells to define the grid topology.
   */
  void GetPolyhedralGridCells(int numberCells, int numFaces, svtkUnstructuredGrid* unstruct) const;

  /**
   * This function fills an allocated svtkUnstructuredGrid object with numberCells
   * polygonal cells to define the grid topology.
   */
  void GetPolygonalGridCells(int numFaces, int numEdges, svtkUnstructuredGrid* unstruct) const;

  /**
   * This function creates a svtkUnstructuredGrid object made up of a set of
   * points and the associated attributes (node-based and / or cell-based)
   * extracted from a point-packing (i.e., tuple-based) zone. This
   * svtkUnstructuredGrid is then inserted, with a specified zone name, to a
   * svtkMultiBlockDataSet object.
   */
  void GetUnstructuredGridFromPointPackingZone(int numNodes, int numCells, const char* cellType,
    int zoneIndx, const char* zoneName, svtkMultiBlockDataSet* multZone);

  /**
   * This function fills an allocated svtkUnstructuredGrid object with numberCells
   * cells of type cellTypeStr to define the grid topology.
   */
  void GetUnstructuredGridCells(
    int numberCells, const char* cellTypeStr, svtkUnstructuredGrid* unstrctGrid);

  int NumberOfVariables;
  char* FileName;
  svtkCallbackCommand* SelectionObserver;
  svtkDataArraySelection* DataArraySelection;
  svtkTecplotReaderInternal* Internal;

  std::string DataTitle;
  std::vector<int> CellBased;
  std::vector<std::string> ZoneNames;
  std::vector<std::string> Variables;

private:
  svtkTecplotReader(const svtkTecplotReader&) = delete;
  void operator=(const svtkTecplotReader&) = delete;
};

#endif
