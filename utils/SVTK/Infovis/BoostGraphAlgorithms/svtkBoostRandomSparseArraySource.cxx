/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoostRandomSparseArraySource.cxx

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

#include "svtkBoostRandomSparseArraySource.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkSparseArray.h"

#include <boost/random.hpp>

// ----------------------------------------------------------------------

svtkStandardNewMacro(svtkBoostRandomSparseArraySource);

// ----------------------------------------------------------------------

svtkBoostRandomSparseArraySource::svtkBoostRandomSparseArraySource()
  : Extents(2, 2)
  , ElementProbabilitySeed(123)
  , ElementProbability(0.5)
  , ElementValueSeed(456)
  , MinValue(0.0)
  , MaxValue(1.0)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

svtkBoostRandomSparseArraySource::~svtkBoostRandomSparseArraySource() {}

// ----------------------------------------------------------------------

void svtkBoostRandomSparseArraySource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Extents: " << this->Extents << endl;
  os << indent << "ElementProbabilitySeed: " << this->ElementProbabilitySeed << endl;
  os << indent << "ElementProbability: " << this->ElementProbability << endl;
  os << indent << "ElementValueSeed: " << this->ElementValueSeed << endl;
  os << indent << "MinValue: " << this->MinValue << endl;
  os << indent << "MaxValue: " << this->MaxValue << endl;
}

void svtkBoostRandomSparseArraySource::SetExtents(const svtkArrayExtents& extents)
{
  if (extents == this->Extents)
    return;

  this->Extents = extents;
  this->Modified();
}

svtkArrayExtents svtkBoostRandomSparseArraySource::GetExtents()
{
  return this->Extents;
}

// ----------------------------------------------------------------------

int svtkBoostRandomSparseArraySource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  boost::mt19937 pattern_generator(static_cast<boost::uint32_t>(this->ElementProbabilitySeed));
  boost::bernoulli_distribution<> pattern_distribution(this->ElementProbability);
  boost::variate_generator<boost::mt19937&, boost::bernoulli_distribution<> > pattern(
    pattern_generator, pattern_distribution);

  boost::mt19937 value_generator(static_cast<boost::uint32_t>(this->ElementValueSeed));
  boost::uniform_real<> value_distribution(this->MinValue, this->MaxValue);
  boost::variate_generator<boost::mt19937&, boost::uniform_real<> > values(
    value_generator, value_distribution);

  svtkSparseArray<double>* const array = svtkSparseArray<double>::New();
  array->Resize(this->Extents);

  svtkArrayCoordinates coordinates;
  for (svtkArray::SizeT n = 0; n != this->Extents.GetSize(); ++n)
  {
    this->Extents.GetRightToLeftCoordinatesN(n, coordinates);

    // Although it seems wasteful, we calculate a value for every element in the array
    // so the results stay consistent as the ElementProbability varies
    const double value = values();
    if (pattern())
    {
      array->AddValue(coordinates, value);
    }
  }

  svtkArrayData* const output = svtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(array);
  array->Delete();

  return 1;
}
