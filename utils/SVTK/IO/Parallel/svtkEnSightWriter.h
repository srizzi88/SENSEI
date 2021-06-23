/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEnSightWriter.h

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
 * @class   svtkEnSightWriter
 * @brief   write svtk unstructured grid data as an EnSight file
 *
 * svtkEnSightWriter is a source object that writes binary
 * unstructured grid data files in EnSight format. See EnSight Manual for
 * format details
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 * Be sure to specify the endian-ness of the file when reading it into EnSight
 */

#ifndef svtkEnSightWriter_h
#define svtkEnSightWriter_h

#include "svtkIOParallelModule.h" // For export macro
#include "svtkWriter.h"

class svtkUnstructuredGrid;

class SVTKIOPARALLEL_EXPORT svtkEnSightWriter : public svtkWriter
{

public:
  svtkTypeMacro(svtkEnSightWriter, svtkWriter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**

   */
  static svtkEnSightWriter* New();

  //@{
  /**
   * Specify which process this writer is
   */
  svtkSetMacro(ProcessNumber, int);
  svtkGetMacro(ProcessNumber, int);
  //@}

  //@{
  /**
   * Specify path of EnSight data files to write.
   */
  svtkSetStringMacro(Path);
  svtkGetStringMacro(Path);
  //@}

  //@{
  /**
   * Specify base name of EnSight data files to write.
   */
  svtkSetStringMacro(BaseName);
  svtkGetStringMacro(BaseName);
  //@}

  //@{
  /**
   * Specify the path and base name of the output files.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specify the Timestep that this data is for
   */
  svtkSetMacro(TimeStep, int);
  svtkGetMacro(TimeStep, int);
  //@}

  //@{
  /**
   * Specify the number of ghost levels to include in output files
   */
  svtkSetMacro(GhostLevel, int);
  svtkGetMacro(GhostLevel, int);
  //@}

  //@{
  /**
   * Specify whether the geometry changes each timestep
   * if false, geometry is only written at timestep 0
   */
  svtkSetMacro(TransientGeometry, bool);
  svtkGetMacro(TransientGeometry, bool);
  //@}

  //@{
  /**
   * set the number of block ID's
   */
  svtkSetMacro(NumberOfBlocks, int);
  svtkGetMacro(NumberOfBlocks, int);
  //@}

  //@{
  /**
   * set the array of Block ID's
   * this class keeps a reference to the array and will not delete it
   */
  virtual void SetBlockIDs(int* val) { BlockIDs = val; }
  virtual int* GetBlockIDs() { return BlockIDs; }
  //@}

  //@{
  /**
   * Specify the input data or filter.
   */
  virtual void SetInputData(svtkUnstructuredGrid* input);
  virtual svtkUnstructuredGrid* GetInput();
  //@}

  //@{
  /**
   * Writes the case file that EnSight is capable of reading
   * The other data files must be written before the case file
   * and the input must be one of the time steps
   * variables must be the same for all time steps or the case file will be
   * missing variables
   */
  virtual void WriteCaseFile(int TotalTimeSteps);
  virtual void WriteSOSCaseFile(int NumProcs);
  //@}

protected:
  svtkEnSightWriter();
  ~svtkEnSightWriter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  void WriteData() override; // method to allow this class to be instantiated and delegated to

  virtual void WriteStringToFile(const char* string, FILE* file);
  virtual void WriteTerminatedStringToFile(const char* string, FILE* file);
  virtual void WriteIntToFile(const int i, FILE* file);
  virtual void WriteFloatToFile(const float f, FILE* file);
  virtual void WriteElementTypeToFile(int ElementType, FILE* fd);

  virtual bool ShouldWriteGeometry();
  virtual void SanitizeFileName(char* name);
  virtual FILE* OpenFile(char* name);

  void ComputeNames();
  void DefaultNames();

  int GetExodusModelIndex(int* ElementArray, int NumberElements, int PartID);

  char* Path;
  char* BaseName;
  char* FileName;
  int TimeStep;
  int GhostLevelMultiplier;
  int ProcessNumber;
  int NumberOfProcesses;
  int NumberOfBlocks;
  int* BlockIDs;
  bool TransientGeometry;
  int GhostLevel;
  svtkUnstructuredGrid* TmpInput;

  svtkEnSightWriter(const svtkEnSightWriter&) = delete;
  void operator=(const svtkEnSightWriter&) = delete;
};

#endif
