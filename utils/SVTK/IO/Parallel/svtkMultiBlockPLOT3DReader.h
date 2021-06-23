/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockPLOT3DReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiBlockPLOT3DReader
 * @brief   read PLOT3D data files
 *
 * svtkMultiBlockPLOT3DReader is a reader object that reads PLOT3D formatted
 * files and generates structured grid(s) on output. PLOT3D is a computer
 * graphics program designed to visualize the grids and solutions of
 * computational fluid dynamics. This reader also supports the variant
 * of the PLOT3D format used by NASA's OVERFLOW CFD software, including
 * full support for all Q variables. Please see the "PLOT3D User's Manual"
 * available from NASA Ames Research Center, Moffett Field CA.
 *
 * PLOT3D files consist of a grid file (also known as XYZ file), an
 * optional solution file (also known as a Q file), and an optional function
 * file that contains user created data (currently unsupported). The Q file
 * contains solution information as follows: the four parameters free stream
 * mach number (Fsmach), angle of attack (Alpha), Reynolds number (Re), and
 * total integration time (Time). This information is stored in an array
 * called Properties in the FieldData of each output (tuple 0: fsmach, tuple 1:
 * alpha, tuple 2: re, tuple 3: time). In addition, the solution file contains
 * the flow density (scalar), flow momentum (vector), and flow energy (scalar).
 *
 * This reader supports a limited form of time series data which are stored
 * as a series of Q files. Using the AddFileName() method provided by the
 * superclass, one can define a file series. For other cases, for example where
 * the XYZ or function files vary over time, use svtkPlot3DMetaReader.
 *
 * The reader can generate additional scalars and vectors (or "functions")
 * from this information. To use svtkMultiBlockPLOT3DReader, you must specify the
 * particular function number for the scalar and vector you want to visualize.
 * This implementation of the reader provides the following functions. The
 * scalar functions are:
 *    -1  - don't read or compute any scalars
 *    100 - density
 *    110 - pressure
 *    111 - pressure coefficient (requires Overflow file with Gamma)
 *    112 - mach number (requires Overflow file with Gamma)
 *    113 - sounds speed (requires Overflow file with Gamma)
 *    120 - temperature
 *    130 - enthalpy
 *    140 - internal energy
 *    144 - kinetic energy
 *    153 - velocity magnitude
 *    163 - stagnation energy
 *    170 - entropy
 *    184 - swirl
 *    211 - vorticity magnitude
 *
 * The vector functions are:
 *    -1  - don't read or compute any vectors
 *    200 - velocity
 *    201 - vorticity
 *    202 - momentum
 *    210 - pressure gradient.
 *    212 - strain rate
 *
 * (Other functions are described in the PLOT3D spec, but only those listed are
 * implemented here.) Note that by default, this reader creates the density
 * scalar (100), stagnation energy (163) and momentum vector (202) as output.
 * (These are just read in from the solution file.) Please note that the validity
 * of computation is a function of this class's gas constants (R, Gamma) and the
 * equations used. They may not be suitable for your computational domain.
 *
 * Additionally, you can read other data and associate it as a svtkDataArray
 * into the output's point attribute data. Use the method AddFunction()
 * to list all the functions that you'd like to read. AddFunction() accepts
 * an integer parameter that defines the function number.
 *
 * @sa
 * svtkMultiBlockDataSet svtkStructuredGrid svtkPlot3DMetaReader
 */

#ifndef svtkMultiBlockPLOT3DReader_h
#define svtkMultiBlockPLOT3DReader_h

#include "svtkIOParallelModule.h" // For export macro
#include "svtkParallelReader.h"
#include <vector> // For holding function-names

class svtkDataArray;
class svtkDataSetAttributes;
class svtkIntArray;
class svtkMultiBlockPLOT3DReaderRecord;
class svtkMultiProcessController;
class svtkStructuredGrid;
class svtkUnsignedCharArray;
struct svtkMultiBlockPLOT3DReaderInternals;
class svtkMultiBlockDataSet;

