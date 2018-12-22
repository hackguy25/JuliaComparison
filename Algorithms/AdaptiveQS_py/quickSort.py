import numpy as np
import multiprocessing as mp

def generatePivot (src: np.ndarray):

    if src.size > 15:

        sum = 0.0
        sum += np.random.choice(src)
        sum += np.random.choice(src)
        sum += np.random.choice(src)
        sum /= 3
        return round(sum)
    else:

        return int(np.random.choice(src))

def onePassSwaps (src: np.ndarray, pivot):

    # inicializacija
    i = -1
    j = src.size

    # Hoarova particija - https://en.wikipedia.org/wiki/Quicksort
    while i < j:

        i += 1
        while i < src.size and src[i] < pivot:
            i += 1

        j -= 1
        while j >= 0 and src[j] > pivot:
            j -= 1

        if i >= j:
            return (i, src.size - i) # dolžina levega in desnega dela tabele

        src[i], src[j] = src[j], src[i]

def onePassSwapsThreadable (src_obj: mp.RawArray, dims: tuple, pivot, ret: mp.Queue):

    # inicializacija
    i = -1
    j = dims[1] - dims[0]
    src = np.frombuffer(src_obj, dtype = np.int64)[dims[0]:dims[1]]

    # Hoarova particija - https://en.wikipedia.org/wiki/Quicksort
    while i < j:

        i += 1
        while i < src.size and src[i] < pivot:
            i += 1

        j -= 1
        while j >= 0 and src[j] > pivot:
            j -= 1

        if i >= j:
            ret.put((i, src.size - i)) # dolžina levega in desnega dela tabele
            return

        src[i], src[j] = src[j], src[i]

def copyArray (src: np.ndarray, dest: np.ndarray):

    for i in range(src.size):
        dest[i] = src[i]

def copyArrayThreadable (src_obj: mp.RawArray, dest_obj: mp.RawArray, src_dims: tuple, dest_dims: tuple):

    src  = np.frombuffer(src_obj,  dtype = np.int64)[src_dims[0]:src_dims[1]]
    dest = np.frombuffer(dest_obj, dtype = np.int64)[dest_dims[0]:dest_dims[1]]
    
    for i in range(src.size):
        dest[i] = src[i]

def inPlaceQuickSort (src: np.ndarray):

    if src.size > 1:

        pivot = generatePivot(src)
        sizes = onePassSwaps(src, pivot)
        inPlaceQuickSort(src[0:sizes[0]])
        inPlaceQuickSort(src[sizes[0]:src.size])

def outwardQuickSort (src: np.ndarray, dest: np.ndarray):

    if src.size > 1:

        copyArray(src, dest)
        pivot = generatePivot(dest)
        sizes = onePassSwaps(dest, pivot)
        inPlaceQuickSort(dest[0:sizes[0]])
        inPlaceQuickSort(dest[sizes[0]:dest.size])
