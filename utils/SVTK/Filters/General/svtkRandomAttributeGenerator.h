/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRandomAttributeGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRandomAttributeGenerator
 * @brief   generate and create random data attributes
 *
 * svtkRandomAttributeGenerator is a filter that creates random attributes
 * including scalars, vectors, normals, tensors, texture coordinates and/or
 * general data arrays. These attributes can be generated as point data, cell
 * data or general field data. The generation of each component is normalized
 * between a user-specified minimum and maximum value.
 *
 * This filter provides that capability to specify the data type of the
 * attributes, the range for each of the components, and the number of
 * components. Note, however, that this flexibility only goes so far because
 * some attributes (e.g., normals, vectors and tensors) are fixed in the
 * number of components, and in the case of normals and tensors, are
 * constrained in the values that some of the components can take (i.e.,
 * normals have magnitude one, and tensors are symmetric).
 *
 * @warning
 * In general this class is used for debugging or testing purposes.
 *
 * @warning
 * It is possible to generate multiple attributes simultaneously.
 *
 * @warning
 * By default, no data is generated. Make sure to enable the generation of
 * some attributes if you want this filter to affect the output. Also note
 * that this filter passes through input geometry, topology and attributes.
 * Newly created attributes may replace attribute data that would have
 * otherwise been passed through.
 *
 * @sa
 * svtkBrownianPoints
 */

#ifndef svtkRandomAttributeGenerator_h
#define svtkRandomAttributeGenerator_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class svtkDataSet;
class svtkCompositeDataSet;

