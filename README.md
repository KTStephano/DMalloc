# DMalloc

Citation :: http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf

A custom implementation of dynamic memory management malloc/free.

The standard malloc/free are _malloc/_free with this implementation. Along with this, _malloc behaves more like _calloc since it takes two arguments (size of each element and number of elements).

There are also two modes to this dynamic memory manager. If memFree is called with a flag of 1, _malloc and _free behave just like the standard C implementation.

However, if memFree is called with a flag of 0, it means that this implementation becomes very strict about how it uses memory. For one thing, unless flush() is explicitly called, it will never return memory to the OS. It will also keep like-sized blocks together rather than merging them to form larger blocks (so that it performs more like a pool allocator).
