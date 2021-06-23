/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExodusIIReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkPExodusIIReader
 * @brief   Read Exodus II files (.exii)
 *
 * svtkPExodusIIReader is a unstructured grid source object that reads
 * ExodusII files. Most of the meta data associated with the
 * file is loaded when UpdateInformation is called. This includes
 * information like Title, number of blocks, number and names of
 * arrays. This data can be retrieved from methods in this
 * reader. Separate arrays that are meant to be a single vector, are
 * combined internally for convenience. To be combined, the array
 * names have to be identical except for a trailing X,Y and Z (or
 * x,y,z). By default all cell and point arrays are loaded. However,
 * the user can flag arrays not to load with the methods
 * "SetPointDataArrayLoadFlag" and "SetCellDataArrayLoadFlag". The
 * reader responds to piece requests by loading only a range of the
 * possible blocks. Unused points are filtered out internally.
 */

#ifndef svtkPExodusIIReader_h
#define svtkPExodusIIReader_h

#include "svtkExodusIIReader.h"
#include "svtkIOParallelExodusModule.h" // For export macro

#include <vector> // Required for vector

class svtkTimerLog;
class svtkMultiProcessController;

class SVTKIOPARALLELEXODUS_EXPORT svtkPExodusIIReader : public svtkExodusIIReader
{
public:
  static svtkPExodusIIReader* New();
  svtkTypeMacro(svtkPExodusIIReader, svtkExodusIIReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/get the communication object used to relay a list of files
   * from the rank 0 process to all others. This is the only interprocess
   * communication required by svtkPExodusIIReader.
   */
  void SetController(svtkMultiProcessController* c);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * These methods tell the reader that the data is distributed across
   * multiple files. This is for distributed execution. It this case,
   * pieces are mapped to files. The pattern should have one %d to
   * format the file number. FileNumberRange is used to generate file
   * numbers. I was thinking of having an arbitrary list of file
   * numbers. This may happen in the future. (That is why there is no
   * GetFileNumberRange method.
   */
  svtkSetStringMacro(FilePattern);
  svtkGetStringMacro(FilePattern);
  svtkSetStringMacro(FilePrefix);
  svtkGetStringMacro(FilePrefix);
  //@}

  //@{
  /**
   * Set the range of files that are being loaded. The range for single
   * file should add to 0.
   */
  void SetFileRange(int, int);
  void SetFileRange(int* r) { this->SetFileRange(r[0], r[1]); }
  svtkGetVector2Macro(FileRange, int);
  //@}

  /**
   * Provide an arbitrary list of file names instead of a prefix,
   * pattern and range.  Overrides any prefix, pattern and range
   * that is specified.  svtkPExodusIIReader makes it's own copy
   * of your file names.
   */
  void SetFileNames(int nfiles, const char** names);

  void SetFileName(const char* name) override;

  /**
   * Return pointer to list of file names set in SetFileNames
   */
  char** GetFileNames() { return this->FileNames; }

  /**
   * Return number of file names set in SetFileNames
   */
  int GetNumberOfFileNames() { return this->NumberOfFileNames; }

  //@{
  /**
   * Return the number of files to be read.
   */
  svtkGetMacro(NumberOfFiles, int);
  //@}

  svtkIdType GetTotalNumberOfElements() override;
  svtkIdType GetTotalNumberOfNodes() override;

  /**
   * Sends metadata (that read from the input file, not settings modified
   * through this API) from the rank 0 node to all other processes in a job.
   */
  virtual void Broadcast(svtkMultiProcessController* ctrl);

  //@{
  /**
   * The size of the variable cache in MegaByes. This represents the maximum
   * size of cache that a single partition reader can have while reading. When
   * a reader is finished its cache size will be set to a fraction of this based
   * on the number of partitions.
   * The Default for this is 100MiB.
   * Note that because each reader still holds
   * a fraction of the cache size after reading the total amount of data cached
   * can be at most twice this size.
   */
  svtkGetMacro(VariableCacheSize, double);
  svtkSetMacro(VariableCacheSize, double);
  //@}

protected:
  svtkPExodusIIReader();
  ~svtkPExodusIIReader() override;

  //@{
  /**
   * Try to "guess" the pattern of files.
   */
  int DeterminePattern(const char* file);
  static int DetermineFileId(const char* file);
  //@}

  // holds the size of the variable cache in GigaBytes
  double VariableCacheSize;

  // **KEN** Previous discussions concluded with std classes in header
  // files is bad.  Perhaps we should change ReaderList.

  svtkMultiProcessController* Controller;
  svtkIdType ProcRank;
  svtkIdType ProcSize;
  char* FilePattern;
  char* CurrentFilePattern;
  char* FilePrefix;
  char* CurrentFilePrefix;
  char* MultiFileName;
  int FileRange[2];
  int CurrentFileRange[2];
  int NumberOfFiles;
  char** FileNames;
  int NumberOfFileNames;

  std::vector<svtkExodusIIReader*> ReaderList;
  std::vector<int> NumberOfPointsPerFile;
  std::vector<int> NumberOfCellsPerFile;

  int LastCommonTimeStep;

  int Timing;
  svtkTimerLog* TimerLog;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPExodusIIReader(const svtkPExodusIIReader&) = delete;
  void operator=(const svtkPExodusIIReader&) = delete;
};

#endif
