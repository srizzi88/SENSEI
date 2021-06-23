/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockPLOT3DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <sstream>

#include "svtkMultiBlockPLOT3DReader.h"

#include "svtkByteSwap.h"
#include "svtkCellData.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDoubleArray.h"
#include "svtkDummyController.h"
#include "svtkErrorCode.h"
#include "svtkExtentTranslator.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkUnsignedCharArray.h"
#include <svtksys/SystemTools.hxx>

#include "svtkMultiBlockPLOT3DReaderInternals.h"
#include "svtkSMPTools.h"

#include <vector>

svtkObjectFactoryNewMacro(svtkMultiBlockPLOT3DReader);
svtkInformationKeyMacro(svtkMultiBlockPLOT3DReader, INTERMEDIATE_RESULT, Integer);
svtkCxxSetObjectMacro(svtkMultiBlockPLOT3DReader, Controller, svtkMultiProcessController);

namespace
{
// helper class used to keep a FILE handle to close when the instance goes out
// of scope.
class svtkPlot3DCFile
{
  FILE* Handle;
  bool CloseOnDelete;

public:
  svtkPlot3DCFile(FILE* handle = nullptr)
    : Handle(handle)
    , CloseOnDelete(true)
  {
  }
  ~svtkPlot3DCFile()
  {
    if (this->Handle && this->CloseOnDelete)
    {
      fclose(this->Handle);
    }
  }
  operator FILE*&() { return this->Handle; }
  // This may be needed to tell svtkPlot3DCFile not to close on delete, instead
  // we're taking over the calling close on the file.
  void DisableClose() { this->CloseOnDelete = false; }
};
}

namespace Functors
{
class ComputeFunctor
{
public:
  svtkMultiBlockPLOT3DReader* Reader = nullptr;
  svtkStructuredGrid* Grid = nullptr;
  svtkDataArray* Result = nullptr; // The target data of the computations

  // I made these members because they are not computed and frequently required in the computations
  svtkDataArray* Density = nullptr;
  svtkDataArray* Momentum = nullptr;
  svtkDataArray* Energy = nullptr;
  svtkDataArray* Gamma = nullptr;
  svtkDataArray* Properties = nullptr;

  ComputeFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : Reader(reader)
    , Grid(grid)
  {
    svtkPointData* pd = grid->GetPointData();
    svtkFieldData* fd = grid->GetFieldData();
    Density = pd->GetArray("Density");
    Momentum = pd->GetArray("Momentum");
    Energy = pd->GetArray("StagnationEnergy");
    Gamma = pd->GetArray("Gamma");
    Properties = fd->GetArray("Properties");
  }

  virtual void operator()(svtkIdType, svtkIdType) {}
  virtual ~ComputeFunctor() = default;

protected:
  // Check if the dependent data are existing, if it is missing and computable, then compute it,
  // otherwise return nullptr Compute the target data with number of components specified
  svtkDataArray* Execute(
    const std::vector<std::string>& dependencies, const std::string& target, svtkIdType numComps = 1)
  {
    std::string message;
    //  Check that the required data is available
    //
    svtkPointData* pd = Grid->GetPointData();
    svtkFieldData* fd = Grid->GetFieldData();
    if ((Result = pd->GetArray(target.c_str())))
    {
      // already computed.
      return Result;
    }
    // Check the dependencies
    for (auto& val : dependencies)
    {
      // Some variables depend on other variables that should be computed, rather than read
      // Some variables require points
      if (val == "Velocity")
      {
        Reader->ComputeVelocity(Grid);
      }
      else if (val == "Vorticity")
      {
        Reader->ComputeVorticity(Grid);
      }
      else if (val == "Pressure")
      {
        Reader->ComputePressure(Grid);
      }
      else if ((val == "Points" && !Grid->GetPoints()) ||
        (val != "Points" && !pd->GetArray(val.c_str()) && !fd->GetArray(val.c_str())))
      {
        message = "Cannot compute ";
        message = message + target;
        svtkErrorWithObjectMacro(nullptr, << message);
        return nullptr;
      }
    }
    // Allocate memory for the target
    svtkIdType numPts = pd->GetArray(dependencies[0].c_str())->GetNumberOfTuples();
    Result = Reader->NewFloatArray();
    Result->SetNumberOfComponents(numComps);
    Result->SetNumberOfTuples(numPts);
    // Compute
    svtkSMPTools::For(0, numPts, *this);
    // Set name to the result and attach it to the grid
    Result->SetName(target.c_str());
    pd->AddArray(Result);
    // Clean up
    Result->GetInformation()->Set(Reader->INTERMEDIATE_RESULT(), 1);
    Result->Delete();
    message = "Created ";
    message = message + target;
    svtkDebugWithObjectMacro(Reader, << message);
    return Result;
  }
};

class ComputeTemperatureFunctor : public ComputeFunctor
{
public:
  ComputeTemperatureFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double e, rr, u, v, w, v2, p, d, rrgas, m[3];
    rrgas = 1.0 / Reader->R;
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      e = Energy->GetComponent(i, 0);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      p = (Reader->GetGamma(i, Gamma) - 1.) * (e - 0.5 * d * v2);
      Result->SetTuple1(i, p * rr * rrgas);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute({ "Density", "Momentum", "StagnationEnergy" }, "Temperature");
  }
};

class ComputePressureFunctor : public ComputeFunctor
{
public:
  ComputePressureFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double e, rr, u, v, w, v2, p, d, m[3];
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      e = Energy->GetComponent(i, 0);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      p = (Reader->GetGamma(i, Gamma) - 1.) * (e - 0.5 * d * v2);
      Result->SetTuple1(i, p);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute({ "Density", "Momentum", "StagnationEnergy" }, "Pressure");
  }
};

class ComputePressureCoefficientFunctor : public ComputeFunctor
{
public:
  ComputePressureCoefficientFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double e, u, v, w, v2, p, d, rr, pc, gi, pi, fsm, den, m[3];
    gi = Properties->GetComponent(0, 4);
    fsm = Properties->GetComponent(0, 0);
    den = .5 * fsm * fsm;
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      e = Energy->GetComponent(i, 0);
      pi = 1.0 / gi;
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      p = (Reader->GetGamma(i, Gamma) - 1.) * (e - 0.5 * d * v2);
      pc = (p - pi) / den;
      Result->SetTuple1(i, pc);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute(
      { "Density", "Momentum", "StagnationEnergy", "Properties" }, "PressureCoefficient");
  }
};

class ComputeMachNumberFunctor : public ComputeFunctor
{
public:
  ComputeMachNumberFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double e, u, v, w, v2, a2, d, g, rr, m[3];
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      e = Energy->GetComponent(i, 0);
      g = Reader->GetGamma(i, Gamma);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      a2 = g * (g - 1.) * (e * rr - .5 * v2);
      Result->SetTuple1(i, sqrt(v2 / a2));
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute({ "Density", "Momentum", "StagnationEnergy" }, "MachNumber");
  }
};

class ComputeSoundSpeedFunctor : public ComputeFunctor
{
public:
  ComputeSoundSpeedFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double e, u, v, w, v2, p, d, g, rr, m[3];
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      e = Energy->GetComponent(i, 0);
      g = Reader->GetGamma(i, Gamma);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      p = (g - 1.) * (e - 0.5 * d * v2);
      Result->SetTuple1(i, sqrt(g * p * rr));
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute({ "Density", "Momentum", "StagnationEnergy" }, "SoundSpeed");
  }
};

class ComputeEnthalpyFunctor : public ComputeFunctor
{
public:
  ComputeEnthalpyFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double e, u, v, w, v2, d, rr, m[3];
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      e = Energy->GetComponent(i, 0);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      Result->SetTuple1(i, Reader->GetGamma(i, Gamma) * (e * rr - 0.5 * v2));
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute({ "Density", "Momentum", "StagnationEnergy" }, "Enthalpy");
  }
};

class ComputeKineticEnergyFunctor : public ComputeFunctor
{
public:
  ComputeKineticEnergyFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double u, v, w, v2, d, rr, m[3];
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      Result->SetTuple1(i, 0.5 * v2);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute({ "Density", "Momentum" }, "KineticEnergy");
  }
};

class ComputeVelocityMagnitudeFunctor : public ComputeFunctor
{
public:
  ComputeVelocityMagnitudeFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double m[3], u, v, w, v2, d, rr;
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      Result->SetTuple1(i, sqrt((double)v2));
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute(
      { "Density", "Momentum", "StagnationEnergy" }, "VelocityMagnitude");
  }
};

class ComputeEntropyFunctor : public ComputeFunctor
{
public:
  ComputeEntropyFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double u, v, w, v2, d, rr, s, p, e, m[3];
    double rhoinf = 1.0;
    double cinf = 1.0;
    double pinf = ((rhoinf * cinf) * (rhoinf * cinf) / Reader->GammaInf);
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      e = Energy->GetComponent(i, 0);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      p = (Reader->GetGamma(i, Gamma) - 1.) * (e - 0.5 * d * v2);
      double cv = Reader->R / (Reader->GetGamma(i, Gamma) - 1.0);
      s = cv * log((p / pinf) / pow(d / rhoinf, Reader->GetGamma(i, Gamma)));
      Result->SetTuple1(i, s);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute({ "Density", "Momentum", "StagnationEnergy" }, "Entropy");
  }
};

class ComputeSwirlFunctor : public ComputeFunctor
{
public:
  ComputeSwirlFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double d, rr, u, v, w, v2, s, m[3], vort[3];
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      svtkDataArray* Vorticity = Grid->GetPointData()->GetArray("Vorticity");
      Vorticity->GetTuple(i, vort);
      rr = 1.0 / d;
      u = m[0] * rr;
      v = m[1] * rr;
      w = m[2] * rr;
      v2 = u * u + v * v + w * w;
      if (v2 != 0.0)
      {
        s = (vort[0] * m[0] + vort[1] * m[1] + vort[2] * m[2]) / v2;
      }
      else
      {
        s = 0.0;
      }
      Result->SetTuple1(i, s);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute(
      { "Density", "Momentum", "StagnationEnergy", "Vorticity" }, "Swirl");
  }
};

class ComputeVelocityFunctor : public ComputeFunctor
{
public:
  ComputeVelocityFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    double m[3], v[3], d, rr;
    for (svtkIdType i = begin; i < end; ++i)
    {
      d = Density->GetComponent(i, 0);
      d = (d != 0.0 ? d : 1.0);
      Momentum->GetTuple(i, m);
      rr = 1.0 / d;
      v[0] = m[0] * rr;
      v[1] = m[1] * rr;
      v[2] = m[2] * rr;
      Result->SetTuple(i, v);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute({ "Density", "Momentum", "StagnationEnergy" }, "Velocity", 3);
  }
};

class ComputeVorticityMagnitudeFunctor : public ComputeFunctor
{
public:
  ComputeVorticityMagnitudeFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    svtkDataArray* Vorticity = Grid->GetPointData()->GetArray("Vorticity");
    double vort[3];
    for (svtkIdType i = begin; i < end; ++i)
    {
      Vorticity->GetTuple(i, vort);
      double magnitude = sqrt(vort[0] * vort[0] + vort[1] * vort[1] + vort[2] * vort[2]);
      Result->SetTuple1(i, magnitude);
    }
  }

  svtkDataArray* Execute() { return ComputeFunctor::Execute({ "Vorticity" }, "VorticityMagnitude"); }
};

class ComputePressureGradientFunctor : public ComputeFunctor
{
public:
  ComputePressureGradientFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    int dims[3], ijsize;
    int i, j, k, idx, idx2, ii, temp;
    double g[3], xp[3], xm[3], pp, pm, factor;
    double xxi, yxi, zxi, pxi;
    double xeta, yeta, zeta, peta;
    double xzeta, yzeta, zzeta, pzeta;
    double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;

    svtkPoints* points = Grid->GetPoints();
    svtkDataArray* pressure = Grid->GetPointData()->GetArray("Pressure");
    Grid->GetDimensions(dims);

    ijsize = dims[0] * dims[1];

