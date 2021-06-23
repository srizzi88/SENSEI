/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenFOAMReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenFOAMReader
 * @brief   reads a dataset in OpenFOAM format
 *
 * svtkOpenFOAMReader creates a multiblock dataset. It reads mesh
 * information and time dependent data.  The polyMesh folders contain
 * mesh information. The time folders contain transient data for the
 * cells. Each folder can contain any number of data files.
 *
 * @par Thanks:
 * Thanks to Terry Jordan of SAIC at the National Energy
 * Technology Laboratory who developed this class.
 * Please address all comments to Terry Jordan (terry.jordan@sa.netl.doe.gov).
 * GUI Based selection of mesh regions and fields available in the case,
 * minor bug fixes, strict memory allocation checks,
 * minor performance enhancements by Philippose Rajan (sarith@rocketmail.com).
 *
 * Token-based FoamFile format lexer/parser,
 * performance/stability/compatibility enhancements, gzipped file
 * support, lagrangian field support, variable timestep support,
 * builtin cell-to-point filter, pointField support, polyhedron
 * decomposition support, OF 1.5 extended format support, multiregion
 * support, old mesh format support, parallelization support for
 * decomposed cases in conjunction with svtkPOpenFOAMReader, et. al. by
 * Takuya Oshima of Niigata University, Japan (oshima@eng.niigata-u.ac.jp).
 *
 * Misc cleanup, bugfixes, improvements
 * Mark Olesen (OpenCFD Ltd.)
 */

#ifndef svtkOpenFOAMReader_h
#define svtkOpenFOAMReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkCollection;
class svtkCharArray;
class svtkDataArraySelection;
class svtkDoubleArray;
class svtkStdString;
class svtkStringArray;

class svtkOpenFOAMReaderPrivate;

