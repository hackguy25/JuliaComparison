QuickSort p_thread

NUM_THREADS => število niti
SIZE => velikost tabele, ki jo sortiramo
REPEAT => število ponavljanj za izračun povprečja
SORT_CHECK => default -> 0 // preverjanje pravilnosti sortiranja -> 1

Pararelno izvajanje:
  - OneSwapPass
  - FinalQuickSort

Izpuščeno:
  - "Merge"
  
Deluje za poljubno število niti in velikosti tabele.
