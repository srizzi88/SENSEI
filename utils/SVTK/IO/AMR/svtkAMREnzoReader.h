/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMREnzoReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMREnzoReader
 *
 *
 * A concrete instance of svtkAMRBaseReader that implements functionality
 * for reading Enzo AMR datasets.
 */

#ifndef svtkAMREnzoReader_h
#define svtkAMREnzoReader_h

#include "svtkAMRBaseReader.h"
#include "svtkIOAMRModule.h" // For export macro

#include <map>    // For STL map
#include <string> // For std::string

class svtkOverlappingAMR;
class svtkEnzoReaderInternal;

class SVTKIOAMR_EXPORT svtkAMREnzoReader : public svtkAMRBaseReader
{
public:
  static svtkAMREnzoReader* New();
  svtkTypeMacro(svtkAMREnzoReader, svtkAMRBaseReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get whether data should be converted to CGS
   */
  svtkSetMacro(ConvertToCGS, svtkTypeBool);
  svtkGetMacro(ConvertToCGS, svtkTypeBool);
  svtkBooleanMacro(ConvertToCGS, svtkTypeBool);
  //@}

  /**
   * See svtkAMRBaseReader::GetNumberOfBlocks
   */
  int GetNumberOfBlocks() override;

  /**
   * See svtkAMRBaseReader::GetNumberOfLevels
   */
  int GetNumberOfLevels() override;

  /**
   * See svtkAMRBaseReader::SetFileName
   */
  void SetFileName(const char* fileName) override;

protected:
  svtkAMREnzoReader();
  ~svtkAMREnzoReader() override;

  /**
   * Parses the parameters file and extracts the
   * conversion factors that are used to convert
   * to CGS units.
   */
  void ParseConversionFactors();

  /**
   * Given an array name of the form "array[idx]" this method
   * extracts and returns the corresponding index idx.
   */
  int GetIndexFromArrayName(std::string arrayName);

  /**
   * Given the label string, this method parses the attribute label and
   * the string index.
   */
  void ParseLabel(const std::string& labelString, int& idx, std::string& label);

  /**
   * Given the label string, this method parses the corresponding attribute
   * index and conversion factor
   */
  void ParseCFactor(const std::string& labelString, int& idx, double& factor);

  /**
   * Given the variable name, return the conversion factor used to convert
   * the data to CGS. These conversion factors are read directly from the
   * parameters file when the filename is set.
   */
  double GetConversionFactor(const std::string& name);

  /**
   * See svtkAMRBaseReader::ReadMetaData
   */
  void ReadMetaData() override;

  /**
   * See svtkAMRBaseReader::GetBlockLevel
   */
  int GetBlockLevel(const int blockIdx) override;

  void ComputeStats(
    svtkEnzoReaderInternal* internal, std::vector<int>& blocksPerLevel, double min[3]);

  /**
   * See svtkAMRBaseReader::FillMetaData
   */
  int FillMetaData() override;

  /**
   * See svtkAMRBaseReader::GetAMRGrid
   */
  svtkUniformGrid* GetAMRGrid(const int blockIdx) override;

  /**
   * See svtkAMRBaseReader::GetAMRGridData
   */
  void GetAMRGridData(const int blockIdx, svtkUniformGrid* block, const char* field) override;

  /**
   * See svtkAMRBaseReader::GetAMRGridData
   */
  void GetAMRGridPointData(const int svtkNotUsed(blockIdx), svtkUniformGrid* svtkNotUsed(block),
    const char* svtkNotUsed(field)) override
  {
  }

  /**
   * See svtkAMRBaseReader::SetUpDataArraySelections
   */
  void SetUpDataArraySelections() override;

  svtkTypeBool ConvertToCGS;
  bool IsReady;

private:
  svtkAMREnzoReader(const svtkAMREnzoReader&) = delete;
  void operator=(const svtkAMREnzoReader&) = delete;

  svtkEnzoReaderInternal* Internal;

  std::map<std::string, int> label2idx;
  std::map<int, double> conversionFactors;
};

#endif /* svtkAMREnzoReader_h */
