function jpeg(imageIn, imageOut, q_matrix, width, height)
    
    PI = 3.141592f0
    
    l_id0 = threadIdx().x
    l_id1 = threadIdx().y
    
    g_id0 = blockIdx().x
    g_id1 = blockIdx().y
    
    t_y   = (g_id0 - 1) * blockDim().x + l_id0
    t_x   = (g_id1 - 1) * blockDim().y + l_id1
    
    idct                = @cuStaticSharedMem(Float32, (8, 8))
    dct                 = @cuStaticSharedMem(Float32, (8, 8))
    local_image         = @cuStaticSharedMem(UInt8,   (8, 8))
    quantization_matrix = @cuStaticSharedMem(UInt32,  (8, 8))
    
    local_image[l_id0, l_id1] = imageIn[(t_y - 1) * width + t_x]
	quantization_matrix[l_id0, l_id1] = q_matrix[(l_id0 - 1) * 8 + l_id1]
    
    sync_threads() # WAIT FOR ALL THREADS TO WRITE IN LOCAL MEMORY
    
    sum = 0.0f0
    for n = 1:8
        
        sum += local_image[l_id0, n] * CUDAnative.cos((PI / 8) * (n - 0.5f0) * (l_id1 - 1))
    end
    
    dct[l_id0, l_id1] = sum
    
    sync_threads() # WAIT FOR ALL THREADS TO CALCULATE 1D DCT
    
    sum = 0.0f0
    for n = 1:8
        
        sum += dct[n, l_id1] * CUDAnative.cos((PI / 8) * (n - 0.5f0) * (l_id0 - 1))
    end
    dct[l_id0, l_id1] = sum
    
    sync_threads() # WAIT FOR ALL THREADS TO CALCULATE 2D DCT
    
    dct[l_id0, l_id1] = dct[l_id0, l_id1] / quantization_matrix[l_id0, l_id1]
	dct[l_id0, l_id1] = round(dct[l_id0, l_id1])
	dct[l_id0, l_id1] = dct[l_id0, l_id1] * quantization_matrix[l_id0, l_id1]
    
    sync_threads() # WAIT FOR ALL THREADS TO DIVIDE DCT WITH Q_MATRIX AND ROUND AND MULTIPLY
    
    sum = 0.125f0 * dct[1, l_id1]
    for n = 2:8
        
        sum += 0.25f0 * dct[n, l_id1] * CUDAnative.cos((PI / 8) * (n - 1) * (l_id0 - 0.5f0))
    end
    idct[l_id0, l_id1] = sum

	sync_threads() # WAIT FOR ALL THREADS TO CALCULATE 1D inverse-DCT
    
    sum = 0.125f0 * idct[l_id0, 1]
    for n = 2:8
        
        sum += 0.25f0 * idct[l_id0, n] * CUDAnative.cos((PI / 8) * (n - 1) * (l_id1 - 0.5f0))
    end
    idct[l_id0, l_id1] = sum
    
    sync_threads() # WAIT FOR ALL THREADS TO CALCULATE 2D inverse-DCT
    
    tmpOut = idct[l_id0, l_id1]
    tmpOut = round(tmpOut)
    tmpOut = clamp(tmpOut, 0, 255)
    #tmpOut = CUDAnative.Int32(tmpOut)
    imageOut[(t_y - 1) * width + t_x] = tmpOut
    
    # imageOut[(t_y - 1) * width + t_x] = UInt8(clamp(CUDAnative.round(idct[l_id0, l_id1]), 0, 255))
    
    return nothing
end
