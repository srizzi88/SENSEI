// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSLACReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   svtkSLACReader
 *
 *
 *
 * A reader for a data format used by Omega3p, Tau3p, and several other tools
 * used at the Standford Linear Accelerator Center (SLAC).  The underlying
 * format uses netCDF to store arrays, but also imposes several conventions
 * to form an unstructured grid of elements.
 *
 */

#ifndef svtkSLACReader_h
#define svtkSLACReader_h

#include "svtkIONetCDFModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

#include "svtkSmartPointer.h" // For internal method.

class svtkDataArraySelection;
class svtkDoubleArray;
class svtkIdTypeArray;
class svtkInformationIntegerKey;
class svtkInformationObjectBaseKey;

class SVTKIONETCDF_EXPORT svtkSLACReader : public svtkMultiBlockDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkSLACReader, svtkMultiBlockDataSetAlgorithm);
  static svtkSLACReader* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkGetStringMacro(MeshFileName);
  svtkSetStringMacro(MeshFileName);

  //@{
  /**
   * There may be one mode file (usually for actual modes) or multiple mode
   * files (which usually actually represent time series).  These methods
   * set and clear the list of mode files (which can be a single mode file).
   */
  virtual void AddModeFileName(const char* fname);
  virtual void RemoveAllModeFileNames();
  virtual unsigned int GetNumberOfModeFileNames();
  virtual const char* GetModeFileName(unsigned int idx);
  //@}

  //@{
  /**
   * If on, reads the internal volume of the data set.  Set to off by default.
   */
  svtkGetMacro(ReadInternalVolume, svtkTypeBool);
  svtkSetMacro(ReadInternalVolume, svtkTypeBool);
  svtkBooleanMacro(ReadInternalVolume, svtkTypeBool);
  //@}

  //@{
  /**
   * If on, reads the external surfaces of the data set.  Set to on by default.
   */
  svtkGetMacro(ReadExternalSurface, svtkTypeBool);
  svtkSetMacro(ReadExternalSurface, svtkTypeBool);
  svtkBooleanMacro(ReadExternalSurface, svtkTypeBool);
  //@}

  //@{
  /**
   * If on, reads midpoint information for external surfaces and builds
   * quadratic surface triangles.  Set to on by default.
   */
  svtkGetMacro(ReadMidpoints, svtkTypeBool);
  svtkSetMacro(ReadMidpoints, svtkTypeBool);
  svtkBooleanMacro(ReadMidpoints, svtkTypeBool);
  //@}

  //@{
  /**
   * Variable array selection.
   */
  virtual int GetNumberOfVariableArrays();
  virtual const char* GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char* name);
  virtual void SetVariableArrayStatus(const char* name, int status);
  //@}

  //@{
  /**
   * Sets the scale factor for each mode. Each scale factor is reset to 1.
   */
  virtual void ResetFrequencyScales();
  virtual void SetFrequencyScale(int index, double scale);
  //@}

  //@{
  /**
   * Sets the phase offset for each mode. Each shift is reset to 0.
   */
  virtual void ResetPhaseShifts();
  virtual void SetPhaseShift(int index, double shift);
  //@}

  //@{
  /**
   * NOTE: This is not thread-safe.
   */
  virtual svtkDoubleArray* GetFrequencyScales();
  virtual svtkDoubleArray* GetPhaseShifts();
  //@}

  /**
   * Returns true if the given file can be read by this reader.
   */
  static int CanReadFile(const char* filename);

  /**
   * This key is attached to the metadata information of all data sets in the
   * output that are part of the internal volume.
   */
  static svtkInformationIntegerKey* IS_INTERNAL_VOLUME();

  /**
   * This key is attached to the metadata information of all data sets in the
   * output that are part of the external surface.
   */
  static svtkInformationIntegerKey* IS_EXTERNAL_SURFACE();

  //@{
  /**
   * All the data sets stored in the multiblock output share the same point
   * data.  For convenience, the point coordinates (svtkPoints) and point data
   * (svtkPointData) are saved under these keys in the svtkInformation of the
   * output data set.
   */
  static svtkInformationObjectBaseKey* POINTS();
  static svtkInformationObjectBaseKey* POINT_DATA();
  //@}

  //@{
  /**
   * Simple class used internally to define an edge based on the endpoints.  The
   * endpoints are canonically identified by the lower and higher values.
   */
  class SVTKIONETCDF_EXPORT EdgeEndpoints
  {
  public:
    EdgeEndpoints()
      : MinEndPoint(-1)
      , MaxEndPoint(-1)
    {
    }
    EdgeEndpoints(svtkIdType endpointA, svtkIdType endpointB)
    {
      if (endpointA < endpointB)
      {
        this->MinEndPoint = endpointA;
        this->MaxEndPoint = endpointB;
      }
      else
      {
        this->MinEndPoint = endpointB;
        this->MaxEndPoint = endpointA;
      }
    }
    inline svtkIdType GetMinEndPoint() const { return this->MinEndPoint; }
    inline svtkIdType GetMaxEndPoint() const { return this->MaxEndPoint; }
    inline bool operator==(const EdgeEndpoints& other) const
    {
      return ((this->GetMinEndPoint() == other.GetMinEndPoint()) &&
        (this->GetMaxEndPoint() == other.GetMaxEndPoint()));
    }

  protected:
    svtkIdType MinEndPoint;
    svtkIdType MaxEndPoint;
  };
  //@}

  //@{
  /**
   * Simple class used internally for holding midpoint information.
   */
  class SVTKIONETCDF_EXPORT MidpointCoordinates
  {
  public:
    MidpointCoordinates() {}
    MidpointCoordinates(const double coord[3], svtkIdType id)
    {
      this->Coordinate[0] = coord[0];
      this->Coordinate[1] = coord[1];
      this->Coordinate[2] = coord[2];
      this->ID = id;
    }
    double Coordinate[3];
    svtkIdType ID;
  };
  //@}

  enum
  {
    SURFACE_OUTPUT = 0,
    VOLUME_OUTPUT = 1,
    NUM_OUTPUTS = 2
  };

