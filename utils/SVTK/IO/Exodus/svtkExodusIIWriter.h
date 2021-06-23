/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExodusIIWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkExodusIIWriter
 * @brief   Write Exodus II files
 *
 *     This is a svtkWriter that writes it's svtkUnstructuredGrid
 *     input out to an Exodus II file.  Go to http://endo.sandia.gov/SEACAS/
 *     for more information about the Exodus II format.
 *
 *     Exodus files contain much information that is not captured
 *     in a svtkUnstructuredGrid, such as time steps, information
 *     lines, node sets, and side sets.  This information can be
 *     stored in a svtkModelMetadata object.
 *
 *     The svtkExodusReader and svtkPExodusReader can create
 *     a svtkModelMetadata object and embed it in a svtkUnstructuredGrid
 *     in a series of field arrays.  This writer searches for these
 *     field arrays and will use the metadata contained in them
 *     when creating the new Exodus II file.
 *
 *     You can also explicitly give the svtkExodusIIWriter a
 *     svtkModelMetadata object to use when writing the file.
 *
 *     In the absence of the information provided by svtkModelMetadata,
 *     if this writer is not part of a parallel application, we will use
 *     reasonable defaults for all the values in the output Exodus file.
 *     If you don't provide a block ID element array, we'll create a
 *     block for each cell type that appears in the unstructured grid.
 *
 *     However if this writer is part of a parallel application (hence
 *     writing out a distributed Exodus file), then we need at the very
 *     least a list of all the block IDs that appear in the file.  And
 *     we need the element array of block IDs for the input unstructured grid.
 *
 *     In the absence of a svtkModelMetadata object, you can also provide
 *     time step information which we will include in the output Exodus
 *     file.
 *
 * @warning
 *     If the input floating point field arrays and point locations are all
 *     floats or all doubles, this class will operate more efficiently.
 *     Mixing floats and doubles will slow you down, because Exodus II
 *     requires that we write only floats or only doubles.
 *
 * @warning
 *     We use the terms "point" and "node" interchangeably.
 *     Also, we use the terms "element" and "cell" interchangeably.
 */

#ifndef svtkExodusIIWriter_h
#define svtkExodusIIWriter_h

#include "svtkIOExodusModule.h" // For export macro
#include "svtkSmartPointer.h"   // For svtkSmartPointer
#include "svtkWriter.h"

#include <map>    // STL Header
#include <string> // STL Header
#include <vector> // STL Header

class svtkModelMetadata;
class svtkDoubleArray;
class svtkIntArray;
class svtkUnstructuredGrid;

class SVTKIOEXODUS_EXPORT svtkExodusIIWriter : public svtkWriter
{
public:
  static svtkExodusIIWriter* New();
  svtkTypeMacro(svtkExodusIIWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Specify the svtkModelMetadata object which contains the Exodus file
   * model information (metadata) absent in the svtkUnstructuredGrid.  If you
   * have this object, you don't need to set any other values before writing.
   * (Just the FileName and the Input.)
   * Note that the svtkExodusReader can create and attach a svtkModelMetadata
   * object to it's output.  If this has happened, the ExodusIIWriter will
   * find it and use it.
   */

  void SetModelMetadata(svtkModelMetadata*);
  svtkGetObjectMacro(ModelMetadata, svtkModelMetadata);

  /**
   * Name for the output file.  If writing in parallel, the number
   * of processes and the process rank will be appended to the name,
   * so each process is writing out a separate file.
   * If not set, this class will make up a file name.
   */

  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);

  /**
   * If StoreDoubles is ON, the floating point fields in the Exodus file
   * will be double precision fields.  The default is determined by the
   * max precision of the input.  If the field data appears to be doubles,
   * then StoreDoubles will be ON, otherwise StoreDoubles will be OFF.
   */

  svtkSetMacro(StoreDoubles, int);
  svtkGetMacro(StoreDoubles, int);

  /**
   * We never write out ghost cells.  This variable is here to satisfy
   * the behavior of ParaView on invoking a parallel writer.
   */

  svtkSetMacro(GhostLevel, int);
  svtkGetMacro(GhostLevel, int);

  /**
   * By default, the integer array containing the global Block Ids of the
   * cells is not included when the new Exodus II file is written out.  If
   * you do want to include this array, set WriteOutBlockIdArray to ON.
   */

