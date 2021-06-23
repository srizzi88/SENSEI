// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNetCDFCFReader.h

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
 * @class   svtkNetCDFCFReader
 *
 *
 *
 * Reads netCDF files that follow the CF convention.  Details on this convention
 * can be found at <http://cf-pcmdi.llnl.gov/>.
 *
 */

#ifndef svtkNetCDFCFReader_h
#define svtkNetCDFCFReader_h

#include "svtkIONetCDFModule.h" // For export macro
#include "svtkNetCDFReader.h"

#include "svtkStdString.h" // Used for ivars.

class svtkImageData;
class svtkPoints;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkUnstructuredGrid;

class SVTKIONETCDF_EXPORT svtkNetCDFCFReader : public svtkNetCDFReader
{
public:
  svtkTypeMacro(svtkNetCDFCFReader, svtkNetCDFReader);
  static svtkNetCDFCFReader* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If on (the default), then 3D data with latitude/longitude dimensions
   * will be read in as curvilinear data shaped like spherical coordinates.
   * If false, then the data will always be read in Cartesian coordinates.
   */
  svtkGetMacro(SphericalCoordinates, svtkTypeBool);
  svtkSetMacro(SphericalCoordinates, svtkTypeBool);
  svtkBooleanMacro(SphericalCoordinates, svtkTypeBool);
  //@}

  //@{
  /**
   * The scale and bias of the vertical component of spherical coordinates.  It
   * is common to write the vertical component with respect to something other
   * than the center of the sphere (for example, the surface).  In this case, it
   * might be necessary to scale and/or bias the vertical height.  The height
   * will become height*scale + bias.  Keep in mind that if the positive
   * attribute of the vertical dimension is down, then the height is negated.
   * By default the scale is 1 and the bias is 0 (that is, no change).  The
   * scaling will be adjusted if it results in invalid (negative) vertical
   * values.
   */
  svtkGetMacro(VerticalScale, double);
  svtkSetMacro(VerticalScale, double);
  svtkGetMacro(VerticalBias, double);
  svtkSetMacro(VerticalBias, double);
  //@}

  //@{
  /**
   * Set/get the data type of the output.  The index used is taken from the list
   * of SVTK data types in svtkType.h.  Valid types are SVTK_IMAGE_DATA,
   * SVTK_RECTILINEAR_GRID, SVTK_STRUCTURED_GRID, and SVTK_UNSTRUCTURED_GRID.  In
   * addition you can set the type to -1 (the default), and this reader will
   * pick the data type best suited for the dimensions being read.
   */
  svtkGetMacro(OutputType, int);
  virtual void SetOutputType(int type);
  void SetOutputTypeToAutomatic() { this->SetOutputType(-1); }
  void SetOutputTypeToImage() { this->SetOutputType(SVTK_IMAGE_DATA); }
  void SetOutputTypeToRectilinear() { this->SetOutputType(SVTK_RECTILINEAR_GRID); }
  void SetOutputTypeToStructured() { this->SetOutputType(SVTK_STRUCTURED_GRID); }
  void SetOutputTypeToUnstructured() { this->SetOutputType(SVTK_UNSTRUCTURED_GRID); }
  //@}

  /**
   * Returns true if the given file can be read.
   */
  static int CanReadFile(const char* filename);

protected:
  svtkNetCDFCFReader();
  ~svtkNetCDFCFReader() override;

  svtkTypeBool SphericalCoordinates;

  double VerticalScale;
  double VerticalBias;

  int OutputType;

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //@{
  /**
   * Interprets the special conventions of COARDS.
   */
  int ReadMetaData(int ncFD) override;
  int IsTimeDimension(int ncFD, int dimId) override;
  svtkSmartPointer<svtkDoubleArray> GetTimeValues(int ncFD, int dimId) override;
  //@}

