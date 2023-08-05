using JuMP, GLPK
using BenchmarkTools
using Distributions

# Define the problem
n = 30 # number of tasks
p = 2 # number of processors
tasks = collect(1:n)
processors = collect(1:p)
times = round.(Int, rand(Normal(10,2), length(tasks)))
m = Model(with_optimizer(GLPK.Optimizer))
@variable(m, x[1:n,1:p], Bin)
@variable(m, Cmax >= 0)
# Define the objective function
@objective(m, Min, Cmax)
# Define the constraints
for i in tasks
    @constraint(m, sum(x[i,j] for j in processors) == 1)
end
for j in processors
    @constraint(m, sum(x[i,j]*times[i] for i in tasks) <= Cmax)
end
for i in tasks
    for j in processors
        @constraint(m, x[i,j] * (times[i]) <= Cmax)
    end
end
# Solve the problem
@time optimize!(m)
# Print the solution
println("Status: ", termination_status(m))
println("Minimum makespan: ", JuMP.objective_value(m))
for i in tasks
    for j in processors
        if JuMP.value(x[i,j]) == 1
            println("Task ", i, " (time ", times[i], ") assigned to Processor ", j)
        end
    end
end


# List Scheduling
@time begin
    last_time = zeros(p)
    t = sort(times)
    for i in t
        last_time[argmin(last_time)] += i
    end
    maximum(last_time)
end


