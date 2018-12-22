


# Julia

using Distributed
@everywhere using Distributed
@everywhere using SharedArrays

include("quickSort.jl")

function adaptiveQS!(tabela::AbstractArray{T,1} where T<:Number,
                     tempTabela::AbstractArray{T,1} where T<:Number;
                     forward::Bool=true,
                     procesi::Integer=-1)
    
    if procesi == -1
        procesi = nprocs()
    end
    
    if procesi == 1
        if forward
            inPlaceQuickSort!(tabela)
        else
            outwardQuickSort!(tempTabela, tabela)
        end
    else
    
        # display(nprocs())
    
        # inicializacija začasne tabele
        # if tempTabela == nothing
            # tempTabela = Array{eltype(tabela)}(undef, size(tabela))
        # end
        
        # generiranje pivota, indeksov podtabel
        pivot  = generatePivot(tabela)
        myID   = myid()
        step   = size(tabela)[1] / procesi
        starts = [Int(floor((i - 1) * step)) + 1 for i in 1:procesi]
        ends   = [Int(floor(i * step)) for i in 1:procesi]
        ends[procesi] = size(tabela)[1] # za vsak primer
        if forward
            src = tabela
        else
            src = tempTabela
        end
        
        # klicanje asinhronih metod
        ret = [@spawnat (i + myID - 1) onePassSwaps!(view(src, starts[i]:ends[i]), pivot) for i in 1:procesi]
        sizes = [fetch(i) for i in ret]
        
        # display(sizes)
        
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
        
        # display(lowers)
        # display(highers)
        
        # premikanje
        if forward
            
            # tabela -> temp
            # lowers
            ret = [@spawnat (i + myID - 1) copyArray!(view(tabela,     starts[i]:starts[i]+sizes[i][1]-1),
                                                  view(tempTabela, lowers[i]:lowers[i]+sizes[i][1]-1)) for i in 1:procesi]
            ret = [fetch(i) for i in ret]
            
            
            # highers
            ret = [@spawnat (i + myID - 1) copyArray!(view(tabela,     starts[i]+sizes[i][1]:starts[i]+sizes[i][1]+sizes[i][2]-1),
                                                  view(tempTabela, highers[i]:highers[i]+sizes[i][2]-1)) for i in 1:procesi]
            ret = [fetch(i) for i in ret]
            
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
            ret = [@spawnat (i + myID - 1) copyArray!(view(tempTabela, starts[i]:starts[i]+sizes[i][1]-1),
                                                  view(tabela,     lowers[i]:lowers[i]+sizes[i][1]-1)) for i in 1:procesi]
            ret = [fetch(i) for i in ret]
            
            # display("succ!")
            
            # highers
            ret = [@spawnat (i + myID - 1) copyArray!(view(tempTabela, starts[i]+sizes[i][1]:starts[i]+sizes[i][1]+sizes[i][2]-1),
                                                  view(tabela,     highers[i]:highers[i]+sizes[i][2]-1)) for i in 1:procesi]
            ret = [fetch(i) for i in ret]
        end
        
        # razpoložljive procese razdelimo čim bolj "pravično" na dva dela
        prviDel = Int(floor((lowers[procesi] + sizes[procesi][1] - 1) * procesi / size(tabela)[1])) + 1
        if prviDel <= 0
            prviDel = 1
        elseif prviDel >= procesi
            prviDel = procesi-1
        end
        
        # asinhrona rekurzivna klica
        ret = [ @spawnat myID             adaptiveQS!(view(tabela,     1:lowers[procesi]+sizes[procesi][1]-1),
                                                      view(tempTabela, 1:lowers[procesi]+sizes[procesi][1]-1),
                                                      forward=!forward,
                                                      procesi=prviDel),
                @spawnat (myID + prviDel) adaptiveQS!(view(tabela,     lowers[procesi]+sizes[procesi][1]:size(tabela)[1]),
                                                      view(tempTabela, lowers[procesi]+sizes[procesi][1]:size(tabela)[1]),
                                                      forward=!forward,
                                                      procesi=procesi-prviDel)
              ]
        
        ret = [fetch(i) for i in ret]
        
        # na tej točki je tabela sortirana -> konec!
    end
    
    return 0;
end

function _hitriTest(n)
    
    a = SharedArray{Int,1}(n)
    a[1:n] = collect(n:-1:1)
    display(a)
    
    temp = SharedArray{Int,1}(n)
    adaptiveQS!(a, temp)
    
    display(nprocs())
    
    display(a)
    display(issorted(a))
    
    return a;
end