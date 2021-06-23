/*=========================================================================
This source has no copyright.  It is intended to be copied by users
wishing to create their own SVTK classes locally.
=========================================================================*/
/**
 * @class   svtkLocalExample
 * @brief   Example class using SVTK.
 *
 * svtkLocalExample is a simple class that uses SVTK.  This class can be
 * copied and modified to produce your own classes.
 */

#ifndef svtkLocalExample_h
#define svtkLocalExample_h

#include "svtkLocalExampleModule.h" // export macro
#include "svtkObject.h"

class SVTKLOCALEXAMPLE_EXPORT svtkLocalExample : public svtkObject
{
public:
  static svtkLocalExample* New();
  svtkTypeMacro(svtkLocalExample, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkLocalExample();
  ~svtkLocalExample() override;

private:
  svtkLocalExample(const svtkLocalExample&) = delete;
  void operator=(const svtkLocalExample&) = delete;
};

#endif