namespace Functors
{
class ComputeFunctor;
class ComputeTemperatureFunctor;
class ComputePressureFunctor;
class ComputePressureCoefficientFunctor;
class ComputeMachNumberFunctor;
class ComputeSoundSpeedFunctor;
class ComputeEnthalpyFunctor;
class ComputeKinecticEnergyFunctor;
class ComputeVelocityMagnitudeFunctor;
class ComputeEntropyFunctor;
class ComputeSwirlFunctor;
class ComputeVelocityFunctor;
class ComputeVorticityMagnitudeFunctor;
class ComputePressureGradientFunctor;
class ComputeVorticityFunctor;
class ComputeStrainRateFunctor;
}

class SVTKIOPARALLEL_EXPORT svtkMultiBlockPLOT3DReader : public svtkParallelReader
{
  friend class Functors::ComputeFunctor;
  friend class Functors::ComputeTemperatureFunctor;
  friend class Functors::ComputePressureFunctor;
  friend class Functors::ComputePressureCoefficientFunctor;
  friend class Functors::ComputeMachNumberFunctor;
  friend class Functors::ComputeSoundSpeedFunctor;
  friend class Functors::ComputeEnthalpyFunctor;
  friend class Functors::ComputeKinecticEnergyFunctor;
  friend class Functors::ComputeVelocityMagnitudeFunctor;
  friend class Functors::ComputeEntropyFunctor;
  friend class Functors::ComputeSwirlFunctor;
  friend class Functors::ComputeVelocityFunctor;
  friend class Functors::ComputeVorticityMagnitudeFunctor;
  friend class Functors::ComputePressureGradientFunctor;
  friend class Functors::ComputeVorticityFunctor;
  friend class Functors::ComputeStrainRateFunctor;

public:
  static svtkMultiBlockPLOT3DReader* New();
  svtkTypeMacro(svtkMultiBlockPLOT3DReader, svtkParallelReader);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkMultiBlockDataSet* GetOutput();
  svtkMultiBlockDataSet* GetOutput(int);
  //@}

  //@{
  /**
   * Set/Get the PLOT3D geometry filename.
   */
  void SetFileName(const char* name) { this->SetXYZFileName(name); }
  const char* GetFileName() { return this->GetXYZFileName(); }
  const char* GetFileName(int i) { return this->svtkParallelReader::GetFileName(i); }
  virtual void SetXYZFileName(const char*);
  svtkGetStringMacro(XYZFileName);
  //@}

  //@{
  /**
   * Set/Get the PLOT3D solution filename. This adds a filename
   * using the superclass' AddFileName() method. To read a series
   * of q files, use the AddFileName() interface directly to add
   * multiple q filenames in the appropriate order. If the files
   * are of Overflow format, the reader will read the time values
   * from the files. Otherwise, it will use an integer sequence.
   * Use a meta reader to support time values for non-Overflow file
   * sequences.
   */
  void SetQFileName(const char* name);
  const char* GetQFileName();
  //@}

  //@{
  /**
   * Set/Get the PLOT3D function filename.
   */
  svtkSetStringMacro(FunctionFileName);
  svtkGetStringMacro(FunctionFileName);
  //@}

  //@{
  /**
   * When this option is turned on, the reader will try to figure
   * out the values of various options such as byte order, byte
   * count etc. automatically. This options works only for binary
   * files. When it is turned on, the reader should be able to read
   * most Plot3D files automatically. The default is OFF for backwards
   * compatibility reasons. For binary files, it is strongly recommended
   * that you turn on AutoDetectFormat and leave the other file format
   * related options untouched.
   */
  svtkSetMacro(AutoDetectFormat, svtkTypeBool);
  svtkGetMacro(AutoDetectFormat, svtkTypeBool);
  svtkBooleanMacro(AutoDetectFormat, svtkTypeBool);
  //@}