protected:
  svtkSLACReader();
  ~svtkSLACReader() override;

  class svtkInternal;
  svtkInternal* Internal;

  // Friend so svtkInternal can access MidpointIdMap
  // (so Sun CC compiler doesn't complain).
  friend class svtkInternal;

  char* MeshFileName;

  svtkTypeBool ReadInternalVolume;
  svtkTypeBool ReadExternalSurface;
  svtkTypeBool ReadMidpoints;

  /**
   * True if reading from a proper mode file.  Set in RequestInformation.
   */
  bool ReadModeData;

  /**
   * True if "mode" files are a sequence of time steps.
   */
  bool TimeStepModes;

  /**
   * True if mode files describe vibrating fields.
   */
  bool FrequencyModes;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Callback registered with the VariableArraySelection.
   */
  static void SelectionModifiedCallback(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  /**
   * Convenience function that checks the dimensions of a 2D netCDF array that
   * is supposed to be a set of tuples.  It makes sure that the number of
   * dimensions is expected and that the number of components in each tuple
   * agree with what is expected.  It then returns the number of tuples.  An
   * error is emitted and 0 is returned if the checks fail.
   */
  virtual svtkIdType GetNumTuplesInVariable(int ncFD, int varId, int expectedNumComponents);

  /**
   * Checks the winding of the tetrahedra in the mesh file.  Returns 1 if
   * the winding conforms to SVTK, 0 if the winding needs to be corrected.
   */
  virtual int CheckTetrahedraWinding(int meshFD);

  /**
   * Read the connectivity information from the mesh file.  Returns 1 on
   * success, 0 on failure.
   */
  virtual int ReadConnectivity(
    int meshFD, svtkMultiBlockDataSet* surfaceOutput, svtkMultiBlockDataSet* volumeOutput);

  //@{
  /**
   * Reads tetrahedron connectivity arrays.  Called by ReadConnectivity.
   */
  virtual int ReadTetrahedronInteriorArray(int meshFD, svtkIdTypeArray* connectivity);
  virtual int ReadTetrahedronExteriorArray(int meshFD, svtkIdTypeArray* connectivity);
  //@}

  /**
   * Reads point data arrays.  Called by ReadCoordinates and ReadFieldData.
   */
  virtual svtkSmartPointer<svtkDataArray> ReadPointDataArray(int ncFD, int varId);

  /**
   * Helpful constants equal to the amount of identifiers per tet.
   */
  enum
  {
    NumPerTetInt = 5,
    NumPerTetExt = 9
  };

  //@{
  /**
   * Manages a map from edges to midpoint coordinates.
   */
  class SVTKIONETCDF_EXPORT MidpointCoordinateMap
  {
  public:
    MidpointCoordinateMap();
    ~MidpointCoordinateMap();
    //@}

    void AddMidpoint(const EdgeEndpoints& edge, const MidpointCoordinates& midpoint);
    void RemoveMidpoint(const EdgeEndpoints& edge);
    void RemoveAllMidpoints();
    svtkIdType GetNumberOfMidpoints() const;

    /**
     * Finds the coordinates for the given edge or returns nullptr if it
     * does not exist.
     */
    MidpointCoordinates* FindMidpoint(const EdgeEndpoints& edge);

  protected:
    class svtkInternal;
    svtkInternal* Internal;

  private:
    // Too lazy to implement these.
    MidpointCoordinateMap(const MidpointCoordinateMap&);
    void operator=(const MidpointCoordinateMap&);
  };

  //@{
  /**
   * Manages a map from edges to the point id of the midpoint.
   */
  class SVTKIONETCDF_EXPORT MidpointIdMap
  {
  public:
    MidpointIdMap();
    ~MidpointIdMap();
    //@}

    void AddMidpoint(const EdgeEndpoints& edge, svtkIdType midpoint);
    void RemoveMidpoint(const EdgeEndpoints& edge);
    void RemoveAllMidpoints();
    svtkIdType GetNumberOfMidpoints() const;

    /**
     * Finds the id for the given edge or returns nullptr if it does not exist.
     */
    svtkIdType* FindMidpoint(const EdgeEndpoints& edge);

    /**
     * Initialize iteration.  The iteration can occur in any order.
     */
    void InitTraversal();
    /**
     * Get the next midpoint in the iteration.  Return 0 if the end is reached.
     */
    bool GetNextMidpoint(EdgeEndpoints& edge, svtkIdType& midpoint);

  protected:
    class svtkInternal;
    svtkInternal* Internal;

  private:
    // Too lazy to implement these.
    MidpointIdMap(const MidpointIdMap&);
    void operator=(const MidpointIdMap&);
  };

  /**
   * Read in the point coordinate data from the mesh file.  Returns 1 on
   * success, 0 on failure.
   */
  virtual int ReadCoordinates(int meshFD, svtkMultiBlockDataSet* output);

  /**
   * Reads in the midpoint coordinate data from the mesh file and returns a map
   * from edges to midpoints.  This method is called by ReadMidpointData.
   * Returns 1 on success, 0 on failure.
   */
  virtual int ReadMidpointCoordinates(
    int meshFD, svtkMultiBlockDataSet* output, MidpointCoordinateMap& map);

  /**
   * Read in the midpoint data from the mesh file.  Returns 1 on success,
   * 0 on failure.  Also fills a midpoint id map that will be passed into
   * InterpolateMidpointFieldData.
   */
  virtual int ReadMidpointData(int meshFD, svtkMultiBlockDataSet* output, MidpointIdMap& map);

  /**
   * Instead of reading data from the mesh file, restore the data from the
   * previous mesh file read.
   */
  virtual int RestoreMeshCache(svtkMultiBlockDataSet* surfaceOutput,
    svtkMultiBlockDataSet* volumeOutput, svtkMultiBlockDataSet* compositeOutput);

  /**
   * Read in the field data from the mode file.  Returns 1 on success, 0
   * on failure.
   */
  virtual int ReadFieldData(const int* modeFDArray, int numModeFDs, svtkMultiBlockDataSet* output);

  /**
   * Takes the data read on the fields and interpolates data for the midpoints.
   * map is the same map that was created in ReadMidpointData.
   */
  virtual int InterpolateMidpointData(svtkMultiBlockDataSet* output, MidpointIdMap& map);

  /**
   * A time stamp for the last time the mesh file was read.  This is used to
   * determine whether the mesh needs to be read in again or if we just need
   * to read the mode data.
   */
  svtkTimeStamp MeshReadTime;

  /**
   * Returns 1 if the mesh is up to date, 0 if the mesh needs to be read from
   * disk.
   */
  virtual int MeshUpToDate();

private:
  svtkSLACReader(const svtkSLACReader&) = delete;
  void operator=(const svtkSLACReader&) = delete;
};

#endif // svtkSLACReader_h
