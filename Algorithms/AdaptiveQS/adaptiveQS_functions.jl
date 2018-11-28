using Distributed
@everywhere using Distributed


function generatePivot() end

function onePassSwaps!(src::AbstractArray{T,1} where T<:Number,
                       pivot::Number)
    
    # inicializacija
    i = 0
    j = size(src)[1] + 1
    
    # Hoarova particija - https://en.wikipedia.org/wiki/Quicksort
    while i < j
    
        i += 1
        while src[i] < pivot
            i += 1
        end
        
        j -= 1
        while src[j] > pivot
            j -= 1
        end
        
        if i >= j
            return (j, size(src)[1] - j) # dolžina levega dela tabele
        end
        
        src[i], src[j] = src[j], src[i]
        
    end
end

function inPlaceQuickSort!() end

function outwardQuickSort!() end

function adaptiveQS!(tabela::AbstractArray{T,1} where T<:Number;
                     tempTabela::Union{AbstractArray{T,1},Nothing}=nothing where T<:Number,
                     forward::Bool=true,
                     procesi::Integer=4)
    
    if procesi == 1
        if forward
            inPlaceQuickSort(tabela)
        else
            outwardQuickSort(tabela, tempTabela)
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
        #TODO
        
    end
end
