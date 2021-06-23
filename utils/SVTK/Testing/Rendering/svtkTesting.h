/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTesting.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTesting
 * @brief   a unified SVTK regression testing framework
 *
 *
 *  This is a SVTK regression testing framework. Looks like this:
 *
 *  svtkTesting* t = svtkTesting::New();
 *
 *  Two options for setting arguments
 *
 *  Option 1:
 *  for ( cc = 1; cc < argc; cc ++ )
 *    {
 *    t->AddArgument(argv[cc]);
 *    }
 *
 *  Option 2:
 *  t->AddArgument("-D");
 *  t->AddArgument(my_data_dir);
 *  t->AddArgument("-V");
 *  t->AddArgument(my_valid_image);
 *
 *  ...
 *
 *  Two options of doing testing:
 *
 *  Option 1:
 *  t->SetRenderWindow(renWin);
 *  int res = t->RegressionTest(threshold);
 *
 *  Option 2:
 *  int res = t->RegressionTest(test_image, threshold);
 *
 *  ...
 *
 *  if (res == svtkTesting::PASSED)
 *    {
 *    Test passed
 *    }
 *  else
 *    {
 *    Test failed
 *    }
 *
 */

#ifndef svtkTesting_h
#define svtkTesting_h

#include "svtkObject.h"
#include "svtkTestingRenderingModule.h" // For export macro
#include <string>                      // STL Header used for argv
#include <vector>                      // STL Header used for argv

class svtkAlgorithm;
class svtkRenderWindow;
class svtkImageData;
class svtkDataArray;
class svtkDataSet;
class svtkRenderWindowInteractor;

/**
 * A unit test may return this value to tell ctest to skip the test. This can
 * be used to abort a test when an unsupported runtime configuration is
 * detected.
 */
const int SVTK_SKIP_RETURN_CODE = 125;

