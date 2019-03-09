
/* Struct for describing a node in a nested translation tree.

   For each input, either the corresponding element in `inputs` (if
   the element is the result of a sub-translation tree) or
   `input_uuids` (if the input is an existing instance) is not NULL.
*/
typedef struct _TTree {
  char *name;              /* Name of translator */
  char *output_uri;        /* Output metadata URI */
  int ninput;              /* Number of inputs */
  struct _TTree **inputs;  /* Array of input sub-trees */
  char **input_uuids;      /* Array of input instance UUIDs */
} TTree;


/*
  Returns a
 */
TTree *translator_tree(const char *ouput_uri, )
