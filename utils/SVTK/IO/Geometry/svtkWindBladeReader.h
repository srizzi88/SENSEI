/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWindBladeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWindBladeReader
 * @brief   class for reading WindBlade data files
 *
 * svtkWindBladeReader is a source object that reads WindBlade files
 * which are block binary files with tags before and after each block
 * giving the number of bytes within the block.  The number of data
 * variables dumped varies.  There are 3 output ports with the first
 * being a structured grid with irregular spacing in the Z dimension.
 * The second is an unstructured grid only read on on process 0 and
 * used to represent the blade.  The third is also a structured grid
 * with irregular spacing on the Z dimension.  Only the first and
 * second output ports have time dependent data.
 */

#ifndef svtkWindBladeReader_h
#define svtkWindBladeReader_h

#include "svtkIOGeometryModule.h" // For export macro
#include "svtkStructuredGridAlgorithm.h"

class svtkDataArraySelection;
class svtkCallbackCommand;
class svtkStringArray;
class svtkFloatArray;
class svtkIntArray;
class svtkPoints;
class svtkStructuredGrid;
class svtkUnstructuredGrid;
class svtkMultiBlockDataSetAglorithm;
class svtkStructuredGridAlgorithm;
class WindBladeReaderInternal;

class SVTKIOGEOMETRY_EXPORT svtkWindBladeReader : public svtkStructuredGridAlgorithm
{
public:
  static svtkWindBladeReader* New();
  svtkTypeMacro(svtkWindBladeReader, svtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkSetStringMacro(Filename);
  svtkGetStringMacro(Filename);

  svtkSetVector6Macro(WholeExtent, int);
  svtkGetVector6Macro(WholeExtent, int);

  svtkSetVector6Macro(SubExtent, int);
  svtkGetVector6Macro(SubExtent, int);

  /**
   * Get the reader's output
   */
  svtkStructuredGrid* GetFieldOutput();   // Output port 0
  svtkUnstructuredGrid* GetBladeOutput(); // Output port 1
  svtkStructuredGrid* GetGroundOutput();  // Output port 2

  //@{
  /**
   * The following methods allow selective reading of solutions fields.
   * By default, ALL data fields on the nodes are read, but this can
   * be modified.
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  //@}

  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);

  void DisableAllPointArrays();
  void EnableAllPointArrays();

protected:
  static float DRY_AIR_CONSTANT;
  static int NUM_PART_SIDES;       // Blade parts rhombus
  static const int NUM_BASE_SIDES; // Base pyramid
  static const int LINE_SIZE;
  static int DIMENSION;
  static int BYTES_PER_DATA;
  static int SCALAR;
  static int VECTOR;
  static int FLOAT;
  static int INTEGER;

  svtkWindBladeReader();
  ~svtkWindBladeReader() override;

  char* Filename; // Base file name

  // Extent information
  svtkIdType NumberOfTuples; // Number of tuples in subextent

  // Field
  int WholeExtent[6]; // Extents of entire grid
  int SubExtent[6];   // Processor grid extent
  int UpdateExtent[6];
  int Dimension[3];    // Size of entire grid
  int SubDimension[3]; // Size of processor grid

  // Ground
  int GExtent[6];    // Extents of ground grid
  int GSubExtent[6]; // Processor grid extent
  int GDimension[3]; // Size of ground grid

  float Step[3];               // Spacing delta
  int UseTopographyFile;       // Topography or flat
  svtkStdString TopographyFile; // Name of topography data file
  svtkPoints* Points;           // Structured grid geometry
  svtkPoints* GPoints;          // Structured grid geometry for ground
  svtkPoints* BPoints;          // Unstructured grid geometry
  float Compression;           // Stretching at Z surface [0,1]
  float Fit;                   // Cubic or quadratic [0,1]

  // Rectilinear coordinate spacing
  svtkFloatArray* XSpacing;
  svtkFloatArray* YSpacing;
  svtkFloatArray* ZSpacing;
  float* ZTopographicValues;
  float ZMinValue;

  // Variable information
  int NumberOfFileVariables;    // Number of variables in data file
  int NumberOfDerivedVariables; // Number of variables derived from file
  int NumberOfVariables;        // Number of variables to display

  svtkStringArray* DivideVariables; // Divide data by density at read
  svtkStdString* VariableName;      // Names of each variable
  int* VariableStruct;             // SCALAR or VECTOR
  int* VariableCompSize;           // Number of components
  int* VariableBasicType;          // FLOAT or INTEGER
  int* VariableByteCount;          // Number of bytes in basic type
  long int* VariableOffset;        // Offset into data file
  size_t BlockSize;                // Size of every data block
  size_t GBlockSize;               // Size of every data block

  svtkFloatArray** Data;       // Actual data arrays
  svtkStdString RootDirectory; // Directory where the .wind file is.
  svtkStdString DataDirectory; // Location of actual data
  svtkStdString DataBaseName;  // Base name of files

  // Time step information
  int NumberOfTimeSteps; // Number of time steps
  int TimeStepFirst;     // First time step
  int TimeStepLast;      // Last time step
  int TimeStepDelta;     // Delta on time steps
  double* TimeSteps;     // Actual times available for request

  // Turbine information
  int NumberOfBladeTowers; // Number of turbines
  int NumberOfBladePoints; // Points for drawing parts of blades
  int NumberOfBladeCells;  // Turbines * Blades * Parts

  svtkFloatArray* XPosition;    // Location of tower
  svtkFloatArray* YPosition;    // Location of tower
  svtkFloatArray* HubHeight;    // Height of tower
  svtkFloatArray* AngularVeloc; // Angular Velocity
  svtkFloatArray* BladeLength;  // Blade length
  svtkIntArray* BladeCount;     // Number of blades per tower

  int UseTurbineFile;            // Turbine data available
  svtkStdString TurbineDirectory; // Turbine unstructured data
  svtkStdString TurbineTowerName; // Name of tower file
  svtkStdString TurbineBladeName; // Base name of time series blade data
  int NumberOfLinesToSkip;       // New format has lines that need to be skipped in
                                 // blade files

  // Selected field of interest
  svtkDataArraySelection* PointDataArraySelection;

  // Observer to modify this object when array selections are modified
  svtkCallbackCommand* SelectionObserver;

  // Read the header file describing the dataset
  virtual bool ReadGlobalData();
  void ReadDataVariables(istream& inStr);
  virtual bool FindVariableOffsets();

  // Turbine methods
  virtual void SetupBladeData();
  virtual void LoadBladeData(int timeStep);

  // Calculate the coordinates
  void FillCoordinates();
  void FillGroundCoordinates();
  void CreateCoordinates();
  virtual void CreateZTopography(float* zdata);
  float GDeform(float sigma, float sigmaMax, int flag);
  void Spline(float* x, float* y, int n, float yp1, float ypn, float* y2);
  void Splint(float* xa, float* ya, float* y2a, int n, float x, float* y, int);

  // Load a variable from data file
  virtual void LoadVariableData(int var);

  // Variables which must be divided by density after being read from file
  void DivideByDensity(const char* name);

  // Calculate derived variables
  virtual void CalculatePressure(int pres, int prespre, int tempg, int density);
  virtual void CalculateVorticity(int vort, int uvw, int density);

  // convenience functions shared between serial and parallel version
  void InitFieldData(
    svtkInformationVector* outVector, std::ostringstream& fileName, svtkStructuredGrid* field);
  void SetUpFieldVars(svtkStructuredGrid* field);
  void InitBladeData(svtkInformationVector* outVector);
  void SetUpGroundData(svtkInformationVector* outVector);
  void InitPressureData(int pressure, int prespre, float*& pressureData, float*& prespreData);
  void SetUpPressureData(
    float* pressureData, float* prespreData, const float* tempgData, const float* densityData);
  void SetUpVorticityData(float* uData, float* vData, const float* densityData, float* vortData);
  void InitVariableData(
    int var, int& numberOfComponents, float*& varData, int& planeSize, int& rowSize);
  bool SetUpGlobalData(const std::string& fileName, std::stringstream& inStr);
  void ProcessZCoords(float* topoData, float* zValues);
  void ReadBladeHeader(const std::string& fileName, std::stringstream& inStr, int& numColumns);
  void ReadBladeData(std::stringstream& inStr);

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  static void SelectionCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  static void EventCallback(svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  int FillOutputPortInformation(int, svtkInformation*) override;

  /**
   * We intercept the requests to check for which port
   * information is being requested for and if there is
   * a REQUEST_DATA_NOT_GENERATED request then we mark
   * which ports won't have data generated for that request.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

private:
  WindBladeReaderInternal* Internal;

  svtkWindBladeReader(const svtkWindBladeReader&) = delete;
  void operator=(const svtkWindBladeReader&) = delete;
};
#endif
