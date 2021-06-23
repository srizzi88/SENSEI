/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataReader
 * @brief   helper superclass for objects that read svtk data files
 *
 * svtkDataReader is a helper superclass that reads the svtk data file header,
 * dataset type, and attribute data (point and cell attributes such as
 * scalars, vectors, normals, etc.) from a svtk data file.  See text for
 * the format of the various svtk file types.
 *
 * @sa
 * svtkPolyDataReader svtkStructuredPointsReader svtkStructuredGridReader
 * svtkUnstructuredGridReader svtkRectilinearGridReader
 */

#ifndef svtkDataReader_h
#define svtkDataReader_h

#include "svtkIOLegacyModule.h" // For export macro
#include "svtkSimpleReader.h"
#include "svtkStdString.h" // For API using strings

#include <svtkSmartPointer.h> // for smart pointer

#include <locale> // For locale settings

#define SVTK_ASCII 1
#define SVTK_BINARY 2

class svtkAbstractArray;
class svtkCharArray;
class svtkCellArray;
class svtkDataSet;
class svtkDataSetAttributes;
class svtkFieldData;
class svtkGraph;
class svtkPointSet;
class svtkRectilinearGrid;
class svtkTable;

class SVTKIOLEGACY_EXPORT svtkDataReader : public svtkSimpleReader
{
public:
  enum FieldType
  {
    POINT_DATA,
    CELL_DATA,
    FIELD_DATA
  };

