//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

#ifndef svtk_m_filter_ContourTreeUniform_hxx
#define svtk_m_filter_ContourTreeUniform_hxx


#include <svtkm/cont/ErrorFilterExecution.h>

#include <svtkm/worklet/ContourTreeUniform.h>

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
ContourTreeMesh2D::ContourTreeMesh2D()
  : svtkm::filter::FilterCell<ContourTreeMesh2D>()
{
  this->SetOutputFieldName("saddlePeak");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
svtkm::cont::DataSet ContourTreeMesh2D::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw svtkm::cont::ErrorFilterExecution("ContourTreeMesh2D expects point field input.");
  }

  // Collect sizing information from the dataset
  const auto& dynamicCellSet = input.GetCellSet();
  svtkm::cont::CellSetStructured<2> cellSet;
  dynamicCellSet.CopyTo(cellSet);

  svtkm::Id2 pointDimensions = cellSet.GetPointDimensions();
  svtkm::Id nRows = pointDimensions[0];
  svtkm::Id nCols = pointDimensions[1];

  svtkm::cont::ArrayHandle<svtkm::Pair<svtkm::Id, svtkm::Id>> saddlePeak;

  svtkm::worklet::ContourTreeMesh2D worklet;
  worklet.Run(field, nRows, nCols, saddlePeak);

  return CreateResultFieldCell(input, saddlePeak, this->GetOutputFieldName());
}
//-----------------------------------------------------------------------------
ContourTreeMesh3D::ContourTreeMesh3D()
  : svtkm::filter::FilterCell<ContourTreeMesh3D>()
{
  this->SetOutputFieldName("saddlePeak");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
svtkm::cont::DataSet ContourTreeMesh3D::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  const svtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  // Collect sizing information from the dataset
  svtkm::cont::CellSetStructured<3> cellSet;
  input.GetCellSet().CopyTo(cellSet);

  svtkm::Id3 pointDimensions = cellSet.GetPointDimensions();
  svtkm::Id nRows = pointDimensions[0];
  svtkm::Id nCols = pointDimensions[1];
  svtkm::Id nSlices = pointDimensions[2];

  svtkm::cont::ArrayHandle<svtkm::Pair<svtkm::Id, svtkm::Id>> saddlePeak;

  svtkm::worklet::ContourTreeMesh3D worklet;
  worklet.Run(field, nRows, nCols, nSlices, saddlePeak);

  return CreateResult(input, saddlePeak, this->GetOutputFieldName(), fieldMeta);
}
}
} // namespace svtkm::filter

#endif
