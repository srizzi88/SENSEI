//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_filter_Lagrangian_hxx
#define svtk_m_filter_Lagrangian_hxx

#include <svtkm/Types.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DataSetBuilderExplicit.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/ErrorFilterExecution.h>
#include <svtkm/io/writer/SVTKDataSetWriter.h>
#include <svtkm/worklet/ParticleAdvection.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/particleadvection/GridEvaluators.h>
#include <svtkm/worklet/particleadvection/Integrators.h>
#include <svtkm/worklet/particleadvection/Particles.h>

#include <cstring>
#include <sstream>
#include <string.h>

static svtkm::Id cycle = 0;
static svtkm::cont::ArrayHandle<svtkm::Particle> BasisParticles;
static svtkm::cont::ArrayHandle<svtkm::Particle> BasisParticlesOriginal;
static svtkm::cont::ArrayHandle<svtkm::Id> BasisParticlesValidity;

namespace
{
class ValidityCheck : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn end_point, FieldInOut output);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  ValidityCheck(svtkm::Bounds b)
    : bounds(b)
  {
  }

  template <typename ValidityType>
  SVTKM_EXEC void operator()(const svtkm::Particle& end_point, ValidityType& res) const
  {
    svtkm::Id steps = end_point.NumSteps;
    if (steps > 0 && res == 1)
    {
      if (end_point.Pos[0] >= bounds.X.Min && end_point.Pos[0] <= bounds.X.Max &&
          end_point.Pos[1] >= bounds.Y.Min && end_point.Pos[1] <= bounds.Y.Max &&
          end_point.Pos[2] >= bounds.Z.Min && end_point.Pos[2] <= bounds.Z.Max)
      {
        res = 1;
      }
      else
      {
        res = 0;
      }
    }
    else
    {
      res = 0;
    }
  }

private:
  svtkm::Bounds bounds;
};
}

namespace svtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline SVTKM_CONT Lagrangian::Lagrangian()
  : svtkm::filter::FilterDataSetWithField<Lagrangian>()
  , rank(0)
  , initFlag(true)
  , extractFlows(false)
  , resetParticles(true)
  , stepSize(1.0f)
  , x_res(0)
  , y_res(0)
  , z_res(0)
  , cust_res(0)
  , SeedRes(svtkm::Id3(1, 1, 1))
  , writeFrequency(0)
{
}

//-----------------------------------------------------------------------------
inline void Lagrangian::WriteDataSet(svtkm::Id cycle,
                                     const std::string& filename,
                                     svtkm::cont::DataSet dataset)
{
  std::stringstream file_stream;
  file_stream << filename << cycle << ".svtk";
  svtkm::io::writer::SVTKDataSetWriter writer(file_stream.str());
  writer.WriteDataSet(dataset);
}

//-----------------------------------------------------------------------------
inline void Lagrangian::UpdateSeedResolution(const svtkm::cont::DataSet input)
{
  svtkm::cont::DynamicCellSet cell_set = input.GetCellSet();

  if (cell_set.IsSameType(svtkm::cont::CellSetStructured<1>()))
  {
    svtkm::cont::CellSetStructured<1> cell_set1 = cell_set.Cast<svtkm::cont::CellSetStructured<1>>();
    svtkm::Id dims1 = cell_set1.GetPointDimensions();
    this->SeedRes[0] = dims1;
    if (this->cust_res)
    {
      this->SeedRes[0] = dims1 / this->x_res;
    }
  }
  else if (cell_set.IsSameType(svtkm::cont::CellSetStructured<2>()))
  {
    svtkm::cont::CellSetStructured<2> cell_set2 = cell_set.Cast<svtkm::cont::CellSetStructured<2>>();
    svtkm::Id2 dims2 = cell_set2.GetPointDimensions();
    this->SeedRes[0] = dims2[0];
    this->SeedRes[1] = dims2[1];
    if (this->cust_res)
    {
      this->SeedRes[0] = dims2[0] / this->x_res;
      this->SeedRes[1] = dims2[1] / this->y_res;
    }
  }
  else if (cell_set.IsSameType(svtkm::cont::CellSetStructured<3>()))
  {
    svtkm::cont::CellSetStructured<3> cell_set3 = cell_set.Cast<svtkm::cont::CellSetStructured<3>>();
    svtkm::Id3 dims3 = cell_set3.GetPointDimensions();
    this->SeedRes[0] = dims3[0];
    this->SeedRes[1] = dims3[1];
    this->SeedRes[2] = dims3[2];
    if (this->cust_res)
    {
      this->SeedRes[0] = dims3[0] / this->x_res;
      this->SeedRes[1] = dims3[1] / this->y_res;
      this->SeedRes[2] = dims3[2] / this->z_res;
    }
  }
}


