/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXMLReader
 * @brief   Superclass for SVTK's XML format readers.
 *
 * svtkXMLReader uses svtkXMLDataParser to parse a
 * <a href="http://www.svtk.org/Wiki/SVTK_XML_Formats">SVTK XML</a> input file.
 * Concrete subclasses then traverse the parsed file structure and extract data.
 */

#ifndef svtkXMLReader_h
#define svtkXMLReader_h

#include "svtkAlgorithm.h"
#include "svtkIOXMLModule.h" // For export macro

#include <string> // for std::string

class svtkAbstractArray;
class svtkCallbackCommand;
class svtkDataArraySelection;
class svtkDataSet;
class svtkDataSetAttributes;
class svtkXMLDataElement;
class svtkXMLDataParser;
class svtkInformationVector;
class svtkInformation;
class svtkCommand;

class SVTKIOXML_EXPORT svtkXMLReader : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkXMLReader, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum FieldType
  {
    POINT_DATA,
    CELL_DATA,
    OTHER
  };

  //@{
  /**
   * Get/Set the name of the input file.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable reading from an InputString instead of the default, a file.
   */
  svtkSetMacro(ReadFromInputString, svtkTypeBool);
  svtkGetMacro(ReadFromInputString, svtkTypeBool);
  svtkBooleanMacro(ReadFromInputString, svtkTypeBool);
  void SetInputString(const std::string& s) { this->InputString = s; }
  //@}

  /**
   * Test whether the file (type) with the given name can be read by this
   * reader. If the file has a newer version than the reader, we still say
   * we can read the file type and we fail later, when we try to read the file.
   * This enables clients (ParaView) to distinguish between failures when we
   * need to look for another reader and failures when we don't.
   */
  virtual int CanReadFile(const char* name);

  //@{
  /**
   * Get the output as a svtkDataSet pointer.
   */
  svtkDataSet* GetOutputAsDataSet();
  svtkDataSet* GetOutputAsDataSet(int index);
  //@}

  //@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  svtkGetObjectMacro(PointDataArraySelection, svtkDataArraySelection);
  svtkGetObjectMacro(CellDataArraySelection, svtkDataArraySelection);
  svtkGetObjectMacro(ColumnArraySelection, svtkDataArraySelection);
  //@}

  //@{
  /**
   * Get the number of point, cell or column arrays available in the input.
   */
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  int GetNumberOfColumnArrays();
  //@}

  //@{
  /**
   * Get the name of the point, cell or column array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  const char* GetColumnArrayName(int index);
  //@}

  //@{
  /**
   * Get/Set whether the point, cell or column array with the given name is to
   * be read.
   */
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);
  int GetColumnArrayStatus(const char* name);
  void SetColumnArrayStatus(const char* name, int status);
  //@}

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  virtual void CopyOutputInformation(svtkInformation* svtkNotUsed(outInfo), int svtkNotUsed(port)) {}

  //@{
  /**
   * Which TimeStep to read.
   */
  svtkSetMacro(TimeStep, int);
  svtkGetMacro(TimeStep, int);
  //@}

  svtkGetMacro(NumberOfTimeSteps, int);
  //@{
  /**
   * Which TimeStepRange to read
   */
  svtkGetVector2Macro(TimeStepRange, int);
  svtkSetVector2Macro(TimeStepRange, int);
  //@}

  /**
   * Returns the internal XML parser. This can be used to access
   * the XML DOM after RequestInformation() was called.
   */
  svtkXMLDataParser* GetXMLParser() { return this->XMLParser; }

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //@{
  /**
   * Set/get the ErrorObserver for the internal reader
   * This is useful for applications that want to catch error messages.
   */
  void SetReaderErrorObserver(svtkCommand*);
  svtkGetObjectMacro(ReaderErrorObserver, svtkCommand);
  //@}

  //@{
  /**
   * Set/get the ErrorObserver for the internal xml parser
   * This is useful for applications that want to catch error messages.
   */
  void SetParserErrorObserver(svtkCommand*);
  svtkGetObjectMacro(ParserErrorObserver, svtkCommand);
  //@}

