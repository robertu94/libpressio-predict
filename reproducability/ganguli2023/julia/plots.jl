using Distributions
using DataFrames
using StatsPlots
using Plots

# compressor name are per compressor timings
# common_all is where all metrics are needed
# common_errbound is where just the error bound is needed
#times are in seconds
#accuracy absolute percent error in %, (10% (lower bound), 50% (median), 90% (upper bound))

# compressors
compressors = Dict(
    "sz" => Normal(.009,.014),
    "sz3" =>  Normal(.032, .0005),
    "zfp" => Normal(.0009, .0003),
    "bit_grooming" => Normal(.0049, .0002),
    "digit_rounding" =>  Normal(.0099, .0002),
    "mgard" => Normal(.2083, .0084),
    "tthresh" => Normal(.4303, .0034),
)
# dingwen block method
dingwen = Dict(
    "sz" => Normal(.009, .005),
    "sz3" => Normal(.0038, .0012),
    "zfp" => Normal(.0003, .001),
    "bit_grooming" => Normal(.0007, .0015),
    "digit_rounding" => Normal(.0009,.0015),
    "mgard" => Normal(1.232, .0061),
    "tthresh" => Normal(.0102, .0021),
    "accuracy" => (82, 90, 93)
)
#david
david = Dict(
    "est_all" => Normal(.3252,.0253),
    "est_errbnd" => Normal(.0041, .0004),
    "accuracy" => (.16, .71, 3.5),
)
# arka (current)
arka = Dict(
    "est_all" => Normal(.2279, .0229),
    "est_errbnd" => Normal(.0021, .0058),
    "accuracy" => (.9, 2.8 , 3.8),
)
#optzconfig -- can only be used in the HDF5 case
#est_all = compressors * 30
#est_errbnd = compressor * 30
#accuracy = (17, 28, 121)
#whitebox lu (sz2 only, can be used in just the OptZConfig and HDF5 cases)
lu = Dict(
    "sz" => compressors["sz"]
)
accuracy = (157, 193, 256)


################################################################################
# Use Case 1: FRAZ
################################################################################
# don't use optzconfig here
est_methods = Dict(
    "Ours" => arka,
    "Underwood" => david,
    "Tao" => dingwen,
    "Lu" => lu,
    "No Estimation" => compressors
)
results = DataFrame(compressor=String[], method=String[], mean=Float64[], stddev=Float64[])
for comp_id in keys(compressors)
    for (method_id, method) in est_methods
        if method_id == "Lu" && comp_id != "sz"
            continue
        end
        est_errbnd = get(method, "est_errbnd", compressors[comp_id])
        est_all = get(method, "est_all", compressors[comp_id])
        comp = get(method, comp_id, compressors[comp_id])
        d = rand(est_errbnd, 49, 50)
        d = vcat(d, rand(est_all, 50)')
        d = vcat(d, rand(comp, 50)')
        m = mean(sum(d, dims=1))
        s = std(sum(d, dims=1))
        push!(results, (comp_id, method_id, m,s))
    end
end
noest = filter(:method => x -> x == "No Estimation", results)
speedup = innerjoin(results, noest, on=:compressor, renamecols=""=>"_baseline")
speedup.speedup .= log2.(speedup.mean_baseline ./ speedup.mean)
speedup = filter(:method => x -> x != "No Estimation", speedup)
speedup = filter(:compressor => x -> x != "zfp", speedup)
speedup = filter(:compressor => x -> x != "bit_grooming", speedup)
sort!(speedup, :speedup)
p = @df speedup groupedbar(:compressor, :speedup, group=:method, legend=:outerright, ylabel="speedup", xlabel="compressor", yaxis=(formatter=y->string(2^y)), ylim=(-1,5), title="OptZConfig (Usecase A)")
savefig(p,"/tmp/optzconfig.png")
p


################################################################################
# Use Case 2: Choose the best CR from several compressors
################################################################################
results = DataFrame(method=String[], mean=Float64[])
for (method_id, method) in est_methods
    if method_id == "Lu"
        continue
    elseif haskey(method, "est_all")
        m = mean(method["est_all"])
    else
        m = 0
        for c in keys(compressors)
            m += mean(get(method, c, compressors[c]))
        end
    end
    push!(results, (method_id, m))
end
noest = first(filter(:method => x -> x == "No Estimation", results).mean)
results.speedup .= noest ./ results.mean 
results = filter(:method => x -> x != "No Estimation", results)
sort!(results, :speedup)
p = @df results bar(:method, :speedup, legend=false, ylabel="speedup", xlabel="method", title="Best CR for Error Bound (Usecase B)")
hline!(p, [1], color=:black)



################################################################################
# Use Case 3: Parallel Writes
################################################################################

processors = 128
buffers = 128 * 30
writes = Normal(0.027, 0.0020240934699923114)
for (method_id, method) in est_methods
    for comp_id in keys(compressors)
        if method_id == "No Estimation"
            try_times = rand(compressors[comp_id], buffers)
            write_times = rand(writes, buffers)
            serial_time = 0
            for i = 1:processors:buffers
                e = min(processors, buffers-i)
                serial_time += maximum(write_times[i:e], init=0)
            end
            for i = 1:processors:buffers
                e = min(processors, buffers-i)
                serial_time += maximum(try_times[i:e], init=0)
            end
            @show comp_id, method_id, serial_time
        else
            est_times = rand(get(method, "est_all", get(method, comp_id, compressors[comp_id])), buffers)
            try_times = rand(compressors[comp_id], buffers)
            write_times = rand(writes, buffers)
            a_time = 0
            process_times = zeros(processors)
            for est_time in est_times
                process_times[argmin(process_times)] += est_time
            end
            a_time = maximum(process_times)
            work_times = try_times + write_times
            sort!(work_times, rev=true)
            process_times = zeros(processors)
            for work_time in work_times
                process_times[argmin(process_times)] += work_time
            end
            a_time += maximum(process_times)
            @show comp_id, method_id, a_time
        end
    end
end