  //@{
  /**
   * Is the file to be read written in binary format (as opposed
   * to ascii).
   */
  svtkSetMacro(BinaryFile, svtkTypeBool);
  svtkGetMacro(BinaryFile, svtkTypeBool);
  svtkBooleanMacro(BinaryFile, svtkTypeBool);
  //@}

  //@{
  /**
   * Does the file to be read contain information about number of
   * grids. In some PLOT3D files, the first value contains the number
   * of grids (even if there is only 1). If reading such a file,
   * set this to true.
   */
  svtkSetMacro(MultiGrid, svtkTypeBool);
  svtkGetMacro(MultiGrid, svtkTypeBool);
  svtkBooleanMacro(MultiGrid, svtkTypeBool);
  //@}

  //@{
  /**
   * Were the arrays written with leading and trailing byte counts ?
   * Usually, files written by a fortran program will contain these
   * byte counts whereas the ones written by C/C++ won't.
   */
  svtkSetMacro(HasByteCount, svtkTypeBool);
  svtkGetMacro(HasByteCount, svtkTypeBool);
  svtkBooleanMacro(HasByteCount, svtkTypeBool);
  //@}

  //@{
  /**
   * Is there iblanking (point visibility) information in the file.
   * If there is iblanking arrays, these will be read and assigned
   * to the PointVisibility array of the output.
   */
  svtkSetMacro(IBlanking, svtkTypeBool);
  svtkGetMacro(IBlanking, svtkTypeBool);
  svtkBooleanMacro(IBlanking, svtkTypeBool);
  //@}

  //@{
  /**
   * If only two-dimensional data was written to the file,
   * turn this on.
   */
  svtkSetMacro(TwoDimensionalGeometry, svtkTypeBool);
  svtkGetMacro(TwoDimensionalGeometry, svtkTypeBool);
  svtkBooleanMacro(TwoDimensionalGeometry, svtkTypeBool);
  //@}

  //@{
  /**
   * Is this file in double precision or single precision.
   * This only matters for binary files.
   * Default is single.
   */
  svtkSetMacro(DoublePrecision, svtkTypeBool);
  svtkGetMacro(DoublePrecision, svtkTypeBool);
  svtkBooleanMacro(DoublePrecision, svtkTypeBool);
  //@}

