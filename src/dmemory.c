/******************************************************************/
/*Justin Hall                                                     */
/*                                                                */
/*dmemory.c contains the code for a custom implementation of      */
/*  malloc and free. For the most part the two can be used        */
/*  as-is on both Windows and Linux, though they contain some     */
/*  additional features. The biggest is the ability to take       */
/*  control of when _malloc actually frees the memory it gets     */
/*  from the OS. This can be done by calling toggleMemFree(0).    */
/*  When this happens, malloc will not return any of the memory   */
/*  it comes to manage and will enforce a strict policy of        */
/*  container-based memory management. Make sure to call          */
/*  flush() if you choose to turn memFree off.                    */
/*                                                                */
/*While the early working and late version of this program were   */
/*  designed by me, a guide helped me to fix parts                */
/*  of the early version that weren't working right and gain a    */
/*  better understanding of how basic memory allocation is        */
/*  performed on Linux.                                           */
/*                                                                */
/*For citation purposes a link to the guide is found below:       */
/*  http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf               */
/******************************************************************/

#include <stdio.h>
#include "dmemory.h"

#define CHUNK 10

#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef linux
#include <unistd.h>
#endif

typedef struct block_s
{
  int_t size;
  int_t elemSize;
  char free;
  struct block_s *next;
  struct block_s *prev;
} block_t;

static block_t *blocks = NULL;
static char memFree = 1;

/**************************************************/
/*block_t *block:                                 */
/*  in/out,                                       */
/*  pointer to a block of memory,                 */
/*  should be a valid pointer to valid memory.    */
/*int_t size                                      */
/*  in,                                           */
/*  size needed for the active memory request,    */
/*  should be within the bounds of the memory     */
/*  block.                                        */
/*No return.                                      */
/*This function takes a block of memory that is   */
/*  too large for the current request, carves     */
/*  off a chunk to satisfy only the current       */
/*  request and sets up the remaining memory as   */
/*  a free block.                                 */
/*newBlock is created and is set to point to      */
/*  a location sizeof block_t + size distance     */
/*  from the current *block pointer. The reasoning*/
/*  is that + block_t = a pointer to the start    */
/*  of *block's existing memory, and a further    */
/*  + size = the offset from that point to the    */
/*  start of where the new, free block of memory  */
/*  should start (thus splitting *block in two).  */
/*  newBlock is then established as a free block  */
/*  starting after *block, and block's next       */
/*  pointer and size variable are then adjusted   */
/*  to reflect the changes.                       */
/**************************************************/
void split(block_t *block, int_t size)
{
  if (block->size - size < sizeof(block_t)) return;

  block_t *newBlock = (block_t*)(((char*)block) + sizeof(block_t) + size);
  newBlock->size = block->size - (size + sizeof(block_t));
  newBlock->elemSize = block->elemSize;
  newBlock->free = 1;
  newBlock->prev = block;
  newBlock->next = block->next;
  block->next = newBlock;
  block->size = size;
}

/**************************************************/
/*block_t *block:                                 */
/*  in/out,                                       */
/*  pointer to a block of memory,                 */
/*  should be a valid pointer to valid memory.    */
/*Returns a pointer to the new start of the       */
/*  block of memory after merging.                */
/*The purpose of this function is to merge        */
/*  two blocks of memory that are consecutive     */
/*  into one single block so that _malloc knows   */
/*  exactly how much space each free block        */
/*  actually represents.                          */
/*This function simply adjusts the size of        */
/*  the *block pointer to be + sizeof block_t     */
/*  + size of the next block of memory so that    */
/*  the entire space can be taken as a single     */
/*  unit. The next pointer of *block is then      */
/*  adjusted to point to the next->next block,    */
/*  skipping over the old block that was once     */
/*  supposed to be between them. If there         */
/*  is a next->next block, its previous pointer   */
/*  is set to point to *block.                    */
/**************************************************/
block_t *mergeNext(block_t *block)
{
  if (!block->next) return block;

  block->size += sizeof(block_t) + block->next->size;
  block->next = block->next->next;
  if (block->next) block->next->prev = block;

  return block;
}

