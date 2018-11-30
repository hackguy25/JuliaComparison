using Distributed
@everywhere using Distributed


function generatePivot(src::AbstractArray{T,1} where T<:Number)
    
    if size(src)[1] > 15
        range = 1:size(src)[1]
        sum = Float64(0)
        sum += src[rand(range)]
        sum += src[rand(range)]
        sum += src[rand(range)]
        sum /= 3
        return Int(floor(sum))
    else
        return src[rand(1:size(src)[1])]
    end
end

function onePassSwaps!(src::AbstractArray{T,1} where T<:Number,
                       pivot::Number)
    
    # inicializacija
    srcSize = size(src)[1]
    i = 0
    j = srcSize + 1
    
    # Hoarova particija - https://en.wikipedia.org/wiki/Quicksort
    while i < j
    
        i += 1
        while i <= srcSize && src[i] < pivot
            i += 1
        end
        
        j -= 1
        while j > 0 && src[j] > pivot
            j -= 1
        end
        
        if i >= j
            return (j, srcSize - j) # dolžina levega dela tabele
        end
        
        src[i], src[j] = src[j], src[i]
        
    end
end

function onePassSwapsOutward!(src::AbstractArray{T,1} where T<:Number,
                              dest::AbstractArray{T,1} where T<:Number,
                              pivot::Number)
    
    # inicializacija
    srcSize = size(src)[1]
    i = 0
    j = srcSize + 1
    
    
    # Hoarova particija - https://en.wikipedia.org/wiki/Quicksort
    while i < j
    
        i += 1
        dest[i] = src[i]
        while i <= srcSize < pivot
            i += 1
            dest[i] = src[i]
        end
        
        j -= 1
        dest[j] = src[j]
        while j > 0 && src[j] > pivot
            j -= 1
            dest[j] = src[j]
        end
        
        if i >= j
            return (j, srcSize - j) # dolžina levega dela tabele
        end
        
        dest[i], dest[j] = dest[j], dest[i]
        
    end
end

function copyArray!(src::AbstractArray{T,1} where T<:Number,
                    dest::AbstractArray{T,1} where T<:Number)
    
    dest[:] = src[:]
end

function inPlaceQuickSort!(src::AbstractArray{T,1} where T<:Number)
    
    if (size(src)[1] > 1)
        
        pivot = generatePivot(src)
        sizes = onePassSwaps!(src, pivot)
        inPlaceQuickSort!(view(src, 1:sizes[1]))
        inPlaceQuickSort!(view(src, (sizes[1]+1):(size(src)[1])))
    end
end

function outwardQuickSort!(src::AbstractArray{T,1} where T<:Number,
                           dest::AbstractArray{T,1} where T<:Number)
    
    if (size(src)[1] > 1)
        
        pivot = generatePivot(src)
        sizes = onePassSwapsOutward!(src, dest, pivot)
        inPlaceQuickSort!(view(dest, 1:sizes[1]))
        inPlaceQuickSort!(view(dest, sizes[1]+1:size(src)[1]))
    else
        dest[1] = src[1]
    end
end