  svtkSetMacro(WriteOutBlockIdArray, svtkTypeBool);
  svtkGetMacro(WriteOutBlockIdArray, svtkTypeBool);
  svtkBooleanMacro(WriteOutBlockIdArray, svtkTypeBool);

  /**
   * By default, the integer array containing the global Node Ids
   * is not included when the new Exodus II file is written out.  If
   * you do want to include this array, set WriteOutGlobalNodeIdArray to ON.
   */

  svtkSetMacro(WriteOutGlobalNodeIdArray, svtkTypeBool);
  svtkGetMacro(WriteOutGlobalNodeIdArray, svtkTypeBool);
  svtkBooleanMacro(WriteOutGlobalNodeIdArray, svtkTypeBool);

  /**
   * By default, the integer array containing the global Element Ids
   * is not included when the new Exodus II file is written out.  If you
   * do want to include this array, set WriteOutGlobalElementIdArray to ON.
   */

  svtkSetMacro(WriteOutGlobalElementIdArray, svtkTypeBool);
  svtkGetMacro(WriteOutGlobalElementIdArray, svtkTypeBool);
  svtkBooleanMacro(WriteOutGlobalElementIdArray, svtkTypeBool);

  /**
   * When WriteAllTimeSteps is turned ON, the writer is executed once for
   * each timestep available from the reader.
   */

  svtkSetMacro(WriteAllTimeSteps, svtkTypeBool);
  svtkGetMacro(WriteAllTimeSteps, svtkTypeBool);
  svtkBooleanMacro(WriteAllTimeSteps, svtkTypeBool);

  svtkSetStringMacro(BlockIdArrayName);
  svtkGetStringMacro(BlockIdArrayName);

  /**
   * In certain cases we know that metadata doesn't exist and
   * we want to ignore that warning.
   */

  svtkSetMacro(IgnoreMetaDataWarning, bool);
  svtkGetMacro(IgnoreMetaDataWarning, bool);
  svtkBooleanMacro(IgnoreMetaDataWarning, bool);

protected:
  svtkExodusIIWriter();
  ~svtkExodusIIWriter() override;

  svtkModelMetadata* ModelMetadata;

  char* BlockIdArrayName;

  char* FileName;
  int fid;

  int NumberOfProcesses;
  int MyRank;

  int PassDoubles;

  int StoreDoubles;
  int GhostLevel;
  svtkTypeBool WriteOutBlockIdArray;
  svtkTypeBool WriteOutGlobalNodeIdArray;
  svtkTypeBool WriteOutGlobalElementIdArray;
  svtkTypeBool WriteAllTimeSteps;
  int NumberOfTimeSteps;

  int CurrentTimeIndex;
  int FileTimeOffset;
  bool TopologyChanged;
  bool IgnoreMetaDataWarning;

  svtkDataObject* OriginalInput;
  std::vector<svtkSmartPointer<svtkUnstructuredGrid> > FlattenedInput;
  std::vector<svtkSmartPointer<svtkUnstructuredGrid> > NewFlattenedInput;

  std::vector<svtkStdString> FlattenedNames;
  std::vector<svtkStdString> NewFlattenedNames;

  std::vector<svtkIntArray*> BlockIdList;

  struct Block
  {
    Block()
    {
      this->Name = nullptr;
      this->Type = 0;
      this->NumElements = 0;
      this->ElementStartIndex = -1;
      this->NodesPerElement = 0;
      this->EntityCounts = std::vector<int>();
      this->EntityNodeOffsets = std::vector<int>();
      this->GridIndex = 0;
      this->OutputIndex = -1;
      this->NumAttributes = 0;
      this->BlockAttributes = nullptr;
    };
    const char* Name;
    int Type;
    int NumElements;
    int ElementStartIndex;
    int NodesPerElement;
    std::vector<int> EntityCounts;
    std::vector<int> EntityNodeOffsets;
    size_t GridIndex;
    // std::vector<int> CellIndex;
    int OutputIndex;
    int NumAttributes;
    float* BlockAttributes; // Owned by metamodel or null.  Don't delete.
  };
  std::map<int, Block> BlockInfoMap;
  int NumCells, NumPoints, MaxId;

