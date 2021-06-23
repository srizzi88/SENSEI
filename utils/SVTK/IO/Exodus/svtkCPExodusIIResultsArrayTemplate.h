/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIIResultsArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkCPExodusIIResultsArrayTemplate
 * @brief   Map native Exodus II results arrays
 * into the svtkDataArray interface.
 *
 *
 * Map native Exodus II results arrays into the svtkDataArray interface. Use
 * the svtkCPExodusIIInSituReader to read an Exodus II file's data into this
 * structure.
 */

#ifndef svtkCPExodusIIResultsArrayTemplate_h
#define svtkCPExodusIIResultsArrayTemplate_h

#include "svtkMappedDataArray.h"

#include "svtkObjectFactory.h" // for svtkStandardNewMacro

template <class Scalar>
class svtkCPExodusIIResultsArrayTemplate : public svtkMappedDataArray<Scalar>
{
public:
  svtkAbstractTemplateTypeMacro(
    svtkCPExodusIIResultsArrayTemplate<Scalar>, svtkMappedDataArray<Scalar>)
    svtkMappedDataArrayNewInstanceMacro(
      svtkCPExodusIIResultsArrayTemplate<Scalar>) static svtkCPExodusIIResultsArrayTemplate* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  typedef typename Superclass::ValueType ValueType;

  //@{
  /**
   * Set the arrays to be used and the number of tuples in each array.
   * The save option can be set to true to indicate that this class
   * should not delete the actual allocated memory. By default it will
   * delete the array with the 'delete []' method.
   */
  void SetExodusScalarArrays(std::vector<Scalar*> arrays, svtkIdType numTuples);
  void SetExodusScalarArrays(std::vector<Scalar*> arrays, svtkIdType numTuples, bool save);
  //@}

  // Reimplemented virtuals -- see superclasses for descriptions:
  void Initialize() override;
  void GetTuples(svtkIdList* ptIds, svtkAbstractArray* output) override;
  void GetTuples(svtkIdType p1, svtkIdType p2, svtkAbstractArray* output) override;
  void Squeeze() override;
  SVTK_NEWINSTANCE svtkArrayIterator* NewIterator() override;
  svtkIdType LookupValue(svtkVariant value) override;
  void LookupValue(svtkVariant value, svtkIdList* ids) override;
  svtkVariant GetVariantValue(svtkIdType idx) override;
  void ClearLookup() override;
  double* GetTuple(svtkIdType i) override;
  void GetTuple(svtkIdType i, double* tuple) override;
  svtkIdType LookupTypedValue(Scalar value) override;
  void LookupTypedValue(Scalar value, svtkIdList* ids) override;
  ValueType GetValue(svtkIdType idx) const override;
  ValueType& GetValueReference(svtkIdType idx) override;
  void GetTypedTuple(svtkIdType idx, Scalar* t) const override;

  //@{
  /**
   * This container is read only -- this method does nothing but print a
   * warning.
   */
  svtkTypeBool Allocate(svtkIdType sz, svtkIdType ext) override;
  svtkTypeBool Resize(svtkIdType numTuples) override;
  void SetNumberOfTuples(svtkIdType number) override;
  void SetTuple(svtkIdType i, svtkIdType j, svtkAbstractArray* source) override;
  void SetTuple(svtkIdType i, const float* source) override;
  void SetTuple(svtkIdType i, const double* source) override;
  void InsertTuple(svtkIdType i, svtkIdType j, svtkAbstractArray* source) override;
  void InsertTuple(svtkIdType i, const float* source) override;
  void InsertTuple(svtkIdType i, const double* source) override;
  void InsertTuples(svtkIdList* dstIds, svtkIdList* srcIds, svtkAbstractArray* source) override;
  void InsertTuples(
    svtkIdType dstStart, svtkIdType n, svtkIdType srcStart, svtkAbstractArray* source) override;
  svtkIdType InsertNextTuple(svtkIdType j, svtkAbstractArray* source) override;
  svtkIdType InsertNextTuple(const float* source) override;
  svtkIdType InsertNextTuple(const double* source) override;
  void DeepCopy(svtkAbstractArray* aa) override;
  void DeepCopy(svtkDataArray* da) override;
  void InterpolateTuple(
    svtkIdType i, svtkIdList* ptIndices, svtkAbstractArray* source, double* weights) override;
  void InterpolateTuple(svtkIdType i, svtkIdType id1, svtkAbstractArray* source1, svtkIdType id2,
    svtkAbstractArray* source2, double t) override;
  void SetVariantValue(svtkIdType idx, svtkVariant value) override;
  void InsertVariantValue(svtkIdType idx, svtkVariant value) override;
  void RemoveTuple(svtkIdType id) override;
  void RemoveFirstTuple() override;
  void RemoveLastTuple() override;
  void SetTypedTuple(svtkIdType i, const Scalar* t) override;
  void InsertTypedTuple(svtkIdType i, const Scalar* t) override;
  svtkIdType InsertNextTypedTuple(const Scalar* t) override;
  void SetValue(svtkIdType idx, Scalar value) override;
  svtkIdType InsertNextValue(Scalar v) override;
  void InsertValue(svtkIdType idx, Scalar v) override;
  //@}

protected:
  svtkCPExodusIIResultsArrayTemplate();
  ~svtkCPExodusIIResultsArrayTemplate() override;

  std::vector<Scalar*> Arrays;

private:
  svtkCPExodusIIResultsArrayTemplate(const svtkCPExodusIIResultsArrayTemplate&) = delete;
  void operator=(const svtkCPExodusIIResultsArrayTemplate&) = delete;

  svtkIdType Lookup(const Scalar& val, svtkIdType startIndex);
  double* TempDoubleArray;
  //@{
  /**
   * By default Save is false.
   */
  bool Save;
  //@}
};

#include "svtkCPExodusIIResultsArrayTemplate.txx"

#endif // svtkCPExodusIIResultsArrayTemplate_h

// SVTK-HeaderTest-Exclude: svtkCPExodusIIResultsArrayTemplate.h
