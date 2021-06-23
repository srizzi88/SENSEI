/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkValueSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkValueSelector.h"

#include "svtkArrayDispatch.h"
#include "svtkAssume.h"
#include "svtkDataArrayRange.h"
#include "svtkDataObject.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkSMPTools.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkSortDataArray.h"
#include "svtkStringArray.h"

#include <cassert>
#include <type_traits>
namespace
{

struct ThresholdSelectionListReshaper
{
protected:
  svtkDataArray* FixedArray;

public:
  ThresholdSelectionListReshaper(svtkDataArray* toFill)
    : FixedArray(toFill)
  {
  }
  // If the input selection list for a threshold has one component we need
  // to reshape it into an array with two component tuples (ranges) so it
  // is interpreted correctly later.
  template <typename SelectionListArrayType>
  void operator()(SelectionListArrayType* originalList)
  {
    // created with NewInstance from the originalList, we know it is the same type
    auto fixedList = SelectionListArrayType::FastDownCast(this->FixedArray);
    assert(originalList->GetNumberOfComponents() == 1);
    assert(fixedList->GetNumberOfComponents() == 2);

    const auto orig = svtk::DataArrayValueRange<1>(originalList);
    auto fixed = svtk::DataArrayValueRange<2>(fixedList);
    std::copy(orig.cbegin(), orig.cend(), fixed.begin());
  }
};

//----------------------------------------------------------------------------
// This is used for the cases where the SelectionList is a 1-component array,
// implying that the values are exact matches.
struct ArrayValueMatchFunctor
{
  svtkSignedCharArray* InsidednessArray;
  int ComponentNo;

  ArrayValueMatchFunctor(svtkSignedCharArray* insidednessArray, int comp)
    : InsidednessArray(insidednessArray)
    , ComponentNo(comp)
  {
  }

  // this is used for selecting entries where the field array has matching
  // values.
  template <typename InputArrayType, typename SelectionListArrayType>
  void operator()(InputArrayType* fArray, SelectionListArrayType* selList)
  {
    static_assert(std::is_same<svtk::GetAPIType<InputArrayType>,
                    svtk::GetAPIType<SelectionListArrayType> >::value,
      "value types mismatched!");
    SVTK_ASSUME(selList->GetNumberOfComponents() == 1);
    SVTK_ASSUME(fArray->GetNumberOfComponents() > this->ComponentNo);

    using ValueType = svtk::GetAPIType<SelectionListArrayType>;

    const ValueType* haystack_begin = selList->GetPointer(0);
    const ValueType* haystack_end = haystack_begin + selList->GetNumberOfValues();
    const int comp = fArray->GetNumberOfComponents() == 1 ? 0 : this->ComponentNo;

    svtkSignedCharArray* insidednessArray = this->InsidednessArray;
    SVTK_ASSUME(insidednessArray->GetNumberOfTuples() == fArray->GetNumberOfTuples());
    if (comp >= 0)
    {
      svtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](svtkIdType begin, svtkIdType end) {
        const auto fRange = svtk::DataArrayTupleRange(fArray, begin, end);
        auto insideRange = svtk::DataArrayValueRange<1>(insidednessArray, begin, end);
        auto insideIter = insideRange.begin();
        for (auto i = fRange.cbegin(); i != fRange.cend(); ++i, ++insideIter)
        {
          auto result = std::binary_search(haystack_begin, haystack_end, (*i)[comp]);
          *insideIter = result ? 1 : 0;
        }
      });
    }
    else
    {
      // compare vector magnitude.
      svtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](svtkIdType begin, svtkIdType end) {
        const auto fRange = svtk::DataArrayTupleRange(fArray, begin, end);
        auto insideRange = svtk::DataArrayValueRange<1>(insidednessArray, begin, end);
        using FTupleCRefType = typename decltype(fRange)::ConstTupleReferenceType;
        std::transform(fRange.cbegin(), fRange.cend(), insideRange.begin(),
          [&](FTupleCRefType fTuple) -> signed char {
            ValueType val = ValueType(0);
            for (const ValueType fComp : fTuple)
            {
              val += fComp * fComp;
            }
            const auto mag = static_cast<ValueType>(std::sqrt(val));
            return std::binary_search(haystack_begin, haystack_end, mag) ? 1 : 0;
          });
      });
    }
  }

  // this is used to select indices
  template <typename SelectionListArrayType>
  void operator()(SelectionListArrayType* selList)
  {
    using T = svtk::GetAPIType<SelectionListArrayType>;
    assert(selList->GetNumberOfComponents() == 1);

    const svtkIdType numDataValues = this->InsidednessArray->GetNumberOfTuples();
    const auto selRange = svtk::DataArrayValueRange<1>(selList);

    this->InsidednessArray->FillValue(0);
    std::for_each(selRange.cbegin(), selRange.cend(), [&](const T selVal) {
      const auto cid = static_cast<svtkIdType>(selVal);
      if (cid >= 0 && cid < numDataValues)
      {
        this->InsidednessArray->SetValue(cid, 1);
      }
    });
  }
};