//-----------------------------------------------------------------------------
inline void Lagrangian::InitializeUniformSeeds(const svtkm::cont::DataSet& input)
{
  svtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  Lagrangian::UpdateSeedResolution(input);

  svtkm::Float64 x_spacing = 0.0, y_spacing = 0.0, z_spacing = 0.0;
  if (this->SeedRes[0] > 1)
    x_spacing = (double)(bounds.X.Max - bounds.X.Min) / (double)(this->SeedRes[0] - 1);
  if (this->SeedRes[1] > 1)
    y_spacing = (double)(bounds.Y.Max - bounds.Y.Min) / (double)(this->SeedRes[1] - 1);
  if (this->SeedRes[2] > 1)
    z_spacing = (double)(bounds.Z.Max - bounds.Z.Min) / (double)(this->SeedRes[2] - 1);
  // Divide by zero handling for 2D data set. How is this handled

  BasisParticles.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
  BasisParticlesValidity.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);

  auto portal1 = BasisParticles.GetPortalControl();
  auto portal2 = BasisParticlesValidity.GetPortalControl();

  svtkm::Id count = 0, id = 0;
  for (int x = 0; x < this->SeedRes[0]; x++)
  {
    svtkm::FloatDefault xi = static_cast<svtkm::FloatDefault>(x * x_spacing);
    for (int y = 0; y < this->SeedRes[1]; y++)
    {
      svtkm::FloatDefault yi = static_cast<svtkm::FloatDefault>(y * y_spacing);
      for (int z = 0; z < this->SeedRes[2]; z++)
      {
        svtkm::FloatDefault zi = static_cast<svtkm::FloatDefault>(z * z_spacing);
        portal1.Set(count,
                    svtkm::Particle(Vec3f(static_cast<svtkm::FloatDefault>(bounds.X.Min) + xi,
                                         static_cast<svtkm::FloatDefault>(bounds.Y.Min) + yi,
                                         static_cast<svtkm::FloatDefault>(bounds.Z.Min) + zi),
                                   id));
        portal2.Set(count, 1);
        count++;
        id++;
      }
    }
  }
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet Lagrangian::DoExecute(
  const svtkm::cont::DataSet& input,
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMeta,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{

  if (cycle == 0)
  {
    InitializeUniformSeeds(input);
    BasisParticlesOriginal.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
    svtkm::cont::ArrayCopy(BasisParticles, BasisParticlesOriginal);
  }

  if (!fieldMeta.IsPointField())
  {
    throw svtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->writeFrequency == 0)
  {
    throw svtkm::cont::ErrorFilterExecution(
      "Write frequency can not be 0. Use SetWriteFrequency().");
  }
  svtkm::cont::ArrayHandle<svtkm::Particle> basisParticleArray;
  svtkm::cont::ArrayCopy(BasisParticles, basisParticleArray);

  cycle += 1;
  const svtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const svtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  svtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  using FieldHandle = svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, StorageType>;
  using GridEvalType = svtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = svtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  svtkm::worklet::ParticleAdvection particleadvection;
  svtkm::worklet::ParticleAdvectionResult res;

  GridEvalType gridEval(coords, cells, field);
  RK4Type rk4(gridEval, static_cast<svtkm::Float32>(this->stepSize));

  res = particleadvection.Run(rk4, basisParticleArray, 1); // Taking a single step

  auto particles = res.Particles;
  auto particlePortal = particles.GetPortalControl();

  auto start_position = BasisParticlesOriginal.GetPortalControl();
  auto portal_validity = BasisParticlesValidity.GetPortalControl();

  svtkm::cont::DataSet outputData;
  svtkm::cont::DataSetBuilderExplicit dataSetBuilder;

  if (cycle % this->writeFrequency == 0)
  {
    int connectivity_index = 0;
    std::vector<svtkm::Id> connectivity;
    std::vector<svtkm::Vec<T, 3>> pointCoordinates;
    std::vector<svtkm::UInt8> shapes;
    std::vector<svtkm::IdComponent> numIndices;

    for (svtkm::Id index = 0; index < particlePortal.GetNumberOfValues(); index++)
    {
      auto start_point = start_position.Get(index);
      auto end_point = particlePortal.Get(index).Pos;
      auto steps = particlePortal.Get(index).NumSteps;

      if (steps > 0 && portal_validity.Get(index) == 1)
      {
        if (bounds.Contains(end_point))
        {
          connectivity.push_back(connectivity_index);
          connectivity.push_back(connectivity_index + 1);
          connectivity_index += 2;
          pointCoordinates.push_back(
            svtkm::Vec3f(static_cast<svtkm::FloatDefault>(start_point.Pos[0]),
                        static_cast<svtkm::FloatDefault>(start_point.Pos[1]),
                        static_cast<svtkm::FloatDefault>(start_point.Pos[2])));
          pointCoordinates.push_back(svtkm::Vec3f(static_cast<svtkm::FloatDefault>(end_point[0]),
                                                 static_cast<svtkm::FloatDefault>(end_point[1]),
                                                 static_cast<svtkm::FloatDefault>(end_point[2])));
          shapes.push_back(svtkm::CELL_SHAPE_LINE);
          numIndices.push_back(2);
        }
        else
        {
          portal_validity.Set(index, 0);
        }
      }
      else
      {
        portal_validity.Set(index, 0);
      }
    }

    outputData = dataSetBuilder.Create(pointCoordinates, shapes, numIndices, connectivity);
    std::stringstream file_path;
    file_path << "output/basisflows_" << this->rank << "_";
    auto f_path = file_path.str();
    WriteDataSet(cycle, f_path, outputData);
    if (this->resetParticles)
    {
      InitializeUniformSeeds(input);
      BasisParticlesOriginal.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
      svtkm::cont::ArrayCopy(BasisParticles, BasisParticlesOriginal);
    }
    else
    {
      svtkm::cont::ArrayCopy(particles, BasisParticles);
    }
  }
  else
  {
    ValidityCheck check(bounds);
    this->Invoke(check, particles, BasisParticlesValidity);
    svtkm::cont::ArrayCopy(particles, BasisParticles);
  }

  return outputData;
}

//---------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT bool Lagrangian::DoMapField(svtkm::cont::DataSet&,
                                             const svtkm::cont::ArrayHandle<T, StorageType>&,
                                             const svtkm::filter::FieldMetadata&,
                                             const svtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace svtkm::filter

#endif
