#ifndef svtkMultiBaselineRegressionTest_h
#define svtkMultiBaselineRegressionTest_h

#include "svtkNew.h"
#include "svtkRenderWindow.h"
#include "svtkTesting.h"

#include "svtksys/SystemTools.hxx"

#include <string.h>

/**\brief Run a regression test with an explicitly-provided image filename.
 *
 * Unlike the traditional C++ image-based test macro (svtkRegressionTestImage),
 * this templated function accepts the name of a baseline image.
 * It uses the existing svtkTesting infrastructure to expand the image name
 * into a full path by replacing the implied filename component of the valid
 * image (specified with "-V" on the command line) with the given \a img
 * value. The directory portion of the valid image path preceding is untouched.
 */
template <typename T>
int RegressionTestWithImageName(
  int argc, char* argv[], T* rw, const std::string& img, double thresh = 10.)
{
  svtkNew<svtkTesting> testing;
  bool isImgPath = false;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "-V"))
    {
      isImgPath = true;
    }
    else if (isImgPath)
    {
      isImgPath = false;
      std::vector<std::string> components;
      std::string originalImage = argv[i];
      svtksys::SystemTools::SplitPath(originalImage, components);
      // Substitute image filename for last component;
      components.back() = img;
      std::string tryme = svtksys::SystemTools::JoinPath(components);
      testing->AddArgument(tryme.c_str());
      continue;
    }

    testing->AddArgument(argv[i]);
  }

  if (testing->IsInteractiveModeSpecified())
  {
    return svtkTesting::DO_INTERACTOR;
  }

  if (testing->IsValidImageSpecified())
  {
    testing->SetRenderWindow(rw);
    return testing->RegressionTestAndCaptureOutput(thresh, cout);
  }

  return svtkTesting::NOT_RUN;
}

#endif
// SVTK-HeaderTest-Exclude: svtkMultiBaselineRegressionTest.h
