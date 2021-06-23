/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkADIOS2CoreImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkADIOS2CoreImageReader
 * @brief   Read ADIOS2 bp files.
 *
 * svtkADIOS2CoreImageReader reads ADIOS2 bp data files so that they can be processed
 * or visualized using SVTK as svtkImageDatas.
 * When processing data, the assumption is that the all variables share the same number
 * of blocks. If the the data has multiple time steps, the user can specify the name of
 * the time array then reader will use it to calculate the number of time steps.
 * By default we flips the dimensions as svtk data array uses column major order whereas
 * adios2 uses row major order. Check IsColumnMajor flag for more details.
 * This reader can be launched either in serial or parallel.
 // TODO: Expose attribute info in PV GUI.
*/

#ifndef svtkADIOS2CoreImageReader_h
#define svtkADIOS2CoreImageReader_h

#include <map>    // For independently time stepped array indexing
#include <memory> // For std::unique_ptr
#include <string> // For variable name index mapping
#include <vector> // For independently time stepped array indexing

#include "svtkDataObjectAlgorithm.h"
#include "svtkMultiProcessController.h" // For the process controller
#include "svtkSetGet.h"                 // For property get/set macros
#include "svtkSmartPointer.h"           // For the object cache

#include "svtkIOADIOS2Module.h" // For export macro

class svtkCellArray;
class svtkDataArray;
class svtkDataObject;
class svtkDataSet;
class svtkDataSetAttributes;
class svtkFieldData;
class svtkImageData;
class svtkMultiBlockDataSet;
class svtkStringArray;

//----------------------------------------------------------------------------