//----------------------------------------------------------------------------
// This is used for the cases where the SelectionList is a 2-component array,
// implying that the values are ranges.
struct ArrayValueRangeFunctor
{
  svtkSignedCharArray* InsidednessArray;
  int ComponentNo;

  ArrayValueRangeFunctor(svtkSignedCharArray* insidednessArray, int comp)
    : InsidednessArray(insidednessArray)
    , ComponentNo(comp)
  {
  }

  // for selecting using field array values
  template <typename InputArrayType, typename SelectionListArrayType>
  void operator()(InputArrayType* fArray, SelectionListArrayType* selList)
  {
    static_assert(std::is_same<svtk::GetAPIType<InputArrayType>,
                    svtk::GetAPIType<SelectionListArrayType> >::value,
      "value types mismatched!");

    using ValueType = svtk::GetAPIType<SelectionListArrayType>;

    SVTK_ASSUME(selList->GetNumberOfComponents() == 2);
    SVTK_ASSUME(fArray->GetNumberOfComponents() > this->ComponentNo);
    SVTK_ASSUME(this->InsidednessArray->GetNumberOfTuples() == fArray->GetNumberOfTuples());

    const int comp = fArray->GetNumberOfComponents() == 1 ? 0 : this->ComponentNo;

    if (comp >= 0)
    {
      svtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](svtkIdType begin, svtkIdType end) {
        const auto fRange = svtk::DataArrayTupleRange(fArray, begin, end);
        const auto selRange = svtk::DataArrayTupleRange<2>(selList);
        auto insideRange = svtk::DataArrayValueRange<1>(this->InsidednessArray, begin, end);

        using FTupleCRefType = typename decltype(fRange)::ConstTupleReferenceType;
        using STupleCRefType = typename decltype(selRange)::ConstTupleReferenceType;

        auto insideIter = insideRange.begin();
        for (FTupleCRefType fTuple : fRange)
        {
          const ValueType val = fTuple[comp];
          auto matchIter = std::find_if(selRange.cbegin(), selRange.cend(),
            [&](STupleCRefType range) -> bool { return val >= range[0] && val <= range[1]; });
          *insideIter++ = matchIter != selRange.cend() ? 1 : 0;
        }
      });
    }
    else
    {
      // compare vector magnitude.
      svtkSMPTools::For(0, fArray->GetNumberOfTuples(), [=](svtkIdType begin, svtkIdType end) {
        const auto fRange = svtk::DataArrayTupleRange(fArray, begin, end);
        const auto selRange = svtk::DataArrayTupleRange<2>(selList);
        auto insideRange = svtk::DataArrayValueRange<1>(this->InsidednessArray, begin, end);

        using FTupleCRefType = typename decltype(fRange)::ConstTupleReferenceType;
        using STupleCRefType = typename decltype(selRange)::ConstTupleReferenceType;

        auto insideIter = insideRange.begin();
        for (FTupleCRefType fTuple : fRange)
        {
          ValueType val{ 0 };
          for (const ValueType fComp : fTuple)
          {
            val += fComp * fComp;
          }
          const auto mag = static_cast<ValueType>(std::sqrt(val));
          auto matchIter = std::find_if(selRange.cbegin(), selRange.cend(),
            [&](STupleCRefType range) -> bool { return mag >= range[0] && mag <= range[1]; });
          *insideIter++ = matchIter != selRange.cend() ? 1 : 0;
        }
      });
    }
  }

  // this is used to select indices
  template <typename SelectionListArrayType>
  void operator()(SelectionListArrayType* selList)
  {
    assert(selList->GetNumberOfComponents() == 2);

    const auto selRange = svtk::DataArrayTupleRange<2>(selList);
    using SelRangeCRefT = typename decltype(selRange)::ConstTupleReferenceType;

    const svtkIdType numValues = this->InsidednessArray->GetNumberOfTuples();

    this->InsidednessArray->FillValue(0);
    for (SelRangeCRefT range : selRange)
    {
      const svtkIdType start = std::min(static_cast<svtkIdType>(range[0]), numValues - 1);
      const svtkIdType last = std::min(static_cast<svtkIdType>(range[1]), numValues - 1);
      if (start >= 0 && last >= start)
      {
        auto inside = svtk::DataArrayValueRange<1>(this->InsidednessArray, start, last + 1);
        std::fill(inside.begin(), inside.end(), 1);
      }
    }
  }
};
}