  static svtkDataReader* New();
  svtkTypeMacro(svtkDataReader, svtkSimpleReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify file name of svtk data file to read. This is just
   * a convenience method that calls the superclass' AddFileName
   * method.
   */
  void SetFileName(const char* fname);
  const char* GetFileName() const;
  const char* GetFileName(int i) const { return this->svtkSimpleReader::GetFileName(i); }
  //@}

  //@{
  /**
   * Is the file a valid svtk file of the passed dataset type ?
   * The dataset type is passed as a lower case string.
   */
  int IsFileValid(const char* dstype);
  int IsFileStructuredPoints() { return this->IsFileValid("structured_points"); }
  int IsFilePolyData() { return this->IsFileValid("polydata"); }
  int IsFileStructuredGrid() { return this->IsFileValid("structured_grid"); }
  int IsFileUnstructuredGrid() { return this->IsFileValid("unstructured_grid"); }
  int IsFileRectilinearGrid() { return this->IsFileValid("rectilinear_grid"); }
  //@}

  //@{
  /**
   * Specify the InputString for use when reading from a character array.
   * Optionally include the length for binary strings. Note that a copy
   * of the string is made and stored. If this causes exceedingly large
   * memory consumption, consider using InputArray instead.
   */
  void SetInputString(const char* in);
  svtkGetStringMacro(InputString);
  void SetInputString(const char* in, int len);
  svtkGetMacro(InputStringLength, int);
  void SetBinaryInputString(const char*, int len);
  void SetInputString(const svtkStdString& input)
  {
    this->SetBinaryInputString(input.c_str(), static_cast<int>(input.length()));
  }
  //@}

  //@{
  /**
   * Specify the svtkCharArray to be used  when reading from a string.
   * If set, this array has precedence over InputString.
   * Use this instead of InputString to avoid the extra memory copy.
   * It should be noted that if the underlying char* is owned by the
   * user ( svtkCharArray::SetArray(array, 1); ) and is deleted before
   * the reader, bad things will happen during a pipeline update.
   */
  virtual void SetInputArray(svtkCharArray*);
  svtkGetObjectMacro(InputArray, svtkCharArray);
  //@}

  //@{
  /**
   * Get the header from the svtk data file.
   */
  svtkGetStringMacro(Header);
  //@}

  //@{
  /**
   * Enable reading from an InputString or InputArray instead of the default,
   * a file.
   */
  svtkSetMacro(ReadFromInputString, svtkTypeBool);
  svtkGetMacro(ReadFromInputString, svtkTypeBool);
  svtkBooleanMacro(ReadFromInputString, svtkTypeBool);
  //@}

  //@{
  /**
   * Get the type of file (ASCII or BINARY). Returned value only valid
   * after file has been read.
   */
  svtkGetMacro(FileType, int);
  //@}

  /**
   * How many attributes of various types are in this file? This
   * requires reading the file, so the filename must be set prior
   * to invoking this operation. (Note: file characteristics are
   * cached, so only a single read is necessary to return file
   * characteristics.)
   */
  int GetNumberOfScalarsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfScalarsInFile;
  }
  int GetNumberOfVectorsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfVectorsInFile;
  }
  int GetNumberOfTensorsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfTensorsInFile;
  }
  int GetNumberOfNormalsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfNormalsInFile;
  }
  int GetNumberOfTCoordsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfTCoordsInFile;
  }
  int GetNumberOfFieldDataInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfFieldDataInFile;
  }

  //@{
  /**
   * What is the name of the ith attribute of a certain type
   * in this file? This requires reading the file, so the filename
   * must be set prior to invoking this operation.
   */
  const char* GetScalarsNameInFile(int i);
  const char* GetVectorsNameInFile(int i);
  const char* GetTensorsNameInFile(int i);
  const char* GetNormalsNameInFile(int i);
  const char* GetTCoordsNameInFile(int i);
  const char* GetFieldDataNameInFile(int i);
  //@}

  //@{
  /**
   * Set the name of the scalar data to extract. If not specified, first
   * scalar data encountered is extracted.
   */
  svtkSetStringMacro(ScalarsName);
  svtkGetStringMacro(ScalarsName);
  //@}

  //@{
  /**
   * Set the name of the vector data to extract. If not specified, first
   * vector data encountered is extracted.
   */
  svtkSetStringMacro(VectorsName);
  svtkGetStringMacro(VectorsName);
  //@}

  //@{
  /**
   * Set the name of the tensor data to extract. If not specified, first
   * tensor data encountered is extracted.
   */
  svtkSetStringMacro(TensorsName);
  svtkGetStringMacro(TensorsName);
  //@}

  //@{
  /**
   * Set the name of the normal data to extract. If not specified, first
   * normal data encountered is extracted.
   */
  svtkSetStringMacro(NormalsName);
  svtkGetStringMacro(NormalsName);
  //@}

  //@{
  /**
   * Set the name of the texture coordinate data to extract. If not specified,
   * first texture coordinate data encountered is extracted.
   */
  svtkSetStringMacro(TCoordsName);
  svtkGetStringMacro(TCoordsName);
  //@}

  //@{
  /**
   * Set the name of the lookup table data to extract. If not specified, uses
   * lookup table named by scalar. Otherwise, this specification supersedes.
   */
  svtkSetStringMacro(LookupTableName);
  svtkGetStringMacro(LookupTableName);
  //@}

  //@{
  /**
   * Set the name of the field data to extract. If not specified, uses
   * first field data encountered in file.
   */
  svtkSetStringMacro(FieldDataName);
  svtkGetStringMacro(FieldDataName);
  //@}

  //@{
  /**
   * Enable reading all scalars.
   */
  svtkSetMacro(ReadAllScalars, svtkTypeBool);
  svtkGetMacro(ReadAllScalars, svtkTypeBool);
  svtkBooleanMacro(ReadAllScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable reading all vectors.
   */
  svtkSetMacro(ReadAllVectors, svtkTypeBool);
  svtkGetMacro(ReadAllVectors, svtkTypeBool);
  svtkBooleanMacro(ReadAllVectors, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable reading all normals.
   */
  svtkSetMacro(ReadAllNormals, svtkTypeBool);
  svtkGetMacro(ReadAllNormals, svtkTypeBool);
  svtkBooleanMacro(ReadAllNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable reading all tensors.
   */
  svtkSetMacro(ReadAllTensors, svtkTypeBool);
  svtkGetMacro(ReadAllTensors, svtkTypeBool);
  svtkBooleanMacro(ReadAllTensors, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable reading all color scalars.
   */
  svtkSetMacro(ReadAllColorScalars, svtkTypeBool);
  svtkGetMacro(ReadAllColorScalars, svtkTypeBool);
  svtkBooleanMacro(ReadAllColorScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable reading all tcoords.
   */
  svtkSetMacro(ReadAllTCoords, svtkTypeBool);
  svtkGetMacro(ReadAllTCoords, svtkTypeBool);
  svtkBooleanMacro(ReadAllTCoords, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable reading all fields.
   */
  svtkSetMacro(ReadAllFields, svtkTypeBool);
  svtkGetMacro(ReadAllFields, svtkTypeBool);
  svtkBooleanMacro(ReadAllFields, svtkTypeBool);
  //@}

  /**
   * Open a svtk data file. Returns zero if error.
   */
  int OpenSVTKFile(const char* fname = nullptr);

  /**
   * Read the header of a svtk data file. Returns 0 if error.
   */
  int ReadHeader(const char* fname = nullptr);

  /**
   * Read the cell data of a svtk data file. The number of cells (from the
   * dataset) must match the number of cells defined in cell attributes (unless
   * no geometry was defined).
   */
  int ReadCellData(svtkDataSet* ds, svtkIdType numCells);

  /**
   * Read the point data of a svtk data file. The number of points (from the
   * dataset) must match the number of points defined in point attributes
   * (unless no geometry was defined).
   */
  int ReadPointData(svtkDataSet* ds, svtkIdType numPts);

  /**
   * Read point coordinates. Return 0 if error.
   */
  int ReadPointCoordinates(svtkPointSet* ps, svtkIdType numPts);

  /**
   * Read point coordinates. Return 0 if error.
   */
  int ReadPointCoordinates(svtkGraph* g, svtkIdType numPts);

  /**
   * Read the vertex data of a svtk data file. The number of vertices (from the
   * graph) must match the number of vertices defined in vertex attributes
   * (unless no geometry was defined).
   */
  int ReadVertexData(svtkGraph* g, svtkIdType numVertices);

  /**
   * Read the edge data of a svtk data file. The number of edges (from the
   * graph) must match the number of edges defined in edge attributes
   * (unless no geometry was defined).
   */
  int ReadEdgeData(svtkGraph* g, svtkIdType numEdges);

  /**
   * Read the row data of a svtk data file.
   */
  int ReadRowData(svtkTable* t, svtkIdType numEdges);

  /**
   * Read cells in a svtkCellArray, and update the smartpointer reference passed
   * in. If no cells are present in the file, cellArray will be set to nullptr.
   * Returns 0 if error.
   */
  int ReadCells(svtkSmartPointer<svtkCellArray>& cellArray);

  /**
   * Read a bunch of "cells". Return 0 if error.
   * @note Legacy implementation for file versions < 5.0.
   */
  int ReadCellsLegacy(svtkIdType size, int* data);

  /**
   * Read a piece of the cells (for streaming compliance)
   */
  int ReadCellsLegacy(svtkIdType size, int* data, int skip1, int read2, int skip3);

  /**
   * Read the coordinates for a rectilinear grid. The axes parameter specifies
   * which coordinate axes (0,1,2) is being read.
   */
  int ReadCoordinates(svtkRectilinearGrid* rg, int axes, int numCoords);

  //@{
  /**
   * Helper functions for reading data.
   */
  svtkAbstractArray* ReadArray(const char* dataType, svtkIdType numTuples, svtkIdType numComp);
  svtkFieldData* ReadFieldData(FieldType fieldType = FIELD_DATA);
  //@}

  //@{
  /**
   * Return major and minor version of the file.
   * Returns version 3.0 if the version cannot be read from file.
   */
  svtkGetMacro(FileMajorVersion, int);
  svtkGetMacro(FileMinorVersion, int);
  //@}

  //@{
  /**
   * Internal function to read in a value.  Returns zero if there was an
   * error.
   */
  int Read(char*);
  int Read(unsigned char*);
  int Read(short*);
  int Read(unsigned short*);
  int Read(int*);
  int Read(unsigned int*);
  int Read(long*);
  int Read(unsigned long*);
  int Read(long long* result);
  int Read(unsigned long long* result);
  int Read(float*);
  int Read(double*);
  //@}

  /**
   * Read @a n character from the stream into @a str, then reset the stream
   * position. Returns the number of characters actually read.
   */
  size_t Peek(char* str, size_t n);

  /**
   * Close the svtk file.
   */
  void CloseSVTKFile();

  /**
   * Internal function to read in a line up to 256 characters.
   * Returns zero if there was an error.
   */
  int ReadLine(char result[256]);

  /**
   * Internal function to read in a string up to 256 characters.
   * Returns zero if there was an error.
   */
  int ReadString(char result[256]);

  /**
   * Helper method for reading in data.
   */
  char* LowerCase(char* str, const size_t len = 256);

  /**
   * Return the istream being used to read in the data.
   */
  istream* GetIStream() { return this->IS; }

  //@{
  /**
   * Overridden to handle reading from a string. The
   * superclass only knows about files.
   */
  int ReadTimeDependentMetaData(int timestep, svtkInformation* metadata) override;
  int ReadMesh(int piece, int npieces, int nghosts, int timestep, svtkDataObject* output) override;
  int ReadPoints(int /*piece*/, int /*npieces*/, int /*nghosts*/, int /*timestep*/,
    svtkDataObject* /*output*/) override
  {
    return 1;
  }
  int ReadArrays(int /*piece*/, int /*npieces*/, int /*nghosts*/, int /*timestep*/,
    svtkDataObject* /*output*/) override
  {
    return 1;
  }
  //@}

  //@{
  /**
   * Overridden with default implementation of doing nothing
   * so that subclasses only override what is needed (usually
   * only ReadMesh).
   */
  int ReadMeshSimple(const std::string& /*fname*/, svtkDataObject* /*output*/) override { return 1; }
  int ReadPointsSimple(const std::string& /*fname*/, svtkDataObject* /*output*/) override
  {
    return 1;
  }
  int ReadArraysSimple(const std::string& /*fname*/, svtkDataObject* /*output*/) override
  {
    return 1;
  }
  //@}

protected:
  svtkDataReader();
  ~svtkDataReader() override;

  std::string CurrentFileName;
  int FileType;
  istream* IS;

  char* ScalarsName;
  char* VectorsName;
  char* TensorsName;
  char* TCoordsName;
  char* NormalsName;
  char* LookupTableName;
  char* FieldDataName;
  char* ScalarLut;

  svtkTypeBool ReadFromInputString;
  char* InputString;
  int InputStringLength;
  int InputStringPos;

  void SetScalarLut(const char* lut);
  svtkGetStringMacro(ScalarLut);

  char* Header;

  int ReadScalarData(svtkDataSetAttributes* a, svtkIdType num);
  int ReadVectorData(svtkDataSetAttributes* a, svtkIdType num);
  int ReadNormalData(svtkDataSetAttributes* a, svtkIdType num);
  int ReadTensorData(svtkDataSetAttributes* a, svtkIdType num, svtkIdType numComp = 9);
  int ReadCoScalarData(svtkDataSetAttributes* a, svtkIdType num);
  int ReadLutData(svtkDataSetAttributes* a);
  int ReadTCoordsData(svtkDataSetAttributes* a, svtkIdType num);
  int ReadGlobalIds(svtkDataSetAttributes* a, svtkIdType num);
  int ReadPedigreeIds(svtkDataSetAttributes* a, svtkIdType num);
  int ReadEdgeFlags(svtkDataSetAttributes* a, svtkIdType num);

  /**
   * Format is detailed \ref IOLegacyInformationFormat "here".
   */
  int ReadInformation(svtkInformation* info, svtkIdType numKeys);

  int ReadDataSetData(svtkDataSet* ds);

  // This supports getting additional information from svtk files
  int NumberOfScalarsInFile;
  char** ScalarsNameInFile;
  int ScalarsNameAllocSize;
  int NumberOfVectorsInFile;
  char** VectorsNameInFile;
  int VectorsNameAllocSize;
  int NumberOfTensorsInFile;
  char** TensorsNameInFile;
  int TensorsNameAllocSize;
  int NumberOfTCoordsInFile;
  char** TCoordsNameInFile;
  int TCoordsNameAllocSize;
  int NumberOfNormalsInFile;
  char** NormalsNameInFile;
  int NormalsNameAllocSize;
  int NumberOfFieldDataInFile;
  char** FieldDataNameInFile;
  int FieldDataNameAllocSize;
  svtkTimeStamp CharacteristicsTime;

  svtkTypeBool ReadAllScalars;
  svtkTypeBool ReadAllVectors;
  svtkTypeBool ReadAllNormals;
  svtkTypeBool ReadAllTensors;
  svtkTypeBool ReadAllColorScalars;
  svtkTypeBool ReadAllTCoords;
  svtkTypeBool ReadAllFields;
  int FileMajorVersion;
  int FileMinorVersion;

  std::locale CurrentLocale;

  void InitializeCharacteristics();
  int CharacterizeFile(); // read entire file, storing important characteristics
  void CheckFor(const char* name, char* line, int& num, char**& array, int& allocSize);

  svtkCharArray* InputArray;

  /**
   * Decode a string. This method is the inverse of
   * svtkWriter::EncodeString.  Returns the length of the
   * result string.
   */
  int DecodeString(char* resname, const char* name);

  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

private:
  svtkDataReader(const svtkDataReader&) = delete;
  void operator=(const svtkDataReader&) = delete;

  void ConvertGhostLevelsToGhostType(FieldType fieldType, svtkAbstractArray* data) const;
};

#endif