class SVTKIOADIOS2_EXPORT svtkADIOS2CoreImageReader : public svtkDataObjectAlgorithm
{
public:
  enum class VarType
  {
    PointData,
    CellData
  };
  using Params = std::map<std::string, std::string>;
  using StringToParams = std::map<std::string, Params>;
  using InquireVariablesType = std::vector<std::pair<std::string, VarType> >;
  static svtkADIOS2CoreImageReader* New(void);
  svtkTypeMacro(svtkADIOS2CoreImageReader, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Test whether or not a given file should even be attempted for use with this
   * reader.
   */
  int CanReadFile(const std::string& name);

  virtual int CanReadFile(const char* filename);

  //@{
  /**
   * Get/Set the input filename
   */
  svtkSetMacro(FileName, std::string);
  svtkGetMacro(FileName, std::string);
  //@}

  void SetFileName(const char* filename);

  //@{
  /**
   * Get/Set the origin of output svtkImageData.
   * Default to be the origin point.
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * Get/Set the spacing of output svtkImageData
   * Default to be 1.0, 1.0, 1.0.
   */
  svtkSetVector3Macro(Spacing, double);
  svtkGetVector3Macro(Spacing, double);
  //@}

  //@{
  /**
   * Get/Set the name of the array to deduce the dimension
   * of svtkImageData. You can toggle the DimensionArrayAsCell
   * flag as needed.
   */
  svtkStringArray* GetAllDimensionArrays();
  svtkSetMacro(DimensionArray, std::string);
  svtkGetMacro(DimensionArray, std::string);
  //@}

  //@{
  /**
   * Enable/Disable the assumption that the dimension array is cell data.
   * On by default.
   */
  svtkSetMacro(DimensionArrayAsCell, bool);
  svtkGetMacro(DimensionArrayAsCell, bool);
  svtkBooleanMacro(DimensionArrayAsCell, bool);
  //@}

  svtkStringArray* GetAllTimeStepArrays();
  //@{
  /**
   * Get/Set the name of the time step array. Once it's set, svtk will try to populate the
   * time step info from this array.
   */
  svtkSetMacro(TimeStepArray, std::string);
  svtkGetMacro(TimeStepArray, std::string);
  //@}

  //@{
  /**
   * Get information about arrays. As is typical with readers this
   * is only valid after the filename is set and UpdateInformation() has been
   * called.
   * Since adios2 does not differentiate between cell arrays and point arrays,
   * the dimensions info is appended to the name so that it can used to
   *kdetermine the type of the array.
   * The array name includes its dimension.
   */
  int GetNumberOfArrays();
  const char* GetArrayName(int index);

  //@{
  /**
   * Set the array that should be read in. Based on the dimension info,
   * proper adios2 arrays will be read in as point or cell dota.
   */
  void SetArrayStatus(const char* name, int status);
  int GetArrayStatus(const char* name);
  //@}

  //@{
  /**
   * Enable/Disable the assumption that the order of input data is column major.
   * Off by default.
   * As SVTK's order is column major(Fortran order) whereas adios2 uses row major(C order),
   * we **flip the dimensions** here to avoid a deep copy.
   *
   */
  svtkSetMacro(IsColumnMajor, bool);
  svtkGetMacro(IsColumnMajor, bool);
  svtkBooleanMacro(IsColumnMajor, bool);
  //@}

  //@{
  /**
   * Get/Set the active scalar on each image block
   */
  void SetActiveScalar(const std::pair<std::string, VarType>& inqVars);
  std::pair<std::string, VarType>& GetActiveScalar();
  const std::pair<std::string, VarType>& GetActiveScalar() const;

  //@{
  /**
   * Get the available variables. Call this function after calling RequestInformation
   */
  StringToParams& GetAvilableVariables();
  const StringToParams& GetAvilableVariables() const;

  //@{
  /**
   * Get the available attributes. Call this function after calling RequestInformation
   */
  StringToParams& GetAvailableAttributes();
  const StringToParams& GetAvailableAttributes() const;

  //@{
  /**
   * Set the MPI controller
   */
  void SetController(svtkMultiProcessController*);
  //@}

  /**
   * The main interface which triggers the reader to start
   */
  virtual int ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkADIOS2CoreImageReader();
  ~svtkADIOS2CoreImageReader() override;

  int RequestDataObjectInternal(svtkInformationVector*);

  virtual int RequestInformation(
    svtkInformation* request, svtkInformationVector** input, svtkInformationVector* output) override;
  virtual int RequestData(
    svtkInformation* request, svtkInformationVector** input, svtkInformationVector* output) override;

  std::string FetchTypeStringFromVarName(const std::string& name);

  void UpdateDimensionFromDimensionArray();

  // Read available variables and attributes in the file
  bool OpenAndReadMetaData();

  // Convert the array selection into inquire variables.
  void ConvertArraySelectionToInqVar();

  // Init the workDistribution based on the first inquired variable
  bool InitWorkDistribution();

  void ReadImageBlocks(svtkMultiBlockDataSet* mbds);

  // Gather time steps info from the time step array
  bool GatherTimeSteps();

  // Helper function for InitWorkDistribution to calculate how many blocks each process shall read
  template <typename T>
  void CalculateWorkDistribution(const std::string& varName);

  // Helper function for ReadImageBlocks to populate svtk data array from adios variable
  template <typename T, template <typename...> class U>
  svtkSmartPointer<svtkAbstractArray> PopulateDataArrayFromVar(
    const std::string& varName, size_t blockIndex);

  // Helper function to gather time steps from adios time array
  template <typename T>
  void GatherTimeStepsFromADIOSTimeArray();

  std::string FileName;

  bool DimensionArrayAsCell;
  bool IsColumnMajor;

  std::string DimensionArray;
  std::string TimeStepArray;

  double Origin[3];
  double Spacing[3];
  int Dimension[3];

  double RequestTimeStep;

  svtkSmartPointer<svtkMultiProcessController> Controller;

  struct svtkADIOS2CoreImageReaderImpl;
  std::unique_ptr<svtkADIOS2CoreImageReaderImpl> Impl;

private:
  svtkADIOS2CoreImageReader(const svtkADIOS2CoreImageReader&) = delete;
  void operator=(const svtkADIOS2CoreImageReader&) = delete;
};
#endif
