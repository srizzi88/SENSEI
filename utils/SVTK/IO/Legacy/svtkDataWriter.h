/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataWriter
 * @brief   helper class for objects that write svtk data files
 *
 * svtkDataWriter is a helper class that opens and writes the svtk header and
 * point data (e.g., scalars, vectors, normals, etc.) from a svtk data file.
 * See text for various formats.
 *
 * @sa
 * svtkDataSetWriter svtkPolyDataWriter svtkStructuredGridWriter
 * svtkStructuredPointsWriter svtkUnstructuredGridWriter
 * svtkFieldDataWriter svtkRectilinearGridWriter
 */

#ifndef svtkDataWriter_h
#define svtkDataWriter_h

#include "svtkIOLegacyModule.h" // For export macro
#include "svtkWriter.h"

#include <locale> // For locale settings

class svtkCellArray;
class svtkDataArray;
class svtkDataSet;
class svtkFieldData;
class svtkGraph;
class svtkInformation;
class svtkInformationKey;
class svtkPoints;
class svtkTable;

class SVTKIOLEGACY_EXPORT svtkDataWriter : public svtkWriter
{
public:
  svtkTypeMacro(svtkDataWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Created object with default header, ASCII format, and default names for
   * scalars, vectors, tensors, normals, and texture coordinates.
   */
  static svtkDataWriter* New();

  //@{
  /**
   * Specify file name of svtk polygon data file to write.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  svtkSetMacro(WriteToOutputString, svtkTypeBool);
  svtkGetMacro(WriteToOutputString, svtkTypeBool);
  svtkBooleanMacro(WriteToOutputString, svtkTypeBool);
  //@}

  //@{
  /**
   * When WriteToOutputString in on, then a string is allocated, written to,
   * and can be retrieved with these methods.  The string is deleted during
   * the next call to write ...
   */
  svtkGetMacro(OutputStringLength, svtkIdType);
  svtkGetStringMacro(OutputString);
  unsigned char* GetBinaryOutputString()
  {
    return reinterpret_cast<unsigned char*>(this->OutputString);
  }
  //@}

  /**
   * When WriteToOutputString is on, this method returns a copy of the
   * output string in a svtkStdString.
   */
  svtkStdString GetOutputStdString();

  /**
   * This convenience method returns the string, sets the IVAR to nullptr,
   * so that the user is responsible for deleting the string.
   * I am not sure what the name should be, so it may change in the future.
   */
  char* RegisterAndGetOutputString();

  //@{
  /**
   * Specify the header for the svtk data file.
   */
  svtkSetStringMacro(Header);
  svtkGetStringMacro(Header);
  //@}

  //@{
  /**
   * If true, svtkInformation objects attached to arrays and array component
   * nameswill be written to the output. Default is true.
   */
  svtkSetMacro(WriteArrayMetaData, bool);
  svtkGetMacro(WriteArrayMetaData, bool);
  svtkBooleanMacro(WriteArrayMetaData, bool);
  //@}

  //@{
  /**
   * Specify file type (ASCII or BINARY) for svtk data file.
   */
  svtkSetClampMacro(FileType, int, SVTK_ASCII, SVTK_BINARY);
  svtkGetMacro(FileType, int);
  void SetFileTypeToASCII() { this->SetFileType(SVTK_ASCII); }
  void SetFileTypeToBinary() { this->SetFileType(SVTK_BINARY); }
  //@}

  //@{
  /**
   * Give a name to the scalar data. If not specified, uses default
   * name "scalars".
   */
  svtkSetStringMacro(ScalarsName);
  svtkGetStringMacro(ScalarsName);
  //@}

  //@{
  /**
   * Give a name to the vector data. If not specified, uses default
   * name "vectors".
   */
  svtkSetStringMacro(VectorsName);
  svtkGetStringMacro(VectorsName);
  //@}

  //@{
  /**
   * Give a name to the tensors data. If not specified, uses default
   * name "tensors".
   */
  svtkSetStringMacro(TensorsName);
  svtkGetStringMacro(TensorsName);
  //@}

  //@{
  /**
   * Give a name to the normals data. If not specified, uses default
   * name "normals".
   */
  svtkSetStringMacro(NormalsName);
  svtkGetStringMacro(NormalsName);
  //@}

  //@{
  /**
   * Give a name to the texture coordinates data. If not specified, uses
   * default name "textureCoords".
   */
  svtkSetStringMacro(TCoordsName);
  svtkGetStringMacro(TCoordsName);
  //@}

  //@{
  /**
   * Give a name to the global ids data. If not specified, uses
   * default name "global_ids".
   */
  svtkSetStringMacro(GlobalIdsName);
  svtkGetStringMacro(GlobalIdsName);
  //@}

  //@{
  /**
   * Give a name to the pedigree ids data. If not specified, uses
   * default name "pedigree_ids".
   */
  svtkSetStringMacro(PedigreeIdsName);
  svtkGetStringMacro(PedigreeIdsName);
  //@}

  //@{
  /**
   * Give a name to the edge flags data. If not specified, uses
   * default name "edge_flags".
   */
  svtkSetStringMacro(EdgeFlagsName);
  svtkGetStringMacro(EdgeFlagsName);
  //@}

  //@{
  /**
   * Give a name to the lookup table. If not specified, uses default
   * name "lookupTable".
   */
  svtkSetStringMacro(LookupTableName);
  svtkGetStringMacro(LookupTableName);
  //@}

  //@{
  /**
   * Give a name to the field data. If not specified, uses default
   * name "field".
   */
  svtkSetStringMacro(FieldDataName);
  svtkGetStringMacro(FieldDataName);
  //@}

  /**
   * Open a svtk data file. Returns nullptr if error.
   */
  virtual ostream* OpenSVTKFile();

  /**
   * Write the header of a svtk data file. Returns 0 if error.
   */
  int WriteHeader(ostream* fp);

  /**
   * Write out the points of the data set.
   */
  int WritePoints(ostream* fp, svtkPoints* p);

  /**
   * Write out coordinates for rectilinear grids.
   */
  int WriteCoordinates(ostream* fp, svtkDataArray* coords, int axes);

  /**
   * Write out the cells of the data set.
   */
  int WriteCells(ostream* fp, svtkCellArray* cells, const char* label);

  /**
   * Write the cell data (e.g., scalars, vectors, ...) of a svtk dataset.
   * Returns 0 if error.
   */
  int WriteCellData(ostream* fp, svtkDataSet* ds);

  /**
   * Write the point data (e.g., scalars, vectors, ...) of a svtk dataset.
   * Returns 0 if error.
   */
  int WritePointData(ostream* fp, svtkDataSet* ds);

  /**
   * Write the edge data (e.g., scalars, vectors, ...) of a svtk graph.
   * Returns 0 if error.
   */
  int WriteEdgeData(ostream* fp, svtkGraph* g);

  /**
   * Write the vertex data (e.g., scalars, vectors, ...) of a svtk graph.
   * Returns 0 if error.
   */
  int WriteVertexData(ostream* fp, svtkGraph* g);

  /**
   * Write the row data (e.g., scalars, vectors, ...) of a svtk table.
   * Returns 0 if error.
   */
  int WriteRowData(ostream* fp, svtkTable* g);

  /**
   * Write out the field data.
   */
  int WriteFieldData(ostream* fp, svtkFieldData* f);

  /**
   * Write out the data associated with the dataset (i.e. field data owned by
   * the dataset itself - distinct from that owned by the cells or points).
   */
  int WriteDataSetData(ostream* fp, svtkDataSet* ds);

  /**
   * Close a svtk file.
   */
  void CloseSVTKFile(ostream* fp);

protected:
  svtkDataWriter();
  ~svtkDataWriter() override;

  svtkTypeBool WriteToOutputString;
  char* OutputString;
  svtkIdType OutputStringLength;

  void WriteData() override; // dummy method to allow this class to be instantiated and delegated to

  char* FileName;
  char* Header;
  int FileType;

  bool WriteArrayMetaData;

  char* ScalarsName;
  char* VectorsName;
  char* TensorsName;
  char* TCoordsName;
  char* NormalsName;
  char* LookupTableName;
  char* FieldDataName;
  char* GlobalIdsName;
  char* PedigreeIdsName;
  char* EdgeFlagsName;

  std::locale CurrentLocale;

  int WriteArray(ostream* fp, int dataType, svtkAbstractArray* data, const char* format,
    svtkIdType num, svtkIdType numComp);
  int WriteScalarData(ostream* fp, svtkDataArray* s, svtkIdType num);
  int WriteVectorData(ostream* fp, svtkDataArray* v, svtkIdType num);
  int WriteNormalData(ostream* fp, svtkDataArray* n, svtkIdType num);
  int WriteTCoordData(ostream* fp, svtkDataArray* tc, svtkIdType num);
  int WriteTensorData(ostream* fp, svtkDataArray* t, svtkIdType num);
  int WriteGlobalIdData(ostream* fp, svtkDataArray* g, svtkIdType num);
  int WritePedigreeIdData(ostream* fp, svtkAbstractArray* p, svtkIdType num);
  int WriteEdgeFlagsData(ostream* fp, svtkDataArray* edgeFlags, svtkIdType num);

  bool CanWriteInformationKey(svtkInformation* info, svtkInformationKey* key);

  /**
   * Format is detailed \ref IOLegacyInformationFormat "here".
   */
  int WriteInformation(ostream* fp, svtkInformation* info);

private:
  svtkDataWriter(const svtkDataWriter&) = delete;
  void operator=(const svtkDataWriter&) = delete;
};

#endif
