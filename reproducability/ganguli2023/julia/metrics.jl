#!/usr/bin/env julia

using CUDA
using BenchmarkTools
using LinearAlgebra
using TiledIteration
using DataStructures
using Base.Iterators
using Statistics
using Atomix

using Plots

data = Array{Float32}(undef, 500, 500, 100);
read!(expanduser("~/git/datasets/hurricane/100x500x500/CLOUDf48.bin.f32"), data)
slice = data[:,:,20]

data = Array{Float32}(undef, 1200, 1200, 98);
read!(expanduser("~/git/datasets/SCALE_98x1200x1200/QG-98x1200x1200.dat"), data)
slice = data[:,:,20]

slice = rand(Float32, 1000,1000)

heatmap(slice)


# spatial variability
# spatial correlation
# coding gain
# SVD truncation
# distortion

# A = (500,500)
# tile_size 25

A = rand(500,500);
tile_size=25;

function ent(A)
    counts = DefaultDict{eltype(A),Int}(0)
    total=0
    for i in A
        counts[i] += 1
        total+=1
    end
    h = 0.0
    for v in values(counts)
        p = float(v)/float(total)
        h += log2(p) * p
    end
    return -h
end
function qent(A, err_bnd)
    Q = round.(Int, A ./ err_bnd) .* err_bnd
    ent(Q)
end
function gnorm(v, scratch)
    scratch .= v.*v
    sqrt(sum(scratch))
end
function gnorm1(v)
    (sum(v.*v))
end
function gpmean(v)
    sum(v)/length(v)
end
function gpstd(v)
    sqrt(sum((v.-gpmean(v)).^2))
end
function gcor(x,y)
    ((gpmean(x).-x).*(gpmean(y).-y))/(gpstd(x)*gpstd(y))
end
function compute_metrics(A, tile_size=10, err_bnd=1e-6)
        tilespace = TileIterator(axes(A), tuple(repeated(tile_size, ndims(A))...));
        wb_intra = zeros(length(tilespace));
        wb_inter = zeros(length(tilespace));
        wb_inter_dem  = zeros(length(tilespace));
        wb_inter_num  = zeros(length(tilespace));
        R_inter_num  = zeros(length(tilespace));
        scratch = zeros(tile_size*tile_size)
        D_sum  = zeros(1);
        count_b  = zeros(length(tilespace));
        i = 1
        sigma = cu(zeros(tile_size^ndims(A), tile_size^ndims(A)))
        tiles_end = last(tilespace)
        #parallel
        cuA = cu(A)
        locs = collect(product(enumerate(tilespace),tilespace))
        SigmaLk = ReentrantLock()
        Threads.@threads for ((i,a),b) in locs
            if a < b
                continue
            end
            #TODO standard scale the tile with the tile
            if a == b
                cutab = vec(view(cuA,a...))
                tab = vec(view(A,a...))
                wb_intra[i] = cutab'*cutab / length(tab)
                lock(SigmaLk) do
                    sigma += cutab*cutab' #hotspot
                end
                Atomix.@atomic count_b[i] += 1
                Atomix.@atomic D_sum[1] += 1.0/12.0 * 2.0 ^ (2.0*ent(tab)) * 2.0 ^ (-2.0*qent(tab,err_bnd)/length(tab))
            end
            ra = collect(first.(a))
            rb = collect(first.(b))
            X_a = vec(view(A,a...))
            X_b = vec(view(A,b...))
            d = norm(X_a-X_b) #hotspot
            R = cor(X_a, X_b) #hotspot
            SD = norm(ra-rb,1)
            Atomix.@atomic wb_inter_dem[i] += (SD)
            Atomix.@atomic wb_inter_num[i] += (d*SD)
            Atomix.@atomic R_inter_num[i] += (R*SD)
        end
    svar = 0
    scor_num = 0
    scor_dem = 0
    sigma ./= length(tilespace)
    gsigma_singluarvals = svd(sigma).S
    sigma_singluarvals = Array(gsigma_singluarvals)
    stc = sigma_singluarvals ./ sum(i^2 for i in sigma_singluarvals)
    svd_trunc = findfirst(x -> x >.99, cumsum(stc)/sum(stc))/length(stc)*100
    #cumsum and find first value >= .99
    for i = 1:length(tilespace)
        wb_inter[i] = wb_inter_num[i] / wb_inter_dem[i]
        p_b = count_b[i]/length(tilespace)
        svar += wb_intra[i] * wb_inter[i] * p_b * log2(p_b)
        scor_num += R_inter_num[i]*wb_inter[i] 
        scor_dem += wb_inter[i]
    end
    # does NOT depend on error bound
    svar *= -1
    scor = scor_num/scor_dem
    coding_gain = prod(diag(sigma)) ^ (1/length(diag(sigma))) / prod(sigma_singluarvals) ^ (1/length(diag(sigma)))
    # depends on error bound
    D = D_sum[1]/length(tilespace)
    return (;
        spatial_variation=svar,
        spatial_correlation=scor,
        svd_truncation=svd_trunc,
        coding_gain=coding_gain,
        distortion=D
    )
end
function compute_distort(A, tile_size=10, err_bnd=1e-6)
        tilespace = TileIterator(axes(A), tuple(repeated(tile_size, ndims(A))...));
        D_sum  = zeros(1);
        i = 1
        locs = collect(tilespace)
        SigmaLk = ReentrantLock()
        Threads.@threads for a in locs
            tab = vec(view(A,a...))
            Atomix.@atomic D_sum[1] += 1.0/12.0 * 2.0 ^ (2.0*ent(tab)) * 2.0 ^ (-2.0*qent(tab,err_bnd)/length(tab))
        end
        D = D_sum[1]/length(tilespace)
        return (;
            distortion=D
        )
end

using StatsBase
using Statistics
using Pressio
using Base.Iterators
using TiledIteration
function dingwen(A, tile_size, comp_id)
        tilespace = TileIterator(axes(A), tuple(repeated(tile_size, ndims(A))...));
        locs = collect(tilespace)
        sz = compressor(pressio(), comp_id)
        for i in sample(locs, 30)
            tab = copy(view(A,i...))
            dtab = similar(tab)
            compressed = compress(sz, tab)
        end
end



ccall((:libpressio_register_all, "liblibpressio_meta.so"), Cvoid, ())

for comp_id in ["sz", "zfp", "sz3", "mgard", "tthresh", "bit_grooming", "digit_rounding"]
    z = compressor(pressio(), comp_id)
    println("run compress $comp_id")
    display(@benchmark compress($z,slice))
    println("run block_est compress $comp_id")
    display(@benchmark dingwen(slice, 25, $comp_id))
end
