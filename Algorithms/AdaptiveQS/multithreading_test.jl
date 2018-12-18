
include("quickSort.jl")

function threadSafePut!(c::Channel, m::Threads.Mutex, e)
    
    Base.lock(m)
    put!(c, e)
    Base.unlock(m)
end

function threadSafeTake!(c::Channel, m::Threads.Mutex)
    
    Base.lock(m)
    while !isready(c)
        Base.unlock(m)  # will this hackery work?
        Base.sleep(0.001)
        Base.lock(m)    # maybe!
    end
    ret = take!(c)
    Base.unlock(m)
    return ret
end

function threadWork(channels, subSizes, channels_ready, sizes_ready, debugChannel)
    
    descending = true # spuščamo se v globino
    myID = Threads.threadid()
    
    while descending
        
        args = threadSafeTake!(channels[myID], channels_ready[myID])
        
        if args[1] == "prep"
            
            tabela, tempTabela, size, forward, procesi = args[2]
            
            if procesi == 1
                
                descending = false
                
                if forward
                    inPlaceQuickSort!(tabela)
                else
                    outwardQuickSort!(tempTabela, tabela)
                end
            else
            
                # generiranje pivota, indeksov podtabel
                pivot  = generatePivot(tabela)
                
                put!(debugChannel, pivot)
                
                step   = size / procesi
                starts = [Int(floor((i - 1) * step)) + 1 for i in 1:procesi]
                ends   = [Int(floor(i * step)) for i in 1:procesi]
                ends[procesi] = size # za vsak primer
                if forward
                    src = tabela
                    tgt = tempTabela
                else
                    src = tempTabela
                    tgt = tabela
                end
                
                # klicanje asinhronih metod
                for i = 1:procesi
                    
                    threadSafePut!(channels[i + myID - 1], channels_ready[i + myID - 1], ("swaps", (view(src, starts[i]:ends[i]), pivot)))
                end
                
                # priprava za organiziranje
                threadSafePut!(channels[myID], channels_ready[myID], ("organize", (src, tgt, size, procesi, forward)))
            end
        elseif args[1] == "organize"
            
            src, tgt, size, procesi, forward = args[2]
            
            sizes = [threadSafeTake!(subSizes[i + myID - 1], sizes_ready[i + myID - 1]) for i in 1:procesi]
            
            # računanje velikosti delov in odmikov
            lowers  = Array{Int, 1}(undef, procesi)
            highers = Array{Int, 1}(undef, procesi)
            starts  = Array{Int, 1}(undef, procesi)
            
            lowers[1]  = 1
            highers[1] = 1
            starts[1]  = 1
            
            for (i, lens) in enumerate(sizes[1:procesi-1])
                lowers[i+1]  = lowers[i]  + lens[1]
                highers[i+1] = highers[i] + lens[2]
                starts[i+1]  = starts[i]  + lens[1] + lens[2]
            end
            
            highers .+= lowers[procesi] + sizes[procesi][1] - 1 #luškan mali trik
            
            # klicanje asinhronih metod
            for i = 1:procesi
                
                threadSafePut!(channels[i + myID - 1], channels_ready[i + myID - 1],
                               ("copy", (view(src, starts[i]:starts[i]+sizes[i][1]-1),
                                         view(tgt, lowers[i]:lowers[i]+sizes[i][1]-1))))
            end
            
            # priprava za drugo kopiranje
            threadSafePut!(channels[myID], channels_ready[myID], ("organize2", (src, tgt, size, procesi, lowers, highers, starts, sizes, forward)))
        elseif args[1] == "organize2"
            
            src, tgt, size, procesi, lowers, highers, starts, sizes, forward = args[2]
            
            # čakanje ostalih niti
            [threadSafeTake!(subSizes[i + myID - 1], sizes_ready[i + myID - 1]) for i in 1:procesi]
            
            # klicanje asinhronih metod
            for i = 1:procesi
                
                threadSafePut!(channels[i + myID - 1], channels_ready[i + myID - 1],
                               ("copy", (view(src, starts[i]+sizes[i][1]:starts[i]+sizes[i][1]+sizes[i][2]-1),
                                         view(tgt, highers[i]:highers[i]+sizes[i][2]-1))))
            end
            
            # priprava za deljenje
            threadSafePut!(channels[myID], channels_ready[myID], ("divide", (src, tgt, size, procesi, lowers, sizes, forward)))
        elseif args[1] == "divide"
            
            src, tgt, size, procesi, lowers, sizes, forward = args[2]
            
            # razpoložljive procese razdelimo čim bolj "pravično" na dva dela
            prviDel = Int(floor((lowers[procesi] + sizes[procesi][1] - 1) * procesi / size)) + 1
            if prviDel <= 0
                prviDel = 1
            elseif prviDel >= procesi
                prviDel = procesi-1
            end
            
            # povrnemo začetno orientacijo
            if !forward
                src, tgt = tgt, src
            end
            
            # računanje velikosti tabel
            size1 = lowers[procesi] + sizes[procesi][1] - 1
            size2 = size - size1
            
            # asinhrona rekurzivna klica
            threadSafePut!(channels[myID], channels_ready[myID],
                           ("prep", (view(src, 1:lowers[procesi]+sizes[procesi][1]-1),
                                     view(tgt, 1:lowers[procesi]+sizes[procesi][1]-1),
                                     size1, !forward, prviDel)))
            threadSafePut!(channels[myID + prviDel], channels_ready[myID + prviDel],
                           ("prep", (view(src, lowers[procesi]+sizes[procesi][1]:size),
                                     view(tgt, lowers[procesi]+sizes[procesi][1]:size),
                                     size2, !forward, procesi - prviDel)))
        elseif args[1] == "swaps"
            
            src, pivot = args[2]
            size = onePassSwaps!(src, pivot)
            threadSafePut!(subSizes[myID], sizes_ready[myID], size)
        elseif args[1] == "copy"
            
            src, dest = args[2]
            copyArray!(src, dest)
            threadSafePut!(subSizes[myID], sizes_ready[myID], true)
        else
            
            error("Neznan korak: " * args[1])
        end
    end
    
    return nothing
end

channels = [Channel(2) for i = 1:Threads.nthreads()] # element = (tip::String, args::Tuple)
subSizes = [Channel(1) for i = 1:Threads.nthreads()]

channels_ready = [Threads.Mutex() for i = 1:Threads.nthreads()]
sizes_ready    = [Threads.Mutex() for i = 1:Threads.nthreads()]

src  = rand(Int, 1000000)
temp = similar(src)

display(src)

debugChannel = Channel(100)

threadSafePut!(channels[1], channels_ready[1], ("prep", (src, temp, size(src)[1], true, Threads.nthreads())))

Threads.@threads for i = 1:Threads.nthreads()
    threadWork(channels, subSizes, channels_ready, sizes_ready, debugChannel)
end

display(src)
issorted(src)
