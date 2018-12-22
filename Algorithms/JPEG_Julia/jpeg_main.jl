__precompile__(true) # ko vse deluje

using FileIO
using Images
using CuArrays, CUDAnative, CUDAdrv

#file_name = "image_512.png"
#file_name = "image_1024.png"
#file_name = "image_2048.png"
file_name = "image_4096.png"

image_in = Gray.(load("../JPEG_basic/" * file_name))

height, width = size(image_in)
println("IMAGE_RESOLUTION: ", size(image_in)[1], "x", size(image_in)[2])

image_in = Array{UInt8, 1}(reshape(rawview(channelview(image_in))', :))

quantization_matrix = UInt32[
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68,109,103, 77,
    24, 35, 55, 64, 81,104,113, 92,
    49, 64, 78, 87,103,121,120,101,
    72, 92, 95, 98,112,100,103, 99
] # WARNING !! 1D ARRAY - NOT 2D

println("GPU start...");

t1 = time_ns()

include("jpeg_kernel.jl") # contains jpeg(imgIn, imgOut, qMatrix)

imagein_mem_obj  = CuArrays.CuArray(image_in)
qmatrix_mem_obj  = CuArrays.CuArray(quantization_matrix)
imageout_mem_obj = Float32.(similar(imagein_mem_obj))

WORKGROUP_SIZE = 8

threadNum = (        WORKGROUP_SIZE,          WORKGROUP_SIZE)
groupNum  = (width ÷ WORKGROUP_SIZE, height ÷ WORKGROUP_SIZE)

@cuda threads=threadNum blocks=groupNum jpeg(imagein_mem_obj, imageout_mem_obj, qmatrix_mem_obj, width, height)

image_out = Array(imageout_mem_obj)
image_out = UInt8.(image_out)

t2 = time_ns()

display(image_out)

time2 = (t2 - t1) / 1000000
println("GPU time: ", Int(round(time2)), " ms");

image_out = colorview(Gray, reinterpret(N0f8, reshape(image_out, width, height)'))
save("image_compressed.png", image_out)

nothing
