/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3Reader.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXdmf3Reader
 * @brief   Reads <tt>eXtensible Data Model and Format</tt> files
 *
 * svtkXdmf3Reader reads XDMF data files so that they can be visualized using
 * SVTK. The output data produced by this reader depends on the number of grids
 * in the data file. If the data file has a single domain with a single grid,
 * then the output type is a svtkDataSet subclass of the appropriate type,
 * otherwise it's a svtkMultiBlockDataSet.
 *
 * @warning
 * Uses the XDMF API (http://www.xdmf.org)
 */

#ifndef svtkXdmf3Reader_h
#define svtkXdmf3Reader_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkIOXdmf3Module.h" // For export macro

class svtkXdmf3ArraySelection;
class svtkGraph;

class SVTKIOXDMF3_EXPORT svtkXdmf3Reader : public svtkDataObjectAlgorithm
{
public:
  static svtkXdmf3Reader* New();
  svtkTypeMacro(svtkXdmf3Reader, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set tells the reader the name of a single top level xml file to read.
   */
  void SetFileName(const char* filename);

  //@{
  /**
   * Add and remove give the reader a list of top level xml files to read.
   * Whether the set is treated as a spatial or temporal collection depends
   * on FileSeriestAsTime.
   */
  virtual void AddFileName(const char* filename);
  virtual void RemoveAllFileNames();
  //@}

  //@{
  /**
   * When true (the default) the reader treats a series of files as a temporal
   * collection. When false it treats it as a spatial partition and uses
   * an optimized top level partitioning strategy.
   */
  svtkSetMacro(FileSeriesAsTime, bool);
  svtkGetMacro(FileSeriesAsTime, bool);
  //@}

  /**
   * Determine if the file can be read with this reader.
   */
  virtual int CanReadFile(const char* filename);

  /**
   * Get information about point-based arrays. As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfPointArrays();

  /**
   * Returns the name of point array at the give index. Returns nullptr if index is
   * invalid.
   */
  const char* GetPointArrayName(int index);

  //@{
  /**
   * Get/Set the point array status.
   */
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  //@}

  //@{
  /**
   * Get information about cell-based arrays.  As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int index);
  void SetCellArrayStatus(const char* name, int status);
  int GetCellArrayStatus(const char* name);
  //@}

  //@{
  /**
   * Get information about unaligned arrays.  As is typical with readers this
   * in only valid after the filename is set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfFieldArrays();
  const char* GetFieldArrayName(int index);
  void SetFieldArrayStatus(const char* name, int status);
  int GetFieldArrayStatus(const char* name);
  //@}

  //@{
  /**
   * Get/Set information about grids. As is typical with readers this is valid
   * only after the filename as been set and UpdateInformation() has been
   * called.
   */
  int GetNumberOfGrids();
  const char* GetGridName(int index);
  void SetGridStatus(const char* gridname, int status);
  int GetGridStatus(const char* gridname);
  //@}

  //@{
  /**
   * Get/Set information about sets. As is typical with readers this is valid
   * only after the filename as been set and UpdateInformation() has been
   * called. Note that sets with non-zero Ghost value are not treated as sets
   * that the user can select using this API.
   */
  int GetNumberOfSets();
  const char* GetSetName(int index);
  void SetSetStatus(const char* gridname, int status);
  int GetSetStatus(const char* gridname);
  //@}

  /**
   * These methods are provided to make it easier to use the Sets in ParaView.
   */
  int GetNumberOfSetArrays() { return this->GetNumberOfSets(); }
  const char* GetSetArrayName(int index) { return this->GetSetName(index); }
  int GetSetArrayStatus(const char* name) { return this->GetSetStatus(name); }

  /**
   * SIL describes organization of/relationships between classifications
   * eg. blocks/materials/hierarchies.
   */
  virtual svtkGraph* GetSIL();

  /**
   * Every time the SIL is updated a this will return a different value.
   */
  int GetSILUpdateStamp();

protected:
  svtkXdmf3Reader();
  ~svtkXdmf3Reader() override;

  const char* FileNameInternal;
  svtkSetStringMacro(FileNameInternal);

  // Overridden to announce that we make general DataObjects.
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  // Overridden to handle RDO requests the way we need to
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // Overridden to create the correct svtkDataObject subclass for the file.
  virtual int RequestDataObjectInternal(svtkInformationVector*);

  // Overridden to announce temporal information and to participate in
  // structured extent splitting.
  virtual int RequestInformation(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // Read the XDMF and HDF input files and fill in svtk data objects.
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkXdmf3ArraySelection* GetFieldArraySelection();
  svtkXdmf3ArraySelection* GetCellArraySelection();
  svtkXdmf3ArraySelection* GetPointArraySelection();
  svtkXdmf3ArraySelection* GetGridsSelection();
  svtkXdmf3ArraySelection* GetSetsSelection();
  svtkXdmf3ArraySelection* FieldArraysCache;
  svtkXdmf3ArraySelection* CellArraysCache;
  svtkXdmf3ArraySelection* PointArraysCache;
  svtkXdmf3ArraySelection* GridsCache;
  svtkXdmf3ArraySelection* SetsCache;

private:
  svtkXdmf3Reader(const svtkXdmf3Reader&) = delete;
  void operator=(const svtkXdmf3Reader&) = delete;

  bool FileSeriesAsTime;

  class Internals;
  Internals* Internal;
};

#endif
