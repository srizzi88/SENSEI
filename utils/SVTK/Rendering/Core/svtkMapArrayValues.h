/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMapArrayValues.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMapArrayValues
 * @brief   Map values in an input array to different values in
 *   an output array of (possibly) different type.
 *
 *
 * svtkMapArrayValues allows you to associate certain values of an attribute array
 * (on either a vertex, edge, point, or cell) with different values in a
 * newly created attribute array.
 *
 * svtkMapArrayValues manages an internal STL map of svtkVariants that can be added to
 * or cleared. When this filter executes, each "key" is searched for in the
 * input array and the indices of the output array at which there were matches
 * the set to the mapped "value".
 *
 * You can control whether the input array values are passed to the output
 * before the mapping occurs (using PassArray) or, if not, what value to set
 * the unmapped indices to (using FillValue).
 *
 * One application of this filter is to help address the dirty data problem.
 * For example, using svtkMapArrayValues you could associate the vertex values
 * "Foo, John", "Foo, John.", and "John Foo" with a single entity.
 */

#ifndef svtkMapArrayValues_h
#define svtkMapArrayValues_h

#include "svtkPassInputTypeAlgorithm.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkMapType;
class svtkVariant;

class SVTKRENDERINGCORE_EXPORT svtkMapArrayValues : public svtkPassInputTypeAlgorithm
{
public:
  svtkTypeMacro(svtkMapArrayValues, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkMapArrayValues* New();

  //@{
  /**
   * Set/Get where the data is located that is being mapped.
   * See FieldType enumeration for possible values.
   * Default is POINT_DATA.
   */
  svtkSetMacro(FieldType, int);
  svtkGetMacro(FieldType, int);
  //@}

  //@{
  /**
   * Set/Get whether to copy the data from the input array to the output array
   * before the mapping occurs. If turned off, FillValue is used to initialize
   * any unmapped array indices. Default is off.
   */
  svtkSetMacro(PassArray, svtkTypeBool);
  svtkGetMacro(PassArray, svtkTypeBool);
  svtkBooleanMacro(PassArray, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get whether to copy the data from the input array to the output array
   * before the mapping occurs. If turned off, FillValue is used to initialize
   * any unmapped array indices. Default is -1.
   */
  svtkSetMacro(FillValue, double);
  svtkGetMacro(FillValue, double);
  //@}

  //@{
  /**
   * Set/Get the name of the input array. This must be set prior to execution.
   */
  svtkSetStringMacro(InputArrayName);
  svtkGetStringMacro(InputArrayName);
  //@}

  //@{
  /**
   * Set/Get the name of the output array. Default is "ArrayMap".
   */
  svtkSetStringMacro(OutputArrayName);
  svtkGetStringMacro(OutputArrayName);
  //@}

  //@{
  /**
   * Set/Get the type of the output array. See svtkSetGet.h for possible values.
   * Default is SVTK_INT.
   */
  svtkGetMacro(OutputArrayType, int);
  svtkSetMacro(OutputArrayType, int);
  //@}

  //@{
  /**
   * Add to the internal STL map. "from" should be a value in the input array and
   * "to" should be the new value it gets assigned in the output array.
   */
  void AddToMap(svtkVariant from, svtkVariant to);
  void AddToMap(int from, int to);
  void AddToMap(int from, const char* to);
  void AddToMap(const char* from, int to);
  void AddToMap(const char* from, const char* to);
  //@}

  /**
   * Clear the internal map.
   */
  void ClearMap();

  /**
   * Get the size of the internal map.
   */
  int GetMapSize();

  // Always keep NUM_ATTRIBUTE_LOCS as the last entry
  enum FieldType
  {
    POINT_DATA = 0,
    CELL_DATA = 1,
    VERTEX_DATA = 2,
    EDGE_DATA = 3,
    ROW_DATA = 4,
    NUM_ATTRIBUTE_LOCS
  };

protected:
  svtkMapArrayValues();
  ~svtkMapArrayValues() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  char* InputArrayName;
  char* OutputArrayName;
  int OutputArrayType;
  int FieldType;
  int MapType;
  svtkTypeBool PassArray;
  double FillValue;

  // PIMPL idiom to hide map implementation.
  svtkMapType* Map;

private:
  svtkMapArrayValues(const svtkMapArrayValues&) = delete;
  void operator=(const svtkMapArrayValues&) = delete;
};

#endif
