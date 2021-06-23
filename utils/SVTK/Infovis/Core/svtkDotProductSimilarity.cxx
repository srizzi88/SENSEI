/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDotProductSimilarity.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDotProductSimilarity.h"
#include "svtkArrayData.h"
#include "svtkCommand.h"
#include "svtkDenseArray.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#include <algorithm>
#include <map>
#include <stdexcept>

// threshold_multimap
// This strange little fellow is used by the svtkDotProductSimilarity
// implementation.  It provides the interface
// of a std::multimap, but it enforces several constraints on its contents:
//
// There is an upper-limit on the number of values stored in the container.
// There is a lower threshold on key-values stored in the container.
// The key threshold can be overridden by specifying a lower-limit on the
// number of values stored in the container.

template <typename KeyT, typename ValueT>
class threshold_multimap : public std::multimap<KeyT, ValueT, std::less<KeyT> >
{
  typedef std::multimap<KeyT, ValueT, std::less<KeyT> > container_t;

public:
  threshold_multimap(KeyT minimum_threshold, size_t minimum_count, size_t maximum_count)
    : MinimumThreshold(minimum_threshold)
    , MinimumCount(std::max(static_cast<size_t>(0), minimum_count))
    , MaximumCount(std::max(static_cast<size_t>(0), maximum_count))
  {
  }

  void insert(const typename container_t::value_type& value)
  {
    // Insert the value into the container ...
    container_t::insert(value);

    // Prune small values down to our minimum size ...
    while ((this->size() > this->MinimumCount) && (this->begin()->first < this->MinimumThreshold))
      this->erase(this->begin());

    // Prune small values down to our maximum size ...
    while (this->size() > this->MaximumCount)
      this->erase(this->begin());
  }

private:
  typename container_t::iterator insert(
    typename container_t::iterator where, const typename container_t::value_type& value);
  template <class InIt>
  void insert(InIt first, InIt last);

  KeyT MinimumThreshold;
  size_t MinimumCount;
  size_t MaximumCount;
};

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkDotProductSimilarity);

// ----------------------------------------------------------------------

svtkDotProductSimilarity::svtkDotProductSimilarity()
  : VectorDimension(1)
  , MinimumThreshold(1)
  , MinimumCount(1)
  , MaximumCount(10)
  , UpperDiagonal(true)
  , Diagonal(false)
  , LowerDiagonal(false)
  , FirstSecond(true)
  , SecondFirst(true)
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkDotProductSimilarity::~svtkDotProductSimilarity() = default;

// ----------------------------------------------------------------------

void svtkDotProductSimilarity::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VectorDimension: " << this->VectorDimension << endl;
  os << indent << "MinimumThreshold: " << this->MinimumThreshold << endl;
  os << indent << "MinimumCount: " << this->MinimumCount << endl;
  os << indent << "MaximumCount: " << this->MaximumCount << endl;
  os << indent << "UpperDiagonal: " << this->UpperDiagonal << endl;
  os << indent << "Diagonal: " << this->Diagonal << endl;
  os << indent << "LowerDiagonal: " << this->LowerDiagonal << endl;
  os << indent << "FirstSecond: " << this->FirstSecond << endl;
  os << indent << "SecondFirst: " << this->SecondFirst << endl;
}

