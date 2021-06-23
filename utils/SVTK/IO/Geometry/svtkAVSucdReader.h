/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAVSucdReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAVSucdReader
 * @brief   reads a dataset in AVS "UCD" format
 *
 * svtkAVSucdReader creates an unstructured grid dataset. It reads binary or
 * ASCII files stored in UCD format, with optional data stored at the nodes
 * or at the cells of the model. A cell-based fielddata stores the material
 * id. The class can automatically detect the endian-ness of the binary files.
 *
 * @par Thanks:
 * Thanks to Guenole Harel and Emmanuel Colin (Supelec engineering school,
 * France) and Jean M. Favre (CSCS, Switzerland) who co-developed this class.
 * Thanks to Isabelle Surin (isabelle.surin at cea.fr, CEA-DAM, France) who
 * supervised the internship of the first two authors. Thanks to Daniel
 * Aguilera (daniel.aguilera at cea.fr, CEA-DAM, France) who contributed code
 * and advice. Please address all comments to Jean Favre (jfavre at cscs.ch)
 *
 * @sa
 * svtkGAMBITReader
 */

#ifndef svtkAVSucdReader_h
#define svtkAVSucdReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkIntArray;
class svtkFloatArray;
class svtkIdTypeArray;
class svtkDataArraySelection;

class SVTKIOGEOMETRY_EXPORT svtkAVSucdReader : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkAVSucdReader* New();
  svtkTypeMacro(svtkAVSucdReader, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of AVS UCD datafile to read
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Is the file to be read written in binary format (as opposed to ascii).
   */
  svtkSetMacro(BinaryFile, svtkTypeBool);
  svtkGetMacro(BinaryFile, svtkTypeBool);
  svtkBooleanMacro(BinaryFile, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the total number of cells.
   */
  svtkGetMacro(NumberOfCells, int);
  //@}

  //@{
  /**
   * Get the total number of nodes.
   */
  svtkGetMacro(NumberOfNodes, int);
  //@}

  //@{
  /**
   * Get the number of data fields at the nodes.
   */
  svtkGetMacro(NumberOfNodeFields, int);
  //@}

  //@{
  /**
   * Get the number of data fields at the cell centers.
   */
  svtkGetMacro(NumberOfCellFields, int);
  //@}

  //@{
  /**
   * Get the number of data fields for the model. Unused because SVTK
   * has no methods for it.
   */
  svtkGetMacro(NumberOfFields, int);
  //@}

  //@{
  /**
   * Get the number of data components at the nodes and cells.
   */
  svtkGetMacro(NumberOfNodeComponents, int);
  svtkGetMacro(NumberOfCellComponents, int);
  //@}

  //@{
  /**
   * Set/Get the endian-ness of the binary file.
   */
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  const char* GetByteOrderAsString();
  //@}

  svtkSetMacro(ByteOrder, int);
  svtkGetMacro(ByteOrder, int);

  //@{
  /**
   * The following methods allow selective reading of solutions fields.  by
   * default, ALL data fields are the nodes and cells are read, but this can
   * be modified.
   */
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);
  //@}

  void DisableAllCellArrays();
  void EnableAllCellArrays();
  void DisableAllPointArrays();
  void EnableAllPointArrays();

  // get min and max value for the index-th value of a cell component
  // index varies from 0 to (veclen - 1)
  void GetCellDataRange(int cellComp, int index, float* min, float* max);

  // get min and max value for the index-th value of a node component
  // index varies from 0 to (veclen - 1)
  void GetNodeDataRange(int nodeComp, int index, float* min, float* max);

protected:
  svtkAVSucdReader();
  ~svtkAVSucdReader() override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* FileName;
  svtkTypeBool BinaryFile;

  int NumberOfNodes;
  int NumberOfCells;
  int NumberOfNodeFields;
  int NumberOfNodeComponents;
  int NumberOfCellComponents;
  int NumberOfCellFields;
  int NumberOfFields;
  int NlistNodes;

  istream* FileStream;

  svtkDataArraySelection* PointDataArraySelection;
  svtkDataArraySelection* CellDataArraySelection;

  int ByteOrder;
  int GetLabel(char* string, int number, char* label);

  enum
  {
    FILE_BIG_ENDIAN = 0,
    FILE_LITTLE_ENDIAN = 1
  };
  enum UCDCell_type
  {
    PT = 0,
    LINE = 1,
    TRI = 2,
    QUAD = 3,
    TET = 4,
    PYR = 5,
    PRISM = 6,
    HEX = 7
  };

  struct DataInfo
  {
    long foffset; // offset in binary file
    int veclen;   // number of components in the node or cell variable
    float min[3]; // pre-calculated data minima (max size 3 for vectors)
    float max[3]; // pre-calculated data maxima (max size 3 for vectors)
  };

  DataInfo* NodeDataInfo;
  DataInfo* CellDataInfo;

private:
  struct idMapping;

  void ReadFile(svtkUnstructuredGrid* output);
  void ReadGeometry(svtkUnstructuredGrid* output, idMapping& nodeMap, idMapping& cellMap);
  void ReadNodeData(svtkUnstructuredGrid* output, const idMapping& nodeMap);
  void ReadCellData(svtkUnstructuredGrid* output, const idMapping& cellMap);

  int ReadFloatBlock(int n, float* block);
  int ReadIntBlock(int n, int* block);
  void ReadXYZCoords(svtkFloatArray* coords, idMapping& nodeMap);
  void ReadBinaryCellTopology(svtkIntArray* material, int* types, svtkIdTypeArray* listcells);
  void ReadASCIICellTopology(svtkIntArray* material, svtkUnstructuredGrid* output,
    const idMapping& nodeMap, idMapping& cellMap);

  svtkAVSucdReader(const svtkAVSucdReader&) = delete;
  void operator=(const svtkAVSucdReader&) = delete;
};

#endif
