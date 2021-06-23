/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRFlashParticlesReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMRFlashParticlesReader
 *
 *
 *  A concrete instance of svtkAMRBaseParticlesReader that implements
 *  functionality for reading flash particle datasets.
 */

#ifndef svtkAMRFlashParticlesReader_h
#define svtkAMRFlashParticlesReader_h

#include "svtkAMRBaseParticlesReader.h"
#include "svtkIOAMRModule.h" // For export macro

class svtkIndent;
class svtkPolyData;
class svtkPointData;
class svtkIdList;
class svtkFlashReaderInternal;

class SVTKIOAMR_EXPORT svtkAMRFlashParticlesReader : public svtkAMRBaseParticlesReader
{
public:
  static svtkAMRFlashParticlesReader* New();
  svtkTypeMacro(svtkAMRFlashParticlesReader, svtkAMRBaseParticlesReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * See svtkAMRBaseParticlesReader::GetTotalNumberOfParticles.
   */
  int GetTotalNumberOfParticles() override;

protected:
  svtkAMRFlashParticlesReader();
  ~svtkAMRFlashParticlesReader() override;

  /**
   * See svtkAMRBaseParticlesReader::ReadMetaData
   */
  void ReadMetaData() override;

  /**
   * See svtkAMRBaseParticlesReader::SetupParticlesDataSelections
   */
  void SetupParticleDataSelections() override;

  /**
   * See svtkAMRBaseParticlesReader::ReadParticles
   */
  svtkPolyData* ReadParticles(const int blkidx) override;

  /**
   * Reads the particlles of the given block from the given file.
   */
  svtkPolyData* GetParticles(const char* file, const int blkidx);

  svtkFlashReaderInternal* Internal;

private:
  svtkAMRFlashParticlesReader(const svtkAMRFlashParticlesReader&) = delete;
  void operator=(const svtkAMRFlashParticlesReader&) = delete;
};

#endif /* svtkAMRFlashParticlesReader_h */
