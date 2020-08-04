
#include "dlite.h"
#include "chemistry_schema.h"
#include "chemistry.h"

/* Forward declarations */
const ChemistrySchema *get_chemistry();



int main()
{
  const DLiteInstance *chem = (DLiteInstance *)get_chemistry();
  dlite_instance_print(chem);




  return 0;
}
