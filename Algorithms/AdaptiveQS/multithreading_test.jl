
include("quickSort.jl")

function threadWork(channels, sizes, channels_ready, sizes_ready)
    
    descending = true # spuščamo se v globino
    myID = Threads.threadid()
    
    while descending
        
        Base.lock(channels_ready[myID])
        args = take!(channels[myID])
        
        if isready(channels[myID])
            Base.unlock(channels_ready[myID])
        end
        
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
                    
                    put!(channels[i + myID - 1], ("swaps", (view(src, starts[i]:ends[i]), pivot)))
                    Base.unlock(channels_ready[i + myID - 1])
                end
                
                # priprava za organiziranje
                put!(channels[myID], ("organize", (src, tgt, size, procesi, forward)))
                # Base.unlock(channels_ready[myID]) -> nepotrebno
            end
        elseif args[1] == "organize"
            
            src, tgt, size, procesi, forward = args[2]
            
            # zbiranje rezultatov
            for i in 1:procesi
                
                Base.lock(sizes_ready[i + myID - 1])
            end
            sizes = [subSizes[i + myID - 1] for i in 1:procesi]
            
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
                
                put!(channels[i + myID - 1], ("copy", (view(src, starts[i]:starts[i]+sizes[i][1]-1),
                                                       view(tgt, lowers[i]:lowers[i]+sizes[i][1]-1))))
                Base.unlock(channels_ready[i + myID - 1])
            end
            
            # priprava za drugo kopiranje
            put!(channels[myID], ("organize2", (src, tgt, size, procesi, lowers, highers, starts, sizes, forward)))
            # Base.unlock(channels_ready[myID]) -> nepotrebno
        elseif args[1] == "organize2"
            
            src, tgt, size, procesi, lowers, highers, starts, sizes, forward = args[2]
            
            # čakanje ostalih niti
            for i in 1:procesi
                
                Base.lock(sizes_ready[i + myID - 1])
            end
            
            # klicanje asinhronih metod
            for i = 1:procesi
                
                put!(channels[i + myID - 1], ("copy", (view(src, starts[i]+sizes[i][1]:starts[i]+sizes[i][1]+sizes[i][2]-1),
                                                       view(tgt, highers[i]:highers[i]+sizes[i][2]-1))))
                Base.unlock(channels_ready[i + myID - 1])
            end
            
            # priprava za deljenje
            put!(channels[myID], ("divide", (src, tgt, size, procesi, lowers, sizes, forward)))
            # Base.unlock(channels_ready[myID]) -> nepotrebno
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
            put!(channels[myID],           ("prep", (view(src, 1:lowers[procesi]+sizes[procesi][1]-1),
                                                     view(tgt, 1:lowers[procesi]+sizes[procesi][1]-1),
                                                     size1, !forward, prviDel)))
            put!(channels[myID + prviDel], ("prep", (view(src, lowers[procesi]+sizes[procesi][1]:size),
                                                     view(tgt, lowers[procesi]+sizes[procesi][1]:size),
                                                     size2, !forward, procesi - prviDel)))
            Base.unlock(channels_ready[myID])
            Base.unlock(channels_ready[myID + prviDel])
        elseif args[1] == "swaps"
            
            src, pivot = args[2]
            size = onePassSwaps!(src, pivot)
            sizes[myID] = size
            Base.unlock(sizes_ready[myID])
        elseif args[1] == "copy"
            
            src, dest = args[2]
            copyArray!(src, dest)
            Base.unlock(sizes_ready[myID])
        else
            
            error("Neznan korak: " * args[1])
        end
    end
    
    return nothing
end

channels = [Channel(2) for i = 1:Threads.nthreads()] # element = (tip::String, args::Tuple)
subSizes = [    (0, 0) for i = 1:Threads.nthreads()]

channels_ready = [Threads.Mutex() for i = 1:Threads.nthreads()]
sizes_ready    = [Threads.Mutex() for i = 1:Threads.nthreads()]

for i = 1:Threads.nthreads()
    
    Base.lock(channels_ready[i])
    Base.lock(sizes_ready[i])
end

src  = collect(20:-1:1)
temp = similar(src)

display(src)

put!(channels[1], ("prep", (src, temp, size(src)[1], true, Threads.nthreads())))
Base.unlock(channels_ready[1])

Threads.@threads for i = 1:Threads.nthreads()
    threadWork(channels, subSizes, channels_ready, sizes_ready)
end

display(src)
issorted(src)
