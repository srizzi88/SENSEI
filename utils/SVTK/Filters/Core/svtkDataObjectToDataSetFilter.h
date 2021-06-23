/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectToDataSetFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataObjectToDataSetFilter
 * @brief   map field data to concrete dataset
 *
 * svtkDataObjectToDataSetFilter is an class that maps a data object (i.e., a field)
 * into a concrete dataset, i.e., gives structure to the field by defining a
 * geometry and topology.
 *
 * To use this filter you associate components in the input field data with
 * portions of the output dataset. (A component is an array of values from
 * the field.) For example, you would specify x-y-z points by assigning
 * components from the field for the x, then y, then z values of the points.
 * You may also have to specify component ranges (for each z-y-z) to make
 * sure that the number of x, y, and z values is the same. Also, you may
 * want to normalize the components which helps distribute the data
 * uniformly. Once you've setup the filter to combine all the pieces of
 * data into a specified dataset (the geometry, topology, point and cell
 * data attributes), the various output methods (e.g., GetPolyData()) are
 * used to retrieve the final product.
 *
 * This filter is often used in conjunction with
 * svtkFieldDataToAttributeDataFilter.  svtkFieldDataToAttributeDataFilter
 * takes field data and transforms it into attribute data (e.g., point and
 * cell data attributes such as scalars and vectors).  To do this, use this
 * filter which constructs a concrete dataset and passes the input data
 * object field data to its output. and then use
 * svtkFieldDataToAttributeDataFilter to generate the attribute data associated
 * with the dataset.
 *
 * @warning
 * Make sure that the data you extract is consistent. That is, if you have N
 * points, extract N x, y, and z components. Also, all the information
 * necessary to define a dataset must be given. For example, svtkPolyData
 * requires points at a minimum; svtkStructuredPoints requires setting the
 * dimensions; svtkStructuredGrid requires defining points and dimensions;
 * svtkUnstructuredGrid requires setting points; and svtkRectilinearGrid
 * requires that you define the x, y, and z-coordinate arrays (by specifying
 * points) as well as the dimensions.
 *
 * @warning
 * If you wish to create a dataset of just points (i.e., unstructured points
 * dataset), create svtkPolyData consisting of points. There will be no cells
 * in such a dataset.
 *
 * @sa
 * svtkDataObject svtkFieldData svtkDataSet svtkPolyData svtkStructuredPoints
 * svtkStructuredGrid svtkUnstructuredGrid svtkRectilinearGrid
 * svtkDataSetAttributes svtkDataArray
 */

#ifndef svtkDataObjectToDataSetFilter_h
#define svtkDataObjectToDataSetFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class svtkCellArray;
class svtkDataArray;
class svtkDataSet;
class svtkPointSet;
class svtkPolyData;
class svtkRectilinearGrid;
class svtkStructuredGrid;
class svtkStructuredPoints;
class svtkUnstructuredGrid;

