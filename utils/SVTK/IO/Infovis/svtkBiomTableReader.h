/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBiomTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBiomTableReader
 * @brief   read svtkTable from a .biom input file
 *
 * svtkBiomTableReader is a source object that reads ASCII biom data files.
 * The output of this reader is a single svtkTable data object.
 * @sa
 * svtkTable svtkTableReader svtkDataReader
 */

#ifndef svtkBiomTableReader_h
#define svtkBiomTableReader_h

#include "svtkIOInfovisModule.h" // For export macro
#include "svtkTableReader.h"

class svtkTable;
class svtkVariant;

class SVTKIOINFOVIS_EXPORT svtkBiomTableReader : public svtkTableReader
{
public:
  static svtkBiomTableReader* New();
  svtkTypeMacro(svtkBiomTableReader, svtkTableReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output of this reader.
   */
  svtkTable* GetOutput();
  svtkTable* GetOutput(int idx);
  void SetOutput(svtkTable* output);
  //@}

  /**
   * Actual reading happens here
   */
  int ReadMeshSimple(const std::string& fname, svtkDataObject* output) override;

protected:
  svtkBiomTableReader();
  ~svtkBiomTableReader() override;

  int FillOutputPortInformation(int, svtkInformation*) override;
  void ParseShape();
  void ParseDataType();
  void ParseSparseness();
  void InitializeData();
  void FillData(svtkVariant v);
  void ParseSparseData();
  void ParseDenseData();
  void InsertValue(int row, int col, const std::string& value);
  void ParseId();
  void ParseColumns();
  void ParseRows();

private:
  std::string FileContents;
  int NumberOfRows;
  int NumberOfColumns;
  int DataType;
  bool Sparse;
  svtkBiomTableReader(const svtkBiomTableReader&) = delete;
  void operator=(const svtkBiomTableReader&) = delete;
};

#endif