class SVTKTESTINGRENDERING_EXPORT svtkTesting : public svtkObject
{
public:
  static svtkTesting* New();
  svtkTypeMacro(svtkTesting, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum ReturnValue
  {
    FAILED = 0,
    PASSED = 1,
    NOT_RUN = 2,
    DO_INTERACTOR = 3
  };

  static int Test(int argc, char* argv[], svtkRenderWindow* rw, double thresh);

  /**
   * This method is intended to be a comprehensive, one line replacement for
   * svtkRegressionTest and for the replay based testing using
   * svtkInteractorEventRecorder, greatly simplifying API and code
   * bloat. It scans the command line specified for the following :
   * - If a "--DisableReplay" is specified, it disables the testing
   * replay. This is particularly useful in enabling the user to
   * exercise the widgets. Typically the widgets are defined by the
   * testing replay, so the user misses out on playing around with the
   * widget definition behaviour.
   * - If a "--Record" is specified, it records the interactions into
   * a "svtkInteractorEventRecorder.log" file. This is useful when
   * creating the playback stream that is plugged into tests. The
   * file can be used to create a const char * variable for playback
   * or can copied into a location as a playback file.
   * - If a "--PlaybackFile filename is specified,the provided file
   * contains the events and is passed to the event recorder.

   * Typical usage in a test for a SVTK widget that needs playback
   * testing / recording is :

   * const char TestFooWidgetLog[] = {
   * ....
   * };

   * int TestFooWidget( int argc, char *argv[] )
   * {
   * ...
   * return svtkTesting::InteractorEventLoop(
   * argc, argv, iren,
   * TestFooWidgetLog );
   * }

   * In tests that playback events from a file:
   * TestFooEventLog.txt stored in  ../Data/Input/TestFooEventLog.txt
   * The CMakeLists.txt file should contain:

   * set(TestFoo_ARGS "--PlaybackFile"
   * "DATA{../Data/Input/TestFooEventLog.txt}")

   * and the API is
   * int TestFoo( int argc, char *argv[] )
   * {
   * ...
   * return svtkTesting::InteractorEventLoop( argc, argv, iren );
   * }

   * In tests where no playback is exercised, the API is simply

   * int TestFoo( int argc, char *argv[] )
   * {
   * ...
   * return svtkTesting::InteractorEventLoop( argc, argv, iren );
   * }
   */
  static int InteractorEventLoop(
    int argc, char* argv[], svtkRenderWindowInteractor* iren, const char* stream = nullptr);

  //@{
  /**
   * Use the front buffer first for regression test comparisons. By
   * default use back buffer first, then try the front buffer if the
   * test fails when comparing to the back buffer.
   */
  svtkBooleanMacro(FrontBuffer, svtkTypeBool);
  svtkGetMacro(FrontBuffer, svtkTypeBool);
  void SetFrontBuffer(svtkTypeBool frontBuffer);
  //@}

  /**
   * Perform the test and return the result. Delegates to
   * RegressionTestAndCaptureOutput, sending the output to cout.
   */
  virtual int RegressionTest(double thresh);

  /**
   * Perform the test and return the result. At the same time, write
   * the output to the output stream os. Includes timing information
   * in the output.
   */
  virtual int RegressionTestAndCaptureOutput(double thresh, ostream& os);

  /**
   * Perform the test and return the result. At the same time, write
   * the output to the output stream os. This method is nearly the
   * same as RegressionTestAndCaptureOutput, but does not include
   * timing information in the output.
   */
  virtual int RegressionTest(double thresh, ostream& os);

  //@{
  /**
   * Perform the test and return result. The test image will be read from the
   * png file at pngFileName.
   */
  virtual int RegressionTest(const std::string& pngFileName, double thresh);
  virtual int RegressionTest(const std::string& pngFileName, double thresh, ostream& os);
  //@}

  //@{
  /**
   * Compare the image with the valid image.
   */
  virtual int RegressionTest(svtkAlgorithm* imageSource, double thresh);
  virtual int RegressionTest(svtkAlgorithm* imageSource, double thresh, ostream& os);
  //@}

  /**
   * Compute the average L2 norm between all point data data arrays
   * of types float and double present in the data sets "dsA" and "dsB"
   * (this includes instances of svtkPoints) Compare the result of
   * each L2 comutation to "tol".
   */
  int CompareAverageOfL2Norm(svtkDataSet* pdA, svtkDataSet* pdB, double tol);

  /**
   * Compute the average L2 norm between two data arrays "daA" and "daB"
   * and compare against "tol".
   */
  int CompareAverageOfL2Norm(svtkDataArray* daA, svtkDataArray* daB, double tol);

  //@{
  /**
   * Set and get the render window that will be used for regression testing.
   */
  virtual void SetRenderWindow(svtkRenderWindow* rw);
  svtkGetObjectMacro(RenderWindow, svtkRenderWindow);
  //@}

  //@{
  /**
   * Set/Get the name of the valid image file
   */
  svtkSetStringMacro(ValidImageFileName);
  const char* GetValidImageFileName();
  //@}

  //@{
  /**
   * Get the image difference.
   */
  svtkGetMacro(ImageDifference, double);
  //@}

  //@{
  /**
   * Pass the command line arguments into this class to be processed. Many of
   * the Get methods such as GetValidImage and GetBaselineRoot rely on the
   * arguments to be passed in prior to retrieving these values. Just call
   * AddArgument for each argument that was passed into the command line
   */
  void AddArgument(const char* argv);
  void AddArguments(int argc, const char** argv);
  void AddArguments(int argc, char** argv);
  //@}

  /**
   * Search for a specific argument by name and return its value
   * (assumed to be the next on the command tail). Up to caller
   * to delete the returned string.
   */
  char* GetArgument(const char* arg);

  /**
   * This method delete all arguments in svtkTesting, this way you can reuse
   * it in a loop where you would have multiple testing.
   */
  void CleanArguments();

  //@{
  /**
   * Get some parameters from the command line arguments, env, or defaults
   */
  const char* GetDataRoot();
  svtkSetStringMacro(DataRoot);
  //@}

  //@{
  /**
   * Get some parameters from the command line arguments, env, or defaults
   */
  const char* GetTempDirectory();
  svtkSetStringMacro(TempDirectory);
  //@}

  /**
   * Is a valid image specified on the command line areguments?
   */
  int IsValidImageSpecified();

  /**
   * Is the interactive mode specified?
   */
  int IsInteractiveModeSpecified();

  /**
   * Is some arbitrary user flag ("-X", "-Z" etc) specified
   */
  int IsFlagSpecified(const char* flag);

  //@{
  /**
   * Number of pixels added as borders to avoid problems with
   * window decorations added by some window managers.
   */
  svtkSetMacro(BorderOffset, int);
  svtkGetMacro(BorderOffset, int);
  //@}

  //@{
  /**
   * Get/Set verbosity level. A level of 0 is quiet.
   */
  svtkSetMacro(Verbose, int);
  svtkGetMacro(Verbose, int);
  //@}

protected:
  svtkTesting();
  ~svtkTesting() override;

  static char* IncrementFileName(const char* fname, int count);
  static int LookForFile(const char* newFileName);

  svtkTypeBool FrontBuffer;
  svtkRenderWindow* RenderWindow;
  char* ValidImageFileName;
  double ImageDifference;
  char* TempDirectory;
  int BorderOffset;
  int Verbose;

  std::vector<std::string> Args;

  char* DataRoot;
  double StartWallTime;
  double StartCPUTime;

private:
  svtkTesting(const svtkTesting&) = delete;
  void operator=(const svtkTesting&) = delete;
};

#endif
