README for CS 61 Problem Set 6
------------------------------
YOU MUST FILL OUT THIS FILE BEFORE SUBMITTING!
** DO NOT PUT YOUR NAME OR YOUR PARTNER'S NAME! **

RACE CONDITIONS
---------------
Write a SHORT paragraph here explaining your strategy for avoiding
race conditions. No more than 400 words please.
We avoided race conditions by enforcing a strict control flow and by providing atomicity. Anything we identified as a shared resource, we use synchronization primitives (i.e. mutexs) to prevent concurrent access to. For example the connection table noted in Phase 3 or the calculating the cool off period noted in Phase 4 and so on. Above those critical sections we documented what primitives protect what resources as well as the assumptions about the synchronization happening.

OTHER COLLABORATORS AND CITATIONS (if any):
N/A


KNOWN BUGS (if any):
All phases work. There may be a bug related to the maximum amount of concurrent connections.


NOTES FOR THE GRADER (if any):
N/A


EXTRA CREDIT ATTEMPTED (if any):
N/A
