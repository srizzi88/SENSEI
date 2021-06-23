/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAMReXGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkAMReXGridReader
 * @brief reader for AMReX plotfiles grid data.
 *
 * svtkAMReXGridReader readers grid data from AMReX plotfiles.
 */

#ifndef svtkAMReXGridReader_h
#define svtkAMReXGridReader_h

#include "svtkAMRBaseReader.h"
#include "svtkIOAMRModule.h" // For export macro
#include "svtkNew.h"         // for svtkNew

#include <string> // for std::string.
#include <vector> // for std::vector.

class svtkOverlappingAMR;
class svtkAMReXGridReaderInternal;

class SVTKIOAMR_EXPORT svtkAMReXGridReader : public svtkAMRBaseReader
{
public:
  static svtkAMReXGridReader* New();
  svtkTypeMacro(svtkAMReXGridReader, svtkAMRBaseReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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
  svtkAMReXGridReader();
  ~svtkAMReXGridReader() override;

  /**
   * See svtkAMRBaseReader::ReadMetaData
   */
  void ReadMetaData() override;

  /**
   * See svtkAMRBaseReader::GetBlockLevel
   */
  int GetBlockLevel(const int blockIdx) override;

  /**
   * GetLevelBlockID
   *
   * @param blockIdx
   *
   * @return int representing block in level blockIdx is contained in
   */
  int GetLevelBlockID(const int blockIdx);

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
    ;
  }

  /**
   * See svtkAMRBaseReader::SetUpDataArraySelections
   */
  void SetUpDataArraySelections() override;

  int GetDimension();
  bool IsReady;

private:
  svtkAMReXGridReader(const svtkAMReXGridReader&) = delete;
  void operator=(const svtkAMReXGridReader&) = delete;

  void ComputeStats(
    svtkAMReXGridReaderInternal* internal, std::vector<int>& numBlocks, double min[3]);
  svtkAMReXGridReaderInternal* Internal;
};

#endif
