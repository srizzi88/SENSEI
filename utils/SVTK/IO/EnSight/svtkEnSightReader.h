/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnSightReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEnSightReader
 * @brief   superclass for EnSight file readers
 */

#ifndef svtkEnSightReader_h
#define svtkEnSightReader_h

#include "svtkGenericEnSightReader.h"
#include "svtkIOEnSightModule.h" // For export macro

class svtkDataSet;
class svtkDataSetCollection;
class svtkEnSightReaderCellIdsType;
class svtkIdList;
class svtkMultiBlockDataSet;

class SVTKIOENSIGHT_EXPORT svtkEnSightReader : public svtkGenericEnSightReader
{
public:
  svtkTypeMacro(svtkEnSightReader, svtkGenericEnSightReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum ElementTypesList
  {
    POINT = 0,
    BAR2 = 1,
    BAR3 = 2,
    NSIDED = 3,
    TRIA3 = 4,
    TRIA6 = 5,
    QUAD4 = 6,
    QUAD8 = 7,
    NFACED = 8,
    TETRA4 = 9,
    TETRA10 = 10,
    PYRAMID5 = 11,
    PYRAMID13 = 12,
    HEXA8 = 13,
    HEXA20 = 14,
    PENTA6 = 15,
    PENTA15 = 16,
    NUMBER_OF_ELEMENT_TYPES = 17
  };

  enum VariableTypesList
  {
    SCALAR_PER_NODE = 0,
    VECTOR_PER_NODE = 1,
    TENSOR_SYMM_PER_NODE = 2,
    SCALAR_PER_ELEMENT = 3,
    VECTOR_PER_ELEMENT = 4,
    TENSOR_SYMM_PER_ELEMENT = 5,
    SCALAR_PER_MEASURED_NODE = 6,
    VECTOR_PER_MEASURED_NODE = 7,
    COMPLEX_SCALAR_PER_NODE = 8,
    COMPLEX_VECTOR_PER_NODE = 9,
    COMPLEX_SCALAR_PER_ELEMENT = 10,
    COMPLEX_VECTOR_PER_ELEMENT = 11
  };

  enum SectionTypeList
  {
    COORDINATES = 0,
    BLOCK = 1,
    ELEMENT = 2
  };

  //@{
  /**
   * Get the Measured file name. Made public to allow access from
   * apps requiring detailed info about the Data contents
   */
  svtkGetStringMacro(MeasuredFileName);
  //@}

  //@{
  /**
   * Get the Match file name. Made public to allow access from
   * apps requiring detailed info about the Data contents
   */
  svtkGetStringMacro(MatchFileName);
  //@}

protected:
  svtkEnSightReader();
  ~svtkEnSightReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ClearForNewCaseFileName() override;

  //@{
  /**
   * Set the Measured file name.
   */
  svtkSetStringMacro(MeasuredFileName);
  //@}

  //@{
  /**
   * Set the Match file name.
   */
  svtkSetStringMacro(MatchFileName);
  //@}

  //@{
  /**
   * Read the case file.  If an error occurred, 0 is returned; otherwise 1.
   */
  int ReadCaseFile();
  int ReadCaseFileGeometry(char* line);
  int ReadCaseFileVariable(char* line);
  int ReadCaseFileTime(char* line);
  int ReadCaseFileFile(char* line);
  //@}

  // set in UpdateInformation to value returned from ReadCaseFile
  int CaseFileRead;

  /**
   * Read the geometry file.  If an error occurred, 0 is returned; otherwise 1.
   */
  virtual int ReadGeometryFile(
    const char* fileName, int timeStep, svtkMultiBlockDataSet* output) = 0;

  /**
   * Read the measured geometry file.  If an error occurred, 0 is returned;
   * otherwise 1.
   */
  virtual int ReadMeasuredGeometryFile(
    const char* fileName, int timeStep, svtkMultiBlockDataSet* output) = 0;

  /**
   * Read the variable files. If an error occurred, 0 is returned; otherwise 1.
   */
  int ReadVariableFiles(svtkMultiBlockDataSet* output);

  /**
   * Read scalars per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadScalarsPerNode(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output, int measured = 0, int numberOfComponents = 1,
    int component = 0) = 0;

  /**
   * Read vectors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadVectorsPerNode(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output, int measured = 0) = 0;

  /**
   * Read tensors per node for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadTensorsPerNode(
    const char* fileName, const char* description, int timeStep, svtkMultiBlockDataSet* output) = 0;

  /**
   * Read scalars per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadScalarsPerElement(const char* fileName, const char* description, int timeStep,
    svtkMultiBlockDataSet* output, int numberOfComponents = 1, int component = 0) = 0;

  /**
   * Read vectors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadVectorsPerElement(
    const char* fileName, const char* description, int timeStep, svtkMultiBlockDataSet* output) = 0;

  /**
   * Read tensors per element for this dataset.  If an error occurred, 0 is
   * returned; otherwise 1.
   */
  virtual int ReadTensorsPerElement(
    const char* fileName, const char* description, int timeStep, svtkMultiBlockDataSet* output) = 0;

  /**
   * Read an unstructured part (partId) from the geometry file and create a
   * svtkUnstructuredGrid output.  Return 0 if EOF reached.
   */
  virtual int CreateUnstructuredGridOutput(
    int partId, char line[80], const char* name, svtkMultiBlockDataSet* output) = 0;

  /**
   * Read a structured part from the geometry file and create a
   * svtkStructuredGridOutput.  Return 0 if EOF reached.
   */
  virtual int CreateStructuredGridOutput(
    int partId, char line[80], const char* name, svtkMultiBlockDataSet* output) = 0;

  /**
   * Add another file name to the list for a particular variable type.
   */
  void AddVariableFileName(const char* fileName1, const char* fileName2 = nullptr);

  /**
   * Add another description to the list for a particular variable type.
   */
  void AddVariableDescription(const char* description);

  /**
   * Record the variable type for the variable line just read.
   */
  void AddVariableType();

  /**
   * Determine the element type from a line read a file.  Return -1 for
   * invalid element type.
   */
  int GetElementType(const char* line);

  /**
   * Determine the section type from a line read a file.  Return -1 for
   * invalid section type.
   */
  int GetSectionType(const char* line);

  /**
   * Replace the *'s in the filename with the given filename number.
   */
  void ReplaceWildcards(char* filename, int num);

  /**
   * Remove leading blank spaces from a string.
   */
  void RemoveLeadingBlanks(char* line);

  // Get the svtkIdList for the given output index and cell type.
  svtkIdList* GetCellIds(int index, int cellType);

  /**
   * Convenience method use to convert the readers from SVTK 5 multiblock API
   * to the current composite data infrastructure.
   */
  void AddToBlock(svtkMultiBlockDataSet* output, unsigned int blockNo, svtkDataSet* dataset);

  /**
   * Convenience method use to convert the readers from SVTK 5 multiblock API
   * to the current composite data infrastructure.
   */
  svtkDataSet* GetDataSetFromBlock(svtkMultiBlockDataSet* output, unsigned int blockNo);

  /**
   * Set the name of a block.
   */
  void SetBlockName(svtkMultiBlockDataSet* output, unsigned int blockNo, const char* name);

  char* MeasuredFileName;
  char* MatchFileName; // may not actually be necessary to read this file

  // pointer to lists of svtkIdLists (cell ids per element type per part)
  svtkEnSightReaderCellIdsType* CellIds;

  // part ids of unstructured outputs
  svtkIdList* UnstructuredPartIds;

  int VariableMode;

  // pointers to lists of filenames
  char** VariableFileNames; // non-complex
  char** ComplexVariableFileNames;

  // array of time sets
  svtkIdList* VariableTimeSetIds;
  svtkIdList* ComplexVariableTimeSetIds;

  // array of file sets
  svtkIdList* VariableFileSetIds;
  svtkIdList* ComplexVariableFileSetIds;

  // collection of filename numbers per time set
  svtkIdListCollection* TimeSetFileNameNumbers;
  svtkIdList* TimeSetsWithFilenameNumbers;

  // collection of filename numbers per file set
  svtkIdListCollection* FileSetFileNameNumbers;
  svtkIdList* FileSetsWithFilenameNumbers;

  // collection of number of steps per file per file set
  svtkIdListCollection* FileSetNumberOfSteps;

  // ids of the time and file sets
  svtkIdList* TimeSetIds;
  svtkIdList* FileSets;

  int GeometryTimeSet;
  int GeometryFileSet;
  int MeasuredTimeSet;
  int MeasuredFileSet;

  float GeometryTimeValue;
  float MeasuredTimeValue;

  svtkTypeBool UseTimeSets;
  svtkSetMacro(UseTimeSets, svtkTypeBool);
  svtkGetMacro(UseTimeSets, svtkTypeBool);
  svtkBooleanMacro(UseTimeSets, svtkTypeBool);

  svtkTypeBool UseFileSets;
  svtkSetMacro(UseFileSets, svtkTypeBool);
  svtkGetMacro(UseFileSets, svtkTypeBool);
  svtkBooleanMacro(UseFileSets, svtkTypeBool);

  int NumberOfGeometryParts;

  // global list of points for measured geometry
  int NumberOfMeasuredPoints;

  int NumberOfNewOutputs;
  int InitialRead;

  int CheckOutputConsistency();

  double ActualTimeValue;

private:
  svtkEnSightReader(const svtkEnSightReader&) = delete;
  void operator=(const svtkEnSightReader&) = delete;
};

#endif