  class svtkDimensionInfo
  {
  public:
    svtkDimensionInfo() {}
    svtkDimensionInfo(int ncFD, int id);
    const char* GetName() const { return this->Name.c_str(); }
    enum UnitsEnum
    {
      UNDEFINED_UNITS,
      TIME_UNITS,
      LATITUDE_UNITS,
      LONGITUDE_UNITS,
      VERTICAL_UNITS
    };
    UnitsEnum GetUnits() const { return this->Units; }
    svtkSmartPointer<svtkDoubleArray> GetCoordinates() { return this->Coordinates; }
    svtkSmartPointer<svtkDoubleArray> GetBounds() { return this->Bounds; }
    bool GetHasRegularSpacing() const { return this->HasRegularSpacing; }
    double GetOrigin() const { return this->Origin; }
    double GetSpacing() const { return this->Spacing; }
    svtkSmartPointer<svtkStringArray> GetSpecialVariables() const { return this->SpecialVariables; }

  protected:
    svtkStdString Name;
    int DimId;
    svtkSmartPointer<svtkDoubleArray> Coordinates;
    svtkSmartPointer<svtkDoubleArray> Bounds;
    UnitsEnum Units;
    bool HasRegularSpacing;
    double Origin, Spacing;
    svtkSmartPointer<svtkStringArray> SpecialVariables;
    int LoadMetaData(int ncFD);
  };
  class svtkDimensionInfoVector;
  friend class svtkDimensionInfoVector;
  svtkDimensionInfoVector* DimensionInfo;
  svtkDimensionInfo* GetDimensionInfo(int dimension);

  class svtkDependentDimensionInfo
  {
  public:
    svtkDependentDimensionInfo()
      : Valid(false)
    {
    }
    svtkDependentDimensionInfo(int ncFD, int varId, svtkNetCDFCFReader* parent);
    bool GetValid() const { return this->Valid; }
    bool GetHasBounds() const { return this->HasBounds; }
    bool GetCellsUnstructured() const { return this->CellsUnstructured; }
    svtkSmartPointer<svtkIntArray> GetGridDimensions() const { return this->GridDimensions; }
    svtkSmartPointer<svtkDoubleArray> GetLongitudeCoordinates() const
    {
      return this->LongitudeCoordinates;
    }
    svtkSmartPointer<svtkDoubleArray> GetLatitudeCoordinates() const
    {
      return this->LatitudeCoordinates;
    }
    svtkSmartPointer<svtkStringArray> GetSpecialVariables() const { return this->SpecialVariables; }

  protected:
    bool Valid;
    bool HasBounds;
    bool CellsUnstructured;
    svtkSmartPointer<svtkIntArray> GridDimensions;
    svtkSmartPointer<svtkDoubleArray> LongitudeCoordinates;
    svtkSmartPointer<svtkDoubleArray> LatitudeCoordinates;
    svtkSmartPointer<svtkStringArray> SpecialVariables;
    int LoadMetaData(int ncFD, int varId, svtkNetCDFCFReader* parent);
    int LoadCoordinateVariable(int ncFD, int varId, svtkDoubleArray* coords);
    int LoadBoundsVariable(int ncFD, int varId, svtkDoubleArray* coords);
    int LoadUnstructuredBoundsVariable(int ncFD, int varId, svtkDoubleArray* coords);
  };
  friend class svtkDependentDimensionInfo;
  class svtkDependentDimensionInfoVector;
  friend class svtkDependentDimensionInfoVector;
  svtkDependentDimensionInfoVector* DependentDimensionInfo;

  // Finds the dependent dimension information for the given set of dimensions.
  // Returns nullptr if no information has been recorded.
  svtkDependentDimensionInfo* FindDependentDimensionInfo(svtkIntArray* dims);

  /**
   * Given the list of dimensions, identify the longitude, latitude, and
   * vertical dimensions.  -1 is returned for any not found.  The results depend
   * on the values in this->DimensionInfo.
   */
  virtual void IdentifySphericalCoordinates(
    svtkIntArray* dimensions, int& longitudeDim, int& latitudeDim, int& verticalDim);

