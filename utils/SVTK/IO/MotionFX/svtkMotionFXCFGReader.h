/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMotionFXCFGReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef svtkMotionFXCFGReader_h
#define svtkMotionFXCFGReader_h

#include "svtkIOMotionFXModule.h" // for export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

#include <string> // for std::string

/**
 * @class svtkMotionFXCFGReader
 * @brief reader for MotionFX motion definitions cfg files.
 *
 * MotionFX files comprise of `motion`s for a collection of STL files. The
 * motions define the transformations to apply to STL geometry to emulate
 * motion like translation, rotation, planetary motion, etc.
 *
 * This reader reads such a CFG file and produces a temporal output for the time
 * range defined in the file. The resolution of time can be controlled using the
 * `SetTimeResolution` method. The output is a multiblock dataset with blocks
 * for each of bodies, identified by an STL file, in the cfg file.
 *
 * The reader uses PEGTL (https://github.com/taocpp/PEGTL)
 * to define and parse the grammar for the CFG file.
 */
class SVTKIOMOTIONFX_EXPORT svtkMotionFXCFGReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkMotionFXCFGReader* New();
  svtkTypeMacro(svtkMotionFXCFGReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the filename.
   */
  void SetFileName(const char* fname);
  const char* GetFileName() const
  {
    return this->FileName.empty() ? nullptr : this->FileName.c_str();
  }
  //@}

  //@{
  /**
   * Get/Set the time resolution for timesteps produced by the reader.
   */
  svtkSetClampMacro(TimeResolution, int, 1, SVTK_INT_MAX);
  svtkGetMacro(TimeResolution, int);
  //@}

protected:
  svtkMotionFXCFGReader();
  ~svtkMotionFXCFGReader() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Reads meta-data. Returns false if file not readable.
   */
  bool ReadMetaData();

  std::string FileName;
  int TimeResolution;
  svtkTimeStamp FileNameMTime;
  svtkTimeStamp MetaDataMTime;

private:
  svtkMotionFXCFGReader(const svtkMotionFXCFGReader&) = delete;
  void operator=(const svtkMotionFXCFGReader&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
};

#endif
