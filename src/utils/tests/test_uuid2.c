/*
 * utest.c
 * Copyright (C) 2016 Kurten Chan <chinkurten@gmail.com>
 *
 * Distributed under terms of the MIT license.
 */
/*
 * Minor modifications by Jesper Friis (2017)
 */

#include <stdio.h>
#include "uuid.h"

/* Get rid of MSVC warnings */
#ifdef _MSC_VER
# pragma warning( disable : 4217 )
#endif


/* puid -- print a UUID */
void puid(uuid_s u)
{
    int i;

    printf("%8.8x-%4.4x-%4.4x-%2.2x%2.2x-", u.time_low, u.time_mid,
    u.time_hi_and_version, u.clock_seq_hi_and_reserved,
    u.clock_seq_low);
    for (i = 0; i < 6; i++)
        printf("%2.2x", u.node[i]);
    printf("\n");
}

/* Simple driver for UUID generator */
int main()
{
    uuid_s u;
    int f;

    uuid_create_random(&u);
    printf("uuid_create(): "); puid(u);

    f = uuid_compare(&u, &u);
    printf("uuid_compare(u,u): %d\n", f);     /* should be 0 */
    f = uuid_compare(&u, &NameSpace_DNS);
    printf("uuid_compare(u, NameSpace_DNS): %d\n", f); /* s.b. 1 */
    f = uuid_compare(&NameSpace_DNS, &u);
    printf("uuid_compare(NameSpace_DNS, u): %d\n", f); /* s.b. -1 */
    uuid_create_md5_from_name(&u, NameSpace_DNS, "www.widgets.com", 15);
    printf("uuid_create_md5_from_name(): "); puid(u);
    uuid_create_sha1_from_name(&u, NameSpace_DNS, "www.widgets.com", 15);
    printf("uuid_create_sha1_from_name(): "); puid(u);

    return 0;
}
