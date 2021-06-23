//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtkm_m_filter_CellSetConnectivity_h
#define svtkm_m_filter_CellSetConnectivity_h

#include <svtkm/filter/FilterDataSet.h>

namespace svtkm
{
namespace filter
{
/// \brief Finds groups of cells that are connected together through their topology.
///
/// Finds and labels groups of cells that are connected together through their topology.
/// Two cells are considered connected if they share an edge. CellSetConnectivity identifies some
/// number of components and assigns each component a unique integer.
/// The result of the filter is a cell field of type svtkm::Id with the default name of 'component'.
/// Each entry in the cell field will be a number that identifies to which component the cell belongs.
class CellSetConnectivity : public svtkm::filter::FilterDataSet<CellSetConnectivity>
{
public:
  using SupportedTypes = svtkm::TypeListScalarAll;
  SVTKM_CONT CellSetConnectivity();

  void SetOutputFieldName(const std::string& name) { this->OutputFieldName = name; }

  SVTKM_CONT
  const std::string& GetOutputFieldName() const { return this->OutputFieldName; }


  template <typename Policy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(const svtkm::cont::DataSet& input,
                                          svtkm::filter::PolicyBase<Policy> policy);

  template <typename Policy>
  SVTKM_CONT bool MapFieldOntoOutput(svtkm::cont::DataSet& result,
                                    const svtkm::cont::Field& field,
                                    const svtkm::filter::PolicyBase<Policy>&)
  {
    result.AddField(field);
    return true;
  }

private:
  std::string OutputFieldName;
};
}
}

#include <svtkm/filter/CellSetConnectivity.hxx>

#endif //svtkm_m_filter_CellSetConnectivity_h