  std::vector<svtkIdType*> GlobalElementIdList;
  std::vector<svtkIdType*> GlobalNodeIdList;

  int AtLeastOneGlobalElementIdList;
  int AtLeastOneGlobalNodeIdList;

  struct VariableInfo
  {
    int NumComponents;
    int InIndex;
    int ScalarOutOffset;
    std::vector<std::string> OutNames;
  };
  std::map<std::string, VariableInfo> GlobalVariableMap;
  std::map<std::string, VariableInfo> BlockVariableMap;
  std::map<std::string, VariableInfo> NodeVariableMap;
  int NumberOfScalarGlobalArrays;
  int NumberOfScalarElementArrays;
  int NumberOfScalarNodeArrays;

  std::vector<std::vector<int> > CellToElementOffset;

  // By BlockId, and within block ID by element variable, with variables
  // appearing in the same order in which they appear in OutputElementArrayNames

  int* BlockElementVariableTruthTable;
  int AllVariablesDefinedInAllBlocks;

  int BlockVariableTruthValue(int blockIdx, int varIdx);

  char* StrDupWithNew(const char* s);
  void StringUppercase(std::string& str);

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  void WriteData() override;

  int FlattenHierarchy(svtkDataObject* input, const char* name, bool& changed);

  int CreateNewExodusFile();
  void CloseExodusFile();

  int IsDouble();
  void RemoveGhostCells();
  int CheckParametersInternal(int NumberOfProcesses, int MyRank);
  virtual int CheckParameters();
  // If writing in parallel multiple time steps exchange after each time step
  // if we should continue the execution. Pass local continueExecution as a
  // parameter and return the global continueExecution.
  virtual int GlobalContinueExecuting(int localContinueExecution);
  int CheckInputArrays();
  virtual void CheckBlockInfoMap();
  int ConstructBlockInfoMap();
  int ConstructVariableInfoMaps();
  int ParseMetadata();
  int CreateDefaultMetadata();
  char* GetCellTypeName(int t);

  int CreateBlockIdMetadata(svtkModelMetadata* em);
  int CreateBlockVariableMetadata(svtkModelMetadata* em);
  int CreateSetsMetadata(svtkModelMetadata* em);

  void ConvertVariableNames(std::map<std::string, VariableInfo>& variableMap);
  char** FlattenOutVariableNames(
    int nScalarArrays, const std::map<std::string, VariableInfo>& variableMap);
  std::string CreateNameForScalarArray(const char* root, int component, int numComponents);

  std::map<svtkIdType, svtkIdType>* LocalNodeIdMap;
  std::map<svtkIdType, svtkIdType>* LocalElementIdMap;

  svtkIdType GetNodeLocalId(svtkIdType id);
  svtkIdType GetElementLocalId(svtkIdType id);
  int GetElementType(svtkIdType id);

  int WriteInitializationParameters();
  int WriteInformationRecords();
  int WritePoints();
  int WriteCoordinateNames();
  int WriteGlobalPointIds();
  int WriteBlockInformation();
  int WriteGlobalElementIds();
  int WriteVariableArrayNames();
  int WriteNodeSetInformation();
  int WriteSideSetInformation();
  int WriteProperties();
  int WriteNextTimeStep();
  svtkIntArray* GetBlockIdArray(const char* BlockIdArrayName, svtkUnstructuredGrid* input);
  static bool SameTypeOfCells(svtkIntArray* cellToBlockId, svtkUnstructuredGrid* input);

  double ExtractGlobalData(const char* name, int comp, int ts);
  int WriteGlobalData(int timestep, svtkDataArray* buffer);
  void ExtractCellData(const char* name, int comp, svtkDataArray* buffer);
  int WriteCellData(int timestep, svtkDataArray* buffer);
  void ExtractPointData(const char* name, int comp, svtkDataArray* buffer);
  int WritePointData(int timestep, svtkDataArray* buffer);

  /**
   * Get the maximum length name in the input data set. If it is smaller
   * than 32 characters long we just return the ExodusII default of 32.
   */
  virtual unsigned int GetMaxNameLength();

private:
  svtkExodusIIWriter(const svtkExodusIIWriter&) = delete;
  void operator=(const svtkExodusIIWriter&) = delete;
};

#endif