//----------------------------------------------------------------------------
class svtkValueSelector::svtkInternals
{
  svtkSmartPointer<svtkAbstractArray> SelectionList;
  std::string FieldName;
  int FieldAssociation;
  int FieldAttributeType;
  int ComponentNo;

public:
  // use this constructor when selection is specified as (assoc, name)
  svtkInternals(svtkAbstractArray* selectionList, int fieldAssociation, const std::string& fieldName,
    int component)
    : svtkInternals(selectionList, fieldName, fieldAssociation, -1, component)
  {
  }

  // use this constructor when selection is specified as (assoc, attribute type)
  svtkInternals(
    svtkAbstractArray* selectionList, int fieldAssociation, int attributeType, int component)
    : svtkInternals(selectionList, "", fieldAssociation, attributeType, component)
  {
    if (attributeType < 0 || attributeType >= svtkDataSetAttributes::NUM_ATTRIBUTES)
    {
      throw std::runtime_error("unsupported attribute type");
    }
  }

  // use this constructor when selection is for ids of element type = assoc.
  svtkInternals(svtkAbstractArray* selectionList, int fieldAssociation)
    : svtkInternals(selectionList, "", fieldAssociation, -1, 0)
  {
  }

  // returns false on any failure or unhandled case.
  bool Execute(svtkDataObject* dobj, svtkSignedCharArray* darray);

private:
  svtkInternals(svtkAbstractArray* selectionList, const std::string& fieldName, int fieldAssociation,
    int attributeType, int component)
    : SelectionList(selectionList)
    , FieldName(fieldName)
    , FieldAssociation(fieldAssociation)
    , FieldAttributeType(attributeType)
    , ComponentNo(component)
  {
    if (fieldAssociation < 0 || fieldAssociation >= svtkDataObject::NUMBER_OF_ASSOCIATIONS ||
      fieldAssociation == svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      throw std::runtime_error("unsupported field association");
    }

    if (selectionList->GetNumberOfComponents() != 1 && selectionList->GetNumberOfComponents() != 2)
    {
      // 1-component == exact value match
      // 2-component == values in range specified by each tuple.
      throw std::runtime_error("Currently, selecting multi-components arrays is not supported.");
    }

    if (selectionList->GetNumberOfComponents() == 1)
    {
      // we sort the selection list to speed up extraction later.
      this->SelectionList.TakeReference(selectionList->NewInstance());
      this->SelectionList->DeepCopy(selectionList);
      svtkSortDataArray::Sort(this->SelectionList);
    }
    else
    {
      // don't bother sorting.
      this->SelectionList = selectionList;
    }
  }

  bool Execute(svtkAbstractArray* darray, svtkSignedCharArray* insidednessArray)
  {
    if (auto dataArray = svtkDataArray::SafeDownCast(darray))
    {
      return this->Execute(dataArray, insidednessArray);
    }
    else if (darray)
    {
      // classes like svtkStringArray may be added later, if needed.
      svtkGenericWarningMacro(<< darray->GetClassName() << " not supported by svtkValueSelector.");
      return false;
    }
    else
    {
      // missing array, this can happen.
      return false;
    }
  }

