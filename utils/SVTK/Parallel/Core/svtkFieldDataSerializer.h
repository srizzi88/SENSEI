/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkFieldDataSerializer.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkFieldDataSerializer
 *
 *
 *  A concrete instance of svtkObject which provides functionality for
 *  serializing and de-serializing field data, primarily used for the purpose
 *  of preparing the data for transfer over MPI or other communication
 *  mechanism.
 *
 * @sa
 * svtkFieldData svtkPointData svtkCellData svtkMultiProcessStream
 */

#ifndef svtkFieldDataSerializer_h
#define svtkFieldDataSerializer_h

#include "svtkObject.h"
#include "svtkParallelCoreModule.h" // For export macro

// Forward declarations
class svtkIdList;
class svtkFieldData;
class svtkDataArray;
class svtkStringArray;
class svtkIntArray;
class svtkMultiProcessStream;

class SVTKPARALLELCORE_EXPORT svtkFieldDataSerializer : public svtkObject
{
public:
  static svtkFieldDataSerializer* New();
  svtkTypeMacro(svtkFieldDataSerializer, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Serializes the metadata of the given field data instance, i.e., the
   * number of arrays, the name of each array and their dimensions.
   */
  static void SerializeMetaData(svtkFieldData* fieldData, svtkMultiProcessStream& bytestream);

  /**
   * Given the serialized field metadata in a bytestream, this method extracts
   * the name, datatype and dimensions of each array. The metadata is
   * deserialized on user-supplied, pre-allocated data structures.
   * (1) names -- an array of strings wherein, each element, names[i],
   * corresponds to the name of array i.
   * (2) datatypes -- an array of ints where each element corresponds
   * to the actual primitive type of each array, e.g.,SVTK_DOUBLE,SVTK_INT, etc.
   * (3) dimensions -- a 2-component array of integers where the first
   * component corresponds to the number of tuples of and the second component
   * corresponds to the number components of array i.
   */
  static void DeserializeMetaData(svtkMultiProcessStream& bytestream, svtkStringArray* names,
    svtkIntArray* datatypes, svtkIntArray* dimensions);

  /**
   * Serializes the given field data (all the field data) into a bytestream.
   */
  static void Serialize(svtkFieldData* fieldData, svtkMultiProcessStream& bytestream);

  /**
   * Serializes the selected tuples from the field data in a byte-stream.
   */
  static void SerializeTuples(
    svtkIdList* tupleIds, svtkFieldData* fieldData, svtkMultiProcessStream& bytestream);

  /**
   * Serializes the given sub-extent of field data of a structured grid
   * in a byte-stream. The field data can be either cell-centered or
   * node-centered depending on what subext and gridExtent actually
   * represents.
   */
  static void SerializeSubExtent(
    int subext[6], int gridExtent[6], svtkFieldData* fieldData, svtkMultiProcessStream& bytestream);

  /**
   * Deserializes the field data from a bytestream to a the given sub-extent.
   * The field data can be either cell-centered or node-centered depending
   * on what subext and gridExtent actually represent.
   */
  static void DeSerializeToSubExtent(
    int subext[6], int gridExtent[6], svtkFieldData* fieldData, svtkMultiProcessStream& bytestream);

  /**
   * Deserializes the field data from a bytestream.
   */
  static void Deserialize(svtkMultiProcessStream& bytestream, svtkFieldData* fieldData);

protected:
  svtkFieldDataSerializer();
  ~svtkFieldDataSerializer() override;

  /**
   * Given an input data array and list of tuples, it extracts the selected
   * tuples in to a new array and returns it.
   */
  static svtkDataArray* ExtractSelectedTuples(svtkIdList* tupleIds, svtkDataArray* inputDataArray);

  /**
   * Given an input data array corresponding to a field on a structured grid,
   * extract the data within the given extent.
   */
  static svtkDataArray* ExtractSubExtentData(
    int subext[6], int gridExtent[6], svtkDataArray* inputDataArray);

  /**
   * Serializes the data array into a bytestream.
   */
  static void SerializeDataArray(svtkDataArray* dataArray, svtkMultiProcessStream& bytestream);

  /**
   * Deserializes the data array from a bytestream
   */
  static void DeserializeDataArray(svtkMultiProcessStream& bytestream, svtkDataArray*& dataArray);

private:
  svtkFieldDataSerializer(const svtkFieldDataSerializer&) = delete;
  void operator=(const svtkFieldDataSerializer&) = delete;
};

#endif /* svtkFieldDataSerializer_h */
