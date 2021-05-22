#ifndef _DLITE_PRINT_H
#define _DLITE_PRINT_H


typedef enum {
  dlitePrintUUID,  /*!< Whether to include uuid */
} DLitePrintFlag;

/**
  Prints instance `inst` to buffer, formatted as JSON.
*/
int dlite_print(char *dest, size_t size, DLiteInstance *inst,
                int indent, DLitePrintFlag flags);


#endif /*_ DLITE_PRINT_H */
