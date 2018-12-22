QuickSort p_thread

Deluje za poljubno število niti in poljubno velike tabele.

DELUJE S THREADI AMPAK!!!

pthread_create
pthread_join

ni isto kot 

pthread_create
pthread_detach

... zaradi join deluje počasneje... proskušal izboljšat v QS_broken

NASTAVITEV OKOLJA:

Linker -> System -> Stack reserve size = 268435456