class SVTKFILTERSCORE_EXPORT svtkDataObjectToDataSetFilter : public svtkDataSetAlgorithm
{
public:
  static svtkDataObjectToDataSetFilter* New();
  svtkTypeMacro(svtkDataObjectToDataSetFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the input to the filter.
   */
  svtkDataObject* GetInput();

  //@{
  /**
   * Control what type of data is generated for output.
   */
  void SetDataSetType(int);
  svtkGetMacro(DataSetType, int);
  void SetDataSetTypeToPolyData() { this->SetDataSetType(SVTK_POLY_DATA); }
  void SetDataSetTypeToStructuredPoints() { this->SetDataSetType(SVTK_STRUCTURED_POINTS); }
  void SetDataSetTypeToStructuredGrid() { this->SetDataSetType(SVTK_STRUCTURED_GRID); }
  void SetDataSetTypeToRectilinearGrid() { this->SetDataSetType(SVTK_RECTILINEAR_GRID); }
  void SetDataSetTypeToUnstructuredGrid() { this->SetDataSetType(SVTK_UNSTRUCTURED_GRID); }
  //@}

  //@{
  /**
   * Get the output in different forms. The particular method invoked
   * should be consistent with the SetDataSetType() method. (Note:
   * GetOutput() will always return a type consistent with
   * SetDataSetType(). Also, GetOutput() will return nullptr if the filter
   * aborted due to inconsistent data.)
   */
  svtkDataSet* GetOutput();
  svtkDataSet* GetOutput(int idx);
  svtkPolyData* GetPolyDataOutput();
  svtkStructuredPoints* GetStructuredPointsOutput();
  svtkStructuredGrid* GetStructuredGridOutput();
  svtkUnstructuredGrid* GetUnstructuredGridOutput();
  svtkRectilinearGrid* GetRectilinearGridOutput();
  //@}

  //@{
  /**
   * Define the component of the field to be used for the x, y, and z values
   * of the points. Note that the parameter comp must lie between (0,2) and
   * refers to the x-y-z (i.e., 0,1,2) components of the points. To define
   * the field component to use you can specify an array name and the
   * component in that array. The (min,max) values are the range of data in
   * the component you wish to extract. (This method should be used for
   * svtkPolyData, svtkUnstructuredGrid, svtkStructuredGrid, and
   * svtkRectilinearGrid.) A convenience method, SetPointComponent(),is also
   * provided which does not require setting the (min,max) component range or
   * the normalize flag (normalize is set to DefaulatNormalize value).
   */
  void SetPointComponent(
    int comp, const char* arrayName, int arrayComp, int min, int max, int normalize);
  void SetPointComponent(int comp, const char* arrayName, int arrayComp)
  {
    this->SetPointComponent(comp, arrayName, arrayComp, -1, -1, this->DefaultNormalize);
  }
  const char* GetPointComponentArrayName(int comp);
  int GetPointComponentArrayComponent(int comp);
  int GetPointComponentMinRange(int comp);
  int GetPointComponentMaxRange(int comp);
  int GetPointComponentNormailzeFlag(int comp);
  //@}

  //@{
  /**
   * Define cell connectivity when creating svtkPolyData. You can define
   * vertices, lines, polygons, and/or triangle strips via these methods.
   * These methods are similar to those for defining points, except
   * that no normalization of the data is possible. Basically, you need to
   * define an array of values that (for each cell) includes the number of
   * points per cell, and then the cell connectivity. (This is the svtk file
   * format described in in the textbook or User's Guide.)
   */
  void SetVertsComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetVertsComponent(const char* arrayName, int arrayComp)
  {
    this->SetVertsComponent(arrayName, arrayComp, -1, -1);
  }
  const char* GetVertsComponentArrayName();
  int GetVertsComponentArrayComponent();
  int GetVertsComponentMinRange();
  int GetVertsComponentMaxRange();
  void SetLinesComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetLinesComponent(const char* arrayName, int arrayComp)
  {
    this->SetLinesComponent(arrayName, arrayComp, -1, -1);
  }
  const char* GetLinesComponentArrayName();
  int GetLinesComponentArrayComponent();
  int GetLinesComponentMinRange();
  int GetLinesComponentMaxRange();
  void SetPolysComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetPolysComponent(const char* arrayName, int arrayComp)
  {
    this->SetPolysComponent(arrayName, arrayComp, -1, -1);
  }
  const char* GetPolysComponentArrayName();
  int GetPolysComponentArrayComponent();
  int GetPolysComponentMinRange();
  int GetPolysComponentMaxRange();
  void SetStripsComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetStripsComponent(const char* arrayName, int arrayComp)
  {
    this->SetStripsComponent(arrayName, arrayComp, -1, -1);
  }
  const char* GetStripsComponentArrayName();
  int GetStripsComponentArrayComponent();
  int GetStripsComponentMinRange();
  int GetStripsComponentMaxRange();
  //@}

  //@{
  /**
   * Define cell types and cell connectivity when creating unstructured grid
   * data.  These methods are similar to those for defining points, except
   * that no normalization of the data is possible. Basically, you need to
   * define an array of cell types (an integer value per cell), and another
   * array consisting (for each cell) of a number of points per cell, and
   * then the cell connectivity. (This is the svtk file format described in
   * in the textbook or User's Guide.)
   */
  void SetCellTypeComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetCellTypeComponent(const char* arrayName, int arrayComp)
  {
    this->SetCellTypeComponent(arrayName, arrayComp, -1, -1);
  }
  const char* GetCellTypeComponentArrayName();
  int GetCellTypeComponentArrayComponent();
  int GetCellTypeComponentMinRange();
  int GetCellTypeComponentMaxRange();
  void SetCellConnectivityComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetCellConnectivityComponent(const char* arrayName, int arrayComp)
  {
    this->SetCellConnectivityComponent(arrayName, arrayComp, -1, -1);
  }
  const char* GetCellConnectivityComponentArrayName();
  int GetCellConnectivityComponentArrayComponent();
  int GetCellConnectivityComponentMinRange();
  int GetCellConnectivityComponentMaxRange();
  //@}

  //@{
  /**
   * Set the default Normalize() flag for those methods setting a default
   * Normalize value (e.g., SetPointComponent).
   */
  svtkSetMacro(DefaultNormalize, svtkTypeBool);
  svtkGetMacro(DefaultNormalize, svtkTypeBool);
  svtkBooleanMacro(DefaultNormalize, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the dimensions to use if generating a dataset that requires
   * dimensions specification (svtkStructuredPoints, svtkStructuredGrid,
   * svtkRectilinearGrid).
   */
  svtkSetVector3Macro(Dimensions, int);
  svtkGetVectorMacro(Dimensions, int, 3);
  //@}

  //@{
  /**
   * Specify the origin to use if generating a dataset whose origin
   * can be set (i.e., a svtkStructuredPoints dataset).
   */
  svtkSetVector3Macro(Origin, double);
  svtkGetVectorMacro(Origin, double, 3);
  //@}

  //@{
  /**
   * Specify the spacing to use if generating a dataset whose spacing
   * can be set (i.e., a svtkStructuredPoints dataset).
   */
  svtkSetVector3Macro(Spacing, double);
  svtkGetVectorMacro(Spacing, double, 3);
  //@}

  //@{
  /**
   * Alternative methods to specify the dimensions, spacing, and origin for those
   * datasets requiring this information. You need to specify the name of an array;
   * the component of the array, and the range of the array (min,max). These methods
   * will override the information given by the previous methods.
   */
  void SetDimensionsComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetDimensionsComponent(const char* arrayName, int arrayComp)
  {
    this->SetDimensionsComponent(arrayName, arrayComp, -1, -1);
  }
  void SetSpacingComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetSpacingComponent(const char* arrayName, int arrayComp)
  {
    this->SetSpacingComponent(arrayName, arrayComp, -1, -1);
  }
  void SetOriginComponent(const char* arrayName, int arrayComp, int min, int max);
  void SetOriginComponent(const char* arrayName, int arrayComp)
  {
    this->SetOriginComponent(arrayName, arrayComp, -1, -1);
  }
  //@}

protected:
  svtkDataObjectToDataSetFilter();
  ~svtkDataObjectToDataSetFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**,
    svtkInformationVector*) override; // generate output data
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char Updating;

  // control flags used to generate the output dataset
  int DataSetType; // the type of dataset to generate

  // Support definition of points
  char* PointArrays[3];                // the name of the arrays
  int PointArrayComponents[3];         // the array components used for x-y-z
  svtkIdType PointComponentRange[3][2]; // the range of the components to use
  int PointNormalize[3];               // flags control normalization

  // These define cells for svtkPolyData
  char* VertsArray;                 // the name of the array
  int VertsArrayComponent;          // the array component
  svtkIdType VertsComponentRange[2]; // the range of the components to use

  char* LinesArray;                 // the name of the array
  int LinesArrayComponent;          // the array component used for cell types
  svtkIdType LinesComponentRange[2]; // the range of the components to use

  char* PolysArray;                 // the name of the array
  int PolysArrayComponent;          // the array component
  svtkIdType PolysComponentRange[2]; // the range of the components to use

  char* StripsArray;                 // the name of the array
  int StripsArrayComponent;          // the array component
  svtkIdType StripsComponentRange[2]; // the range of the components to use

  // Used to define svtkUnstructuredGrid datasets
  char* CellTypeArray;                 // the name of the array
  int CellTypeArrayComponent;          // the array component used for cell types
  svtkIdType CellTypeComponentRange[2]; // the range of the components to use

  char* CellConnectivityArray;                 // the name of the array
  int CellConnectivityArrayComponent;          // the array components used for cell connectivity
  svtkIdType CellConnectivityComponentRange[2]; // the range of the components to use

  // helper methods (and attributes) to construct datasets
  void SetArrayName(char*& name, char* newName);
  svtkIdType ConstructPoints(svtkDataObject* input, svtkPointSet* ps);
  svtkIdType ConstructPoints(svtkDataObject* input, svtkRectilinearGrid* rg);
  int ConstructCells(svtkDataObject* input, svtkPolyData* pd);
  int ConstructCells(svtkDataObject* input, svtkUnstructuredGrid* ug);
  svtkCellArray* ConstructCellArray(svtkDataArray* da, int comp, svtkIdType compRange[2]);

  // Default value for normalization
  svtkTypeBool DefaultNormalize;

  // Couple of different ways to specify dimensions, spacing, and origin.
  int Dimensions[3];
  double Origin[3];
  double Spacing[3];

  char* DimensionsArray;                 // the name of the array
  int DimensionsArrayComponent;          // the component of the array used for dimensions
  svtkIdType DimensionsComponentRange[2]; // the ComponentRange of the array for the dimensions

  char* OriginArray;                 // the name of the array
  int OriginArrayComponent;          // the component of the array used for Origins
  svtkIdType OriginComponentRange[2]; // the ComponentRange of the array for the Origins

  char* SpacingArray;                 // the name of the array
  int SpacingArrayComponent;          // the component of the array used for Spacings
  svtkIdType SpacingComponentRange[2]; // the ComponentRange of the array for the Spacings

  void ConstructDimensions(svtkDataObject* input);
  void ConstructSpacing(svtkDataObject* input);
  void ConstructOrigin(svtkDataObject* input);

private:
  svtkDataObjectToDataSetFilter(const svtkDataObjectToDataSetFilter&) = delete;
  void operator=(const svtkDataObjectToDataSetFilter&) = delete;
};

#endif