int svtkDotProductSimilarity::FillInputPortInformation(int port, svtkInformation* info)
{
  switch (port)
  {
    case 0:
      info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkArrayData");
      return 1;
    case 1:
      info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
      info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkArrayData");
      return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------

static double DotProduct(svtkDenseArray<double>* input_a, svtkDenseArray<double>* input_b,
  const svtkIdType vector_a, const svtkIdType vector_b, const svtkIdType vector_dimension,
  const svtkIdType component_dimension, const svtkArrayRange range_a, const svtkArrayRange range_b)
{
  svtkArrayCoordinates coordinates_a(0, 0);
  svtkArrayCoordinates coordinates_b(0, 0);

  coordinates_a[vector_dimension] = vector_a;
  coordinates_b[vector_dimension] = vector_b;

  double dot_product = 0.0;
  for (svtkIdType component = 0; component != range_a.GetSize(); ++component)
  {
    coordinates_a[component_dimension] = component + range_a.GetBegin();
    coordinates_b[component_dimension] = component + range_b.GetBegin();
    dot_product += input_a->GetValue(coordinates_a) * input_b->GetValue(coordinates_b);
  }
  return dot_product;
}

int svtkDotProductSimilarity::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  try
  {
    // Enforce our preconditions ...
    svtkArrayData* const input_a = svtkArrayData::GetData(inputVector[0]);
    if (!input_a)
      throw std::runtime_error("Missing array data input on input port 0.");
    if (input_a->GetNumberOfArrays() != 1)
      throw std::runtime_error("Array data on input port 0 must contain exactly one array.");
    svtkDenseArray<double>* const input_array_a =
      svtkDenseArray<double>::SafeDownCast(input_a->GetArray(static_cast<svtkIdType>(0)));
    if (!input_array_a)
      throw std::runtime_error("Array on input port 0 must be a svtkDenseArray<double>.");
    if (input_array_a->GetDimensions() != 2)
      throw std::runtime_error("Array on input port 0 must be a matrix.");

    svtkArrayData* const input_b = svtkArrayData::GetData(inputVector[1]);
    svtkDenseArray<double>* input_array_b = nullptr;
    if (input_b)
    {
      if (input_b->GetNumberOfArrays() != 1)
        throw std::runtime_error("Array data on input port 1 must contain exactly one array.");
      input_array_b =
        svtkDenseArray<double>::SafeDownCast(input_b->GetArray(static_cast<svtkIdType>(0)));
      if (!input_array_b)
        throw std::runtime_error("Array on input port 1 must be a svtkDenseArray<double>.");
      if (input_array_b->GetDimensions() != 2)
        throw std::runtime_error("Array on input port 1 must be a matrix.");
    }

    const svtkIdType vector_dimension = this->VectorDimension;
    if (vector_dimension != 0 && vector_dimension != 1)
      throw std::runtime_error("VectorDimension must be zero or one.");

    const svtkIdType component_dimension = 1 - vector_dimension;

    const svtkArrayRange vectors_a = input_array_a->GetExtent(vector_dimension);
    const svtkArrayRange components_a = input_array_a->GetExtent(component_dimension);

    const svtkArrayRange vectors_b =
      input_array_b ? input_array_b->GetExtent(vector_dimension) : svtkArrayRange();
    const svtkArrayRange components_b =
      input_array_b ? input_array_b->GetExtent(component_dimension) : svtkArrayRange();

    if (input_array_b && (components_a.GetSize() != components_b.GetSize()))
      throw std::runtime_error("Input array vector lengths must match.");

    // Get output arrays ...
    svtkTable* const output = svtkTable::GetData(outputVector);

    svtkIdTypeArray* const source_array = svtkIdTypeArray::New();
    source_array->SetName("source");

    svtkIdTypeArray* const target_array = svtkIdTypeArray::New();
    target_array->SetName("target");

    svtkDoubleArray* const similarity_array = svtkDoubleArray::New();
    similarity_array->SetName("similarity");

    // Okay let outside world know that I'm starting
    double progress = 0;
    this->InvokeEvent(svtkCommand::ProgressEvent, &progress);

    typedef threshold_multimap<double, svtkIdType> similarities_t;
    if (input_array_b)
    {
      // Compare the first matrix with the second matrix ...
      if (this->FirstSecond)
      {
        for (svtkIdType vector_a = vectors_a.GetBegin(); vector_a != vectors_a.GetEnd(); ++vector_a)
        {
          similarities_t similarities(
            this->MinimumThreshold, this->MinimumCount, this->MaximumCount);

          for (svtkIdType vector_b = vectors_b.GetBegin(); vector_b != vectors_b.GetEnd();
               ++vector_b)
          {
            // Can't use std::make_pair - see
            // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
            similarities.insert(std::pair<const double, svtkIdType>(
              DotProduct(input_array_a, input_array_b, vector_a, vector_b, vector_dimension,
                component_dimension, components_a, components_b),
              vector_b));
          }

          for (similarities_t::const_iterator similarity = similarities.begin();
               similarity != similarities.end(); ++similarity)
          {
            source_array->InsertNextValue(vector_a);
            target_array->InsertNextValue(similarity->second);
            similarity_array->InsertNextValue(similarity->first);
          }
        }
      }
      // Compare the second matrix with the first matrix ...
      if (this->SecondFirst)
      {
        for (svtkIdType vector_b = vectors_b.GetBegin(); vector_b != vectors_b.GetEnd(); ++vector_b)
        {
          similarities_t similarities(
            this->MinimumThreshold, this->MinimumCount, this->MaximumCount);

          for (svtkIdType vector_a = vectors_a.GetBegin(); vector_a != vectors_a.GetEnd();
               ++vector_a)
          {
            // Can't use std::make_pair - see
            // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
            similarities.insert(std::pair<const double, svtkIdType>(
              DotProduct(input_array_b, input_array_a, vector_b, vector_a, vector_dimension,
                component_dimension, components_b, components_a),
              vector_a));
          }

          for (similarities_t::const_iterator similarity = similarities.begin();
               similarity != similarities.end(); ++similarity)
          {
            source_array->InsertNextValue(vector_b);
            target_array->InsertNextValue(similarity->second);
            similarity_array->InsertNextValue(similarity->first);
          }
        }
      }
    }
    // Compare the one matrix with itself ...
    else
    {
      for (svtkIdType vector_a = vectors_a.GetBegin(); vector_a != vectors_a.GetEnd(); ++vector_a)
      {
        similarities_t similarities(this->MinimumThreshold, this->MinimumCount, this->MaximumCount);

        for (svtkIdType vector_b = vectors_a.GetBegin(); vector_b != vectors_a.GetEnd(); ++vector_b)
        {
          if ((vector_b > vector_a) && !this->UpperDiagonal)
            continue;

          if ((vector_b == vector_a) && !this->Diagonal)
            continue;

          if ((vector_b < vector_a) && !this->LowerDiagonal)
            continue;

          // Can't use std::make_pair - see
          // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
          similarities.insert(std::pair<const double, svtkIdType>(
            DotProduct(input_array_a, input_array_a, vector_a, vector_b, vector_dimension,
              component_dimension, components_a, components_a),
            vector_b));
        }

        for (similarities_t::const_iterator similarity = similarities.begin();
             similarity != similarities.end(); ++similarity)
        {
          source_array->InsertNextValue(vector_a);
          target_array->InsertNextValue(similarity->second);
          similarity_array->InsertNextValue(similarity->first);
        }
      }
    }

    output->AddColumn(source_array);
    output->AddColumn(target_array);
    output->AddColumn(similarity_array);

    source_array->Delete();
    target_array->Delete();
    similarity_array->Delete();

    return 1;
  }
  catch (std::exception& e)
  {
    svtkErrorMacro(<< "unhandled exception: " << e.what());
    return 0;
  }
  catch (...)
  {
    svtkErrorMacro(<< "unknown exception");
    return 0;
  }
}
