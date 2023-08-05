#!/usr/bin/env julia

using Plots
using Distributions
using ColorSchemes

σ0 = 1
σ2 = 1
σe = .1

top=5e-3
p = plot(legend=:bottomleft, c=:Dark2_8, xlim=(1e-12,top*1.2))
for d in [.05, .1]
    for σ in [.01, .1]
        f(σe) =  1 - cdf(Normal(0,1), (d)/(√(2σ^2 + 2σe^2)))
        g(σe) =  1 - cdf(Normal(0,1), (d)/(√(2σ^2)))
        h(σe) =  (g(σe)-f(σe))/g(σe)
        @show d,σ, h(0), h(top)
        plot!(p, h, label="μ0-μi=$d , σ=$σ")
    end
end
p
