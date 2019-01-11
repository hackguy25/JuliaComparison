
include("quickSort.jl")

function adaptiveQS!(src; threads = -1)
    
    tempArray = similar(src)
    if threads < 1
        threads = Threads.nthreads()
    end
    
    parts_ready = [] # src, temp, forward
    to_split    = [(src, tempArray, 1, threads, true)] # src, temp, start_thread, threads, forward
    
    while !isempty(to_split)
        
        parts    = []
        throw_out = []
        # println("To split: ", to_split)
        
        for (idx, part_to_split) in enumerate(to_split)
            
            tabela, tempTabela, myID, procesi, forward = part_to_split
            
            if procesi == 1
                
                push!(parts_ready, (tabela, tempTabela, forward))
                pushfirst!(throw_out, idx)
                continue
            end
            
            pivot = generatePivot(tabela)
            
            step   = size(tabela)[1] / procesi
            starts = [Int(round((i - 1) * step)) + 1 for i in 1:procesi]
            ends   = [Int(round(i * step)) for i in 1:procesi]
            ends[procesi] = size(tabela)[1] # za vsak primer
            
            if forward
                src = tabela
                tgt = tempTabela
            else
                src = tempTabela
                tgt = tabela
            end
            
            for i = 1:procesi
                
                push!(parts, (myID, view(src, starts[i]:ends[i]), pivot))
            end
        end
        
        for i in throw_out
            
            splice!(to_split, i)
        end
        
        # println("Parts ready: ", parts_ready)
        # println("Split, parts: ", parts)
        
        if size(parts)[1] == 0
            break
        end
        
        sizes = Array{Any, 1}(undef, size(parts)[1])
        # println(src)
        
        Threads.@threads for i = 1:size(parts)[1]
            
            sizes[i] = (parts[i][1], onePassSwaps!(parts[i][2], parts[i][3]))
        end
        
        # println("Shuffled!")
        # println("Sizes: ", sizes)
        # println(src)
        
        parts  = []
        extras = Array{Any, 1}(undef, size(to_split)[1])
        
        for (idx, part_to_split) in enumerate(to_split)
            
            tabela, tempTabela, myID, procesi, forward = part_to_split
            
            id_sizes = [i[2] for i in sizes if i[1] == myID]
            
            if forward
                src = tabela
                tgt = tempTabela
            else
                src = tempTabela
                tgt = tabela
            end
            
            # računanje velikosti delov in odmikov
            lowers  = Array{Int, 1}(undef, procesi)
            highers = Array{Int, 1}(undef, procesi)
            starts  = Array{Int, 1}(undef, procesi)
            
            lowers[1]  = 1
            highers[1] = 1
            starts[1]  = 1
            
            for (i, lens) in enumerate(id_sizes[1:procesi-1])
                lowers[i+1]  = lowers[i]  + lens[1]
                highers[i+1] = highers[i] + lens[2]
                starts[i+1]  = starts[i]  + lens[1] + lens[2]
            end
            
            highers .+= lowers[procesi] + id_sizes[procesi][1] - 1 #luškan mali trik
            
            for i = 1:procesi
                
                push!(parts, (view(src, starts[i]:starts[i]+id_sizes[i][1]-1),
                              view(tgt, lowers[i]:lowers[i]+id_sizes[i][1]-1)))
                push!(parts, (view(src, starts[i]+id_sizes[i][1]:starts[i]+id_sizes[i][1]+id_sizes[i][2]-1),
                              view(tgt, highers[i]:highers[i]+id_sizes[i][2]-1)))
            end
            
            extras[idx] = (lowers, highers, id_sizes)
        end
        
        # println("Collected!")
        # println("Extras: ", extras)
        # println("Parts: ", parts)
        # println(src, tempArray)
        
        Threads.@threads for i = 1:size(parts)[1]
            
            copyArray!(parts[i][1], parts[i][2])
        end
        
        # println("Swapped!")
        # println(src, tempArray)
        
        parts = []
        
        for (idx, part_to_split) in enumerate(to_split)
            
            tabela, tempTabela, myID, procesi, forward = part_to_split
            lowers, highers, id_sizes = extras[idx]
            
            if forward
                src = tabela
                tgt = tempTabela
            else
                src = tempTabela
                tgt = tabela
            end
            
            # razpoložljive procese razdelimo čim bolj "pravično" na dva dela
            prviDel = Int(round((lowers[procesi] + id_sizes[procesi][1] - 1) * procesi / size(tabela)[1]))
            if prviDel <= 0
                prviDel = 1
            elseif prviDel >= procesi
                prviDel = procesi-1
            end
            
            # računanje velikosti tabel
            sizeLower = lowers[procesi] + id_sizes[procesi][1] - 1
            # println("Size Lower: ", sizeLower)
            
            # rekurzivna klica
            push!(parts, (view(tabela, 1:sizeLower), view(tempTabela, 1:sizeLower), myID, prviDel, !forward))
            # println("Pushed one!")
            push!(parts, (view(tabela, (sizeLower+1):(size(tabela)[1])), view(tempTabela, (sizeLower+1):(size(tabela)[1])),
                          myID + prviDel, procesi - prviDel, !forward))
            # println("Pushed two!")
        end
        
        # println("Recursed!")
        
        to_split = parts
    end
    
    # println("At the bottom!")
    # println(src)
    # println(tempArray)
    
    partSizes = [size(i[1])[1] for i in parts_ready]
    # println(partSizes)
    
    Threads.@threads for i = 1:size(parts_ready)[1]
            
        srr, tgt, forward = parts_ready[i]
        
        if forward
            inPlaceQuickSort!(srr)
        else
            outwardQuickSort!(tgt, srr)
        end
    end
end

function _hitritest(n)
    
    src = collect(n:-1:1)
    display(src)
    
    adaptiveQS!(src)

    display(src)
    issorted(src)
end

function _hitritest()
    
    src = rand(Int, 100000000)
    display(src)
    
    adaptiveQS!(src, threads = 1000)

    display(src)
    issorted(src)
end

function testKPonovitev(k, n)

    sum = 0.0
    
    # warm-up
    a = rand(Int64, n)
    adaptiveQS!(a)

    a = nothing
    GC.gc() # explicit garbage collect
    
    for i = 1:k

        a = rand(Int64, n)
        delta = 1000 * (@elapsed adaptiveQS!(a))
        sum += delta

        a = nothing
        GC.gc()
        
        println(delta)
    end
    
    sum /= k

    println("Povprečno: ", sum)
end