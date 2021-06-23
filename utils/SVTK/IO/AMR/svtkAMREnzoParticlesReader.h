/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMREnzoParticlesReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkAMREnzoParticlesReader
 *
 *
 *  A concrete instance of the svtkAMRBaseParticlesReader which provides
 *  functionality for loading ENZO AMR particle datasets from ENZO.
 *
 * @sa
 *  svtkAMRBaseParticlesReader
 */

#ifndef svtkAMREnzoParticlesReader_h
#define svtkAMREnzoParticlesReader_h

#include "svtkAMRBaseParticlesReader.h"
#include "svtkIOAMRModule.h" // For export macro

class svtkPolyData;
class svtkDataArray;
class svtkIntArray;
class svtkEnzoReaderInternal;

class SVTKIOAMR_EXPORT svtkAMREnzoParticlesReader : public svtkAMRBaseParticlesReader
{
public:
  static svtkAMREnzoParticlesReader* New();
  svtkTypeMacro(svtkAMREnzoParticlesReader, svtkAMRBaseParticlesReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Returns the requested particle type.
   */
  svtkSetMacro(ParticleType, int);
  svtkGetMacro(ParticleType, int);
  //@}

  /**
   * See svtkAMRBaseParticlesReader::GetTotalNumberOfParticles.
   */
  int GetTotalNumberOfParticles() override;

protected:
  svtkAMREnzoParticlesReader();
  ~svtkAMREnzoParticlesReader() override;

  /**
   * Read the particles from the given particles file for the block
   * corresponding to the given block index.
   */
  svtkPolyData* GetParticles(const char* file, const int blockIdx);

  /**
   * See svtkAMRBaseParticlesReader::ReadMetaData()
   */
  void ReadMetaData() override;

  /**
   * See svtkAMRBaseParticlesReader::SetupParticleDataSelections
   */
  void SetupParticleDataSelections() override;

  /**
   * Filter's by particle type, iff particle_type is included in
   * the given file.
   */
  bool CheckParticleType(const int pIdx, svtkIntArray* ptypes);

  /**
   * Returns the ParticlesType Array
   */
  svtkDataArray* GetParticlesTypeArray(const int blockIdx);

  /**
   * Reads the particles.
   */
  svtkPolyData* ReadParticles(const int blkidx) override;

  int ParticleType;

  svtkEnzoReaderInternal* Internal;

private:
  svtkAMREnzoParticlesReader(const svtkAMREnzoParticlesReader&) = delete;
  void operator=(const svtkAMREnzoParticlesReader&) = delete;
};

#endif /* svtkAMREnzoParticlesReader_h */