  bool Execute(svtkDataArray* darray, svtkSignedCharArray* insidednessArray)
  {
    assert(svtkDataArray::SafeDownCast(this->SelectionList));
    if (darray->GetNumberOfComponents() < this->ComponentNo)
    {
      // array doesn't have request components. nothing to select.
      return false;
    }

    if (this->SelectionList->GetNumberOfComponents() == 1)
    {
      ArrayValueMatchFunctor worker(insidednessArray, this->ComponentNo);
      if (!svtkArrayDispatch::Dispatch2SameValueType::Execute(
            darray, svtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        svtkGenericWarningMacro("Type mismatch in selection list ("
          << this->SelectionList->GetClassName() << ") and field array (" << darray->GetClassName()
          << ").");
        return false;
      }
    }
    else
    {
      ArrayValueRangeFunctor worker(insidednessArray, this->ComponentNo);
      if (!svtkArrayDispatch::Dispatch2SameValueType::Execute(
            darray, svtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // Use the slow data array API for threshold-based selection. Thresholds
        // are assumed to be stored in a svtkDoubleArray, which may very well not be the
        // same as the data array type.
        svtkDataArray* selList = svtkDataArray::SafeDownCast(this->SelectionList);
        const int comp = darray->GetNumberOfComponents() == 1 ? 0 : this->ComponentNo;
        const svtkIdType numRanges = selList->GetNumberOfTuples();

        if (comp >= 0)
        {
          svtkSMPTools::For(0, darray->GetNumberOfTuples(), [=](svtkIdType begin, svtkIdType end) {
            for (svtkIdType cc = begin; cc < end; ++cc)
            {
              const auto val = darray->GetComponent(cc, comp);
              bool match = false;
              for (svtkIdType r = 0; r < numRanges && !match; ++r)
              {
                match = (val >= selList->GetComponent(r, 0) && val <= selList->GetComponent(r, 1));
              }
              insidednessArray->SetValue(cc, match ? 1 : 0);
            }
          });
        }
        else
        {
          const int num_components = darray->GetNumberOfComponents();

          // compare vector magnitude.
          svtkSMPTools::For(0, darray->GetNumberOfTuples(), [=](svtkIdType begin, svtkIdType end) {
            for (svtkIdType cc = begin; cc < end; ++cc)
            {
              double val = double(0);
              for (int kk = 0; kk < num_components; ++kk)
              {
                const auto valKK = darray->GetComponent(cc, comp);
                val += valKK * valKK;
              }
              const auto magnitude = std::sqrt(val);
              bool match = false;
              for (svtkIdType r = 0; r < numRanges && !match; ++r)
              {
                match = (magnitude >= selList->GetComponent(r, 0) &&
                  magnitude <= selList->GetComponent(r, 1));
              }
              insidednessArray->SetValue(cc, match ? 1 : 0);
            }
          });
        }
      }
    }

    insidednessArray->Modified();
    return true;
  }

  // this is used for when selecting elements by ids
  bool Execute(svtkSignedCharArray* insidednessArray)
  {
    assert(svtkDataArray::SafeDownCast(this->SelectionList));

    if (this->SelectionList->GetNumberOfComponents() == 1)
    {
      ArrayValueMatchFunctor worker(insidednessArray, 0);
      if (!svtkArrayDispatch::DispatchByValueType<svtkArrayDispatch::Integrals>::Execute(
            svtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        svtkGenericWarningMacro(
          "Unsupported selection list array type (" << this->SelectionList->GetClassName() << ").");
        return false;
      }
    }
    else
    {
      ArrayValueRangeFunctor worker(insidednessArray, 0);
      if (!svtkArrayDispatch::DispatchByValueType<svtkArrayDispatch::Integrals>::Execute(
            svtkDataArray::SafeDownCast(this->SelectionList), worker))
      {
        // should we use slow data array API?
        svtkGenericWarningMacro(
          "Unsupported selection list array type (" << this->SelectionList->GetClassName() << ").");
        return false;
      }
    }
    insidednessArray->Modified();
    return true;
  }
};

//----------------------------------------------------------------------------
bool svtkValueSelector::svtkInternals::Execute(
  svtkDataObject* dobj, svtkSignedCharArray* insidednessArray)
{
  if (this->FieldAssociation != -1 && !this->FieldName.empty())
  {
    auto* dsa = dobj->GetAttributesAsFieldData(this->FieldAssociation);
    return dsa ? this->Execute(dsa->GetAbstractArray(this->FieldName.c_str()), insidednessArray)
               : false;
  }
  else if (this->FieldAssociation != -1 && this->FieldAttributeType != -1)
  {
    auto* dsa = dobj->GetAttributes(this->FieldAssociation);
    return dsa
      ? this->Execute(dsa->GetAbstractAttribute(this->FieldAttributeType), insidednessArray)
      : false;
  }
  else if (this->FieldAssociation != -1)
  {
    return this->Execute(insidednessArray);
  }
  return false;
}

//============================================================================
svtkStandardNewMacro(svtkValueSelector);
//----------------------------------------------------------------------------
svtkValueSelector::svtkValueSelector()
  : Internals(nullptr)
{
}

//----------------------------------------------------------------------------
svtkValueSelector::~svtkValueSelector() {}

//----------------------------------------------------------------------------
void svtkValueSelector::Initialize(svtkSelectionNode* node)
{
  assert(node);

  this->Superclass::Initialize(node);

  this->Internals.reset();

  try
  {
    svtkSmartPointer<svtkAbstractArray> selectionList = node->GetSelectionList();
    if (!selectionList || selectionList->GetNumberOfTuples() == 0)
    {
      // empty selection list, nothing to do.
      return;
    }

    auto properties = node->GetProperties();

    const int contentType = node->GetContentType();
    const int fieldType = node->GetFieldType();
    const int assoc = svtkSelectionNode::ConvertSelectionFieldToAttributeType(fieldType);
    const int component_no = properties->Has(svtkSelectionNode::COMPONENT_NUMBER())
      ? properties->Get(svtkSelectionNode::COMPONENT_NUMBER())
      : 0;

    switch (contentType)
    {
      case svtkSelectionNode::GLOBALIDS:
        this->Internals.reset(
          new svtkInternals(selectionList, assoc, svtkDataSetAttributes::GLOBALIDS, component_no));
        break;

      case svtkSelectionNode::PEDIGREEIDS:
        this->Internals.reset(
          new svtkInternals(selectionList, assoc, svtkDataSetAttributes::PEDIGREEIDS, component_no));
        break;

      case svtkSelectionNode::THRESHOLDS:
        if (selectionList->GetNumberOfComponents() == 1)
        {
#ifndef SVTK_LEGACY_SILENT
          svtkWarningMacro(
            "Warning: range selections should use two-component arrays to specify the"
            " range.  Using single component arrays with a tuple for the low and high ends of the"
            " range is legacy behavior and may be removed in future releases.");
#endif
          auto selList = svtkDataArray::SafeDownCast(selectionList.GetPointer());
          if (selList)
          {
            selectionList = svtkSmartPointer<svtkAbstractArray>::NewInstance(selList);
            selectionList->SetNumberOfComponents(2);
            selectionList->SetNumberOfTuples(selList->GetNumberOfTuples() / 2);
            selectionList->SetName(selList->GetName());

            ThresholdSelectionListReshaper reshaper(svtkDataArray::SafeDownCast(selectionList));

            if (!svtkArrayDispatch::Dispatch::Execute(selList, reshaper))
            {
              // should never happen, we create an array with the same type
              svtkErrorMacro("Mismatch in selection list fixup code");
              break;
            }
          }
        }
        SVTK_FALLTHROUGH;
      case svtkSelectionNode::VALUES:
        if (selectionList->GetName() == nullptr || selectionList->GetName()[0] == '\0')
        {
          // if selectionList has no name, we're selected scalars (this is old
          // behavior, and we're preserving it).
          this->Internals.reset(
            new svtkInternals(selectionList, assoc, svtkDataSetAttributes::SCALARS, component_no));
        }
        else
        {
          this->Internals.reset(
            new svtkInternals(selectionList, assoc, selectionList->GetName(), component_no));
        }
        break;

      case svtkSelectionNode::INDICES:
        this->Internals.reset(new svtkInternals(selectionList, assoc));
        break;

      default:
        svtkErrorMacro("svtkValueSelector doesn't support content-type: " << contentType);
        break;
    };
  }
  catch (const std::runtime_error& e)
  {
    svtkErrorMacro(<< e.what());
  }
}

//----------------------------------------------------------------------------
void svtkValueSelector::Finalize()
{
  this->Internals.reset();
}

//----------------------------------------------------------------------------
bool svtkValueSelector::ComputeSelectedElements(
  svtkDataObject* input, svtkSignedCharArray* insidednessArray)
{
  assert(input != nullptr && insidednessArray != nullptr);
  return this->Internals ? this->Internals->Execute(input, insidednessArray) : false;
}

//----------------------------------------------------------------------------
void svtkValueSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
