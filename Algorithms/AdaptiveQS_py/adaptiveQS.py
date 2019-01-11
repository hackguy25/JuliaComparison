import quickSort as qs
import numpy as np
import multiprocessing as mp

from os import cpu_count
from time import time_ns

def adaptiveQS_mp (tb_obj: mp.RawArray, tt_obj: mp.RawArray, tb_dims: tuple, forward = True, procesi = -1):

    tabela = np.frombuffer(tb_obj, np.int64)[tb_dims[0]:tb_dims[1]]
    tempTabela = np.frombuffer(tt_obj, np.int64)[tb_dims[0]:tb_dims[1]]

    if procesi < 1:
        procesi = cpu_count()

    if procesi == 1:
        if forward:
            qs.inPlaceQuickSort(tabela)
        else:
            qs.outwardQuickSort(tempTabela, tabela)
        return

    pivot = qs.generatePivot(tabela)
    step = (tb_dims[1] - tb_dims[0]) / procesi
    starts = [round(i * step) for i in range(procesi)]
    ends   = [round((i + 1) * step) for i in range(procesi)]
    ends[procesi - 1] = tb_dims[1] - tb_dims[0] # za vsak primer
    if forward:
        src = tb_obj
        dest = tt_obj
    else:
        src = tt_obj
        dest = tb_obj

    # klicanje asinhronih metod
    sizes = [mp.Queue() for i in range(procesi)]
    ret = [mp.Process(target = qs.onePassSwapsThreadable,
                        args = (src, (tb_dims[0] + starts[i], tb_dims[0] + ends[i]), pivot, sizes[i])) for i in range(procesi)]
    [process.start() for process in ret]
    [process.join()  for process in ret]
    sizes = [q.get() for q in sizes]

    # print(np.frombuffer(tb_obj, np.int64)[tb_dims[0]:tb_dims[1]])
    # print(sizes)

    # računanje velikosti delov in odmikov
    lowers  = [0]
    highers = [0]
    starts  = [0]

    for (i, lens) in enumerate(sizes[0:procesi-1]):
        lowers.append(lowers[i] + lens[0])
        highers.append(highers[i] + lens[1])
        starts.append(starts[i] + lens[0] + lens[1])

    lower_half = lowers[-1] + sizes[-1][0]
    for i in range(procesi):
        highers[i] += lower_half

    # lowers
    ret = [mp.Process(target = qs.copyArrayThreadable,
                      args = (src, dest, (tb_dims[0] + starts[i], tb_dims[0] + starts[i] + sizes[i][0]),
                                         (tb_dims[0] + lowers[i], tb_dims[0] + lowers[i] + sizes[i][0]))) for i in range(procesi)]
    [thread.start() for thread in ret]
    [thread.join() for thread in ret]

    # print(np.frombuffer(tt_obj, np.int64)[tb_dims[0]:tb_dims[1]])

    # highers
    ret = [mp.Process(target = qs.copyArrayThreadable,
                      args = (src, dest, (tb_dims[0] + starts[i] + sizes[i][0], tb_dims[0] + starts[i] + sizes[i][0] + sizes[i][1]),
                                         (tb_dims[0] + highers[i], tb_dims[0] + highers[i] + sizes[i][1]))) for i in range(procesi)]
    [thread.start() for thread in ret]
    [thread.join() for thread in ret]

    # print(np.frombuffer(tt_obj, np.int64)[tb_dims[0]:tb_dims[1]])

    # razpoložljive procese razdelimo čim bolj "pravično" na dva dela
    prviDel = round(lower_half * procesi / (tb_dims[1] - tb_dims[0]))
    if prviDel <= 0:
        prviDel = 1
    elif prviDel >= procesi:
        prviDel = procesi-1

    # asinhrona rekurzivna klica
    ret = [mp.Process(target = adaptiveQS_mp, args = (tb_obj, tt_obj, (tb_dims[0], tb_dims[0] + lower_half), not forward, prviDel)),
           mp.Process(target = adaptiveQS_mp, args = (tb_obj, tt_obj, (tb_dims[0] + lower_half, tb_dims[1]), not forward, procesi - prviDel))]
    [thread.start() for thread in ret]
    [thread.join() for thread in ret]

def adaptiveQS (tabela: np.ndarray, procesi = -1):

    tb_obj = mp.RawArray('q', tabela.size)
    tt_obj = mp.RawArray('q', tabela.size)

    tabela_temp = np.frombuffer(tb_obj, dtype = np.int64)
    np.copyto(tabela_temp, tabela)

    adaptiveQS_mp(tb_obj, tt_obj, (0, tabela.size), True, procesi)
    np.copyto(tabela, np.frombuffer(tb_obj, dtype = np.int64))

def _hitriTest(n, p = -1):

    a = np.random.randint(-9223372036854775808, 9223372036854775808, n, np.int64)
    # a = np.array(range(100, 0, -1))
    # print(a)

    t0 = time_ns()
    adaptiveQS(a, p)
    t1 = time_ns()

    print(((t1 - t0) / 1000000))

    # print(a)
    # print(np.all(a[:-1] <= a[1:])) # najdu na netu

def testKPonovitev(k, n):

    sum = 0.0

    for i in range(k):

        a = np.random.randint(-9223372036854775808, 9223372036854775808, n, np.int64)

        t0 = time_ns()
        adaptiveQS(a, -1)
        t1 = time_ns()

        delta = (t1 - t0) / 1000000
        sum += delta

        print(delta)

    sum /= k

    print("Povprečno:", sum)
