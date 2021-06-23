/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRFlashReader.h

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
 * for reading Flash AMR datasets.
 */

#ifndef svtkAMRFlashReader_h
#define svtkAMRFlashReader_h

#include "svtkAMRBaseReader.h"
#include "svtkIOAMRModule.h" // For export macro

class svtkOverlappingAMR;
class svtkFlashReaderInternal;

class SVTKIOAMR_EXPORT svtkAMRFlashReader : public svtkAMRBaseReader
{
public:
  static svtkAMRFlashReader* New();
  svtkTypeMacro(svtkAMRFlashReader, svtkAMRBaseReader);
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
  svtkAMRFlashReader();
  ~svtkAMRFlashReader() override;

  /**
   * See svtkAMRBaseReader::ReadMetaData
   */
  void ReadMetaData() override;

  /**
   * See svtkAMRBaseReader::GetBlockLevel
   */
  int GetBlockLevel(const int blockIdx) override;

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

  bool IsReady;

private:
  svtkAMRFlashReader(const svtkAMRFlashReader&) = delete;
  void operator=(const svtkAMRFlashReader&) = delete;

  void ComputeStats(svtkFlashReaderInternal* internal, std::vector<int>& numBlocks, double min[3]);
  svtkFlashReaderInternal* Internal;
};

#endif /* svtkAMRFlashReader_h */