  enum CoordinateTypesEnum
  {
    COORDS_UNIFORM_RECTILINEAR,
    COORDS_NONUNIFORM_RECTILINEAR,
    COORDS_REGULAR_SPHERICAL,
    COORDS_2D_EUCLIDEAN,
    COORDS_2D_SPHERICAL,
    COORDS_EUCLIDEAN_4SIDED_CELLS,
    COORDS_SPHERICAL_4SIDED_CELLS,
    COORDS_EUCLIDEAN_PSIDED_CELLS,
    COORDS_SPHERICAL_PSIDED_CELLS
  };

  /**
   * Based on the given dimensions and the current state of the reader, returns
   * how the coordinates should be interpreted.  The returned value is one of
   * the CoordinateTypesEnum identifiers.
   */
  CoordinateTypesEnum CoordinateType(svtkIntArray* dimensions);

  /**
   * Returns false for spherical dimensions, which should use cell data.
   */
  bool DimensionsAreForPointData(svtkIntArray* dimensions) override;

  /**
   * Convenience function that takes piece information and then returns a set of
   * extents to load based on this->WholeExtent.  The result is returned in
   * extent.
   */
  void ExtentForDimensionsAndPiece(
    int pieceNumber, int numberOfPieces, int ghostLevels, int extent[6]);

  /**
   * Overridden to retrieve stored extent for unstructured data.
   */
  void GetUpdateExtentForOutput(svtkDataSet* output, int extent[6]) override;

  //@{
  /**
   * Internal methods for setting rectilinear coordinates.
   */
  void AddRectilinearCoordinates(svtkImageData* imageOutput);
  void AddRectilinearCoordinates(svtkRectilinearGrid* rectilinearOutput);
  void FakeRectilinearCoordinates(svtkRectilinearGrid* rectilinearOutput);
  void Add1DRectilinearCoordinates(svtkPoints* points, const int extent[6]);
  void Add2DRectilinearCoordinates(svtkPoints* points, const int extent[6]);
  void Add1DRectilinearCoordinates(svtkStructuredGrid* structuredOutput);
  void Add2DRectilinearCoordinates(svtkStructuredGrid* structuredOutput);
  void FakeStructuredCoordinates(svtkStructuredGrid* structuredOutput);
  void Add1DRectilinearCoordinates(svtkUnstructuredGrid* unstructuredOutput, const int extent[6]);
  void Add2DRectilinearCoordinates(svtkUnstructuredGrid* unstructuredOutput, const int extent[6]);
  //@}

  //@{
  /**
   * Internal methods for setting spherical coordinates.
   */
  void Add1DSphericalCoordinates(svtkPoints* points, const int extent[6]);
  void Add2DSphericalCoordinates(svtkPoints* points, const int extent[6]);
  void Add1DSphericalCoordinates(svtkStructuredGrid* structuredOutput);
  void Add2DSphericalCoordinates(svtkStructuredGrid* structuredOutput);
  void Add1DSphericalCoordinates(svtkUnstructuredGrid* unstructuredOutput, const int extent[6]);
  void Add2DSphericalCoordinates(svtkUnstructuredGrid* unstructuredOutput, const int extent[6]);
  //@}

  /**
   * Internal method for building unstructred cells that match structured cells.
   */
  void AddStructuredCells(svtkUnstructuredGrid* unstructuredOutput, const int extent[6]);

  //@{
  /**
   * Internal methods for creating unstructured cells.
   */
  void AddUnstructuredRectilinearCoordinates(
    svtkUnstructuredGrid* unstructuredOutput, const int extent[6]);
  void AddUnstructuredSphericalCoordinates(
    svtkUnstructuredGrid* unstructuredOutput, const int extent[6]);
  //@}

private:
  svtkNetCDFCFReader(const svtkNetCDFCFReader&) = delete;
  void operator=(const svtkNetCDFCFReader&) = delete;
};

#endif // svtkNetCDFCFReader_h
