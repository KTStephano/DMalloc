/**************************************************/
/*Justin Hall                                     */
/*                                                */
/*This file contains the interface for            */
/*  custom _malloc and _free. Always call _free   */
/*  for every _malloc, and be sure to use flush   */
/*  if you tell _free not to return any memory to */
/*  the OS.                                       */
/**************************************************/

#ifndef D_MEMORY_H
#define D_MEMORY_H

typedef long long int int_t;
typedef int memhandle_t;

void *_malloc(int_t size, int_t elemSize);
void _free(void *memptr);
void toggleMemFree(char value);
void flush();

#endif // D_MEMORY_H