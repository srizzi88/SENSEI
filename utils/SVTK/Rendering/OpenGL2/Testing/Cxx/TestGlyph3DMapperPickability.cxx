/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositeDataSet.h"
#include "svtkCullerCollection.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkGlyph3DMapper.h"
#include "svtkHardwareSelector.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPlaneSource.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkUnsignedIntArray.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include <functional>
#include <set>

template <typename T, typename U, typename V>
void prepareDisplayAttribute(
  T& expected, U attr, V mbds, std::function<std::pair<bool, bool>(int)> config)
{
  expected.clear();
  auto bit = mbds->NewTreeIterator();
  for (bit->InitTraversal(); !bit->IsDoneWithTraversal(); bit->GoToNextItem())
  {
    int ii = bit->GetCurrentFlatIndex();
    auto cfg = config(ii);
    bool visible = cfg.first;
    bool pickable = cfg.second;
    auto dataObj = bit->GetCurrentDataObject();
    if (visible && pickable)
    {
      auto pd = svtkPolyData::SafeDownCast(dataObj);
      if (pd)
      {
        auto cid = pd->GetCellData()->GetArray("svtkCompositeIndex");
        int idx = cid ? cid->GetTuple1(0) : ii;
        expected.insert(idx);
      }
    }
    attr->SetBlockVisibility(dataObj, visible);
    attr->SetBlockPickability(dataObj, pickable);
  }
  bit->Delete();
}

template <typename T>
void addCompositeIndex(T mbds, int& nextIndex)
{
  int nblk = static_cast<int>(mbds->GetNumberOfBlocks());
  for (int i = 0; i < nblk; ++i)
  {
    auto blk = mbds->GetBlock(i);
    if (blk->IsA("svtkCompositeDataSet"))
    {
      addCompositeIndex(svtkMultiBlockDataSet::SafeDownCast(blk), nextIndex);
    }
    else if (blk->IsA("svtkPolyData"))
    {
      auto pdata = svtkPolyData::SafeDownCast(blk);
      svtkIdType nc = pdata->GetNumberOfCells();
      auto cid = svtkSmartPointer<svtkUnsignedIntArray>::New();
      cid->SetName("svtkCompositeIndex");
      cid->SetNumberOfTuples(nc);
      cid->FillComponent(0, nextIndex);
      pdata->GetCellData()->AddArray(cid);
      ++nextIndex;
    }
  }
}

template <typename T, typename U>
int checkSelection(T seln, const U& expected, int& tt)
{
  std::cout << "Test " << tt << "\n";
  ++tt;
  int numNodes = seln->GetNumberOfNodes();
  U actual;
  for (int nn = 0; nn < numNodes; ++nn)
  {
    auto sn = seln->GetNode(nn);
    auto actor = svtkActor::SafeDownCast(sn->GetProperties()->Get(svtkSelectionNode::PROP()));
    if (actor)
    {
      auto ci = sn->GetProperties()->Get(svtkSelectionNode::COMPOSITE_INDEX());
      actual.insert(ci);
    }
  }

  std::cout << "  Expected:";
  for (auto ee : expected)
  {
    std::cout << " " << ee;
  }
  std::cout << "\n  Actual:";
  for (auto aa : actual)
  {
    std::cout << " " << aa;
  }
  std::cout << "\n";
  int result = (expected == actual ? 1 : 0);
  if (!result)
  {
    svtkGenericWarningMacro("Mismatch between expected selection and actual selection.");
  }
  return result;
}