    for (svtkIdType n = begin; n < end; ++n)
    {
      // Decompose the global counter n into i, j, k components
      // i + j*dims[0] + k*dims[0]*dims[1] = n
      i = n % dims[0];
      temp = (n - i) / dims[0];
      j = temp % dims[1];
      k = (temp - j) / dims[1];
      //  Xi derivatives.
      if (dims[0] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          xp[ii] = xm[ii] = 0.0;
        }
        xp[0] = 1.0;
        pp = pm = 0.0;
      }
      else if (i == 0)
      {
        factor = 1.0;
        idx = (i + 1) + j * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }
      else if (i == (dims[0] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i - 1 + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }
      else
      {
        factor = 0.5;
        idx = (i + 1) + j * dims[0] + k * ijsize;
        idx2 = (i - 1) + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }

      xxi = factor * (xp[0] - xm[0]);
      yxi = factor * (xp[1] - xm[1]);
      zxi = factor * (xp[2] - xm[2]);
      pxi = factor * (pp - pm);

      //  Eta derivatives.
      if (dims[1] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          xp[ii] = xm[ii] = 0.0;
        }
        xp[1] = 1.0;
        pp = pm = 0.0;
      }
      else if (j == 0)
      {
        factor = 1.0;
        idx = i + (j + 1) * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }
      else if (j == (dims[1] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i + (j - 1) * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }
      else
      {
        factor = 0.5;
        idx = i + (j + 1) * dims[0] + k * ijsize;
        idx2 = i + (j - 1) * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }

      xeta = factor * (xp[0] - xm[0]);
      yeta = factor * (xp[1] - xm[1]);
      zeta = factor * (xp[2] - xm[2]);
      peta = factor * (pp - pm);

      //  Zeta derivatives.
      if (dims[2] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          xp[ii] = xm[ii] = 0.0;
        }
        xp[2] = 1.0;
        pp = pm = 0.0;
      }
      else if (k == 0)
      {
        factor = 1.0;
        idx = i + j * dims[0] + (k + 1) * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }
      else if (k == (dims[2] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + (k - 1) * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }
      else
      {
        factor = 0.5;
        idx = i + j * dims[0] + (k + 1) * ijsize;
        idx2 = i + j * dims[0] + (k - 1) * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        pp = pressure->GetComponent(idx, 0);
        pm = pressure->GetComponent(idx2, 0);
      }

      xzeta = factor * (xp[0] - xm[0]);
      yzeta = factor * (xp[1] - xm[1]);
      zzeta = factor * (xp[2] - xm[2]);
      pzeta = factor * (pp - pm);

      //  Now calculate the Jacobian.  Grids occasionally have
      //  singularities, or points where the Jacobian is infinite (the
      //  inverse is zero).  For these cases, we'll set the Jacobian to
      //  zero, which will result in a zero vorticity.
      //
      aj = xxi * yeta * zzeta + yxi * zeta * xzeta + zxi * xeta * yzeta - zxi * yeta * xzeta -
        yxi * xeta * zzeta - xxi * zeta * yzeta;
      if (aj != 0.0)
      {
        aj = 1. / aj;
      }

      //  Xi metrics.
      xix = aj * (yeta * zzeta - zeta * yzeta);
      xiy = -aj * (xeta * zzeta - zeta * xzeta);
      xiz = aj * (xeta * yzeta - yeta * xzeta);

      //  Eta metrics.
      etax = -aj * (yxi * zzeta - zxi * yzeta);
      etay = aj * (xxi * zzeta - zxi * xzeta);
      etaz = -aj * (xxi * yzeta - yxi * xzeta);

      //  Zeta metrics.
      zetax = aj * (yxi * zeta - zxi * yeta);
      zetay = -aj * (xxi * zeta - zxi * xeta);
      zetaz = aj * (xxi * yeta - yxi * xeta);

      //  Finally, the vorticity components.
      g[0] = xix * pxi + etax * peta + zetax * pzeta;
      g[1] = xiy * pxi + etay * peta + zetay * pzeta;
      g[2] = xiz * pxi + etaz * peta + zetaz * pzeta;

      idx = i + j * dims[0] + k * ijsize;
      Result->SetTuple(idx, g);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute(
      { "Density", "Momentum", "StagnationEnergy", "Points", "Pressure" }, "PressureGradient", 3);
  }
};

class ComputeVorticityFunctor : public ComputeFunctor
{
public:
  ComputeVorticityFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    int dims[3], ijsize;
    int i, j, k, idx, idx2, ii, temp;
    double vort[3], xp[3], xm[3], vp[3], vm[3], factor;
    double xxi, yxi, zxi, uxi, vxi, wxi;
    double xeta, yeta, zeta, ueta, veta, weta;
    double xzeta, yzeta, zzeta, uzeta, vzeta, wzeta;
    double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;

    svtkPoints* points = Grid->GetPoints();
    svtkDataArray* velocity = Grid->GetPointData()->GetArray("Velocity");

    Grid->GetDimensions(dims);
    ijsize = dims[0] * dims[1];