  //@{
  /**
   * Try to read a binary file even if the file length seems to be
   * inconsistent with the header information. Use this with caution,
   * if the file length is not the same as calculated from the header.
   * either the file is corrupt or the settings are wrong.
   */
  svtkSetMacro(ForceRead, svtkTypeBool);
  svtkGetMacro(ForceRead, svtkTypeBool);
  svtkBooleanMacro(ForceRead, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the byte order of the file (remember, more Unix workstations
   * write big endian whereas PCs write little endian). Default is
   * big endian (since most older PLOT3D files were written by
   * workstations).
   */
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  svtkSetMacro(ByteOrder, int);
  svtkGetMacro(ByteOrder, int);
  const char* GetByteOrderAsString();
  //@}

  //@{
  /**
   * Set/Get the gas constant. Default is 1.0.
   */
  svtkSetMacro(R, double);
  svtkGetMacro(R, double);
  //@}

  //@{
  /**
   * Set/Get the ratio of specific heats. Default is 1.4.
   */
  svtkSetMacro(Gamma, double);
  svtkGetMacro(Gamma, double);
  //@}

  //@{
  /**
   * When set to true (default), the reader will preserve intermediate computed
   * quantities that were not explicitly requested e.g. if `VelocityMagnitude` is
   * enabled, but not `Velocity`, the reader still needs to compute `Velocity`.
   * If `PreserveIntermediateFunctions` if false, then the output will not have
   * `Velocity` array, only the requested `VelocityMagnitude`. This is useful to
   * avoid using up memory for arrays that are not relevant for the analysis.
   */
  svtkSetMacro(PreserveIntermediateFunctions, bool);
  svtkGetMacro(PreserveIntermediateFunctions, bool);
  svtkBooleanMacro(PreserveIntermediateFunctions, bool);

  //@{
  /**
   * Specify the scalar function to extract. If ==(-1), then no scalar
   * function is extracted.
   */
  void SetScalarFunctionNumber(int num);
  svtkGetMacro(ScalarFunctionNumber, int);
  //@}

  //@{
  /**
   * Specify the vector function to extract. If ==(-1), then no vector
   * function is extracted.
   */
  void SetVectorFunctionNumber(int num);
  svtkGetMacro(VectorFunctionNumber, int);
  //@}

  //@{
  /**
   * Specify additional functions to read. These are placed into the
   * point data as data arrays. Later on they can be used by labeling
   * them as scalars, etc.
   */
  void AddFunction(int functionNumber);
  void RemoveFunction(int);
  void RemoveAllFunctions();
  //@}

  /**
   * Return 1 if the reader can read the given file name. Only meaningful
   * for binary files.
   */
  virtual int CanReadBinaryFile(const char* fname);

  //@{
  /**
   * Set/Get the communicator object (we'll use global World controller
   * if you don't set a different one).
   */
  void SetController(svtkMultiProcessController* c);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  void AddFunctionName(const std::string& name) { FunctionNames.push_back(name); }

  enum
  {
    FILE_BIG_ENDIAN = 0,
    FILE_LITTLE_ENDIAN = 1
  };

  //@{
  /**
   * These methods have to be overwritten from superclass
   * because Plot3D actually uses the XYZ file to read these.
   * This is not recognized by the superclass which returns
   * an error when a filename (Q filename) is not set.
   */
  int ReadMetaData(svtkInformation* metadata) override;
  int ReadMesh(int piece, int npieces, int nghosts, int timestep, svtkDataObject* output) override;
  int ReadPoints(int piece, int npieces, int nghosts, int timestep, svtkDataObject* output) override;
  int ReadArrays(int piece, int npieces, int nghosts, int timestep, svtkDataObject* output) override;
  //@}

protected:
  svtkMultiBlockPLOT3DReader();
  ~svtkMultiBlockPLOT3DReader() override;

  //@{
  /**
   * Overridden from superclass to do actual reading.
   */
  double GetTimeValue(const std::string& fname) override;
  int ReadMesh(
    const std::string& fname, int piece, int npieces, int nghosts, svtkDataObject* output) override;
  int ReadPoints(
    const std::string& fname, int piece, int npieces, int nghosts, svtkDataObject* output) override;
  int ReadArrays(
    const std::string& fname, int piece, int npieces, int nghosts, svtkDataObject* output) override;
  //@}

  svtkDataArray* CreateFloatArray();

  int CheckFile(FILE*& fp, const char* fname);
  int CheckGeometryFile(FILE*& xyzFp);
  int CheckFunctionFile(FILE*& fFp);

  int GetByteCountSize();
  int SkipByteCount(FILE* fp);
  int ReadIntBlock(FILE* fp, int n, int* block);

  svtkIdType ReadValues(FILE* fp, int n, svtkDataArray* scalar);
  virtual int ReadIntScalar(void* vfp, int extent[6], int wextent[6], svtkDataArray* scalar,
    svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& currentRecord);
  virtual int ReadScalar(void* vfp, int extent[6], int wextent[6], svtkDataArray* scalar,
    svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& currentRecord);
  virtual int ReadVector(void* vfp, int extent[6], int wextent[6], int numDims,
    svtkDataArray* vector, svtkTypeUInt64 offset,
    const svtkMultiBlockPLOT3DReaderRecord& currentRecord);
  virtual int OpenFileForDataRead(void*& fp, const char* fname);
  virtual void CloseFile(void* fp);

  int GetNumberOfBlocksInternal(FILE* xyzFp, int allocate);

  int ReadGeometryHeader(FILE* fp);
  int ReadQHeader(FILE* fp, bool checkGrid, int& nq, int& nqc, int& overflow);
  int ReadFunctionHeader(FILE* fp, int* nFunctions);

  void CalculateFileSize(FILE* fp);

  int AutoDetectionCheck(FILE* fp);

  void AssignAttribute(int fNumber, svtkStructuredGrid* output, int attributeType);
  void MapFunction(int fNumber, svtkStructuredGrid* output);

  //@{
  /**
   * Each of these methods compute a derived quantity. On success, the array is
   * added to the output and a pointer to the same is returned.
   */
  svtkDataArray* ComputeTemperature(svtkStructuredGrid* output);
  svtkDataArray* ComputePressure(svtkStructuredGrid* output);
  svtkDataArray* ComputeEnthalpy(svtkStructuredGrid* output);
  svtkDataArray* ComputeKineticEnergy(svtkStructuredGrid* output);
  svtkDataArray* ComputeVelocityMagnitude(svtkStructuredGrid* output);
  svtkDataArray* ComputeEntropy(svtkStructuredGrid* output);
  svtkDataArray* ComputeSwirl(svtkStructuredGrid* output);
  svtkDataArray* ComputeVelocity(svtkStructuredGrid* output);
  svtkDataArray* ComputeVorticity(svtkStructuredGrid* output);
  svtkDataArray* ComputePressureGradient(svtkStructuredGrid* output);
  svtkDataArray* ComputePressureCoefficient(svtkStructuredGrid* output);
  svtkDataArray* ComputeMachNumber(svtkStructuredGrid* output);
  svtkDataArray* ComputeSoundSpeed(svtkStructuredGrid* output);
  svtkDataArray* ComputeVorticityMagnitude(svtkStructuredGrid* output);
  svtkDataArray* ComputeStrainRate(svtkStructuredGrid* output);
  //@}

  // Returns a svtkFloatArray or a svtkDoubleArray depending
  // on DoublePrecision setting
  svtkDataArray* NewFloatArray();

  // Delete references to any existing svtkPoints and
  // I-blank arrays. The next Update() will (re)read
  // the XYZ file.
  void ClearGeometryCache();

  double GetGamma(svtkIdType idx, svtkDataArray* gamma);

  int FillOutputPortInformation(int port, svtkInformation* info) override;

  // plot3d FileNames
  char* XYZFileName;
  char* QFileName;
  char* FunctionFileName;
  svtkTypeBool BinaryFile;
  svtkTypeBool HasByteCount;
  svtkTypeBool TwoDimensionalGeometry;
  svtkTypeBool MultiGrid;
  svtkTypeBool ForceRead;
  int ByteOrder;
  svtkTypeBool IBlanking;
  svtkTypeBool DoublePrecision;
  svtkTypeBool AutoDetectFormat;

  int ExecutedGhostLevels;

  size_t FileSize;

  // parameters used in computing derived functions
  double R;
  double Gamma;
  double GammaInf;

  bool PreserveIntermediateFunctions;

  // named functions from meta data
  std::vector<std::string> FunctionNames;

  // functions to read that are not scalars or vectors
  svtkIntArray* FunctionList;

  int ScalarFunctionNumber;
  int VectorFunctionNumber;

  svtkMultiBlockPLOT3DReaderInternals* Internal;

  svtkMultiProcessController* Controller;

private:
  svtkMultiBlockPLOT3DReader(const svtkMultiBlockPLOT3DReader&) = delete;
  void operator=(const svtkMultiBlockPLOT3DReader&) = delete;

  // Key used to flag intermediate results.
  static svtkInformationIntegerKey* INTERMEDIATE_RESULT();

  /**
   * Remove intermediate results
   */
  void RemoveIntermediateFunctions(svtkDataSetAttributes* dsa);
};

#endif