/**************************************************/
/*int_t size:                                     */
/*  in,                                           */
/*  size needed for the block of memory to be     */
/*  returned,                                     */
/*  should be > 0.                                */
/*int_t elemSize:                                 */
/*  in,                                           */
/*  size of each individual element for the block */
/*  of memory (such as sizeof int).               */
/*Returns a pointer to the free block of memory   */
/*  that was of the size requested (NULL if it    */
/*  failed).                                      */
/*The purpose of this function is to either       */
/*  take an existing free block and repurpose     */
/*  it or to request memory from the OS.          */
/*This function first checks its active blocks    */
/*  to see if something existing can satisfy      */
/*  the request. It enforces a strict policy of   */
/*  matching each block with the labeled element  */
/*  size (categorized memory) to hopefully help   */
/*  prevent some fragmentation. The idea here     */
/*  is that int-sized memory would all be         */
/*  grouped, char-sized memory all grouped, and   */
/*  etc. If no active blocks exist, memory        */
/*  is requested from the OS and is set up as     */
/*  a brand new block.                            */
/**************************************************/
void *_malloc(int_t size, int_t elemSize)
{
  block_t *block = blocks;
  block_t *prev = NULL;

  while (block)
  {
    if (block->size >= size && block->free &&
        block->elemSize == elemSize)
    {
      split(block, size);
      block->free = 0;
      return ((char*)block) + sizeof(block_t);
    }
    prev = block;
    block = block->next;
  }

#ifdef linux

  block = (block_t*)sbrk(0); // get pointer to top of current heap
  // this allocates more than needed and splits it up before return
  sbrk((sizeof(block_t) + elemSize) * (size / elemSize * CHUNK));

  if (block == (void*)-1) return NULL;

#endif
#ifdef _WIN32

  block = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                    (sizeof(block_t) + elemSize) * (size / elemSize * CHUNK));

  if (!block) return NULL;

#endif

  block->next = NULL;
  block->prev = prev;
  block->size = (sizeof(block_t) + elemSize) *
                (size / elemSize * CHUNK) - sizeof(block_t);
  block->elemSize = elemSize;
  block->free = 0;
  split(block, size);

  if (!blocks) blocks = block;
  if (prev) prev->next = block;

  return ((char*)block) + sizeof(block_t);

  return NULL;
}

/**************************************************/
/*void *memptr:                                   */
/*  in,                                           */
/*  pointer to the block of memory to be freed,   */
/*  should *only* be memory received from         */
/*  custom _malloc.                               */
/*No return.                                      */
/*The purpose of this is to free an existing      */
/*  block of memory for later use or to the       */
/*  OS.                                           */
/*This function, if memFree is set, will actively */
/*  combine blocks of memory, even if they        */
/*  are of different element sizes, and will      */
/*  free memory at the end of the current heap    */
/*  whenever it can. If memFree is turned off,    */
/*  this function will strictly enforce           */
/*  keeping memory grouped together by elemSize   */
/*  and will not return any memory back to the OS.*/
/*  The idea here is that memFree = 1 results     */
/*  in a more standard malloc-free setup, while   */
/*  memFree = 0 results in a stricter environment */
/*  that both preserves memory grouping but also  */
/*  forces the user to call flush() when they are */
/*  done.                                         */
/**************************************************/
void _free(void *memptr)
{
  if (!memptr) return;

  block_t *block = (block_t*)(((char*)memptr) - sizeof(block_t));

  if (block->free) return; // was already deallocated
  block->free = 1;

  // first try to merge with the previous
  if (memFree)
  {
    if (block->prev && block->prev->free) block = mergeNext(block->prev);
    // now try to merge with next
    if (block->next && block->next->free) block = mergeNext(block);
  }
  else
  {
    if (block->prev && block->prev->free &&
        block->prev->elemSize == block->elemSize)
    {
      block = mergeNext(block->prev);
    }
    // now try to merge with next
    if (block->next && block->next->free &&
        block->next->elemSize == block->elemSize)
    {
      block = mergeNext(block);
    }
  }

#ifdef linux

  if (!block->next && memFree) // free it
  {
    if (block->prev) block->prev->next = NULL;
    else blocks = NULL;

    brk(block); // move the break back (free memory)
  }

#endif
#ifdef _WIN32

  if (!block->next && memFree) // free it
  {
    if (block->prev) block->prev->next = NULL;
    else blocks = NULL;

    HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (LPVOID*)block, 0);
  }

#endif
}

/**************************************************/
/*char value:                                     */
/*  in,                                           */
/*  the value to set memFree to,                  */
/*  should only be 0 or 1 (0 for off, 1 for on).  */
/*No return.                                      */
/*The purpose of this is to toggle memFree on/off.*/
/*This function sets memFree equal to value       */
/**************************************************/
void toggleMemFree(char value)
{
  memFree = value;
}

/**************************************************/
/*No inputs.                                      */
/*No return.                                      */
/*The purpose of this function is to return all   */
/*  existing memory from _malloc to the OS.       */
/*This function is to be used when memFree = 0.   */
/*  It walks through the existing memory,         */
/*  combining it freely into a single block of    */
/*  memory regardless of elemSize or whether      */
/*  it is still considered active memory. After it*/
/*  does this, this single block containing all   */
/*  memory received from the OS is returned back  */
/*  to the OS. All dynamic pointers are invalid   */
/*  after flush() is called.                      */
/**************************************************/
void flush()
{
  if (!blocks || memFree) return;

  while (blocks->next) blocks = mergeNext(blocks);

#ifdef linux
  brk(blocks);
#endif
#ifdef _WIN32
  HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (LPVOID*)blocks, 0);
#endif

  blocks = NULL;
}