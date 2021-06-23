/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCPExodusIINodalCoordinatesTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkCPExodusIINodalCoordinatesTemplate
 * @brief   Map native Exodus II coordinate
 * arrays into the svtkDataArray interface.
 *
 *
 * Map native Exodus II coordinate arrays into the svtkDataArray interface. Use
 * the svtkCPExodusIIInSituReader to read an Exodus II file's data into this
 * structure.
 */

#ifndef svtkCPExodusIINodalCoordinatesTemplate_h
#define svtkCPExodusIINodalCoordinatesTemplate_h

#include "svtkIOExodusModule.h" // For export macro
#include "svtkMappedDataArray.h"

#include "svtkObjectFactory.h" // for svtkStandardNewMacro

template <class Scalar>
class svtkCPExodusIINodalCoordinatesTemplate : public svtkMappedDataArray<Scalar>
{
public:
  svtkAbstractTemplateTypeMacro(
    svtkCPExodusIINodalCoordinatesTemplate<Scalar>, svtkMappedDataArray<Scalar>)
    svtkMappedDataArrayNewInstanceMacro(svtkCPExodusIINodalCoordinatesTemplate<
      Scalar>) static svtkCPExodusIINodalCoordinatesTemplate* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  typedef typename Superclass::ValueType ValueType;

  /**
   * Set the raw scalar arrays for the coordinate set. This class takes
   * ownership of the arrays and deletes them with delete[].
   */
  void SetExodusScalarArrays(Scalar* x, Scalar* y, Scalar* z, svtkIdType numPoints);

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
  svtkCPExodusIINodalCoordinatesTemplate();
  ~svtkCPExodusIINodalCoordinatesTemplate() override;

  Scalar* XArray;
  Scalar* YArray;
  Scalar* ZArray;

private:
  svtkCPExodusIINodalCoordinatesTemplate(const svtkCPExodusIINodalCoordinatesTemplate&) = delete;
  void operator=(const svtkCPExodusIINodalCoordinatesTemplate&) = delete;

  svtkIdType Lookup(const Scalar& val, svtkIdType startIndex);
  double* TempDoubleArray;
};

#include "svtkCPExodusIINodalCoordinatesTemplate.txx"

#endif // svtkCPExodusIINodalCoordinatesTemplate_h

// SVTK-HeaderTest-Exclude: svtkCPExodusIINodalCoordinatesTemplate.h
