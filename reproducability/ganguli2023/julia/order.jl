#!/usr/bin/env julia

using JuMP
using HiGHS
model = Model(HiGHS.Optimizer)

names = ["cloud", "qcloud", "precip", "qgraup", "qrain", "qsnow", "qice", "tc", "u", "v", "w", "qvapor"]
idx(n) = findfirst(x-> x==n, names)


model = Model(HiGHS.Optimizer)
@variable(model, c[i=1:length(names)], Bin)
@objective(model, Min, sum(c))
@constraint(model, cloud_covered, c[idx("qice")] + c[idx("qgraup")] + c[idx("qsnow")] + 3c[idx("cloud")] >= 3)
@constraint(model, qcloud_covered, c[idx("qrain")] + c[idx("qgraup")] + c[idx("qsnow")] + 3c[idx("qcloud")] >= 3)
@constraint(model, precip_covered, c[idx("qcloud")]  + c[idx("precip")] >= 1)
@constraint(model, qgraup_covered, c[idx("qgraup")]  + c[idx("qcloud")] >= 1)
@constraint(model, qrain_covered, c[idx("qgraup")]  + c[idx("qrain")] >= 1)
@constraint(model, qsnow_covered, c[idx("qgraup")]  + c[idx("qsnow")] >= 1)
@constraint(model, qice_covered, c[idx("cloud")] + c[idx("qgraup")] + c[idx("qsnow")]  + 3c[idx("qsnow")] >= 3)
@constraint(model, tc_covered, c[idx("u")] + c[idx("tc")] >= 1)
@constraint(model, v_covered, c[idx("v")] + c[idx("tc")] >= 1)
optimize!(model)
coverage = [name for (name, b) in zip(names, value.(c)) if b == 1]
