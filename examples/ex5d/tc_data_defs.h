/****************************************************/
/*  Thermo Calc data types definition file          */
/****************************************************/
/*                                                  */
/*   Thermo Calc Data structures                    */
/****************************************************/
#ifdef WIN32
# if defined(_MSC_VER)  &&  (_MSC_VER > 1200)
#include <crtdefs.h>
# endif
#endif

#ifndef TC_VARS

#ifdef WIN64
typedef __int64 TC_INT;
typedef __int64 pointer;
#else
typedef long TC_INT;
typedef long pointer;
#endif

#undef FLAG64
#ifdef WIN64
#define FLAGS64
#endif
#ifdef M64
#define FLAG64
#endif

typedef double TC_FLOAT;

#ifdef WIN32
#define TCHANDLE HINSTANCE
#else
#define TCHANDLE void*
#endif



typedef TC_INT TC_BOOL;
typedef char* TC_STRING;
#ifdef WIN32
#ifdef __GNUC__
typedef long TC_STRING_LENGTH;
#else
typedef size_t TC_STRING_LENGTH;
#endif
#else
typedef long TC_STRING_LENGTH;
#endif

#define TC_NWSG			4000000
#define TC_NWSE			500000

#define TC_STRLEN_SPECIES       25
#define TC_STRLEN_PHASES        25
#define TC_STRLEN_ELEMENTS       3
#define TC_STRLEN_COMPONENTS    25
#define TC_STRLEN_CONSTITUENTS  25
#define TC_STRLEN_DATABASE       9
#define TC_STRLEN_STOICHIOMETRY 81
#define TC_STRLEN_MAX          256
#define TC_STRLEN_PATH_MAX     512
#define TC_STRLEN_REFERENCE   1024

#define TC_MAX_NR_OF_ELEMENTS 40
#define TC_MAX_NR_OF_SPECIES 5000
#define TC_MAX_NR_OF_SUBLATTICES 10
#define TC_MAX_NR_OF_CONSTITUENTS 200
#define TC_MAX_NR_OF_CONST_PER_SUBLATTICE 200
#define TC_MAX_NR_OF_CONST_PER_SUBLATTICE_IN_IDEAL_GAS 5000
#define TC_MAX_NR_OF_DATABASES 130
#define TC_MAX_NR_OF_AXES 5
/*#define TC_MAX_NR_OF_PHASES 500*/
#define TC_MAX_NR_OF_PHASES 4000 /* check ITDBPX in tdbmax.inc */
#define TC_EPS 1.00E-8


/*    Example of use of the tc_elements_strings structure defined below
      Note that the C-definitions of the strings are one character
      longer than the expected fortran strings.


  tc_elements_strings *elements;

  nel=tc_ges5_no_of_elements();
  elements = malloc(nel * sizeof(tc_elements_strings));
  tc_ges5_elements((TC_STRING)elements,TC_STRLEN_ELEMENTS);
  for (i=0; i<nel; i++) {
	fprintf(stdout,"%d %s\n",i+1,elements[i].element);
  }
  free(elements);

*/

typedef struct tc_conditions_as_arrays_of_strings tc_conditions_as_arrays_of_strings;
struct tc_conditions_as_arrays_of_strings
{
  char condition[TC_STRLEN_MAX];
};


typedef struct tc_elements_strings tc_elements_strings;
struct tc_elements_strings
{
  char element[TC_STRLEN_ELEMENTS];
};

typedef struct tc_components_strings tc_components_strings;
struct tc_components_strings
{
  char component[TC_STRLEN_COMPONENTS];
};

typedef struct tc_species_strings tc_species_strings;
struct tc_species_strings
{
  char specie[TC_STRLEN_SPECIES];
};


typedef struct tc_phases_strings tc_phases_strings;
struct tc_phases_strings
{
  char phase[TC_STRLEN_PHASES];
};

typedef struct tc_constituents_strings tc_constituents_strings;
struct tc_constituents_strings
{
  char constituent[TC_STRLEN_CONSTITUENTS];
};

typedef struct tc_databases_strings tc_databases_strings;
struct tc_databases_strings
{
  char database[TC_STRLEN_DATABASE];
};

typedef struct tc_reference_strings tc_reference_strings;
struct tc_reference_strings
{
  char reference[TC_STRLEN_REFERENCE];
};

typedef TC_INT TC_IARR[4];
typedef char TC_LABEL_STRING[127];

#endif
#define TC_VARS
/****************************************************/

/*  General Definitions */
/****************************************************/
#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif
/****************************************************/

/*Export Definitions */
/****************************************************/
#ifdef __GNUC__
#else
#ifndef DllExport
#define DllExport __declspec( dllexport )
#endif
#endif


#ifdef __GNUC__
#define TCFuncExport extern
#else

#ifdef __cplusplus
#define TCFuncExport extern "C" DllExport
#else
#define TCFuncExport extern DllExport
#endif
#endif

/****************************************************/

/* Fortran API */
/****************************************************/

#define     INTEGER_FUNC    extern           TC_INT

#define     INTEGER_FUNC_WIN    INTEGER_FUNC
#define     INTEGER_FUNC_GNU    INTEGER_FUNC
#define     VOID_FUNC_WIN       extern       void
#define     BOOL_FUNC_WIN       extern       TC_BOOL
#define     FLOAT_FUNC_WIN      extern       TC_FLOAT
/****************************************************/
