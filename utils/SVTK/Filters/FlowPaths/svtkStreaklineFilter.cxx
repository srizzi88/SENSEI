/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkStreaklineFilter.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStreaklineFilter.h"
#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSetGet.h"
#include "svtkSmartPointer.h"

#include <algorithm>
#include <cassert>
#include <vector>

#define DEBUGSTREAKLINEFILTER 1
#ifdef DEBUGSTREAKLINEFILTER
#define Assert(a) assert(a)
#else
#define Assert(a)
#endif

class StreakParticle
{
public:
  svtkIdType Id;
  float Age;

  StreakParticle(svtkIdType id, float age)
    : Id(id)
    , Age(age)
  {
  }

  bool operator<(const StreakParticle& other) const { return this->Age > other.Age; }
};

typedef std::vector<StreakParticle> Streak;

svtkObjectFactoryNewMacro(svtkStreaklineFilter);

void StreaklineFilterInternal::Initialize(svtkParticleTracerBase* filter)
{
  this->Filter = filter;
  this->Filter->ForceReinjectionEveryNSteps = 1;
  this->Filter->IgnorePipelineTime = 1;
}

int StreaklineFilterInternal::OutputParticles(svtkPolyData* particles)
{
  this->Filter->Output = particles;

  return 1;
}

void StreaklineFilterInternal::Finalize()
{
  svtkPoints* points = this->Filter->Output->GetPoints();
  if (!points)
  {
    return;
  }

  svtkPointData* pd = this->Filter->Output->GetPointData();
  Assert(pd);
  svtkFloatArray* particleAge = svtkArrayDownCast<svtkFloatArray>(pd->GetArray("ParticleAge"));
  Assert(particleAge);
  svtkIntArray* seedIds = svtkArrayDownCast<svtkIntArray>(pd->GetArray("InjectedPointId"));
  Assert(seedIds);

  if (seedIds)
  {
    std::vector<Streak> streaks; // the streak lines in the current time step
    for (svtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
    {
      int streakId = seedIds->GetValue(i);
      for (int j = static_cast<int>(streaks.size()); j <= streakId; j++)
      {
        streaks.push_back(Streak());
      }
      Streak& streak = streaks[streakId];
      float age = particleAge->GetValue(i);
      streak.push_back(StreakParticle(i, age));
    }

    // sort streaks based on age
    for (unsigned int i = 0; i < streaks.size(); i++)
    {
      Streak& streak(streaks[i]);
      std::sort(streak.begin(), streak.end());
    }

    this->Filter->Output->SetLines(svtkSmartPointer<svtkCellArray>::New());
    this->Filter->Output->SetVerts(nullptr);
    svtkCellArray* outLines = this->Filter->Output->GetLines();
    Assert(outLines->GetNumberOfCells() == 0);
    Assert(outLines);
    for (unsigned int i = 0; i < streaks.size(); i++)
    {
      const Streak& streak(streaks[i]);
      svtkNew<svtkIdList> ids;

      for (unsigned int j = 0; j < streak.size(); j++)
      {
        Assert(j == 0 || streak[j].Age <= streak[j - 1].Age);
        if (j == 0 || streak[j].Age < streak[j - 1].Age)
        {
          ids->InsertNextId(streak[j].Id);
        }
      }
      if (ids->GetNumberOfIds() > 1)
      {
        outLines->InsertNextCell(ids);
      }
    }
  }
}

svtkStreaklineFilter::svtkStreaklineFilter()
{
  this->It.Initialize(this);
}

int svtkStreaklineFilter::OutputParticles(svtkPolyData* particles)
{
  return this->It.OutputParticles(particles);
}

void svtkStreaklineFilter::Finalize()
{
  return this->It.Finalize();
}

void svtkStreaklineFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