    for (svtkIdType n = begin; n < end; ++n)
    {
      // Decompose the global counter n into i, j, k components
      // i + j*dims[0] + k*dims[0]*dims[1] = n
      i = n % dims[0];
      temp = (n - i) / dims[0];
      j = temp % dims[1];
      k = (temp - j) / dims[1];
      //  Xi derivatives.
      if (dims[0] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
        }
        xp[0] = 1.0;
      }
      else if (i == 0)
      {
        factor = 1.0;
        idx = (i + 1) + j * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else if (i == (dims[0] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i - 1 + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else
      {
        factor = 0.5;
        idx = (i + 1) + j * dims[0] + k * ijsize;
        idx2 = (i - 1) + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }

      xxi = factor * (xp[0] - xm[0]);
      yxi = factor * (xp[1] - xm[1]);
      zxi = factor * (xp[2] - xm[2]);
      uxi = factor * (vp[0] - vm[0]);
      vxi = factor * (vp[1] - vm[1]);
      wxi = factor * (vp[2] - vm[2]);

      //  Eta derivatives.
      if (dims[1] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
        }
        xp[1] = 1.0;
      }
      else if (j == 0)
      {
        factor = 1.0;
        idx = i + (j + 1) * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else if (j == (dims[1] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i + (j - 1) * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else
      {
        factor = 0.5;
        idx = i + (j + 1) * dims[0] + k * ijsize;
        idx2 = i + (j - 1) * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }

      xeta = factor * (xp[0] - xm[0]);
      yeta = factor * (xp[1] - xm[1]);
      zeta = factor * (xp[2] - xm[2]);
      ueta = factor * (vp[0] - vm[0]);
      veta = factor * (vp[1] - vm[1]);
      weta = factor * (vp[2] - vm[2]);

      //  Zeta derivatives.
      if (dims[2] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
        }
        xp[2] = 1.0;
      }
      else if (k == 0)
      {
        factor = 1.0;
        idx = i + j * dims[0] + (k + 1) * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else if (k == (dims[2] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + (k - 1) * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else
      {
        factor = 0.5;
        idx = i + j * dims[0] + (k + 1) * ijsize;
        idx2 = i + j * dims[0] + (k - 1) * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }

      xzeta = factor * (xp[0] - xm[0]);
      yzeta = factor * (xp[1] - xm[1]);
      zzeta = factor * (xp[2] - xm[2]);
      uzeta = factor * (vp[0] - vm[0]);
      vzeta = factor * (vp[1] - vm[1]);
      wzeta = factor * (vp[2] - vm[2]);

      // Now calculate the Jacobian.  Grids occasionally have
      // singularities, or points where the Jacobian is infinite (the
      // inverse is zero).  For these cases, we'll set the Jacobian to
      // zero, which will result in a zero vorticity.
      //
      aj = xxi * yeta * zzeta + yxi * zeta * xzeta + zxi * xeta * yzeta - zxi * yeta * xzeta -
        yxi * xeta * zzeta - xxi * zeta * yzeta;
      if (aj != 0.0)
      {
        aj = 1. / aj;
      }

      //  Xi metrics.
      xix = aj * (yeta * zzeta - zeta * yzeta);
      xiy = -aj * (xeta * zzeta - zeta * xzeta);
      xiz = aj * (xeta * yzeta - yeta * xzeta);

      //  Eta metrics.
      etax = -aj * (yxi * zzeta - zxi * yzeta);
      etay = aj * (xxi * zzeta - zxi * xzeta);
      etaz = -aj * (xxi * yzeta - yxi * xzeta);

      //  Zeta metrics.
      zetax = aj * (yxi * zeta - zxi * yeta);
      zetay = -aj * (xxi * zeta - zxi * xeta);
      zetaz = aj * (xxi * yeta - yxi * xeta);

      //  Finally, the vorticity components.
      //
      vort[0] = xiy * wxi + etay * weta + zetay * wzeta - xiz * vxi - etaz * veta - zetaz * vzeta;
      vort[1] = xiz * uxi + etaz * ueta + zetaz * uzeta - xix * wxi - etax * weta - zetax * wzeta;
      vort[2] = xix * vxi + etax * veta + zetax * vzeta - xiy * uxi - etay * ueta - zetay * uzeta;
      idx = i + j * dims[0] + k * ijsize;
      Result->SetTuple(idx, vort);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute(
      { "Density", "Momentum", "StagnationEnergy", "Points", "Velocity" }, "Vorticity", 3);
  }
};

class ComputeStrainRateFunctor : public ComputeFunctor
{
public:
  ComputeStrainRateFunctor(svtkMultiBlockPLOT3DReader* reader, svtkStructuredGrid* grid)
    : ComputeFunctor(reader, grid)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end) override
  {
    int dims[3], ijsize;
    int i, j, k, idx, idx2, ii, temp;
    double stRate[3], xp[3], xm[3], vp[3], vm[3], factor;
    double xxi, yxi, zxi, uxi, vxi, wxi;
    double xeta, yeta, zeta, ueta, veta, weta;
    double xzeta, yzeta, zzeta, uzeta, vzeta, wzeta;
    double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;

    svtkPoints* points = Grid->GetPoints();
    svtkDataArray* velocity = Grid->GetPointData()->GetArray("Velocity");

    Grid->GetDimensions(dims);
    ijsize = dims[0] * dims[1];

    for (svtkIdType n = begin; n < end; ++n)
    {
      // Decompose the global counter n into i, j, k components
      // i + j*dims[0] + k*dims[0]*dims[1] = n
      i = n % dims[0];
      temp = (n - i) / dims[0];
      j = temp % dims[1];
      k = (temp - j) / dims[1];
      //  Xi derivatives.
      if (dims[0] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
        }
        xp[0] = 1.0;
      }
      else if (i == 0)
      {
        factor = 1.0;
        idx = (i + 1) + j * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else if (i == (dims[0] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i - 1 + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else
      {
        factor = 0.5;
        idx = (i + 1) + j * dims[0] + k * ijsize;
        idx2 = (i - 1) + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }

      xxi = factor * (xp[0] - xm[0]);
      yxi = factor * (xp[1] - xm[1]);
      zxi = factor * (xp[2] - xm[2]);
      uxi = factor * (vp[0] - vm[0]);
      vxi = factor * (vp[1] - vm[1]);
      wxi = factor * (vp[2] - vm[2]);

      //  Eta derivatives.
      if (dims[1] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
        }
        xp[1] = 1.0;
      }
      else if (j == 0)
      {
        factor = 1.0;
        idx = i + (j + 1) * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else if (j == (dims[1] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i + (j - 1) * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else
      {
        factor = 0.5;
        idx = i + (j + 1) * dims[0] + k * ijsize;
        idx2 = i + (j - 1) * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }

      xeta = factor * (xp[0] - xm[0]);
      yeta = factor * (xp[1] - xm[1]);
      zeta = factor * (xp[2] - xm[2]);
      ueta = factor * (vp[0] - vm[0]);
      veta = factor * (vp[1] - vm[1]);
      weta = factor * (vp[2] - vm[2]);

      //  Zeta derivatives.
      if (dims[2] == 1) // 2D in this direction
      {
        factor = 1.0;
        for (ii = 0; ii < 3; ii++)
        {
          vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
        }
        xp[2] = 1.0;
      }
      else if (k == 0)
      {
        factor = 1.0;
        idx = i + j * dims[0] + (k + 1) * ijsize;
        idx2 = i + j * dims[0] + k * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else if (k == (dims[2] - 1))
      {
        factor = 1.0;
        idx = i + j * dims[0] + k * ijsize;
        idx2 = i + j * dims[0] + (k - 1) * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }
      else
      {
        factor = 0.5;
        idx = i + j * dims[0] + (k + 1) * ijsize;
        idx2 = i + j * dims[0] + (k - 1) * ijsize;
        points->GetPoint(idx, xp);
        points->GetPoint(idx2, xm);
        velocity->GetTuple(idx, vp);
        velocity->GetTuple(idx2, vm);
      }

      xzeta = factor * (xp[0] - xm[0]);
      yzeta = factor * (xp[1] - xm[1]);
      zzeta = factor * (xp[2] - xm[2]);
      uzeta = factor * (vp[0] - vm[0]);
      vzeta = factor * (vp[1] - vm[1]);
      wzeta = factor * (vp[2] - vm[2]);

      // Now calculate the Jacobian.  Grids occasionally have
      // singularities, or points where the Jacobian is infinite (the
      // inverse is zero).  For these cases, we'll set the Jacobian to
      // zero, which will result in a zero vorticity.
      //
      aj = xxi * yeta * zzeta + yxi * zeta * xzeta + zxi * xeta * yzeta - zxi * yeta * xzeta -
        yxi * xeta * zzeta - xxi * zeta * yzeta;
      if (aj != 0.0)
      {
        aj = 1. / aj;
      }

      //  Xi metrics.
      xix = aj * (yeta * zzeta - zeta * yzeta);
      xiy = -aj * (xeta * zzeta - zeta * xzeta);
      xiz = aj * (xeta * yzeta - yeta * xzeta);

      //  Eta metrics.
      etax = -aj * (yxi * zzeta - zxi * yzeta);
      etay = aj * (xxi * zzeta - zxi * xzeta);
      etaz = -aj * (xxi * yzeta - yxi * xzeta);

      //  Zeta metrics.
      zetax = aj * (yxi * zeta - zxi * yeta);
      zetay = -aj * (xxi * zeta - zxi * xeta);
      zetaz = aj * (xxi * yeta - yxi * xeta);

      //  Finally, the strain rate components.
      //
      stRate[0] = xix * uxi + etax * ueta + zetax * uzeta;
      stRate[1] = xiy * vxi + etay * veta + zetay * vzeta;
      stRate[2] = xiz * wxi + etaz * weta + zetaz * wzeta;
      idx = i + j * dims[0] + k * ijsize;
      Result->SetTuple(idx, stRate);
    }
  }

  svtkDataArray* Execute()
  {
    return ComputeFunctor::Execute(
      { "Density", "Momentum", "Points", "Velocity" }, "StrainRate", 3);
  }
};
}

template <class DataType>
class svtkPLOT3DArrayReader
{
public:
  svtkPLOT3DArrayReader()
    : ByteOrder(svtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN)
  {
  }

  svtkIdType ReadScalar(FILE* fp, svtkIdType preskip, svtkIdType n, svtkIdType postskip,
    DataType* scalar,
    const svtkMultiBlockPLOT3DReaderRecord& record = svtkMultiBlockPLOT3DReaderRecord())
  {
    svtkMultiBlockPLOT3DReaderRecord::SubRecordSeparators separators =
      record.GetSubRecordSeparators(svtk_ftell(fp), preskip);

    svtk_fseek(fp,
      preskip * sizeof(DataType) +
        separators.size() * svtkMultiBlockPLOT3DReaderRecord::SubRecordSeparatorWidth,
      SEEK_CUR);

    // Let's see if we encounter markers while reading the data from current
    // position.
    separators = record.GetSubRecordSeparators(svtk_ftell(fp), sizeof(DataType) * n);

    svtkIdType retVal;
    if (separators.empty())
    {
      // no record separators will be encountered, yay! Just read the block.
      retVal = static_cast<svtkIdType>(fread(scalar, sizeof(DataType), n, fp));
    }
    else
    {
      // need to read in chunks to skip separators.
      svtkTypeUInt64 pos = svtk_ftell(fp);
      std::vector<std::pair<svtkTypeUInt64, svtkTypeUInt64> > chunks =
        record.GetChunksToRead(pos, sizeof(DataType) * n, separators);

      svtkTypeUInt64 bytesread = 0;
      for (size_t cc = 0; cc < chunks.size(); ++cc)
      {
        svtk_fseek(fp, chunks[cc].first, SEEK_SET);
        bytesread += static_cast<svtkTypeUInt64>(
          fread(reinterpret_cast<char*>(scalar) + bytesread, 1, chunks[cc].second, fp));
      }
      retVal = static_cast<svtkIdType>(bytesread / sizeof(DataType));
    }

    // Let's count markers we encounter while postskipping. If any, we'll need
    // to step over them as well.
    separators = record.GetSubRecordSeparators(svtk_ftell(fp), sizeof(DataType) * postskip);
    svtk_fseek(fp,
      postskip * sizeof(DataType) +
        separators.size() * svtkMultiBlockPLOT3DReaderRecord::SubRecordSeparatorWidth,
      SEEK_CUR);
    if (this->ByteOrder == svtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
    {
      if (sizeof(DataType) == 4)
      {
        svtkByteSwap::Swap4LERange(scalar, n);
      }
      else
      {
        svtkByteSwap::Swap8LERange(scalar, n);
      }
    }
    else
    {
      if (sizeof(DataType) == 4)
      {
        svtkByteSwap::Swap4BERange(scalar, n);
      }
      else
      {
        svtkByteSwap::Swap8BERange(scalar, n);
      }
    }
    return retVal;
  }

  svtkIdType ReadVector(FILE* fp, int extent[6], int wextent[6], int numDims, DataType* vector,
    const svtkMultiBlockPLOT3DReaderRecord& record)
  {
    svtkIdType n = svtkStructuredData::GetNumberOfPoints(extent);

    // Setting to 0 in case numDims == 0. We still need to
    // populate an array with 3 components but the code below
    // does not read the 3rd component (it doesn't exist
    // in the file)
    memset(vector, 0, n * 3 * sizeof(DataType));

    svtkIdType retVal = 0;
    std::vector<DataType> buffer(n);
    for (int component = 0; component < numDims; component++)
    {
      svtkIdType preskip, postskip;
      svtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
      retVal += this->ReadScalar(fp, preskip, n, postskip, buffer.data(), record);
      for (svtkIdType i = 0; i < n; i++)
      {
        vector[3 * i + component] = buffer[i];
      }
    }
    return retVal;
  }

  int ByteOrder;
};

svtkMultiBlockPLOT3DReader::svtkMultiBlockPLOT3DReader()
{
  this->XYZFileName = nullptr;
  this->QFileName = nullptr;
  this->FunctionFileName = nullptr;
  this->BinaryFile = 1;
  this->HasByteCount = 0;
  this->TwoDimensionalGeometry = 0;
  this->MultiGrid = 0;
  this->ForceRead = 0;
  this->ByteOrder = FILE_BIG_ENDIAN;
  this->IBlanking = 0;
  this->DoublePrecision = 0;
  this->AutoDetectFormat = 0;

  this->ExecutedGhostLevels = 0;

  this->FileSize = 0;

  this->R = 1.0;
  this->Gamma = 1.4;
  this->GammaInf = this->Gamma;

  this->PreserveIntermediateFunctions = true;

  this->FunctionList = svtkIntArray::New();

  this->ScalarFunctionNumber = -1;
  this->SetScalarFunctionNumber(100);
  this->VectorFunctionNumber = -1;
  this->SetVectorFunctionNumber(202);

  this->Internal = new svtkMultiBlockPLOT3DReaderInternals;
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());

  this->SetNumberOfInputPorts(0);
}

svtkMultiBlockPLOT3DReader::~svtkMultiBlockPLOT3DReader()
{
  delete[] this->XYZFileName;
  delete[] this->FunctionFileName;
  this->FunctionList->Delete();
  this->ClearGeometryCache();

  delete this->Internal;

  this->SetController(nullptr);
}

void svtkMultiBlockPLOT3DReader::SetQFileName(const char* name)
{
  if (this->GetNumberOfFileNames() == 1 && this->GetFileName(0) &&
    strcmp(this->GetFileName(0), name) == 0)
  {
    return;
  }
  this->ClearFileNames();
  this->AddFileName(name);
  this->Modified();
}

const char* svtkMultiBlockPLOT3DReader::GetQFileName()
{
  if (this->GetNumberOfFileNames() < 1)
  {
    return nullptr;
  }
  return this->svtkParallelReader::GetFileName(0);
}

svtkMultiBlockDataSet* svtkMultiBlockPLOT3DReader::GetOutput()
{
  return this->GetOutput(0);
}

svtkMultiBlockDataSet* svtkMultiBlockPLOT3DReader::GetOutput(int port)
{
  svtkDataObject* output = this->GetOutputDataObject(port);
  return svtkMultiBlockDataSet::SafeDownCast(output);
}

double svtkMultiBlockPLOT3DReader::GetTimeValue(const std::string& fname)
{
  int rank = 0;
  // For now, only first rank does any reading.
  if (this->Controller)
  {
    rank = this->Controller->GetLocalProcessId();
  }

  double time = svtkMath::Nan();
  bool hasTime = false;
  int retval = 1;
  if (rank == 0)
  {
    try
    {
      if (this->XYZFileName && this->XYZFileName[0] != '\0' &&
        (this->Internal->NeedToCheckXYZFile || this->Internal->Blocks.empty()))
      {
        svtkPlot3DCFile xyzFp;
        if (this->CheckGeometryFile(xyzFp) != SVTK_OK)
        {
          throw Plot3DException();
        }

        this->CalculateFileSize(xyzFp);

        if (!this->AutoDetectionCheck(xyzFp))
        {
          throw Plot3DException();
        }
        this->Internal->NeedToCheckXYZFile = false;
      }

      // We report time from the Q file for meta-type readers that
      // might support file series of Q files.
      if (!fname.empty())
      {
        svtkPlot3DCFile qFp;
        if (this->CheckFile(qFp, fname.c_str()) != SVTK_OK)
        {
          throw Plot3DException();
        }
        int nq, nqc, overflow;
        if (this->ReadQHeader(qFp, false, nq, nqc, overflow) != SVTK_OK)
        {
          throw Plot3DException();
        }

        // I have seen Plot3D files with bogus time values so the only
        // type I have some confidence about having correct time values
        // is Overflow output.
        if (overflow)
        {
          svtkDataArray* properties = this->NewFloatArray();

          this->SkipByteCount(qFp);
          properties->SetNumberOfTuples(4);

          // Read fsmach, alpha, re, time;
          if (this->ReadValues(qFp, 4, properties) != 4)
          {
            svtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
            properties->Delete();
            throw Plot3DException();
          }
          time = properties->GetTuple1(3);
          hasTime = true;
          properties->Delete();
        }
      }
    }
    catch (Plot3DException&)
    {
      retval = 0;
    }
  }

  if (this->Controller)
  {
    int vals[2] = { retval, (hasTime ? 1 : 0) };
    this->Controller->Broadcast(vals, 2, 0);
    retval = vals[0];
    hasTime = (vals[1] == 1);
  }

  if (!retval)
  {
    return svtkMath::Nan();
  }

  if (hasTime)
  {
    if (this->Controller)
    {
      this->Controller->Broadcast(&time, 1, 0);
    }
    return time;
  }

  return svtkMath::Nan();
}

int svtkMultiBlockPLOT3DReader::ReadMetaData(svtkInformation* metadata)
{
  // This is what performs the auto-detection check.
  // It's called here in case no q file name is set.
  this->GetTimeValue(std::string());

  return this->svtkParallelReader::ReadMetaData(metadata);
}

int svtkMultiBlockPLOT3DReader::ReadMesh(
  int piece, int npieces, int nghosts, int, svtkDataObject* output)
{
  return this->ReadMesh(std::string(), piece, npieces, nghosts, output);
}

int svtkMultiBlockPLOT3DReader::ReadPoints(
  int piece, int npieces, int nghosts, int, svtkDataObject* output)
{
  return this->ReadPoints(std::string(), piece, npieces, nghosts, output);
}

int svtkMultiBlockPLOT3DReader::ReadArrays(
  int piece, int npieces, int nghosts, int timestep, svtkDataObject* output)
{
  if (this->GetNumberOfFileNames() > 0)
  {
    this->CurrentFileIndex = timestep;
    if (this->svtkParallelReader::ReadArrays(piece, npieces, nghosts, timestep, output))
    {
      this->CurrentFileIndex = timestep;
    }
    else
    {
      return 0;
    }
  }
  else if (this->FunctionFileName)
  {
    this->ReadArrays(std::string(), piece, npieces, nghosts, output);
  }
  // If no q filename is set, do nothing.
  return 1;
}

int svtkMultiBlockPLOT3DReader::ReadMesh(
  const std::string&, int /*piece*/, int npieces, int nghosts, svtkDataObject* output)
{
  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::SafeDownCast(output);
  if (!mb)
  {
    this->ClearGeometryCache();
    return 0;
  }

  int igl = nghosts;
  if (npieces > 1)
  {
    if (igl == 0)
    {
      igl = 1;
    }
    mb->GetInformation()->Set(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), igl);
  }

  if (igl > this->ExecutedGhostLevels)
  {
    this->ClearGeometryCache();
  }

  this->SetErrorCode(svtkErrorCode::NoError);

  // This may be wrong if geometry is not cached. It is
  // updated below.
  int numBlocks = static_cast<int>(this->Internal->Blocks.size());

  svtkSmartPointer<svtkMultiProcessController> mp;
  if (this->Controller)
  {
    mp = this->Controller;
  }
  else
  {
    mp.TakeReference(svtkDummyController::New());
  }

  int rank = mp->GetLocalProcessId();
  int size = mp->GetNumberOfProcesses();
  int realSize = size;

  int* settings = reinterpret_cast<int*>(&this->Internal->Settings);
  mp->Broadcast(
    settings, sizeof(svtkMultiBlockPLOT3DReaderInternals::InternalSettings) / sizeof(int), 0);

  // Special case where we are reading an ASCII or
  // 2D file in parallel. All the work is done by
  // rank 0 but we still need to communicate numBlocks
  // for other ranks to allocate output with the right
  // shape.
  if (!this->Internal->Settings.BinaryFile || this->Internal->Settings.NumberOfDimensions == 2)
  {
    if (rank > 0)
    {
      this->Controller->Broadcast(&numBlocks, 1, 0);
      mb->SetNumberOfBlocks(numBlocks);
      this->ClearGeometryCache();
      return 1;
    }
    else
    {
      mp.TakeReference(svtkDummyController::New());
      rank = 0;
      size = 1;
    }
  }

  // Before we start reading, if we are using cached datasets, we
  // need to make sure we release all field arrays otherwise we may end up
  // with obsolete arrays (paraview/paraview#17467).
  for (auto biter = this->Internal->Blocks.begin(); biter != this->Internal->Blocks.end(); ++biter)
  {
    if (svtkStructuredGrid* grid = biter->GetPointer())
    {
      // preserve ghost and blanking arrays since those are read from geometry
      // file and not reread if using cache.
      svtkSmartPointer<svtkDataArray> pIBlank = grid->GetPointData()->GetArray("IBlank");
      svtkSmartPointer<svtkDataArray> cGhostArray =
        grid->GetCellData()->GetArray(svtkDataSetAttributes::GhostArrayName());
      // initialize
      grid->GetCellData()->Initialize();
      grid->GetPointData()->Initialize();
      grid->GetFieldData()->Initialize();
      // restore
      if (pIBlank)
      {
        grid->GetPointData()->AddArray(pIBlank);
      }
      if (cGhostArray)
      {
        grid->GetCellData()->AddArray(cGhostArray);
      }
    }
  }

  svtkNew<svtkExtentTranslator> et;
  et->SetPiece(rank);
  et->SetNumberOfPieces(size);
  et->SetSplitModeToZSlab();

  svtkPlot3DCFile xyzFp(nullptr);

  // Don't read the geometry if we already have it!
  if (numBlocks == 0)
  {
    this->ExecutedGhostLevels = igl;

    svtkTypeUInt64 offset = 0;

    int error = 0;

    // Only the first rank does meta-data checking
    // using POSIX IO.
    if (rank == 0)
    {
      try
      {
        if (this->CheckGeometryFile(xyzFp) != SVTK_OK)
        {
          throw Plot3DException();
        }

        if (this->ReadGeometryHeader(xyzFp) != SVTK_OK)
        {
          svtkErrorMacro("Error reading geometry file.");
          throw Plot3DException();
        }

        // Update from the value in the file.
        numBlocks = static_cast<int>(this->Internal->Dimensions.size());

        if (this->Internal->Settings.BinaryFile)
        {
          offset = svtk_ftell(xyzFp);
        }
      }
      catch (Plot3DException&)
      {
        error = 1;
      }
    }

    mp->Broadcast(&error, 1, 0);
    if (error)
    {
      svtkErrorMacro("Error reading file " << this->XYZFileName);
      this->ClearGeometryCache();
      return 0;
    }

    // All meta-data needs to be broadcasted.
    mp->Broadcast(&numBlocks, 1, 0);
    if (rank > 0)
    {
      this->Internal->Dimensions.resize(numBlocks);
    }

    int* rawdims = reinterpret_cast<int*>(&this->Internal->Dimensions[0]);
    mp->Broadcast(rawdims, 3 * numBlocks, 0);

    mp->Broadcast(&offset, 1, 0);

    // Heavy reading is done collectively. POSIX in this
    // class but MPI-IO in subclass.
    void* xyzFp2;
    if (this->Internal->Settings.BinaryFile)
    {
      this->OpenFileForDataRead(xyzFp2, this->XYZFileName);
    }
    else
    {
      // For ASCII files, the first rank keeps reading without
      // worrying about offsets and such.
      xyzFp2 = xyzFp;
      xyzFp.DisableClose();
    }

    this->Internal->Blocks.resize(numBlocks);

    for (int i = 0; i < numBlocks; i++)
    {
      //**************** RECORD START *********************************
      // precond: offset is at start of a record in the file.
      svtkMultiBlockPLOT3DReaderRecord record;
      if (!record.Initialize(xyzFp, offset, this->Internal->Settings, this->Controller))
      {
        svtkErrorMacro("Encountered premature end-of-file while reading "
                      "the geometry file (or the file is corrupt).");
        this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(xyzFp2);
        this->ClearGeometryCache();
        return 0;
      }

      // we now have determined how many (sub)records are part of this block.
      assert(record.AtStart(offset));

      offset += this->GetByteCountSize();

      // Read the geometry of this grid.

      int* dims = this->Internal->Dimensions[i].Values;
      int wextent[6] = { 0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1 };
      int extent[6];
      et->SetWholeExtent(wextent);
      et->SetGhostLevel(igl);
      et->PieceToExtent();
      et->GetExtent(extent);

      svtkStructuredGrid* nthOutput = this->Internal->Blocks[i];
      if (!nthOutput)
      {
        nthOutput = svtkStructuredGrid::New();
        this->Internal->Blocks[i] = nthOutput;
        nthOutput->SetExtent(extent);
        nthOutput->Delete();
      }

      svtkDataArray* pointArray = this->NewFloatArray();
      pointArray->SetNumberOfComponents(3);
      svtkIdType npts = svtkStructuredData::GetNumberOfPoints(extent);
      svtkIdType nTotalPts = (svtkIdType)dims[0] * dims[1] * dims[2];
      pointArray->SetNumberOfTuples(npts);

      svtkPoints* points = svtkPoints::New();
      points->SetData(pointArray);
      pointArray->Delete();
      nthOutput->SetPoints(points);
      points->Delete();
      if (this->ReadVector(xyzFp2, extent, wextent, this->Internal->Settings.NumberOfDimensions,
            pointArray, offset, record) == 0)
      {
        svtkErrorMacro("Encountered premature end-of-file while reading "
                      "the geometry file (or the file is corrupt).");
        this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(xyzFp2);
        this->ClearGeometryCache();
        return 0;
      }
      // Increment the offset for next read. This points to the
      // beginning of next block.
      offset += record.GetLengthWithSeparators(offset,
        this->Internal->Settings.NumberOfDimensions * nTotalPts *
          this->Internal->Settings.Precision);

      if (this->Internal->Settings.IBlanking)
      {
        svtkIntArray* iblank = svtkIntArray::New();
        iblank->SetName("IBlank");
        iblank->SetNumberOfTuples(npts);
        if (this->ReadIntScalar(xyzFp2, extent, wextent, iblank, offset, record) == 0)
        {
          svtkErrorMacro("Encountered premature end-of-file while reading "
                        "the xyz file (or the file is corrupt).");
          this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
          this->CloseFile(xyzFp2);
          this->ClearGeometryCache();
          return 0;
        }

        int* ib = iblank->GetPointer(0);
        nthOutput->GetPointData()->AddArray(iblank);
        iblank->Delete();
        offset += record.GetLengthWithSeparators(offset, nTotalPts * sizeof(int));

        svtkUnsignedCharArray* ghosts = svtkUnsignedCharArray::New();
        ghosts->SetNumberOfValues(nthOutput->GetNumberOfCells());
        ghosts->SetName(svtkDataSetAttributes::GhostArrayName());
        svtkIdList* ids = svtkIdList::New();
        ids->SetNumberOfIds(8);
        svtkIdType numCells = nthOutput->GetNumberOfCells();
        for (svtkIdType cellId = 0; cellId < numCells; cellId++)
        {
          nthOutput->GetCellPoints(cellId, ids);
          svtkIdType numIds = ids->GetNumberOfIds();
          unsigned char value = 0;
          for (svtkIdType ptIdx = 0; ptIdx < numIds; ptIdx++)
          {
            if (ib[ids->GetId(ptIdx)] == 0)
            {
              value |= svtkDataSetAttributes::HIDDENCELL;
              break;
            }
          }
          ghosts->SetValue(cellId, value);
        }
        ids->Delete();
        nthOutput->GetCellData()->AddArray(ghosts);
        ghosts->Delete();
      }

      if (igl > 0)
      {
        et->SetGhostLevel(0);
        et->PieceToExtent();
        int zeroExtent[6];
        et->GetExtent(zeroExtent);
        nthOutput->GenerateGhostArray(zeroExtent, true);
      }

      offset += this->GetByteCountSize();
      assert(record.AtEnd(offset));
      //**************** RECORD END *********************************
    }

    this->CloseFile(xyzFp2);
  }

  // Special case where we are reading an ASCII or
  // 2D file in parallel. All the work is done by
  // rank 0 but we still need to communicate numBlocks
  // for other ranks to allocate output with the right
  // shape.
  if (!this->Internal->Settings.BinaryFile || this->Internal->Settings.NumberOfDimensions == 2)
  {
    if (realSize > 1)
    {
      // This needs to broadcast with this->Controller
      // because mp is a dummy controller.
      this->Controller->Broadcast(&numBlocks, 1, 0);
    }
  }

  mb->SetNumberOfBlocks(numBlocks);
  for (int i = 0; i < numBlocks; i++)
  {
    svtkStructuredGrid* nthOutput = this->Internal->Blocks[i];
    mb->SetBlock(i, nthOutput);
  }

  return 1;
}

int svtkMultiBlockPLOT3DReader::ReadPoints(const std::string&, int, int, int, svtkDataObject*)
{
  return 1;
}

int svtkMultiBlockPLOT3DReader::ReadArrays(
  const std::string& fname, int /*piece*/, int npieces, int nghosts, svtkDataObject* output)
{
  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::SafeDownCast(output);
  if (!mb)
  {
    this->ClearGeometryCache();
    return 0;
  }

  int igl = nghosts;
  if (npieces > 1)
  {
    if (igl == 0)
    {
      igl = 1;
    }
    mb->GetInformation()->Set(svtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), igl);
  }

  int numBlocks = mb->GetNumberOfBlocks();

  svtkSmartPointer<svtkMultiProcessController> mp;
  if (this->Controller)
  {
    mp = this->Controller;
  }
  else
  {
    mp.TakeReference(svtkDummyController::New());
  }

  int rank = mp->GetLocalProcessId();
  int size = mp->GetNumberOfProcesses();

  svtkNew<svtkExtentTranslator> et;
  et->SetPiece(rank);
  et->SetNumberOfPieces(size);
  et->SetSplitModeToZSlab();

  // Now read the solution.
  if (!fname.empty())
  {
    svtkPlot3DCFile qFp(nullptr);
    int nq = 0, nqc = 0, isOverflow = 0;

    int error = 0;
    if (rank == 0)
    {
      try
      {
        if (this->CheckFile(qFp, fname.c_str()) != SVTK_OK)
        {
          throw Plot3DException();
        }

        if (this->ReadQHeader(qFp, true, nq, nqc, isOverflow) != SVTK_OK)
        {
          throw Plot3DException();
        }
      }
      catch (Plot3DException&)
      {
        error = 1;
      }
    }

    mp->Broadcast(&error, 1, 0);
    if (error)
    {
      svtkErrorMacro("Error reading file " << this->XYZFileName);
      this->ClearGeometryCache();
      return 0;
    }

    int vals[3] = { nq, nqc, isOverflow };
    mp->Broadcast(vals, 3, 0);
    nq = vals[0];
    nqc = vals[1];
    isOverflow = vals[2];

    svtkTypeUInt64 offset = 0;

    void* qFp2;
    if (this->Internal->Settings.BinaryFile)
    {
      this->OpenFileForDataRead(qFp2, fname.c_str());
    }
    else
    {
      // We treat ASCII specially. We don't worry about
      // offsets and let the file move forward while reading
      // from the original file handle.
      qFp2 = qFp;
      qFp.DisableClose();
    }

    for (int i = 0; i < numBlocks; i++)
    {
      svtkStructuredGrid* nthOutput = this->Internal->Blocks[i];

      // Save the properties first
      svtkDataArray* properties = this->NewFloatArray();
      properties->SetName("Properties");

      int numProperties = 4;
      if (rank == 0)
      {
        int count = this->SkipByteCount(qFp);
        // We have a byte count to tell us how many Q values to
        // read. If this is more that 4, this is probably an Overflow
        // file.
        if (isOverflow)
        {
          // We take 4 bytes because there is an int there that
          // we will throw away
          numProperties = (count - 4) / this->Internal->Settings.Precision + 1;
        }
      }
      mp->Broadcast(&numProperties, 1, 0);
      properties->SetNumberOfTuples(numProperties + 1);
      properties->SetTuple1(numProperties, this->Gamma);

      error = 0;
      if (rank == 0)
      {
        try
        {
          // Read fsmach, alpha, re, time;
          if (this->ReadValues(qFp, 4, properties) != 4)
          {
            svtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
            properties->Delete();
            throw Plot3DException();
          }

          if (isOverflow)
          {
            // We create a dummy array to use with ReadValues
            svtkDataArray* dummyArray = properties->NewInstance();
            dummyArray->SetVoidArray(properties->GetVoidPointer(4), 3, 1);

            // Read GAMINF, BETA, TINF
            if (this->ReadValues(qFp, 3, dummyArray) != 3)
            {
              svtkErrorMacro("Encountered premature end-of-file while reading "
                            "the q file (or the file is corrupt).");
              this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
              properties->Delete();
              throw Plot3DException();
            }
            properties->SetTuple1(numProperties, dummyArray->GetTuple1(0));

            // igam is an int
            int igam;
            this->ReadIntBlock(qFp, 1, &igam);
            properties->SetTuple1(7, igam);

            dummyArray->SetVoidArray(properties->GetVoidPointer(8), 3, 1);
            // Read the rest of properties
            if (this->ReadValues(qFp, numProperties - 8, dummyArray) != numProperties - 8)
            {
              svtkErrorMacro("Encountered premature end-of-file while reading "
                            "the q file (or the file is corrupt).");
              this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
              properties->Delete();
              throw Plot3DException();
            }
            dummyArray->Delete();
          }
          this->SkipByteCount(qFp);
        }
        catch (Plot3DException&)
        {
          error = 1;
        }
      }
      mp->Broadcast(&error, 1, 0);
      if (error)
      {
        svtkErrorMacro("Error reading file " << fname.c_str());
        this->ClearGeometryCache();
        return 0;
      }

      mp->Broadcast(properties, 0);

      // We don't support different GammaInf values for blocks.
      // The value from the last block is used across.
      this->GammaInf = properties->GetTuple1(numProperties);

      nthOutput->GetFieldData()->AddArray(properties);
      properties->Delete();

      if (mp->GetLocalProcessId() == 0 && this->Internal->Settings.BinaryFile)
      {
        offset = svtk_ftell(qFp);
      }
      mp->Broadcast(&offset, 1, 0);

      int* dims = this->Internal->Dimensions[i].Values;
      int wextent[6] = { 0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1 };
      int extent[6];
      et->SetWholeExtent(wextent);
      et->SetGhostLevel(igl);
      et->PieceToExtent();
      et->GetExtent(extent);

      int ldims[3];
      svtkStructuredData::GetDimensionsFromExtent(extent, ldims);

      svtkIdType npts = svtkStructuredData::GetNumberOfPoints(extent);
      svtkIdType nTotalPts = (svtkIdType)dims[0] * dims[1] * dims[2];

      //**************** RECORD START *********************************
      // precond: offset is at start of a record in the file.
      svtkMultiBlockPLOT3DReaderRecord record;
      if (!record.Initialize(qFp, offset, this->Internal->Settings, this->Controller))
      {
        svtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(qFp2);
        this->ClearGeometryCache();
        return 0;
      }

      // we now have determined how many (sub)records are part of this block.
      assert(record.AtStart(offset));

      offset += this->GetByteCountSize();

      svtkDataArray* density = this->NewFloatArray();
      density->SetNumberOfComponents(1);
      density->SetNumberOfTuples(npts);
      density->SetName("Density");
      if (this->ReadScalar(qFp2, extent, wextent, density, offset, record) == 0)
      {
        svtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(qFp2);
        density->Delete();
        this->ClearGeometryCache();
        return 0;
      }
      nthOutput->GetPointData()->AddArray(density);
      density->Delete();
      offset +=
        record.GetLengthWithSeparators(offset, nTotalPts * this->Internal->Settings.Precision);

      svtkDataArray* momentum = this->NewFloatArray();
      momentum->SetNumberOfComponents(3);
      momentum->SetNumberOfTuples(npts);
      momentum->SetName("Momentum");
      if (this->ReadVector(qFp2, extent, wextent, this->Internal->Settings.NumberOfDimensions,
            momentum, offset, record) == 0)
      {
        svtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(qFp2);
        momentum->Delete();
        this->ClearGeometryCache();
        return 0;
      }
      nthOutput->GetPointData()->AddArray(momentum);
      momentum->Delete();
      offset += record.GetLengthWithSeparators(offset,
        this->Internal->Settings.NumberOfDimensions * nTotalPts *
          this->Internal->Settings.Precision);

      svtkDataArray* se = this->NewFloatArray();
      se->SetNumberOfComponents(1);
      se->SetNumberOfTuples(npts);
      se->SetName("StagnationEnergy");
      if (this->ReadScalar(qFp2, extent, wextent, se, offset, record) == 0)
      {
        svtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->CloseFile(qFp2);
        se->Delete();
        this->ClearGeometryCache();
        return 0;
      }
      nthOutput->GetPointData()->AddArray(se);
      se->Delete();
      offset +=
        record.GetLengthWithSeparators(offset, nTotalPts * this->Internal->Settings.Precision);

      if (isOverflow)
      {
        if (nq >= 6) /// super new
        {
          svtkDataArray* gamma = this->NewFloatArray();
          gamma->SetNumberOfComponents(1);
          gamma->SetNumberOfTuples(npts);
          gamma->SetName("Gamma");
          if (this->ReadScalar(qFp2, extent, wextent, gamma, offset, record) == 0)
          {
            svtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->CloseFile(qFp2);
            gamma->Delete();
            this->ClearGeometryCache();
            return 0;
          }
          nthOutput->GetPointData()->AddArray(gamma);
          gamma->Delete();
          offset +=
            record.GetLengthWithSeparators(offset, nTotalPts * this->Internal->Settings.Precision);
        } /// end of new

        char res[100];
        // Read species and turbulence variables for overflow q files
        for (int j = 0; j < nqc; j++)
        {
          svtkDataArray* temp = this->NewFloatArray();
          temp->SetNumberOfComponents(1);
          temp->SetNumberOfTuples(npts);
          int k = j + 1;
          snprintf(res, sizeof(res), "Species Density #%d", k);
          temp->SetName(res);
          if (this->ReadScalar(qFp2, extent, wextent, temp, offset, record) == 0)
          {
            svtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->CloseFile(qFp2);
            temp->Delete();
            this->ClearGeometryCache();
            return 0;
          }
          nthOutput->GetPointData()->AddArray(temp);
          offset +=
            record.GetLengthWithSeparators(offset, nTotalPts * this->Internal->Settings.Precision);
          temp->Delete();
        }
        float d, r;
        for (int v = 0; v < nqc; v++)
        {
          svtkDataArray* rat = this->NewFloatArray();
          snprintf(res, sizeof(res), "Species Density #%d", v + 1);
          svtkPointData* outputPD = nthOutput->GetPointData();
          svtkDataArray* spec = outputPD->GetArray(res);
          svtkDataArray* dens = outputPD->GetArray("Density");
          rat->SetNumberOfComponents(1);
          rat->SetNumberOfTuples(ldims[0] * ldims[1] * ldims[2]);
          snprintf(res, sizeof(res), "Spec Dens #%d / rho", v + 1);
          rat->SetName(res);
          for (int w = 0; w < npts; w++)
          {
            r = dens->GetComponent(w, 0);
            r = (r != 0.0 ? r : 1.0);
            d = spec->GetComponent(w, 0);
            rat->SetTuple1(w, d / r);
          }
          nthOutput->GetPointData()->AddArray(rat);
          rat->Delete();
        }
        for (int a = 0; a < nq - 6 - nqc; a++)
        {
          svtkDataArray* temp = this->NewFloatArray();
          temp->SetNumberOfComponents(1);
          temp->SetNumberOfTuples(ldims[0] * ldims[1] * ldims[2]);
          int k = a + 1;
          snprintf(res, sizeof(res), "Turb Field Quant #%d", k);
          temp->SetName(res);
          if (this->ReadScalar(qFp2, extent, wextent, temp, offset, record) == 0)
          {
            svtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->CloseFile(qFp2);
            temp->Delete();
            this->ClearGeometryCache();
            return 0;
          }
          nthOutput->GetPointData()->AddArray(temp);
          offset +=
            record.GetLengthWithSeparators(offset, nTotalPts * this->Internal->Settings.Precision);
          temp->Delete();
        }
      }

      offset += this->GetByteCountSize();
      assert(record.AtEnd(offset));
      //**************** RECORD END *********************************

      if (rank == 0 && this->Internal->Settings.BinaryFile)
      {
        svtk_fseek(qFp, offset, SEEK_SET);
      }

      if (this->FunctionList->GetNumberOfTuples() > 0)
      {
        int fnum;
        for (int tup = 0; tup < this->FunctionList->GetNumberOfTuples(); tup++)
        {
          if ((fnum = this->FunctionList->GetValue(tup)) >= 0)
          {
            this->MapFunction(fnum, nthOutput);
          }
        }
      }
      // Remove intermediate results, if requested.
      if (this->PreserveIntermediateFunctions == false)
      {
        this->RemoveIntermediateFunctions(nthOutput->GetPointData());
        this->RemoveIntermediateFunctions(nthOutput->GetCellData());
      }
      this->AssignAttribute(this->ScalarFunctionNumber, nthOutput, svtkDataSetAttributes::SCALARS);
      this->AssignAttribute(this->VectorFunctionNumber, nthOutput, svtkDataSetAttributes::VECTORS);
    }
    this->CloseFile(qFp2);
  }

  // Now read the functions.
  if (this->FunctionFileName && this->FunctionFileName[0] != '\0')
  {
    svtkTypeUInt64 offset = 0;

    svtkPlot3DCFile fFp(nullptr);

    std::vector<int> nFunctions(numBlocks);
    int error = 0;
    if (rank == 0)
    {
      try
      {
        if (this->CheckFunctionFile(fFp) != SVTK_OK)
        {
          throw Plot3DException();
        }

        if (this->ReadFunctionHeader(fFp, &nFunctions[0]) != SVTK_OK)
        {
          throw Plot3DException();
        }
        offset = svtk_ftell(fFp);
      }
      catch (Plot3DException&)
      {
        error = 1;
      }
    }
    mp->Broadcast(&error, 1, 0);
    if (error)
    {
      svtkErrorMacro("Error reading file " << this->XYZFileName);
      this->ClearGeometryCache();
      return 0;
    }

    mp->Broadcast(&nFunctions[0], numBlocks, 0);
    mp->Broadcast(&offset, 1, 0);

    void* fFp2;
    if (this->Internal->Settings.BinaryFile)
    {
      this->OpenFileForDataRead(fFp2, this->FunctionFileName);
    }
    else
    {
      fFp2 = fFp;
      fFp.DisableClose();
    }

    for (int i = 0; i < numBlocks; i++)
    {
      svtkStructuredGrid* nthOutput = this->Internal->Blocks[i];

      int* dims = this->Internal->Dimensions[i].Values;
      int wextent[6] = { 0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1 };
      int extent[6];
      et->SetWholeExtent(wextent);
      et->SetSplitModeToZSlab();
      et->PieceToExtent();
      et->GetExtent(extent);

      svtkIdType npts = svtkStructuredData::GetNumberOfPoints(extent);
      svtkIdType nTotalPts = (svtkIdType)dims[0] * dims[1] * dims[2];

      //**************** RECORD START *********************************
      // precond: offset is at start of a record in the file.
      svtkMultiBlockPLOT3DReaderRecord record;
      if (!record.Initialize(fFp, offset, this->Internal->Settings, this->Controller))
      {
        svtkErrorMacro("Encountered premature end-of-file while reading "
                      "the function file (or the file is corrupt).");
        this->CloseFile(fFp2);
        this->ClearGeometryCache();
        return 0;
      }

      // we now have determined how many (sub)records are part of this block.
      assert(record.AtStart(offset));

      offset += this->GetByteCountSize();

      for (int j = 0; j < nFunctions[i]; j++)
      {
        svtkDataArray* functionArray = this->NewFloatArray();
        functionArray->SetNumberOfTuples(npts);
        std::ostringstream stream;
        (j < static_cast<int>(this->FunctionNames.size())) ? stream << this->FunctionNames[j]
                                                           : stream << "Function" << j;
        const std::string functionName = stream.str();
        functionArray->SetName(functionName.c_str());
        if (this->ReadScalar(fFp2, extent, wextent, functionArray, offset, record) == 0)
        {
          svtkErrorMacro("Encountered premature end-of-file while reading "
                        "the function file (or the file is corrupt).");
          this->CloseFile(fFp2);
          functionArray->Delete();
          this->ClearGeometryCache();
          return 0;
        }
        offset +=
          record.GetLengthWithSeparators(offset, nTotalPts * this->Internal->Settings.Precision);
        nthOutput->GetPointData()->AddArray(functionArray);
        functionArray->Delete();
      }

      offset += this->GetByteCountSize();
      assert(record.AtEnd(offset));
      //**************** RECORD END *********************************
    }
    this->CloseFile(fFp2);
  }

  return 1;
}

double svtkMultiBlockPLOT3DReader::GetGamma(svtkIdType idx, svtkDataArray* gamma)
{
  if (gamma)
  {
    return gamma->GetComponent(idx, 0);
  }

  return this->GammaInf;
}

void svtkMultiBlockPLOT3DReader::ClearGeometryCache()
{
  this->Internal->Blocks.clear();
}

int svtkMultiBlockPLOT3DReader::AutoDetectionCheck(FILE* fp)
{
  this->Internal->CheckBinaryFile(fp, this->FileSize);

  if (!this->Internal->Settings.BinaryFile)
  {
    svtkDebugMacro("Auto-detection only works with binary files.");
    if (this->BinaryFile)
    {
      svtkWarningMacro("This appears to be an ASCII file. Please make sure "
                      "that all settings are correct to read it correctly.");
    }
    this->Internal->Settings.ByteOrder = this->ByteOrder;
    this->Internal->Settings.HasByteCount = this->HasByteCount;
    this->Internal->Settings.MultiGrid = this->MultiGrid;
    this->Internal->Settings.NumberOfDimensions = this->TwoDimensionalGeometry ? 2 : 3;
    this->Internal->Settings.Precision = this->DoublePrecision ? 8 : 4;
    this->Internal->Settings.IBlanking = this->IBlanking;
    return 1;
  }

  if (!this->Internal->CheckByteOrder(fp))
  {
    svtkErrorMacro("Could not determine big/little endianness of file.");
    return 0;
  }
  if (!this->Internal->CheckByteCount(fp))
  {
    svtkErrorMacro("Could not determine if file has Fortran byte counts.");
    return 0;
  }

  if (!this->Internal->Settings.HasByteCount)
  {
    if (!this->Internal->CheckCFile(fp, this->FileSize))
    {
      svtkErrorMacro("Could not determine settings for file. Cannot read.");
      return 0;
    }
  }
  else
  {
    if (!this->Internal->CheckMultiGrid(fp))
    {
      svtkErrorMacro("Could not determine settings for file. Cannot read.");
      return 0;
    }
    if (!this->Internal->Check2DGeom(fp))
    {
      svtkErrorMacro("Could not determine settings for file. Cannot read.");
      return 0;
    }
    if (!this->Internal->CheckBlankingAndPrecision(fp))
    {
      svtkErrorMacro("Could not determine settings for file. Cannot read.");
      return 0;
    }
  }
  if (!this->AutoDetectFormat)
  {
    if (!this->ForceRead &&
      (this->Internal->Settings.BinaryFile != this->BinaryFile ||
        this->Internal->Settings.ByteOrder != this->ByteOrder ||
        this->Internal->Settings.HasByteCount != this->HasByteCount ||
        this->Internal->Settings.MultiGrid != this->MultiGrid ||
        this->Internal->Settings.NumberOfDimensions != (this->TwoDimensionalGeometry ? 2 : 3) ||
        this->Internal->Settings.Precision != (this->DoublePrecision ? 8 : 4) ||
        this->Internal->Settings.IBlanking != this->IBlanking))
    {
      svtkErrorMacro(<< "The settings that you provided do not match what was auto-detected "
                    << "in the file. The detected settings are: "
                    << "\n"
                    << "BinaryFile: " << (this->Internal->Settings.BinaryFile ? 1 : 0) << "\n"
                    << "ByteOrder: " << this->Internal->Settings.ByteOrder << "\n"
                    << "HasByteCount: " << (this->Internal->Settings.HasByteCount ? 1 : 0) << "\n"
                    << "MultiGrid: " << (this->Internal->Settings.MultiGrid ? 1 : 0) << "\n"
                    << "NumberOfDimensions: " << this->Internal->Settings.NumberOfDimensions << "\n"
                    << "DoublePrecision: " << (this->Internal->Settings.Precision == 4 ? 0 : 1)
                    << "\n"
                    << "IBlanking: " << (this->Internal->Settings.IBlanking ? 1 : 0) << endl);
      return 0;
    }
    this->Internal->Settings.BinaryFile = this->BinaryFile;
    this->Internal->Settings.ByteOrder = this->ByteOrder;
    this->Internal->Settings.HasByteCount = this->HasByteCount;
    this->Internal->Settings.MultiGrid = this->MultiGrid;
    this->Internal->Settings.NumberOfDimensions = this->TwoDimensionalGeometry ? 2 : 3;
    this->Internal->Settings.Precision = this->DoublePrecision ? 8 : 4;
    this->Internal->Settings.IBlanking = this->IBlanking;
    return 1;
  }
  return 1;
}

int svtkMultiBlockPLOT3DReader::OpenFileForDataRead(void*& fp, const char* fname)
{
  if (this->BinaryFile)
  {
    fp = svtksys::SystemTools::Fopen(fname, "rb");
  }
  else
  {
    fp = svtksys::SystemTools::Fopen(fname, "r");
  }
  if (fp == nullptr)
  {
    this->SetErrorCode(svtkErrorCode::FileNotFoundError);
    svtkErrorMacro(<< "File: " << fname << " not found.");
    return SVTK_ERROR;
  }
  return SVTK_OK;
}

void svtkMultiBlockPLOT3DReader::CloseFile(void* fp)
{
  fclose(reinterpret_cast<FILE*>(fp));
}

int svtkMultiBlockPLOT3DReader::CheckFile(FILE*& fp, const char* fname)
{
  if (this->BinaryFile)
  {
    fp = svtksys::SystemTools::Fopen(fname, "rb");
  }
  else
  {
    fp = svtksys::SystemTools::Fopen(fname, "r");
  }
  if (fp == nullptr)
  {
    this->SetErrorCode(svtkErrorCode::FileNotFoundError);
    svtkErrorMacro(<< "File: " << fname << " not found.");
    return SVTK_ERROR;
  }
  return SVTK_OK;
}

int svtkMultiBlockPLOT3DReader::CheckGeometryFile(FILE*& xyzFp)
{
  if (this->XYZFileName == nullptr || this->XYZFileName[0] == '\0')
  {
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    svtkErrorMacro(<< "Must specify geometry file");
    return SVTK_ERROR;
  }
  return this->CheckFile(xyzFp, this->XYZFileName);
}

int svtkMultiBlockPLOT3DReader::CheckFunctionFile(FILE*& fFp)
{
  if (this->FunctionFileName == nullptr || this->FunctionFileName[0] == '\0')
  {
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    svtkErrorMacro(<< "Must specify geometry file");
    return SVTK_ERROR;
  }
  return this->CheckFile(fFp, this->FunctionFileName);
}

int svtkMultiBlockPLOT3DReader::GetByteCountSize()
{
  if (this->Internal->Settings.BinaryFile && this->Internal->Settings.HasByteCount)
  {
    return sizeof(int);
  }
  return 0;
}

// Skip Fortran style byte count.
int svtkMultiBlockPLOT3DReader::SkipByteCount(FILE* fp)
{
  int byteCountSize = this->GetByteCountSize();
  if (byteCountSize > 0)
  {
    int tmp;
    if (fread(&tmp, byteCountSize, 1, fp) != 1)
    {
      svtkErrorMacro("MultiBlockPLOT3DReader error reading file: "
        << this->XYZFileName << " Premature EOF while reading skipping byte count.");
      fclose(fp);
      return 0;
    }
    if (this->Internal->Settings.ByteOrder == svtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
    {
      svtkByteSwap::Swap4LERange(&tmp, 1);
    }
    else
    {
      svtkByteSwap::Swap4BERange(&tmp, 1);
    }

    return tmp;
  }
  return 0;
}

// Read a block of ints (ascii or binary) and return number read.
int svtkMultiBlockPLOT3DReader::ReadIntBlock(FILE* fp, int n, int* block)
{
  if (this->Internal->Settings.BinaryFile)
  {
    svtkIdType retVal = static_cast<svtkIdType>(fread(block, sizeof(int), n, fp));
    if (this->Internal->Settings.ByteOrder == FILE_LITTLE_ENDIAN)
    {
      svtkByteSwap::Swap4LERange(block, n);
    }
    else
    {
      svtkByteSwap::Swap4BERange(block, n);
    }
    return retVal == n;
  }
  else
  {
    svtkIdType count = 0;
    for (int i = 0; i < n; i++)
    {
      int num = fscanf(fp, "%d", &(block[i]));
      if (num > 0)
      {
        count++;
      }
      else
      {
        return 0;
      }
    }
    return count == n;
  }
}

svtkDataArray* svtkMultiBlockPLOT3DReader::NewFloatArray()
{
  if (this->Internal->Settings.Precision == 4)
  {
    return svtkFloatArray::New();
  }
  else
  {
    return svtkDoubleArray::New();
  }
}

svtkIdType svtkMultiBlockPLOT3DReader::ReadValues(FILE* fp, int n, svtkDataArray* scalar)
{
  if (this->Internal->Settings.BinaryFile)
  {
    if (this->Internal->Settings.Precision == 4)
    {
      svtkPLOT3DArrayReader<float> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      svtkFloatArray* floatArray = static_cast<svtkFloatArray*>(scalar);
      return arrayReader.ReadScalar(fp, 0, n, 0, floatArray->GetPointer(0));
    }
    else
    {
      svtkPLOT3DArrayReader<double> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      svtkDoubleArray* doubleArray = static_cast<svtkDoubleArray*>(scalar);
      return arrayReader.ReadScalar(fp, 0, n, 0, doubleArray->GetPointer(0));
    }
  }
  else
  {
    if (this->Internal->Settings.Precision == 4)
    {
      svtkFloatArray* floatArray = static_cast<svtkFloatArray*>(scalar);
      float* values = floatArray->GetPointer(0);

      int count = 0;
      for (int i = 0; i < n; i++)
      {
        int num = fscanf(fp, "%f", &(values[i]));
        if (num > 0)
        {
          count++;
        }
        else
        {
          return 0;
        }
      }
      return count;
    }
    else
    {
      svtkDoubleArray* doubleArray = static_cast<svtkDoubleArray*>(scalar);
      double* values = doubleArray->GetPointer(0);

      int count = 0;
      for (int i = 0; i < n; i++)
      {
        int num = fscanf(fp, "%lf", &(values[i]));
        if (num > 0)
        {
          count++;
        }
        else
        {
          return 0;
        }
      }
      return count;
    }
  }
}

int svtkMultiBlockPLOT3DReader::ReadIntScalar(void* vfp, int extent[6], int wextent[6],
  svtkDataArray* scalar, svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& record)
{
  FILE* fp = reinterpret_cast<FILE*>(vfp);
  svtkIdType n = svtkStructuredData::GetNumberOfPoints(extent);

  if (this->Internal->Settings.BinaryFile)
  {
    // precond: we assume the offset has been updated properly to step over
    // sub-record markers, if any.
    if (svtk_fseek(fp, offset, SEEK_SET) != 0)
    {
      return 0;
    }

    svtkPLOT3DArrayReader<int> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    svtkIdType preskip, postskip;
    svtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
    svtkIntArray* intArray = static_cast<svtkIntArray*>(scalar);
    return arrayReader.ReadScalar(fp, preskip, n, postskip, intArray->GetPointer(0), record) == n;
  }
  else
  {
    svtkIntArray* intArray = static_cast<svtkIntArray*>(scalar);
    return this->ReadIntBlock(fp, n, intArray->GetPointer(0));
  }
}

int svtkMultiBlockPLOT3DReader::ReadScalar(void* vfp, int extent[6], int wextent[6],
  svtkDataArray* scalar, svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& record)
{
  svtkIdType n = svtkStructuredData::GetNumberOfPoints(extent);

  FILE* fp = reinterpret_cast<FILE*>(vfp);

  if (this->Internal->Settings.BinaryFile)
  {
    // precond: we assume the offset has been updated properly to step over
    // sub-record markers, if any.
    if (svtk_fseek(fp, offset, SEEK_SET) != 0)
    {
      return 0;
    }

    if (this->Internal->Settings.Precision == 4)
    {
      svtkPLOT3DArrayReader<float> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      svtkIdType preskip, postskip;
      svtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
      svtkFloatArray* floatArray = static_cast<svtkFloatArray*>(scalar);
      return arrayReader.ReadScalar(fp, preskip, n, postskip, floatArray->GetPointer(0), record) ==
        n;
    }
    else
    {
      svtkPLOT3DArrayReader<double> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      svtkIdType preskip, postskip;
      svtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
      svtkDoubleArray* doubleArray = static_cast<svtkDoubleArray*>(scalar);
      return arrayReader.ReadScalar(fp, preskip, n, postskip, doubleArray->GetPointer(0), record) ==
        n;
    }
  }
  else
  {
    if (this->Internal->Settings.Precision == 4)
    {
      svtkFloatArray* floatArray = static_cast<svtkFloatArray*>(scalar);
      float* values = floatArray->GetPointer(0);

      int count = 0;
      for (int i = 0; i < n; i++)
      {
        int num = fscanf(fp, "%f", &(values[i]));
        if (num > 0)
        {
          count++;
        }
        else
        {
          return 0;
        }
      }
      return count == n;
    }
    else
    {
      svtkDoubleArray* doubleArray = static_cast<svtkDoubleArray*>(scalar);
      double* values = doubleArray->GetPointer(0);

      int count = 0;
      for (int i = 0; i < n; i++)
      {
        int num = fscanf(fp, "%lf", &(values[i]));
        if (num > 0)
        {
          count++;
        }
        else
        {
          return 0;
        }
      }
      return count == n;
    }
  }
}

int svtkMultiBlockPLOT3DReader::ReadVector(void* vfp, int extent[6], int wextent[6], int numDims,
  svtkDataArray* vector, svtkTypeUInt64 offset, const svtkMultiBlockPLOT3DReaderRecord& record)
{
  svtkIdType n = svtkStructuredData::GetNumberOfPoints(extent);
  svtkIdType nValues = n * numDims;

  FILE* fp = reinterpret_cast<FILE*>(vfp);

  if (this->Internal->Settings.BinaryFile)
  {
    // precond: we assume the offset has been updated properly to step over
    // sub-record markers, if any.
    if (svtk_fseek(fp, offset, SEEK_SET) != 0)
    {
      return 0;
    }
    if (this->Internal->Settings.Precision == 4)
    {
      svtkPLOT3DArrayReader<float> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      svtkFloatArray* floatArray = static_cast<svtkFloatArray*>(vector);
      return arrayReader.ReadVector(
               fp, extent, wextent, numDims, floatArray->GetPointer(0), record) == nValues;
    }
    else
    {
      svtkPLOT3DArrayReader<double> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      svtkDoubleArray* doubleArray = static_cast<svtkDoubleArray*>(vector);
      return arrayReader.ReadVector(
               fp, extent, wextent, numDims, doubleArray->GetPointer(0), record) == nValues;
    }
  }
  else
  {

    // Initialize the 3rd component to 0 in case the input file is
    // 2D
    vector->FillComponent(2, 0);

    svtkIdType count = 0;

    if (this->Internal->Settings.Precision == 4)
    {
      svtkFloatArray* floatArray = static_cast<svtkFloatArray*>(vector);

      svtkFloatArray* tmpArray = svtkFloatArray::New();
      tmpArray->Allocate(n);
      for (int component = 0; component < numDims; component++)
      {
        count += this->ReadValues(fp, n, tmpArray);
        for (svtkIdType i = 0; i < n; i++)
        {
          floatArray->SetValue(3 * i + component, tmpArray->GetValue(i));
        }
      }
      tmpArray->Delete();
    }
    else
    {
      svtkDoubleArray* doubleArray = static_cast<svtkDoubleArray*>(vector);

      svtkDoubleArray* tmpArray = svtkDoubleArray::New();
      tmpArray->Allocate(n);
      for (int component = 0; component < numDims; component++)
      {
        count += this->ReadValues(fp, n, tmpArray);
        for (svtkIdType i = 0; i < n; i++)
        {
          doubleArray->SetValue(3 * i + component, tmpArray->GetValue(i));
        }
      }
      tmpArray->Delete();
    }

    return count == nValues;
  }
}

// Read a block of floats (ascii or binary) and return number read.
void svtkMultiBlockPLOT3DReader::CalculateFileSize(FILE* fp)
{
  svtk_off_t curPos = svtk_ftell(fp);
  svtk_fseek(fp, 0, SEEK_END);
  this->FileSize = static_cast<size_t>(svtk_ftell(fp));
  svtk_fseek(fp, curPos, SEEK_SET);
}

int svtkMultiBlockPLOT3DReader::CanReadBinaryFile(const char* fname)
{
  FILE* xyzFp;

  if (!fname || fname[0] == '\0')
  {
    return 0;
  }

  if (this->CheckFile(xyzFp, fname) != SVTK_OK)
  {
    return 0;
  }

  this->CalculateFileSize(xyzFp);

  if (!this->AutoDetectionCheck(xyzFp))
  {
    fclose(xyzFp);
    return 0;
  }
  rewind(xyzFp);

  int numBlocks = this->GetNumberOfBlocksInternal(xyzFp, 0);
  fclose(xyzFp);
  if (numBlocks != 0)
  {
    return 1;
  }
  return 0;
}

// Read the header and return the number of grids.
int svtkMultiBlockPLOT3DReader::GetNumberOfBlocksInternal(FILE* xyzFp, int svtkNotUsed(allocate))
{
  int numGrid = 0;

  if (this->Internal->Settings.MultiGrid)
  {
    this->SkipByteCount(xyzFp);
    this->ReadIntBlock(xyzFp, 1, &numGrid);
    this->SkipByteCount(xyzFp);
  }
  else
  {
    numGrid = 1;
  }

  this->Internal->Dimensions.resize(numGrid);

  return numGrid;
}

int svtkMultiBlockPLOT3DReader::ReadGeometryHeader(FILE* fp)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 1);
  int i;
  svtkDebugMacro("Geometry number of grids: " << numGrid);
  if (numGrid == 0)
  {
    return SVTK_ERROR;
  }

  // Read and set extents of all blocks.
  this->SkipByteCount(fp);
  for (i = 0; i < numGrid; i++)
  {
    int n[3];
    n[2] = 1;
    this->ReadIntBlock(fp, this->Internal->Settings.NumberOfDimensions, n);
    svtkDebugMacro("Geometry, block " << i << " dimensions: " << n[0] << " " << n[1] << " " << n[2]);
    memcpy(this->Internal->Dimensions[i].Values, n, 3 * sizeof(int));
  }
  this->SkipByteCount(fp);

  return SVTK_OK;
}

int svtkMultiBlockPLOT3DReader::ReadQHeader(
  FILE* fp, bool checkGrid, int& nq, int& nqc, int& overflow)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 0);
  svtkDebugMacro("Q number of grids: " << numGrid);
  if (numGrid == 0)
  {
    return SVTK_ERROR;
  }

  // If the numbers of grids still do not match, the
  // q file is wrong
  if (checkGrid && numGrid != static_cast<int>(this->Internal->Blocks.size()))
  {
    svtkErrorMacro("The number of grids between the geometry "
                  "and the q file do not match.");
    return SVTK_ERROR;
  }

  int bytes = this->SkipByteCount(fp);
  // If the header contains 2 additional ints, then we assume
  // that this is an Overflow file.
  if (bytes > 0 && bytes == (numGrid * this->Internal->Settings.NumberOfDimensions + 2) * 4)
  {
    overflow = 1;
  }
  else
  {
    overflow = 0;
  }
  for (int i = 0; i < numGrid; i++)
  {
    int n[3];
    n[2] = 1;
    this->ReadIntBlock(fp, this->Internal->Settings.NumberOfDimensions, n);
    svtkDebugMacro("Q, block " << i << " dimensions: " << n[0] << " " << n[1] << " " << n[2]);

    if (checkGrid)
    {
      int* dims = this->Internal->Dimensions[i].Values;
      int extent[6] = { 0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1 };
      if (extent[1] != n[0] - 1 || extent[3] != n[1] - 1 || extent[5] != n[2] - 1)
      {
        this->SetErrorCode(svtkErrorCode::FileFormatError);
        svtkErrorMacro("Geometry and data dimensions do not match. "
                      "Data file may be corrupt.");
        this->Internal->Blocks[i]->Initialize();
        return SVTK_ERROR;
      }
    }
  }
  if (overflow)
  {
    this->ReadIntBlock(fp, 1, &nq);
    this->ReadIntBlock(fp, 1, &nqc);
  }
  else
  {
    nq = 5;
    nqc = 0;
  }
  this->SkipByteCount(fp);
  return SVTK_OK;
}

int svtkMultiBlockPLOT3DReader::ReadFunctionHeader(FILE* fp, int* nFunctions)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 0);
  svtkDebugMacro("Function number of grids: " << numGrid);
  if (numGrid == 0)
  {
    return SVTK_ERROR;
  }

  // If the numbers of grids still do not match, the
  // function file is wrong
  if (numGrid != static_cast<int>(this->Internal->Blocks.size()))
  {
    svtkErrorMacro("The number of grids between the geometry "
                  "and the function file do not match.");
    return SVTK_ERROR;
  }

  this->SkipByteCount(fp);
  for (int i = 0; i < numGrid; i++)
  {
    int n[3];
    n[2] = 1;
    this->ReadIntBlock(fp, this->Internal->Settings.NumberOfDimensions, n);
    svtkDebugMacro("Function, block " << i << " dimensions: " << n[0] << " " << n[1] << " " << n[2]);

    int* dims = this->Internal->Dimensions[i].Values;
    int extent[6] = { 0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1 };
    if (extent[1] != n[0] - 1 || extent[3] != n[1] - 1 || extent[5] != n[2] - 1)
    {
      this->SetErrorCode(svtkErrorCode::FileFormatError);
      svtkErrorMacro("Geometry and data dimensions do not match. "
                    "Data file may be corrupt.");
      this->Internal->Blocks[i]->Initialize();
      return SVTK_ERROR;
    }
    this->ReadIntBlock(fp, 1, nFunctions + i);
  }
  this->SkipByteCount(fp);
  return SVTK_OK;
}

void svtkMultiBlockPLOT3DReader::SetXYZFileName(const char* name)
{
  if (this->XYZFileName && name && !strcmp(this->XYZFileName, name))
  {
    return;
  }

  delete[] this->XYZFileName;

  if (name)
  {
    this->XYZFileName = new char[strlen(name) + 1];
    strcpy(this->XYZFileName, name);
  }
  else
  {
    this->XYZFileName = nullptr;
  }

  this->Internal->NeedToCheckXYZFile = true;
  this->ClearGeometryCache();
  this->Modified();
}

void svtkMultiBlockPLOT3DReader::SetScalarFunctionNumber(int num)
{
  if (this->ScalarFunctionNumber == num)
  {
    return;
  }
  if (num >= 0)
  {
    // If this function is not in the list, add it.
    int found = 0;
    for (int i = 0; i < this->FunctionList->GetNumberOfTuples(); i++)
    {
      if (this->FunctionList->GetValue(i) == num)
      {
        found = 1;
      }
    }
    if (!found)
    {
      this->AddFunction(num);
    }
  }
  this->ScalarFunctionNumber = num;
}

void svtkMultiBlockPLOT3DReader::SetVectorFunctionNumber(int num)
{
  if (this->VectorFunctionNumber == num)
  {
    return;
  }
  if (num >= 0)
  {
    // If this function is not in the list, add it.
    int found = 0;
    for (int i = 0; i < this->FunctionList->GetNumberOfTuples(); i++)
    {
      if (this->FunctionList->GetValue(i) == num)
      {
        found = 1;
      }
    }
    if (!found)
    {
      this->AddFunction(num);
    }
  }
  this->VectorFunctionNumber = num;
}

void svtkMultiBlockPLOT3DReader::RemoveFunction(int fnum)
{
  for (int i = 0; i < this->FunctionList->GetNumberOfTuples(); i++)
  {
    if (this->FunctionList->GetValue(i) == fnum)
    {
      this->FunctionList->SetValue(i, -1);
      this->Modified();
    }
  }
}

// Various PLOT3D functions.....................
void svtkMultiBlockPLOT3DReader::MapFunction(int fNumber, svtkStructuredGrid* output)
{
  switch (fNumber)
  {
    case 100: // Density
      break;

    case 110: // Pressure
      if (svtkDataArray* dataArray = this->ComputePressure(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 111: // Pressure Coefficient
      if (svtkDataArray* dataArray = this->ComputePressureCoefficient(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 112: // Mach Number
      if (svtkDataArray* dataArray = this->ComputeMachNumber(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 113: // Sound Speed
      if (svtkDataArray* dataArray = this->ComputeSoundSpeed(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 120: // Temperature
      if (svtkDataArray* dataArray = this->ComputeTemperature(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 130: // Enthalpy
      if (svtkDataArray* dataArray = this->ComputeEnthalpy(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 140: // Internal Energy
      break;

    case 144: // Kinetic Energy
      if (svtkDataArray* dataArray = this->ComputeKineticEnergy(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 153: // Velocity Magnitude
      if (svtkDataArray* dataArray = this->ComputeVelocityMagnitude(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 163: // Stagnation energy
      break;

    case 170: // Entropy
      if (svtkDataArray* dataArray = this->ComputeEntropy(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 184: // Swirl
      if (svtkDataArray* dataArray = this->ComputeSwirl(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 200: // Velocity
      if (svtkDataArray* dataArray = this->ComputeVelocity(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 201: // Vorticity
      if (svtkDataArray* dataArray = this->ComputeVorticity(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 202: // Momentum
      break;

    case 210: // PressureGradient
      if (svtkDataArray* dataArray = this->ComputePressureGradient(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 211: // Vorticity Magnitude
      if (svtkDataArray* dataArray = this->ComputeVorticityMagnitude(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    case 212: // Strain Rate
      if (svtkDataArray* dataArray = this->ComputeStrainRate(output))
      {
        dataArray->GetInformation()->Remove(INTERMEDIATE_RESULT());
      }
      break;

    default:
      svtkErrorMacro(<< "No function number " << fNumber);
  }
}

void svtkMultiBlockPLOT3DReader::AssignAttribute(
  int fNumber, svtkStructuredGrid* output, int attributeType)
{
  switch (fNumber)
  {
    case -1: // empty mapping
      output->GetPointData()->SetActiveAttribute(0, attributeType);
      break;

    case 100: // Density
      output->GetPointData()->SetActiveAttribute("Density", attributeType);
      break;

    case 110: // Pressure
      output->GetPointData()->SetActiveAttribute("Pressure", attributeType);
      break;

    case 120: // Temperature
      output->GetPointData()->SetActiveAttribute("Temperature", attributeType);
      break;

    case 130: // Enthalpy
      output->GetPointData()->SetActiveAttribute("Enthalpy", attributeType);
      break;

    case 140: // Internal Energy
      output->GetPointData()->SetActiveAttribute("StagnationEnergy", attributeType);
      break;

    case 144: // Kinetic Energy
      output->GetPointData()->SetActiveAttribute("KineticEnergy", attributeType);
      break;

    case 153: // Velocity Magnitude
      output->GetPointData()->SetActiveAttribute("VelocityMagnitude", attributeType);
      break;

    case 163: // Stagnation energy
      output->GetPointData()->SetActiveAttribute("StagnationEnergy", attributeType);
      break;

    case 170: // Entropy
      output->GetPointData()->SetActiveAttribute("Entropy", attributeType);
      break;

    case 184: // Swirl
      output->GetPointData()->SetActiveAttribute("Swirl", attributeType);
      break;

    case 200: // Velocity
      output->GetPointData()->SetActiveAttribute("Velocity", attributeType);
      break;

    case 201: // Vorticity
      output->GetPointData()->SetActiveAttribute("Vorticity", attributeType);
      break;

    case 202: // Momentum
      output->GetPointData()->SetActiveAttribute("Momentum", attributeType);
      break;

    case 210: // PressureGradient
      output->GetPointData()->SetActiveAttribute("PressureGradient", attributeType);
      break;

    default:
      svtkErrorMacro(<< "No function number " << fNumber);
  }
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeTemperature(svtkStructuredGrid* output)
{
  Functors::ComputeTemperatureFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputePressure(svtkStructuredGrid* output)
{
  Functors::ComputePressureFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeEnthalpy(svtkStructuredGrid* output)
{
  Functors::ComputeEnthalpyFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeKineticEnergy(svtkStructuredGrid* output)
{
  Functors::ComputeKineticEnergyFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeVelocityMagnitude(svtkStructuredGrid* output)
{
  Functors::ComputeVelocityMagnitudeFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeEntropy(svtkStructuredGrid* output)
{
  Functors::ComputeEntropyFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeSwirl(svtkStructuredGrid* output)
{
  Functors::ComputeSwirlFunctor func(this, output);
  return func.Execute();
}

// Vector functions
svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeVelocity(svtkStructuredGrid* output)
{
  Functors::ComputeVelocityFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeVorticity(svtkStructuredGrid* output)
{
  Functors::ComputeVorticityFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputePressureGradient(svtkStructuredGrid* output)
{
  Functors::ComputePressureGradientFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputePressureCoefficient(svtkStructuredGrid* output)
{
  Functors::ComputePressureCoefficientFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeMachNumber(svtkStructuredGrid* output)
{
  Functors::ComputeMachNumberFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeSoundSpeed(svtkStructuredGrid* output)
{
  Functors::ComputeSoundSpeedFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeVorticityMagnitude(svtkStructuredGrid* output)
{
  Functors::ComputeVorticityMagnitudeFunctor func(this, output);
  return func.Execute();
}

svtkDataArray* svtkMultiBlockPLOT3DReader::ComputeStrainRate(svtkStructuredGrid* output)
{
  Functors::ComputeStrainRateFunctor func(this, output);
  return func.Execute();
}

void svtkMultiBlockPLOT3DReader::RemoveIntermediateFunctions(svtkDataSetAttributes* dsa)
{
  assert(dsa != nullptr);
  int max = dsa->GetNumberOfArrays();
  for (int index = 0; index < max; ++index)
  {
    if (svtkAbstractArray* array = dsa->GetAbstractArray(index))
    {
      if (array->GetInformation()->Has(INTERMEDIATE_RESULT()))
      {
        dsa->RemoveArray(index);
        index--;
        max--;
      }
    }
  }
}

void svtkMultiBlockPLOT3DReader::SetByteOrderToBigEndian()
{
  this->ByteOrder = FILE_BIG_ENDIAN;
}

void svtkMultiBlockPLOT3DReader::SetByteOrderToLittleEndian()
{
  this->ByteOrder = FILE_LITTLE_ENDIAN;
}

const char* svtkMultiBlockPLOT3DReader::GetByteOrderAsString()
{
  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
  {
    return "LittleEndian";
  }
  else
  {
    return "BigEndian";
  }
}

void svtkMultiBlockPLOT3DReader::AddFunction(int functionNumber)
{
  this->FunctionList->InsertNextValue(functionNumber);
  this->Modified();
}

void svtkMultiBlockPLOT3DReader::RemoveAllFunctions()
{
  this->FunctionList->Reset();
  this->Modified();
}

int svtkMultiBlockPLOT3DReader::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

void svtkMultiBlockPLOT3DReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "XYZ File Name: " << (this->XYZFileName ? this->XYZFileName : "(none)") << "\n";
  os << indent << "Q File Name: " << (this->QFileName ? this->QFileName : "(none)") << "\n";
  os << indent
     << "Function File Name: " << (this->FunctionFileName ? this->FunctionFileName : "(none)")
     << "\n";
  os << indent << "BinaryFile: " << this->BinaryFile << endl;
  os << indent << "HasByteCount: " << this->HasByteCount << endl;
  os << indent << "Gamma: " << this->Gamma << endl;
  os << indent << "R: " << this->R << endl;
  os << indent << "ScalarFunctionNumber: " << this->ScalarFunctionNumber << endl;
  os << indent << "VectorFunctionNumber: " << this->VectorFunctionNumber << endl;
  os << indent << "MultiGrid: " << this->MultiGrid << endl;
  os << indent << "ForceRead: " << this->ForceRead << endl;
  os << indent << "IBlanking: " << this->IBlanking << endl;
  os << indent << "ByteOrder: " << this->ByteOrder << endl;
  os << indent << "TwoDimensionalGeometry: " << (this->TwoDimensionalGeometry ? "on" : "off")
     << endl;
  os << indent << "Double Precision:" << this->DoublePrecision << endl;
  os << indent << "Auto Detect Format: " << this->AutoDetectFormat << endl;
  os << indent
     << "PreserveIntermediateFunctions: " << (this->PreserveIntermediateFunctions ? "on" : "off")
     << endl;
}