int TestGlyph3DMapperPickability(int argc, char* argv[])
{
  auto rw = svtkSmartPointer<svtkRenderWindow>::New();
  auto ri = svtkSmartPointer<svtkRenderWindowInteractor>::New();
  auto rr = svtkSmartPointer<svtkRenderer>::New();
  auto ss = svtkSmartPointer<svtkSphereSource>::New();
  auto mp = svtkSmartPointer<svtkGlyph3DMapper>::New();
  auto ac = svtkSmartPointer<svtkActor>::New();
  auto mb = svtkSmartPointer<svtkMultiBlockDataSet>::New();
  auto da = svtkSmartPointer<svtkCompositeDataDisplayAttributes>::New();
  rw->AddRenderer(rr);
  rw->SetMultiSamples(0);
  rw->SetInteractor(ri);
  mp->SetBlockAttributes(da);

  svtkNew<svtkPlaneSource> plane;
  mb->SetNumberOfBlocks(4);
  for (int ii = 0; ii < static_cast<int>(mb->GetNumberOfBlocks()); ++ii)
  {
    double ll[2];
    ll[0] = -0.5 + 1.0 * (ii % 2);
    ll[1] = -0.5 + 1.0 * (ii / 2);
    plane->SetOrigin(ll[0], ll[1], ii);
    plane->SetPoint1(ll[0] + 1.0, ll[1], ii);
    plane->SetPoint2(ll[0], ll[1] + 1.0, ii);
    plane->Update();
    svtkNew<svtkPolyData> pblk;
    pblk->DeepCopy(plane->GetOutputDataObject(0));
    mb->SetBlock(ii, pblk.GetPointer());
  }

  mp->SetInputDataObject(0, mb.GetPointer());
  mp->SetSourceConnection(ss->GetOutputPort());
  ac->SetMapper(mp);
  rr->AddActor(ac);
  rw->SetSize(400, 400);
  rr->RemoveCuller(rr->GetCullers()->GetLastItem());
  rr->ResetCamera();
  rw->Render(); // get the window up

  double rgb[4][3] = { { .5, .5, .5 }, { 0., 1., 1. }, { 1., 1., 0. }, { 1., 0., 1. } };

  auto it = mb->NewIterator();
  int ii = 0;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    auto dataObj = it->GetCurrentDataObject();
    da->SetBlockColor(dataObj, rgb[ii++]);
  }
  it->Delete();

  svtkNew<svtkHardwareSelector> hw;
  hw->SetArea(0, 0, 400, 400);
  hw->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_CELLS);
  hw->SetRenderer(rr);
  hw->SetProcessID(0);

  int testNum = 0;
  std::set<int> expected;

  // Nothing visible, but everything pickable.
  prepareDisplayAttribute(expected, da, mb, [](int) { return std::pair<bool, bool>(false, true); });
  mp->Modified();
  auto sel = hw->Select();
  int retVal = checkSelection(sel, expected, testNum);
  sel->Delete();

  // Everything visible, but nothing pickable.
  prepareDisplayAttribute(expected, da, mb, [](int) { return std::pair<bool, bool>(true, false); });
  mp->Modified();
  sel = hw->Select();
  retVal &= checkSelection(sel, expected, testNum);
  sel->Delete();

  // One block in every possible state.
  prepareDisplayAttribute(expected, da, mb, [](int nn) {
    --nn;
    return std::pair<bool, bool>(!!(nn / 2), !!(nn % 2));
  });
  mp->Modified();
  sel = hw->Select();
  retVal &= checkSelection(sel, expected, testNum);
  sel->Delete();

  // One block in every possible state (but different).
  prepareDisplayAttribute(expected, da, mb, [](int nn) {
    --nn;
    return std::pair<bool, bool>(!(nn / 2), !(nn % 2));
  });
  mp->Modified();
  sel = hw->Select();
  retVal &= checkSelection(sel, expected, testNum);
  sel->Delete();

  // Everything visible and pickable..
  prepareDisplayAttribute(expected, da, mb, [](int) { return std::pair<bool, bool>(true, true); });
  mp->Modified();
  rw->Render();
  sel = hw->Select();
  retVal &= checkSelection(sel, expected, testNum);
  sel->Delete();

  int retTestImage = svtkRegressionTestImage(rw);
  retVal &= retTestImage;
  if (retTestImage == svtkRegressionTester::DO_INTERACTOR)
  {
    ri->Start();
  }

  return !retVal;
}
