using Statistics
using LinearAlgebra
using CUDA
using Profile
using ThreadsX

data = Array{Float32}(undef, 1800, 3600)
read!(expanduser("~/git/datasets/cesm/PRECT_1_1800_3600.f32"), data)

function svdslice(datslice; tile_size=20)
    @time begin
        μ = mean(datslice; dims=1)
        σ = mean(datslice; dims=1)
        if any(σ .==  0)
            return zero(size(datslice,1))
        end
        sp = @. (datslice-μ)/σ
    end
    @time begin
        tiles = []
        for i in collect(Int,1:size(datslice,1)/tile_size)
            for j in collect(Int, 1:size(datslice,2)/tile_size)
                push!(tiles, sp[i:i+(tile_size-1), j:j+(tile_size-1)])
            end
        end
    end
    mat = @time ThreadsX.mapreduce(t -> vec(t)*vec(t)', +, tiles)/length(tiles)
    @time begin
        svd(mat).S
    end
end

using BenchmarkTools

svdslice(cu(data));
