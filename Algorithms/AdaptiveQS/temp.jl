function adaptiveQS!(tabela::AbstractArray{T,1} where T<:Number,
                     tempTabela::Union{AbstractArray{T,1},Nothing}=nothing where T<:Number,
                     forward::Bool=true,
                     procesi::Integer=4)
    
    if procesi == 1
        if forward
            inPlaceQuickSort!(tabela)
        else
            outwardQuickSort!(tempTabela, tabela)
        end
    else
    
        # inicializacija začasne tabele
        if tempTabela == nothing
            tempTabela = Array{eltype(tabela)}(undef, size(tabela))
        end
        
        # generiranje pivota, indeksov podtabel
        pivot  = generatePivot(tabela)
        myID   = myid()
        step   = size(tabela)[1] / procesi
        starts = [Int((i - 1) * step) + 1 for i in 1:procesi]
        ends   = [Int(i * step) for i in 1:procesi]
        ends[procesi] = size(tabela)[1] # za vsak primer
        if forward
            src = tabela
        else
            src = tempTabela
        end
        
        # klicanje asinhronih metod
        ret = [@spawnat (i + myID) onePassSwaps!(view(src, starts[i]:ends[i]), pivot) for i in 1:procesi]
        sizes = [fetch(i) for i in ret]
        
        # računanje velikosti delov in odmikov
        lowers  = Array{Int}(procesi)
        highers = Array{Int}(procesi)
        starts  = Array{Int}(procesi)
        
        lowers[1]  = 1
        highers[1] = 1
        starts[1]  = 1
        
        for (i, lens) in enumerate(sizes[1:procesi-1])
            lowers[i+1]  = lowers[i]  + lens[1]
            highers[i+1] = highers[i] + lens[2]
            starts[i+1]  = starts[i]  + lens[1] + lens[2]
        end
        
        highers .+= lowers[procesi] + sizes[procesi][1] #luškan mali trik
        
        # premikanje
        if forward
            
            # tabela -> temp
            # lowers
            ret = [@spawnat (i + myID) copyArray!(view(tabela,     starts[i]:starts[i]+sizes[i][1]-1),
                                                  view(tempTabela, lowers[i]:lowers[i]+sizes[i][1]-1)) for i in 1:procesi]
            [wait(i) for i in ret]
            
            
            # highers
            ret = [@spawnat (i + myID) copyArray!(view(tabela,     starts[i]+sizes[i][1]:starts[i]+sizes[i][1]+sizes[i][2]-1),
                                                  view(tempTabela, highers[i]:highers[i]+sizes[i][2]-1)) for i in 1:procesi]
            [wait(i) for i in ret]
            
            """ fallback:
            # tabela -> temp
            for i = 1:procesi
                # lowers
                tempTabela[lowers[i]:lowers[i]+sizes[i][1]-1]   = tabela[starts[i]:starts[i]+sizes[i][1]-1]
                # highers
                tempTabela[highers[i]:highers[i]+sizes[i][2]-1] = tabela[starts[i]+sizes[i][1]:starts[i]+sizes[i][1]+sizes[i][2]-1]
            end
            """
        else
            
            # temp -> tabela
            # lowers
            ret = [@spawnat (i + myID) copyArray!(view(tempTabela, starts[i]:starts[i]+sizes[i][1]-1),
                                                  view(tabela,     lowers[i]:lowers[i]+sizes[i][1]-1)) for i in 1:procesi]
            [wait(i) for i in ret]
            
            # highers
            ret = [@spawnat (i + myID) copyArray!(view(tempTabela, starts[i]+sizes[i][1]:starts[i]+sizes[i][1]+sizes[i][2]-1),
                                                  view(tabela,     highers[i]:highers[i]+sizes[i][2]-1)) for i in 1:procesi]
            [wait(i) for i in ret]
        end
        
        # razpoložljive procese razdelimo čim bolj "pravično" na dva dela
        prviDel = Int((lowers[procesi] + sizes[procesi][1] - 1) * procesi / size(tabela)[1]) + 1
        if prviDel <= 0
            prviDel = 1
        elseif prviDel >= procesi
            prviDel = procesi-1
        end
        
        # asinhrona rekurzivna klica
        ret = [ @spawnat myID             adaptiveQS!(view(tabela, 1:lowers[procesi]+sizes[procesi][1]-1),
                                                      tempTabela=view(tabela, 1:lowers[procesi]+sizes[procesi][1]-1),
                                                      forward=!forward,
                                                      procesi=prviDel),
                @spawnat (myID + prviDel) adaptiveQS!(view(tabela, lowers[procesi]+sizes[procesi][1]:size(tabela)[1]),
                                                      tempTabela=view(tabela, lowers[procesi]+sizes[procesi][1]:size(tabela)[1]),
                                                      forward=!forward,
                                                      procesi=procesi-prviDel)
              ]
        
        [wait(i) for i in ret]
        
        # na tej točki je tabela sortirana -> konec!
    end
end