#!/usr/bin/env julia

using Pressio
using BenchmarkTools
using Statistics
using DataFrames
using Plots
using StatsPlots


data = Array{Float32}(undef, 1800, 3600);
read!(expanduser("~/git/datasets/cesm/PRECT_1_1800_3600.f32"), data);


df = DataFrame(compressor=String[], bound=Float64[], time=Float64[], time_std=Float64[])
for comp_id in ["sz3", "sz", "zfp"]
for bound in exp.(range(log(1e-6), log(1e-3), length=30))
    @show bound
    sz = compressor(pressio(), comp_id)
    set_options(sz, Dict{String,Any}("pressio:abs" => bound))
    result = @benchmark begin
        compress(sz, data)
    end
    push!(df, (comp_id, bound, mean(result.times), std(result.times)))
end
end

p = @df df groupedbar(log.(:bound), :time_std./10^6, title="time variation with error bound", xformatter=(x -> round(exp(x), sigdigits=2)), ylabel="compression std (ms)", xlabel="abs error bound", group=:compressor)
savefig(p, "/tmp/sz3-varriation-CESM-PRECT.png")
p

@df df bar(log.(:bound), :time ./10^6, yerr=:time_std./10^6, title="time variation with error bound", xformatter=(x -> round(exp(x), sigdigits=2)), ylabel="compression (ms)", xlabel="abs error bound", group=:compressor)
savefig(p, "/tmp/sz3-abs-CESM-PRECT.png")
p