protected:
  svtkXMLReader();
  ~svtkXMLReader() override;

  // Pipeline execution methods to be defined by subclass.  Called by
  // corresponding RequestData methods after appropriate setup has been
  // done.
  virtual int ReadXMLInformation();
  virtual void ReadXMLData();

  // Get the name of the data set being read.
  virtual const char* GetDataSetName() = 0;

  // Test if the reader can read a file with the given version number.
  virtual int CanReadFileVersion(int major, int minor);

  // Setup the output with no data available.  Used in error cases.
  virtual void SetupEmptyOutput() = 0;

  // Setup the output's information.
  virtual void SetupOutputInformation(svtkInformation* svtkNotUsed(outInfo)) {}

  // Setup the output's data with allocation.
  virtual void SetupOutputData();

  // Read the primary element from the file.  This is the element
  // whose name is the value returned by GetDataSetName().
  virtual int ReadPrimaryElement(svtkXMLDataElement* ePrimary);

  // Read the top-level element from the file.  This is always the
  // SVTKFile element.
  virtual int ReadSVTKFile(svtkXMLDataElement* eSVTKFile);

  /**
   * If the IdType argument is present in the provided XMLDataElement
   * and the provided dataType has the same size with SVTK_ID_TYPE on this build of SVTK,
   * returns SVTK_ID_TYPE. Returns dataType in any other cases.
   */
  int GetLocalDataType(svtkXMLDataElement* da, int datatype);

  // Create a svtkAbstractArray from its cooresponding XML representation.
  // Does not allocate.
  svtkAbstractArray* CreateArray(svtkXMLDataElement* da);

  // Create a svtkInformationKey from its corresponding XML representation.
  // Stores it in the instance of svtkInformationProvided. Does not allocate.
  int CreateInformationKey(svtkXMLDataElement* eInfoKey, svtkInformation* info);

  // Populates the info object with the InformationKey children in infoRoot.
  // Returns false if errors occur.
  bool ReadInformation(svtkXMLDataElement* infoRoot, svtkInformation* info);

  // Internal utility methods.
  virtual int OpenStream();
  virtual void CloseStream();
  virtual int OpenSVTKFile();
  virtual void CloseSVTKFile();
  virtual int OpenSVTKString();
  virtual void CloseSVTKString();
  virtual void CreateXMLParser();
  virtual void DestroyXMLParser();
  void SetupCompressor(const char* type);
  int CanReadFileVersionString(const char* version);

  /**
   * This method is used by CanReadFile() to check if the reader can read an XML
   * with the primary element with the given name. Default implementation
   * compares the name with the text returned by this->GetDataSetName().
   */
  virtual int CanReadFileWithDataType(const char* dsname);

  // Returns the major version for the file being read. -1 when invalid.
  svtkGetMacro(FileMajorVersion, int);

  // Returns the minor version for the file being read. -1 when invalid.
  svtkGetMacro(FileMinorVersion, int);

  // Utility methods for subclasses.
  int IntersectExtents(int* extent1, int* extent2, int* result);
  int Min(int a, int b);
  int Max(int a, int b);
  void ComputePointDimensions(int* extent, int* dimensions);
  void ComputePointIncrements(int* extent, svtkIdType* increments);
  void ComputeCellDimensions(int* extent, int* dimensions);
  void ComputeCellIncrements(int* extent, svtkIdType* increments);
  svtkIdType GetStartTuple(int* extent, svtkIdType* increments, int i, int j, int k);
  void ReadAttributeIndices(svtkXMLDataElement* eDSA, svtkDataSetAttributes* dsa);
  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);

  // Read an Array values starting at the given index and up to numValues.
  // This method assumes that the array is of correct size to
  // accommodate all numValues values. arrayIndex is the value index at which the read
  // values will be put in the array.
  virtual int ReadArrayValues(svtkXMLDataElement* da, svtkIdType arrayIndex, svtkAbstractArray* array,
    svtkIdType startIndex, svtkIdType numValues, FieldType type = OTHER);

  // Setup the data array selections for the input's set of arrays.
  void SetDataArraySelections(svtkXMLDataElement* eDSA, svtkDataArraySelection* sel);

  int SetFieldDataInfo(svtkXMLDataElement* eDSA, int association, svtkIdType numTuples,
    svtkInformationVector*(&infoVector));

  // Check whether the given array element is an enabled array.
  int PointDataArrayIsEnabled(svtkXMLDataElement* ePDA);
  int CellDataArrayIsEnabled(svtkXMLDataElement* eCDA);

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  // Give concrete classes an option to squeeze any output arrays
  // at the end of RequestData.
  virtual void SqueezeOutputArrays(svtkDataObject*) {}

  // The svtkXMLDataParser instance used to hide XML reading details.
  svtkXMLDataParser* XMLParser;

  // The FieldData element representation.
  svtkXMLDataElement* FieldDataElement;

  // The input file's name.
  char* FileName;

  // The stream used to read the input.
  istream* Stream;

  // Whether this object is reading from a string or a file.
  // Default is 0: read from file.
  svtkTypeBool ReadFromInputString;

  // The input string.
  std::string InputString;

  // The array selections.
  svtkDataArraySelection* PointDataArraySelection;
  svtkDataArraySelection* CellDataArraySelection;
  svtkDataArraySelection* ColumnArraySelection;

  // The observer to modify this object when the array selections are
  // modified.
  svtkCallbackCommand* SelectionObserver;

  // Whether there was an error reading the file in RequestInformation.
  int InformationError;

  // Whether there was an error reading the file in RequestData.
  int DataError;

  // incrementally fine-tuned progress updates.
  virtual void GetProgressRange(float* range);
  virtual void SetProgressRange(const float range[2], int curStep, int numSteps);
  virtual void SetProgressRange(const float range[2], int curStep, const float* fractions);
  virtual void UpdateProgressDiscrete(float progress);
  float ProgressRange[2];

  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);
  virtual int RequestDataObject(svtkInformation* svtkNotUsed(request),
    svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
  {
    return 1;
  }
  virtual int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);
  svtkTimeStamp ReadMTime;

  // Whether there was an error reading the XML.
  int ReadError;

  // For structured data keep track of dimensions empty of cells.  For
  // unstructured data these are always zero.  This is used to support
  // 1-D and 2-D cell data.
  int AxesEmpty[3];

  // The timestep currently being read.
  int TimeStep;
  int CurrentTimeStep;
  int NumberOfTimeSteps;
  void SetNumberOfTimeSteps(int num);
  // buffer for reading timestep from the XML file the length is of
  // NumberOfTimeSteps and therefore is always long enough
  int* TimeSteps;
  // Store the range of time steps
  int TimeStepRange[2];

  // Now we need to save what was the last time read for each kind of
  // data to avoid rereading it that is to say we need a var for
  // e.g. PointData/CellData/Points/Cells...
  // See SubClass for details with member vars like PointsTimeStep/PointsOffset

  // Helper function useful to know if a timestep is found in an array of timestep
  static int IsTimeStepInArray(int timestep, int* timesteps, int length);

  svtkDataObject* GetCurrentOutput();
  svtkInformation* GetCurrentOutputInformation();

  // Flag for whether DataProgressCallback should actually update
  // progress.
  int InReadData;

  virtual void ConvertGhostLevelsToGhostType(FieldType, svtkAbstractArray*, svtkIdType, svtkIdType) {}

  void ReadFieldData();

private:
  // The stream used to read the input if it is in a file.
  istream* FileStream;
  // The stream used to read the input if it is in a string.
  std::istringstream* StringStream;
  int TimeStepWasReadOnce;

  int FileMajorVersion;
  int FileMinorVersion;

  svtkDataObject* CurrentOutput;
  svtkInformation* CurrentOutputInformation;

private:
  svtkXMLReader(const svtkXMLReader&) = delete;
  void operator=(const svtkXMLReader&) = delete;

  svtkCommand* ReaderErrorObserver;
  svtkCommand* ParserErrorObserver;
};

#endif
