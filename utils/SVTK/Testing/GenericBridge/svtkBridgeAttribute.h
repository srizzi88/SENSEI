/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBridgeAttribute.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBridgeAttribute
 * @brief   Implementation of svtkGenericAttribute.
 *
 * It is just an example that show how to implement the Generic. It is also
 * used for testing and evaluating the Generic.
 * @sa
 * svtkGenericAttribute, svtkBridgeDataSet
 */

#ifndef svtkBridgeAttribute_h
#define svtkBridgeAttribute_h

#include "svtkBridgeExport.h" //for module export macro
#include "svtkGenericAttribute.h"

class svtkPointData;
class svtkCellData;
class svtkDataSetAttributes;

class SVTKTESTINGGENERICBRIDGE_EXPORT svtkBridgeAttribute : public svtkGenericAttribute
{
public:
  static svtkBridgeAttribute* New();
  svtkTypeMacro(svtkBridgeAttribute, svtkGenericAttribute);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Name of the attribute. (e.g. "velocity")
   * \post result_may_not_exist: result!=0 || result==0
   */
  const char* GetName() override;

  /**
   * Dimension of the attribute. (1 for scalar, 3 for velocity)
   * \post positive_result: result>=0
   */
  int GetNumberOfComponents() override;

  /**
   * Is the attribute centered either on points, cells or boundaries?
   * \post valid_result: (result==svtkCenteringPoints) ||
   * (result==svtkCenteringCells) || (result==svtkCenteringBoundaries)
   */
  int GetCentering() override;

  /**
   * Type of the attribute: scalar, vector, normal, texture coordinate, tensor
   * \post valid_result: (result==svtkDataSetAttributes::SCALARS)
   * ||(result==svtkDataSetAttributes::VECTORS)
   * ||(result==svtkDataSetAttributes::NORMALS)
   * ||(result==svtkDataSetAttributes::TCOORDS)
   * ||(result==svtkDataSetAttributes::TENSORS)
   */
  int GetType() override;

  /**
   * Type of the components of the attribute: int, float, double
   * \post valid_result: (result==SVTK_BIT)           ||(result==SVTK_CHAR)
   * ||(result==SVTK_UNSIGNED_CHAR) ||(result==SVTK_SHORT)
   * ||(result==SVTK_UNSIGNED_SHORT)||(result==SVTK_INT)
   * ||(result==SVTK_UNSIGNED_INT)  ||(result==SVTK_LONG)
   * ||(result==SVTK_UNSIGNED_LONG) ||(result==SVTK_FLOAT)
   * ||(result==SVTK_DOUBLE)        ||(result==SVTK_ID_TYPE)
   */
  int GetComponentType() override;

  /**
   * Number of tuples.
   * \post valid_result: result>=0
   */
  svtkIdType GetSize() override;

  /**
   * Size in kibibytes (1024 bytes) taken by the attribute.
   */
  unsigned long GetActualMemorySize() override;

  /**
   * Range of the attribute component `component'. It returns double, even if
   * GetType()==SVTK_INT.
   * NOT THREAD SAFE
   * \pre valid_component: (component>=0)&&(component<GetNumberOfComponents())
   * \post result_exists: result!=0
   */
  double* GetRange(int component) override;

  /**
   * Range of the attribute component `component'.
   * THREAD SAFE
   * \pre valid_component: (component>=0)&&(component<GetNumberOfComponents())
   */
  void GetRange(int component, double range[2]) override;

  /**
   * Return the maximum euclidean norm for the tuples.
   * \post positive_result: result>=0
   */
  double GetMaxNorm() override;

  /**
   * Attribute at all points of cell `c'.
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \post result_exists: result!=0
   * \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
   */
  double* GetTuple(svtkGenericAdaptorCell* c) override;

  /**
   * Put attribute at all points of cell `c' in `tuple'.
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \pre tuple_exists: tuple!=0
   * \pre valid_tuple: sizeof(tuple)>=GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
   */
  void GetTuple(svtkGenericAdaptorCell* c, double* tuple) override;

  /**
   * Attribute at all points of cell `c'.
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \post result_exists: result!=0
   * \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
   */
  double* GetTuple(svtkGenericCellIterator* c) override;

  /**
   * Put attribute at all points of cell `c' in `tuple'.
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \pre tuple_exists: tuple!=0
   * \pre valid_tuple: sizeof(tuple)>=GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()
   */
  void GetTuple(svtkGenericCellIterator* c, double* tuple) override;

  /**
   * Value of the attribute at position `p'.
   * \pre p_exists: p!=0
   * \pre p_valid: !p->IsAtEnd()
   * \post result_exists: result!=0
   * \post valid_result_size: sizeof(result)==GetNumberOfComponents()
   */
  double* GetTuple(svtkGenericPointIterator* p) override;

  /**
   * Put the value of the attribute at position `p' into `tuple'.
   * \pre p_exists: p!=0
   * \pre p_valid: !p->IsAtEnd()
   * \pre tuple_exists: tuple!=0
   * \pre valid_tuple_size: sizeof(tuple)>=GetNumberOfComponents()
   */
  void GetTuple(svtkGenericPointIterator* p, double* tuple) override;

  /**
   * Put component `i' of the attribute at all points of cell `c' in `values'.
   * \pre valid_component: (i>=0) && (i<GetNumberOfComponents())
   * \pre c_exists: c!=0
   * \pre c_valid: !c->IsAtEnd()
   * \pre values_exist: values!=0
   * \pre valid_values: sizeof(values)>=c->GetCell()->GetNumberOfPoints()
   */
  void GetComponent(int i, svtkGenericCellIterator* c, double* values) override;

  /**
   * Value of the component `i' of the attribute at position `p'.
   * \pre valid_component: (i>=0) && (i<GetNumberOfComponents())
   * \pre p_exists: p!=0
   * \pre p_valid: !p->IsAtEnd()
   */
  double GetComponent(int i, svtkGenericPointIterator* p) override;

  /**
   * Recursive duplication of `other' in `this'.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   */
  void DeepCopy(svtkGenericAttribute* other) override;

  /**
   * Update `this' using fields of `other'.
   * \pre other_exists: other!=0
   * \pre not_self: other!=this
   */
  void ShallowCopy(svtkGenericAttribute* other) override;

  /**
   * Set the current attribute to be centered on points with attribute `i' of
   * `d'.
   * \pre d_exists: d!=0
   * \pre valid_range: (i>=0) && (i<d->GetNumberOfArrays())
   */
  void InitWithPointData(svtkPointData* d, int i);

  /**
   * Set the current attribute to be centered on cells with attribute `i' of
   * `d'.
   * \pre d_exists: d!=0
   * \pre valid_range: (i>=0) && (i<d->GetNumberOfArrays())
   */
  void InitWithCellData(svtkCellData* d, int i);

protected:
  /**
   * Default constructor: empty attribute, not valid
   */
  svtkBridgeAttribute();
  /**
   * Destructor.
   */
  ~svtkBridgeAttribute() override;

  /**
   * If size>InternalTupleCapacity, allocate enough memory.
   * \pre positive_size: size>0
   */
  void AllocateInternalTuple(int size);

  friend class svtkBridgeCell;

  // only one of them is non-null at a time.
  svtkPointData* Pd;
  svtkCellData* Cd;
  svtkDataSetAttributes* Data; // always not-null, equal to either on Pd or Cd
  int AttributeNumber;

  double* InternalTuple; // used by svtkBridgeCell
  int InternalTupleCapacity;

private:
  svtkBridgeAttribute(const svtkBridgeAttribute&) = delete;
  void operator=(const svtkBridgeAttribute&) = delete;
};

#endif
