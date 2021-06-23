#include "svtkGeoProjection.h"

int TestGeoProjection(int, char*[])
{
  int np = svtkGeoProjection::GetNumberOfProjections();
  cout << "Supported projections:\n";
  for (int i = 0; i < np; ++i)
  {
    cout << "Projection: " << svtkGeoProjection::GetProjectionName(i) << "\n";
    cout << "\t" << svtkGeoProjection::GetProjectionDescription(i) << "\n";
  }
  cout << "-------\n";
  svtkGeoProjection* proj = svtkGeoProjection::New();
  const char* projName = "rouss";
  proj->SetName(projName);
  cout << projName << " is " << proj->GetDescription() << "\n";
  proj->Delete();
  return 0;
}
