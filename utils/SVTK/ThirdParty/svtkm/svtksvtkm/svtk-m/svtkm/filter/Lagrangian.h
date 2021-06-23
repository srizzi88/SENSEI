//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Lagrangian_h
#define svtk_m_filter_Lagrangian_h

#include <svtkm/filter/FilterDataSetWithField.h>

namespace svtkm
{
namespace filter
{

class Lagrangian : public svtkm::filter::FilterDataSetWithField<Lagrangian>
{
public:
  using SupportedTypes = svtkm::TypeListFieldVec3;

  SVTKM_CONT
  Lagrangian();

  SVTKM_CONT
  void SetRank(svtkm::Id val) { this->rank = val; }

  SVTKM_CONT
  void SetInitFlag(bool val) { this->initFlag = val; }

  SVTKM_CONT
  void SetExtractFlows(bool val) { this->extractFlows = val; }

  SVTKM_CONT
  void SetResetParticles(bool val) { this->resetParticles = val; }

  SVTKM_CONT
  void SetStepSize(svtkm::Float32 val) { this->stepSize = val; }

  SVTKM_CONT
  void SetWriteFrequency(svtkm::Id val) { this->writeFrequency = val; }

  SVTKM_CONT
  void SetSeedResolutionInX(svtkm::Id val) { this->x_res = val; }

  SVTKM_CONT
  void SetSeedResolutionInY(svtkm::Id val) { this->y_res = val; }

  SVTKM_CONT
  void SetSeedResolutionInZ(svtkm::Id val) { this->z_res = val; }

  SVTKM_CONT
  void SetCustomSeedResolution(svtkm::Id val) { this->cust_res = val; }

  SVTKM_CONT
  void SetSeedingResolution(svtkm::Id3 val) { this->SeedRes = val; }

  SVTKM_CONT
  void UpdateSeedResolution(svtkm::cont::DataSet input);

  SVTKM_CONT
  void WriteDataSet(svtkm::Id cycle, const std::string& filename, svtkm::cont::DataSet dataset);

  SVTKM_CONT
  void InitializeUniformSeeds(const svtkm::cont::DataSet& input);

  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT svtkm::cont::DataSet DoExecute(
    const svtkm::cont::DataSet& input,
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
    const svtkm::filter::FieldMetadata& fieldMeta,
    svtkm::filter::PolicyBase<DerivedPolicy> policy);


  template <typename T, typename StorageType, typename DerivedPolicy>
  SVTKM_CONT bool DoMapField(svtkm::cont::DataSet& result,
                            const svtkm::cont::ArrayHandle<T, StorageType>& input,
                            const svtkm::filter::FieldMetadata& fieldMeta,
                            svtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  svtkm::Id rank;
  bool initFlag;
  bool extractFlows;
  bool resetParticles;
  svtkm::Float32 stepSize;
  svtkm::Id x_res, y_res, z_res;
  svtkm::Id cust_res;
  svtkm::Id3 SeedRes;
  svtkm::Id writeFrequency;
};
}
} // namespace svtkm::filter

#include <svtkm/filter/Lagrangian.hxx>

#endif // svtk_m_filter_Lagrangian_h
