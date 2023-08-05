using Optimization
using OptimizationNLopt
using Distributions
using DataStructures
using StatsPlots
using Plots
using DataFrames

f(x, p) = (x[1]-1.0)*(x[1]+2.0)
function with_error(f, dist)
    entries = DefaultDict{Float64, Float64}() do
        return rand(dist)
    end
    f_prime(x, p) = f(x, p) + entries[x[1]] 
    return f_prime
end

df = DataFrame(i=Float64[], σ=Float64[], sol=Float64[], g=Float64[], f=Float64[])
for i = 1.0:.1:100.0
    σ = i/100*-f([-.5],[0])
    g = with_error(f, Normal(0, σ)) 
    fopt = OptimizationFunction(g)
    x0 = zeros(1)
    p = zeros(1)
    prob = Optimization.OptimizationProblem(fopt, x0, p, lb=[-3.0], ub=[3.0], maxeval=100)
    sol = solve(prob, NLopt.LN_COBYLA())
    push!(df, (i, σ, sol[1], g(sol, [0]), f(sol, [0])))
end

sort!(df, :σ)
df.diff = df.f .+ 2.25 .+ 1e-8
df.cm = accumulate(max, df.diff)
p = @df df scatter(:σ, :diff )
@df df plot!(p, :σ, :cm)

using GLM
points = unique(df, :cm)[!, [:σ, :diff]]
m = lm(@formula(diff ~ exp(σ)), points)
p = @df df scatter(:σ, :diff)
@df df plot!(p, :σ, :cm)
plot!(p, points.σ, predict(m), legend=false)

m = lm(@formula(diff ~ exp(σ)), points)
p = @df df scatter(:σ ./(2.25/100), :diff ./(2.25/100), label="observed errors")
@df df plot!(p, :σ ./(2.25/100), :cm./(2.25/100), label="observed max error")
plot!(p, points.σ./(2.25/100), predict(m)./(2.25/100), legend=:topleft, label="fitted error: diff ~ exp(σ)", ylabel="% Error in Targeted CR", xlabel="Standard Deviation of Prediction Error as a % of True CR")
savefig(p, "/tmp/esterror.png")
p

ndf = DataFrame(σ=2.25/100 .* [.5,1,2,4,8,16,32])
round.(predict(m, ndf)./(2.25/100), digits=1)

