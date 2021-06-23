#ifndef svtkWrappable_h
#define svtkWrappable_h

#include "svtkWrappableModule.h" // for export macro

#include "svtkObject.h"

#include <string> // for std::string

class SVTKWRAPPABLE_EXPORT svtkWrappable : public svtkObject
{
public:
  static svtkWrappable* New();
  svtkTypeMacro(svtkWrappable, svtkObject);
  void PrintSelf(std::ostream& os, svtkIndent indent) override;

  std::string GetString() const;

protected:
  svtkWrappable();
  ~svtkWrappable() override;

private:
  svtkWrappable(const svtkWrappable&) = delete;
  void operator=(const svtkWrappable&) = delete;
};

#endif