class SVTKFILTERSGENERAL_EXPORT svtkRandomAttributeGenerator : public svtkPassInputTypeAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  static svtkRandomAttributeGenerator* New();
  svtkTypeMacro(svtkRandomAttributeGenerator, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the type of array to create (all components of this array are of this
   * type). This holds true for all arrays that are created.
   */
  svtkSetMacro(DataType, int);
  void SetDataTypeToBit() { this->SetDataType(SVTK_BIT); }
  void SetDataTypeToChar() { this->SetDataType(SVTK_CHAR); }
  void SetDataTypeToUnsignedChar() { this->SetDataType(SVTK_UNSIGNED_CHAR); }
  void SetDataTypeToShort() { this->SetDataType(SVTK_SHORT); }
  void SetDataTypeToUnsignedShort() { this->SetDataType(SVTK_UNSIGNED_SHORT); }
  void SetDataTypeToInt() { this->SetDataType(SVTK_INT); }
  void SetDataTypeToUnsignedInt() { this->SetDataType(SVTK_UNSIGNED_INT); }
  void SetDataTypeToLong() { this->SetDataType(SVTK_LONG); }
  void SetDataTypeToLongLong() { this->SetDataType(SVTK_LONG_LONG); }
  void SetDataTypeToUnsignedLong() { this->SetDataType(SVTK_UNSIGNED_LONG); }
  void SetDataTypeToUnsignedLongLong() { this->SetDataType(SVTK_UNSIGNED_LONG_LONG); }
  void SetDataTypeToIdType() { this->SetDataType(SVTK_ID_TYPE); }
  void SetDataTypeToFloat() { this->SetDataType(SVTK_FLOAT); }
  void SetDataTypeToDouble() { this->SetDataType(SVTK_DOUBLE); }
  svtkGetMacro(DataType, int);
  //@}

  //@{
  /**
   * Specify the number of components to generate. This value only applies to those
   * attribute types that take a variable number of components. For example, a vector
   * is only three components so the number of components is not applicable; whereas
   * a scalar may support multiple, varying number of components.
   */
  svtkSetClampMacro(NumberOfComponents, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfComponents, int);
  //@}

  //@{
  /**
   * Set the minimum component value. This applies to all data that is generated,
   * although normals and tensors have internal constraints that must be
   * observed.
   */
  svtkSetMacro(MinimumComponentValue, double);
  svtkGetMacro(MinimumComponentValue, double);
  void SetComponentRange(double minimumValue, double maximumValue)
  {
    this->SetMinimumComponentValue(minimumValue);
    this->SetMaximumComponentValue(maximumValue);
  }
  //@}

  //@{
  /**
   * Set the maximum component value. This applies to all data that is generated,
   * although normals and tensors have internal constraints that must be
   * observed.
   */
  svtkSetMacro(MaximumComponentValue, double);
  svtkGetMacro(MaximumComponentValue, double);
  //@}

  //@{
  /**
   * Specify the number of tuples to generate. This value only applies when creating
   * general field data. In all other cases (i.e., point data or cell data), the number
   * of tuples is controlled by the number of points and cells, respectively.
   */
  svtkSetClampMacro(NumberOfTuples, svtkIdType, 0, SVTK_INT_MAX);
  svtkGetMacro(NumberOfTuples, svtkIdType);
  //@}

  //@{
  /**
   * Indicate that point scalars are to be generated. Note that the specified
   * number of components is used to create the scalar.
   */
  svtkSetMacro(GeneratePointScalars, svtkTypeBool);
  svtkGetMacro(GeneratePointScalars, svtkTypeBool);
  svtkBooleanMacro(GeneratePointScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that point vectors are to be generated. Note that the
   * number of components is always equal to three.
   */
  svtkSetMacro(GeneratePointVectors, svtkTypeBool);
  svtkGetMacro(GeneratePointVectors, svtkTypeBool);
  svtkBooleanMacro(GeneratePointVectors, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that point normals are to be generated. Note that the
   * number of components is always equal to three.
   */
  svtkSetMacro(GeneratePointNormals, svtkTypeBool);
  svtkGetMacro(GeneratePointNormals, svtkTypeBool);
  svtkBooleanMacro(GeneratePointNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that point tensors are to be generated. Note that the
   * number of components is always equal to nine.
   */
  svtkSetMacro(GeneratePointTensors, svtkTypeBool);
  svtkGetMacro(GeneratePointTensors, svtkTypeBool);
  svtkBooleanMacro(GeneratePointTensors, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that point texture coordinates are to be generated. Note that
   * the specified number of components is used to create the texture
   * coordinates (but must range between one and three).
   */
  svtkSetMacro(GeneratePointTCoords, svtkTypeBool);
  svtkGetMacro(GeneratePointTCoords, svtkTypeBool);
  svtkBooleanMacro(GeneratePointTCoords, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that an arbitrary point array is to be generated. The array is
   * added to the points data but is not labeled as one of scalars, vectors,
   * normals, tensors, or texture coordinates (i.e., AddArray() is
   * used). Note that the specified number of components is used to create
   * the array.
   */
  svtkSetMacro(GeneratePointArray, svtkTypeBool);
  svtkGetMacro(GeneratePointArray, svtkTypeBool);
  svtkBooleanMacro(GeneratePointArray, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell scalars are to be generated. Note that the specified
   * number of components is used to create the scalar.
   */
  svtkSetMacro(GenerateCellScalars, svtkTypeBool);
  svtkGetMacro(GenerateCellScalars, svtkTypeBool);
  svtkBooleanMacro(GenerateCellScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell vectors are to be generated. Note that the
   * number of components is always equal to three.
   */
  svtkSetMacro(GenerateCellVectors, svtkTypeBool);
  svtkGetMacro(GenerateCellVectors, svtkTypeBool);
  svtkBooleanMacro(GenerateCellVectors, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell normals are to be generated. Note that the
   * number of components is always equal to three.
   */
  svtkSetMacro(GenerateCellNormals, svtkTypeBool);
  svtkGetMacro(GenerateCellNormals, svtkTypeBool);
  svtkBooleanMacro(GenerateCellNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell tensors are to be generated. Note that the
   * number of components is always equal to nine.
   */
  svtkSetMacro(GenerateCellTensors, svtkTypeBool);
  svtkGetMacro(GenerateCellTensors, svtkTypeBool);
  svtkBooleanMacro(GenerateCellTensors, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that cell texture coordinates are to be generated. Note that
   * the specified number of components is used to create the texture
   * coordinates (but must range between one and three).
   */
  svtkSetMacro(GenerateCellTCoords, svtkTypeBool);
  svtkGetMacro(GenerateCellTCoords, svtkTypeBool);
  svtkBooleanMacro(GenerateCellTCoords, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that an arbitrary cell array is to be generated. The array is
   * added to the cell data but is not labeled as one of scalars, vectors,
   * normals, tensors, or texture coordinates array (i.e., AddArray() is
   * used). Note that the specified number of components is used to create
   * the array.
   */
  svtkSetMacro(GenerateCellArray, svtkTypeBool);
  svtkGetMacro(GenerateCellArray, svtkTypeBool);
  svtkBooleanMacro(GenerateCellArray, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that an arbitrary field data array is to be generated. Note
   * that the specified number of components is used to create the scalar.
   */
  svtkSetMacro(GenerateFieldArray, svtkTypeBool);
  svtkGetMacro(GenerateFieldArray, svtkTypeBool);
  svtkBooleanMacro(GenerateFieldArray, svtkTypeBool);
  //@}

  //@{
  /**
   * Indicate that the generated attributes are
   * constant within a block. This can be used to highlight
   * blocks in a composite dataset.
   */
  svtkSetMacro(AttributesConstantPerBlock, bool);
  svtkGetMacro(AttributesConstantPerBlock, bool);
  svtkBooleanMacro(AttributesConstantPerBlock, bool);
  //@}

  //@{
  /**
   * Convenience methods for generating data: all data, all point data, or all cell data.
   * For example, if all data is enabled, then all point, cell and field data is generated.
   * If all point data is enabled, then point scalars, vectors, normals, tensors, tcoords,
   * and a data array are produced.
   */
  void GenerateAllPointDataOn()
  {
    this->GeneratePointScalarsOn();
    this->GeneratePointVectorsOn();
    this->GeneratePointNormalsOn();
    this->GeneratePointTCoordsOn();
    this->GeneratePointTensorsOn();
    this->GeneratePointArrayOn();
  }
  void GenerateAllPointDataOff()
  {
    this->GeneratePointScalarsOff();
    this->GeneratePointVectorsOff();
    this->GeneratePointNormalsOff();
    this->GeneratePointTCoordsOff();
    this->GeneratePointTensorsOff();
    this->GeneratePointArrayOff();
  }
  void GenerateAllCellDataOn()
  {
    this->GenerateCellScalarsOn();
    this->GenerateCellVectorsOn();
    this->GenerateCellNormalsOn();
    this->GenerateCellTCoordsOn();
    this->GenerateCellTensorsOn();
    this->GenerateCellArrayOn();
  }
  void GenerateAllCellDataOff()
  {
    this->GenerateCellScalarsOff();
    this->GenerateCellVectorsOff();
    this->GenerateCellNormalsOff();
    this->GenerateCellTCoordsOff();
    this->GenerateCellTensorsOff();
    this->GenerateCellArrayOff();
  }
  void GenerateAllDataOn()
  {
    this->GenerateAllPointDataOn();
    this->GenerateAllCellDataOn();
    this->GenerateFieldArrayOn();
  }
  void GenerateAllDataOff()
  {
    this->GenerateAllPointDataOff();
    this->GenerateAllCellDataOff();
    this->GenerateFieldArrayOff();
  }
  //@}

protected:
  svtkRandomAttributeGenerator();
  ~svtkRandomAttributeGenerator() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int DataType;
  int NumberOfComponents;
  svtkIdType NumberOfTuples;
  double MinimumComponentValue;
  double MaximumComponentValue;

  svtkTypeBool GeneratePointScalars;
  svtkTypeBool GeneratePointVectors;
  svtkTypeBool GeneratePointNormals;
  svtkTypeBool GeneratePointTCoords;
  svtkTypeBool GeneratePointTensors;
  svtkTypeBool GeneratePointArray;

  svtkTypeBool GenerateCellScalars;
  svtkTypeBool GenerateCellVectors;
  svtkTypeBool GenerateCellNormals;
  svtkTypeBool GenerateCellTCoords;
  svtkTypeBool GenerateCellTensors;
  svtkTypeBool GenerateCellArray;

  svtkTypeBool GenerateFieldArray;
  bool AttributesConstantPerBlock;

  // Helper functions
  svtkDataArray* GenerateData(int dataType, svtkIdType numTuples, int numComp, int minComp,
    int maxComp, double min, double max);
  int RequestData(svtkDataSet* input, svtkDataSet* output);
  int RequestData(svtkCompositeDataSet* input, svtkCompositeDataSet* output);
  template <class T>
  void GenerateRandomTuples(
    T* data, svtkIdType numTuples, int numComp, int minComp, int maxComp, double min, double max);

private:
  svtkRandomAttributeGenerator(const svtkRandomAttributeGenerator&) = delete;
  void operator=(const svtkRandomAttributeGenerator&) = delete;
};

#endif
