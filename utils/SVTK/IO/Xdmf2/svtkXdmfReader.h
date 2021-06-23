/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmfReader.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXdmfReader
 * @brief   Reads <tt>eXtensible Data Model and Format</tt> files
 *
 * svtkXdmfReader reads XDMF data files so that they can be visualized using
 * SVTK. The output data produced by this reader depends on the number of grids
 * in the data file. If the data file has a single domain with a single grid,
 * then the output type is a svtkDataSet subclass of the appropriate type,
 * otherwise it's a svtkMultiBlockDataSet.
 *
 * Refer to svtkDataReader which provides many methods for controlling the
 * reading of the data file.
 * @warning
 * Uses the XDMF API (http://www.xdmf.org)
 * @sa
 * svtkDataReader
 */

#ifndef svtkXdmfReader_h
#define svtkXdmfReader_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkIOXdmf2Module.h" // For export macro
#include <map>                // for caching
#include <string>             // needed for string API

class svtkXdmfArraySelection;
class svtkXdmfDocument;
class svtkGraph;
class svtkCharArray;

class SVTKIOXDMF2_EXPORT svtkXdmfReader : public svtkDataObjectAlgorithm
{
public:
  static svtkXdmfReader* New();
  svtkTypeMacro(svtkXdmfReader, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Until needed, multiple domains are not supported.
  //// Description:
  //// Returns the number of domains present in the data file. This in valid after
  //// the filename has been set and UpdateInformation() has been called .i.e. the
  //// RequestInformation pipeline pass has happened.
  // unsigned int GetNumberOfDomains();

  //@{
  /**
   * Set the active domain. Only one domain can be selected at a time. By
   * default the first domain in the datafile is chosen. Setting this to null
   * results in the domain being automatically chosen. Note that if the domain
   * name is changed, you should explicitly call UpdateInformation() before
   * accessing information about grids, data arrays etc.
   */
  svtkSetStringMacro(DomainName);
  svtkGetStringMacro(DomainName);
  //@}

  //// Description:
  //// Returns the name for the active domain. Note that this may be different
  //// from what GetDomainName() returns if DomainName is nullptr or invalid.
  // svtkGetStringMacro(ActiveDomainName);

  //@{
  /**
   * Name of the file to read.
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

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

  //@{
  /**
   * Get/Set the stride used to skip points when reading structured datasets.
   * This affects all grids being read.
   */
  svtkSetVector3Macro(Stride, int);
  svtkGetVector3Macro(Stride, int);
  //@}

  /**
   * Determine if the file can be read with this reader.
   */
  virtual int CanReadFile(const char* filename);

  //@{
  /**
   * Every time the SIL is updated a this will return a different value.
   */
  svtkGetMacro(SILUpdateStamp, int);
  //@}

  /**
   * SIL describes organization of/relationships between classifications
   * eg. blocks/materials/hierarchies.
   */
  virtual svtkGraph* GetSIL();

  class XdmfDataSetTopoGeoPath
  {
  public:
    XdmfDataSetTopoGeoPath()
      : dataset(0)
      , topologyPath()
      , geometryPath()
    {
    }
    svtkDataSet* dataset;
    std::string topologyPath;
    std::string geometryPath;
  };

  typedef std::map<int, XdmfDataSetTopoGeoPath> XdmfReaderCachedData;

  /**
   * Get the data set cache
   */
  XdmfReaderCachedData& GetDataSetCache();

  //@{
  /**
   * Enable reading from an InputString or InputArray instead of the default,
   * a file.
   */
  svtkSetMacro(ReadFromInputString, bool);
  svtkGetMacro(ReadFromInputString, bool);
  svtkBooleanMacro(ReadFromInputString, bool);
  //@}

  //@{
  /**
   * Specify the svtkCharArray to be used  when reading from a string.
   * If set, this array has precedence over InputString.
   * Use this instead of InputString to avoid the extra memory copy.
   * It should be noted that if the underlying char* is owned by the
   * user ( svtkCharArray::SetArray(array, 1); ) and is deleted before
   * the reader, bad things will happen during a pipeline update.
   */
  virtual void SetInputArray(svtkCharArray*);
  svtkGetObjectMacro(InputArray, svtkCharArray);
  //@}

  //@{
  /**
   * Specify the InputString for use when reading from a character array.
   * Optionally include the length for binary strings. Note that a copy
   * of the string is made and stored. If this causes exceedingly large
   * memory consumption, consider using InputArray instead.
   */
  void SetInputString(const char* in);
  svtkGetStringMacro(InputString);
  void SetInputString(const char* in, int len);
  svtkGetMacro(InputStringLength, int);
  void SetBinaryInputString(const char*, int len);
  void SetInputString(const std::string& input)
  {
    this->SetBinaryInputString(input.c_str(), static_cast<int>(input.length()));
  }
  //@}

protected:
  svtkXdmfReader();
  ~svtkXdmfReader() override;

  char* FileName;

  bool ReadFromInputString;

  svtkCharArray* InputArray;

  char* InputString;
  int InputStringLength;
  int InputStringPos;

  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  virtual int RequestDataObjectInternal(svtkInformationVector* outputVector);
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  svtkXdmfArraySelection* GetPointArraySelection();
  svtkXdmfArraySelection* GetCellArraySelection();
  svtkXdmfArraySelection* GetGridSelection();
  svtkXdmfArraySelection* GetSetsSelection();
  void PassCachedSelections();

  char* DomainName;
  // char* ActiveDomainName;
  int Stride[3];
  unsigned int LastTimeIndex;

  svtkXdmfDocument* XdmfDocument;

  // Until RequestInformation() is called, the active domain is not set
  // correctly. If SetGridStatus() etc. are called before that happens, then we
  // have no place to save the user choices. So we cache them in these temporary
  // caches. These are passed on to the actual svtkXdmfArraySelection instances
  // used by the active svtkXdmfDomain in RequestInformation().
  // Note that these are only used until the first domain is setup, once that
  // happens, the information set in these is passed to the domain and these
  // are cleared an no longer used, until the active domain becomes invalid
  // again.
  svtkXdmfArraySelection* PointArraysCache;
  svtkXdmfArraySelection* CellArraysCache;
  svtkXdmfArraySelection* GridsCache;
  svtkXdmfArraySelection* SetsCache;

  int SILUpdateStamp;

  XdmfReaderCachedData DataSetCache;

private:
  /**
   * Prepares the XdmfDocument.
   */
  bool PrepareDocument();

  void ClearDataSetCache();

  /**
   * Returns the time-step index requested using the UPDATE_TIME_STEPS from the
   * information.
   */
  int ChooseTimeStep(svtkInformation* outInfo);

private:
  svtkXdmfReader(const svtkXdmfReader&) = delete;
  void operator=(const svtkXdmfReader&) = delete;
};

#endif