class SVTKIOGEOMETRY_EXPORT svtkOpenFOAMReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkOpenFOAMReader* New();
  svtkTypeMacro(svtkOpenFOAMReader, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream&, svtkIndent) override;

  /**
   * Determine if the file can be read with this reader.
   */
  int CanReadFile(const char*);

  //@{
  /**
   * Set/Get the filename.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  /**
   * Get the number of cell arrays available in the input.
   */
  int GetNumberOfCellArrays(void)
  {
    return this->GetNumberOfSelectionArrays(this->CellDataArraySelection);
  }

  /**
   * Get/Set whether the cell array with the given name is to
   * be read.
   */
  int GetCellArrayStatus(const char* name)
  {
    return this->GetSelectionArrayStatus(this->CellDataArraySelection, name);
  }
  void SetCellArrayStatus(const char* name, int status)
  {
    this->SetSelectionArrayStatus(this->CellDataArraySelection, name, status);
  }

  /**
   * Get the name of the cell array with the given index in
   * the input.
   */
  const char* GetCellArrayName(int index)
  {
    return this->GetSelectionArrayName(this->CellDataArraySelection, index);
  }

  /**
   * Turn on/off all cell arrays.
   */
  void DisableAllCellArrays() { this->DisableAllSelectionArrays(this->CellDataArraySelection); }
  void EnableAllCellArrays() { this->EnableAllSelectionArrays(this->CellDataArraySelection); }

  /**
   * Get the number of point arrays available in the input.
   */
  int GetNumberOfPointArrays(void)
  {
    return this->GetNumberOfSelectionArrays(this->PointDataArraySelection);
  }

  /**
   * Get/Set whether the point array with the given name is to
   * be read.
   */
  int GetPointArrayStatus(const char* name)
  {
    return this->GetSelectionArrayStatus(this->PointDataArraySelection, name);
  }
  void SetPointArrayStatus(const char* name, int status)
  {
    this->SetSelectionArrayStatus(this->PointDataArraySelection, name, status);
  }

  /**
   * Get the name of the point array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index)
  {
    return this->GetSelectionArrayName(this->PointDataArraySelection, index);
  }

  /**
   * Turn on/off all point arrays.
   */
  void DisableAllPointArrays() { this->DisableAllSelectionArrays(this->PointDataArraySelection); }
  void EnableAllPointArrays() { this->EnableAllSelectionArrays(this->PointDataArraySelection); }

  /**
   * Get the number of Lagrangian arrays available in the input.
   */
  int GetNumberOfLagrangianArrays(void)
  {
    return this->GetNumberOfSelectionArrays(this->LagrangianDataArraySelection);
  }

  /**
   * Get/Set whether the Lagrangian array with the given name is to
   * be read.
   */
  int GetLagrangianArrayStatus(const char* name)
  {
    return this->GetSelectionArrayStatus(this->LagrangianDataArraySelection, name);
  }
  void SetLagrangianArrayStatus(const char* name, int status)
  {
    this->SetSelectionArrayStatus(this->LagrangianDataArraySelection, name, status);
  }

  /**
   * Get the name of the Lagrangian array with the given index in
   * the input.
   */
  const char* GetLagrangianArrayName(int index)
  {
    return this->GetSelectionArrayName(this->LagrangianDataArraySelection, index);
  }

  /**
   * Turn on/off all Lagrangian arrays.
   */
  void DisableAllLagrangianArrays()
  {
    this->DisableAllSelectionArrays(this->LagrangianDataArraySelection);
  }
  void EnableAllLagrangianArrays()
  {
    this->EnableAllSelectionArrays(this->LagrangianDataArraySelection);
  }

  /**
   * Get the number of Patches (including Internal Mesh) available in the input.
   */
  int GetNumberOfPatchArrays(void)
  {
    return this->GetNumberOfSelectionArrays(this->PatchDataArraySelection);
  }

  /**
   * Get/Set whether the Patch with the given name is to
   * be read.
   */
  int GetPatchArrayStatus(const char* name)
  {
    return this->GetSelectionArrayStatus(this->PatchDataArraySelection, name);
  }
  void SetPatchArrayStatus(const char* name, int status)
  {
    this->SetSelectionArrayStatus(this->PatchDataArraySelection, name, status);
  }

  /**
   * Get the name of the Patch with the given index in
   * the input.
   */
  const char* GetPatchArrayName(int index)
  {
    return this->GetSelectionArrayName(this->PatchDataArraySelection, index);
  }

  /**
   * Turn on/off all Patches including the Internal Mesh.
   */
  void DisableAllPatchArrays() { this->DisableAllSelectionArrays(this->PatchDataArraySelection); }
  void EnableAllPatchArrays() { this->EnableAllSelectionArrays(this->PatchDataArraySelection); }

  //@{
  /**
   * Set/Get whether to create cell-to-point translated data for cell-type data
   */
  svtkSetMacro(CreateCellToPoint, svtkTypeBool);
  svtkGetMacro(CreateCellToPoint, svtkTypeBool);
  svtkBooleanMacro(CreateCellToPoint, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether mesh is to be cached.
   */
  svtkSetMacro(CacheMesh, svtkTypeBool);
  svtkGetMacro(CacheMesh, svtkTypeBool);
  svtkBooleanMacro(CacheMesh, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether polyhedra are to be decomposed.
   */
  svtkSetMacro(DecomposePolyhedra, svtkTypeBool);
  svtkGetMacro(DecomposePolyhedra, svtkTypeBool);
  svtkBooleanMacro(DecomposePolyhedra, svtkTypeBool);
  //@}

  // Option for reading old binary lagrangian/positions format
  //@{
  /**
   * Set/Get whether the lagrangian/positions have additional data or not.
   * For historical reasons, PositionsIsIn13Format is used to denote that
   * the positions only have x,y,z value and the cell of the enclosing cell.
   * In OpenFOAM 1.4-2.4, positions included facei and stepFraction information.
   */
  svtkSetMacro(PositionsIsIn13Format, svtkTypeBool);
  svtkGetMacro(PositionsIsIn13Format, svtkTypeBool);
  svtkBooleanMacro(PositionsIsIn13Format, svtkTypeBool);
  //@}

  //@{
  /**
   * Ignore 0/ time directory, which is normally missing Lagrangian fields
   * and may have many dictionary functionality that we cannot easily handle.
   */
  svtkSetMacro(SkipZeroTime, bool);
  svtkGetMacro(SkipZeroTime, bool);
  svtkBooleanMacro(SkipZeroTime, bool);
  //@}

  //@{
  /**
   * Determine if time directories are to be listed according to controlDict
   */
  svtkSetMacro(ListTimeStepsByControlDict, svtkTypeBool);
  svtkGetMacro(ListTimeStepsByControlDict, svtkTypeBool);
  svtkBooleanMacro(ListTimeStepsByControlDict, svtkTypeBool);
  //@}

  //@{
  /**
   * Add dimensions to array names
   */
  svtkSetMacro(AddDimensionsToArrayNames, svtkTypeBool);
  svtkGetMacro(AddDimensionsToArrayNames, svtkTypeBool);
  svtkBooleanMacro(AddDimensionsToArrayNames, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether zones will be read.
   */
  svtkSetMacro(ReadZones, svtkTypeBool);
  svtkGetMacro(ReadZones, svtkTypeBool);
  svtkBooleanMacro(ReadZones, svtkTypeBool);
  //@}

  //@{
  /**
   * If true, labels are expected to be 64-bit, rather than 32.
   */
  virtual void SetUse64BitLabels(bool val);
  svtkGetMacro(Use64BitLabels, bool);
  svtkBooleanMacro(Use64BitLabels, bool);
  //@}

  //@{
  /**
   * If true, data of the internal mesh are copied to the cell zones.
   * Default is false.
   */
  svtkGetMacro(CopyDataToCellZones, bool);
  svtkSetMacro(CopyDataToCellZones, bool);
  svtkBooleanMacro(CopyDataToCellZones, bool);
  //@}

  //@{
  /**
   * If true, floats are expected to be 64-bit, rather than 32. Note that
   * svtkFloatArrays may still be used in the output if this is true. This flag
   * is only used to ensure that binary data is correctly parsed.
   */
  virtual void SetUse64BitFloats(bool val);
  svtkGetMacro(Use64BitFloats, bool);
  svtkBooleanMacro(Use64BitFloats, bool);
  //@}

  void SetRefresh()
  {
    this->Refresh = true;
    this->Modified();
  }

  void SetParent(svtkOpenFOAMReader* parent) { this->Parent = parent; }
  int MakeInformationVector(svtkInformationVector*, const svtkStdString&);
  bool SetTimeValue(const double);
  svtkDoubleArray* GetTimeValues();
  int MakeMetaDataAtTimeStep(const bool);

  friend class svtkOpenFOAMReaderPrivate;

protected:
  // refresh flag
  bool Refresh;

  // for creating cell-to-point translated data
  svtkTypeBool CreateCellToPoint;

  // for caching mesh
  svtkTypeBool CacheMesh;

  // for decomposing polyhedra on-the-fly
  svtkTypeBool DecomposePolyhedra;

  // for lagrangian/positions without extra data (OF 1.4 - 2.4)
  svtkTypeBool PositionsIsIn13Format;

  // for reading point/face/cell-Zones
  svtkTypeBool ReadZones;

  // Ignore 0/ directory
  bool SkipZeroTime;

  // determine if time directories are listed according to controlDict
  svtkTypeBool ListTimeStepsByControlDict;

  // add dimensions to array names
  svtkTypeBool AddDimensionsToArrayNames;

  // Expect label size to be 64-bit integers instead of 32-bit.
  bool Use64BitLabels;

  // Expect float data to be 64-bit floats instead of 32-bit.
  // Note that svtkFloatArrays may still be used -- this just tells the reader how to
  // parse the binary data.
  bool Use64BitFloats;

  // The data of internal mesh are copied to cell zones
  bool CopyDataToCellZones;

  char* FileName;
  svtkCharArray* CasePath;
  svtkCollection* Readers;

  // DataArraySelection for Patch / Region Data
  svtkDataArraySelection* PatchDataArraySelection;
  svtkDataArraySelection* CellDataArraySelection;
  svtkDataArraySelection* PointDataArraySelection;
  svtkDataArraySelection* LagrangianDataArraySelection;

  // old selection status
  svtkMTimeType PatchSelectionMTimeOld;
  svtkMTimeType CellSelectionMTimeOld;
  svtkMTimeType PointSelectionMTimeOld;
  svtkMTimeType LagrangianSelectionMTimeOld;

  // preserved old information
  svtkStdString* FileNameOld;
  bool SkipZeroTimeOld;
  int ListTimeStepsByControlDictOld;
  int CreateCellToPointOld;
  int DecomposePolyhedraOld;
  int PositionsIsIn13FormatOld;
  int AddDimensionsToArrayNamesOld;
  int ReadZonesOld;
  bool Use64BitLabelsOld;
  bool Use64BitFloatsOld;

  // paths to Lagrangians
  svtkStringArray* LagrangianPaths;

  // number of reader instances
  int NumberOfReaders;
  // index of the active reader
  int CurrentReaderIndex;

  svtkOpenFOAMReader();
  ~svtkOpenFOAMReader() override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void CreateCasePath(svtkStdString&, svtkStdString&);
  void SetTimeInformation(svtkInformationVector*, svtkDoubleArray*);
  void CreateCharArrayFromString(svtkCharArray*, const char*, svtkStdString&);
  void UpdateStatus();
  void UpdateProgress(double);

private:
  svtkOpenFOAMReader* Parent;

  svtkOpenFOAMReader(const svtkOpenFOAMReader&) = delete;
  void operator=(const svtkOpenFOAMReader&) = delete;

  int GetNumberOfSelectionArrays(svtkDataArraySelection*);
  int GetSelectionArrayStatus(svtkDataArraySelection*, const char*);
  void SetSelectionArrayStatus(svtkDataArraySelection*, const char*, int);
  const char* GetSelectionArrayName(svtkDataArraySelection*, int);
  void DisableAllSelectionArrays(svtkDataArraySelection*);
  void EnableAllSelectionArrays(svtkDataArraySelection*);

  void AddSelectionNames(svtkDataArraySelection*, svtkStringArray*);
};

#endif
