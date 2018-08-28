/* A simple colledtion of convinient macros */
#ifndef _MACROS_H
#define _MACROS_H

/* Convenient macros for failing */
#define FAIL(msg) do { \
    err(1, msg); goto fail; } while (0)
#define FAIL1(msg, a1) do { \
    err(1, msg, a1); goto fail; } while (0)
#define FAIL2(msg, a1, a2) do { \
    err(1, msg, a1, a2); goto fail; } while (0)
#define FAIL3(msg, a1, a2, a3) do { \
    err(1, msg, a1, a2, a3); goto fail; } while (0)


#endif /* _MACROS_H */
