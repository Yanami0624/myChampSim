/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 // plain_printer.cc
#include <cmath>
#include <numeric>
#include <ratio>
#include <string_view> // for string_view
#include <utility>
#include <vector>
#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ostream.h>

#include "stats_printer.h"

#include "ooo_cpu.h"
extern std::map<uint64_t, ObjectInfo*> live_table; // 当前活跃对象（地址 → object）
extern std::unordered_map<uint64_t, ObjectInfo> history_table; // 所有对象（object_id → object）
extern std::unordered_map<uint64_t, uint64_t> instr_to_addr; // (指令ip → addr)
extern std::unordered_map<uint64_t, uint64_t> pa_to_va_map;
extern uint64_t peak_live_bytes;
extern ObjectInfo* find_object(uint64_t addr);

namespace
{
template <typename N, typename D>
auto print_ratio(N num, D denom)
{
  if (denom > 0) {
    return fmt::format("{:.4g}", std::ceil(num) / std::ceil(denom));
  }
  return std::string{"-"};
}
} // namespace

std::vector<std::string> champsim::plain_printer::format(O3_CPU::stats_type stats)
{
  constexpr std::array types{branch_type::BRANCH_DIRECT_JUMP, branch_type::BRANCH_INDIRECT,      branch_type::BRANCH_CONDITIONAL,
                             branch_type::BRANCH_DIRECT_CALL, branch_type::BRANCH_INDIRECT_CALL, branch_type::BRANCH_RETURN};
  auto total_branch = std::ceil(
      std::accumulate(std::begin(types), std::end(types), 0LL, [tbt = stats.total_branch_types](auto acc, auto next) { return acc + tbt.value_or(next, 0); }));
  auto total_mispredictions = std::ceil(
      std::accumulate(std::begin(types), std::end(types), 0LL, [btm = stats.branch_type_misses](auto acc, auto next) { return acc + btm.value_or(next, 0); }));

  std::vector<std::string> lines{};
  lines.push_back(fmt::format("{} cumulative IPC: {} instructions: {} cycles: {}", stats.name, ::print_ratio(stats.instrs(), stats.cycles()), stats.instrs(),
                              stats.cycles()));

  lines.push_back(fmt::format("{} Branch Prediction Accuracy: {}% MPKI: {} Average ROB Occupancy at Mispredict: {}", stats.name,
                              ::print_ratio(100 * (total_branch - total_mispredictions), total_branch),
                              ::print_ratio(std::kilo::num * total_mispredictions, stats.instrs()),
                              ::print_ratio(stats.total_rob_occupancy_at_branch_mispredict, total_mispredictions)));

  lines.emplace_back("Branch type MPKI");
  for (auto idx : types) {
    lines.push_back(fmt::format("{}: {}", branch_type_names.at(champsim::to_underlying(idx)),
                                ::print_ratio(std::kilo::num * stats.branch_type_misses.value_or(idx, 0), stats.instrs())));
  }

  

  return lines;
}

std::vector<std::string> champsim::plain_printer::format(CACHE::stats_type stats)
{
  using hits_value_type = typename decltype(stats.hits)::value_type;
  using misses_value_type = typename decltype(stats.misses)::value_type;
  using mshr_merge_value_type = typename decltype(stats.mshr_merge)::value_type;
  using mshr_return_value_type = typename decltype(stats.mshr_return)::value_type;

  std::vector<std::size_t> cpus;

  // build a vector of all existing cpus
  auto stat_keys = {stats.hits.get_keys(), stats.misses.get_keys(), stats.mshr_merge.get_keys(), stats.mshr_return.get_keys()};
  for (auto keys : stat_keys) {
    std::transform(std::begin(keys), std::end(keys), std::back_inserter(cpus), [](auto val) { return val.second; });
  }
  std::sort(std::begin(cpus), std::end(cpus));
  auto uniq_end = std::unique(std::begin(cpus), std::end(cpus));
  cpus.erase(uniq_end, std::end(cpus));

  for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION}) {
    for (auto cpu : cpus) {
      stats.hits.allocate(std::pair{type, cpu});
      stats.misses.allocate(std::pair{type, cpu});
      stats.mshr_merge.allocate(std::pair{type, cpu});
      stats.mshr_return.allocate(std::pair{type, cpu});
    }
  }

  std::vector<std::string> lines{};
  for (auto cpu : cpus) {
    hits_value_type total_hits = 0;
    misses_value_type total_misses = 0;
    mshr_merge_value_type total_mshr_merge = 0;
    mshr_return_value_type total_mshr_return = 0;
    for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION}) {
      total_hits += stats.hits.value_or(std::pair{type, cpu}, hits_value_type{});
      total_misses += stats.misses.value_or(std::pair{type, cpu}, misses_value_type{});
      total_mshr_merge += stats.mshr_merge.value_or(std::pair{type, cpu}, mshr_merge_value_type{});
      total_mshr_return += stats.mshr_return.value_or(std::pair{type, cpu}, mshr_merge_value_type{});
    }

    fmt::format_string<std::string_view, std::string_view, int, int, int> hitmiss_fmtstr{
        "cpu{}->{} {:<12s} ACCESS: {:10d} HIT: {:10d} MISS: {:10d} MSHR_MERGE: {:10d}"};
    lines.push_back(fmt::format(hitmiss_fmtstr, cpu, stats.name, "TOTAL", total_hits + total_misses, total_hits, total_misses, total_mshr_merge));
    for (const auto type : {access_type::LOAD, access_type::RFO, access_type::PREFETCH, access_type::WRITE, access_type::TRANSLATION}) {
      lines.push_back(
          fmt::format(hitmiss_fmtstr, cpu, stats.name, access_type_names.at(champsim::to_underlying(type)),
                      stats.hits.value_or(std::pair{type, cpu}, hits_value_type{}) + stats.misses.value_or(std::pair{type, cpu}, misses_value_type{}),
                      stats.hits.value_or(std::pair{type, cpu}, hits_value_type{}), stats.misses.value_or(std::pair{type, cpu}, misses_value_type{}),
                      stats.mshr_merge.value_or(std::pair{type, cpu}, mshr_merge_value_type{})));
    }

    lines.push_back(fmt::format("cpu{}->{} PREFETCH REQUESTED: {:10} ISSUED: {:10} USEFUL: {:10} USELESS: {:10}", cpu, stats.name, stats.pf_requested,
                                stats.pf_issued, stats.pf_useful, stats.pf_useless));

    uint64_t total_downstream_demands = total_mshr_return - stats.mshr_return.value_or(std::pair{access_type::PREFETCH, cpu}, mshr_return_value_type{});
    lines.push_back(
        fmt::format("cpu{}->{} AVERAGE MISS LATENCY: {} cycles", cpu, stats.name, ::print_ratio(stats.total_miss_latency_cycles, total_downstream_demands)));
  }

  return lines;
}

std::vector<std::string> champsim::plain_printer::format(DRAM_CHANNEL::stats_type stats)
{
  std::vector<std::string> lines{};
  lines.push_back(fmt::format("{} RQ ROW_BUFFER_HIT: {:10}", stats.name, stats.RQ_ROW_BUFFER_HIT));
  lines.push_back(fmt::format("  ROW_BUFFER_MISS: {:10}", stats.RQ_ROW_BUFFER_MISS));
  lines.push_back(fmt::format("  AVG DBUS CONGESTED CYCLE: {}", ::print_ratio(stats.dbus_cycle_congested, stats.dbus_count_congested)));
  lines.push_back(fmt::format("{} WQ ROW_BUFFER_HIT: {:10}", stats.name, stats.WQ_ROW_BUFFER_HIT));
  lines.push_back(fmt::format("  ROW_BUFFER_MISS: {:10}", stats.WQ_ROW_BUFFER_MISS));
  lines.push_back(fmt::format("  FULL: {:10}", stats.WQ_FULL));

  if (stats.refresh_cycles > 0)
    lines.push_back(fmt::format("{} REFRESHES ISSUED: {:10}", stats.name, stats.refresh_cycles));
  else
    lines.push_back(fmt::format("{} REFRESHES ISSUED: -", stats.name));

  return lines;
}

static inline uint64_t compute_lifetime(
    const ObjectInfo& obj)
{
    if (obj.alive) {
        return 0;
    }
    auto diff = obj.free_time - obj.alloc_time;
    return static_cast<uint64_t>(diff.count());
}

#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

void print_object_stats(std::vector<std::string>& lines, const std::unordered_map<uint64_t, ObjectInfo>& history_table, uint64_t current_time, uint64_t total_instr) {
    lines.emplace_back("OBJECT STATISTICS (sorted by size)");
    lines.emplace_back("--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");

    // 精准固定宽度，全局对齐
    auto header = fmt::format(
        "{:<6} {:<10} {:<12} {:<10} {:<12} {:<10} {:<10} {:<9} {:<9} {:<9} "
        "{:<11} {:<11} {:<9} "
        "{:<11} {:<11} {:<9} "
        "{:<11} {:<11} {:<9} "
        "{:<11} {:<11} {:<9} "
        "{:<11} {:<11} {:<9} "
        "{:<11} {:<11} {:<9} "
        "{:<11} {:<11} {:<9} "
        "{:<11} {:<11} {:<9}",

        "ID", "SIZE", "LIFE", "ACCESS", "DPKB", "MPKI", "LAT",
        "L1_MR%", "L2_MR%", "PEAK",

        "L1D_LD_ACC", "L1D_LD_MISS", "L1D_LD_MR%",
        "L1D_RFO_ACC", "L1D_RFO_MISS", "L1D_RFO_MR%",
        "L1D_PF_ACC",  "L1D_PF_MISS",  "L1D_PF_MR%",
        "L1D_WR_ACC",  "L1D_WR_MISS",  "L1D_WR_MR%",

        "L2C_LD_ACC",  "L2C_LD_MISS",  "L2C_LD_MR%",
        "L2C_RFO_ACC", "L2C_RFO_MISS", "L2C_RFO_MR%",
        "L2C_PF_ACC",  "L2C_PF_MISS",  "L2C_PF_MR%",
        "L2C_WR_ACC",  "L2C_WR_MISS",  "L2C_WR_MR%"
    );
    lines.push_back(header);
    lines.emplace_back("--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------");

    std::vector<const ObjectInfo*> objs;
    for (auto& [id, obj] : history_table)
        if (!obj.alive)
            objs.push_back(&obj);

    std::sort(objs.begin(), objs.end(), [](auto* a, auto* b) {
        return a->size > b->size;
    });

    uint64_t total_acc = 0;
    for (const auto* obj : objs) {
        for (int t = 0; t < NUM_ACCESS_TYPE; t++) {
            if (t == TRANSLATION) continue;
            total_acc += obj->access[L1D][t] + obj->access[L2C][t];
        }
    }

    json json_data;
    json metadata;
    metadata["total_instructions"] = total_instr;
    metadata["total_accesses"] = total_acc;
    metadata["sorted_by"] = "object_size_desc";
    json_data["metadata"] = metadata;
    json objects_json = json::array();

    auto mr = [](uint64_t a, uint64_t m) {
        return (a > 0) ? 100.0 * m / a : 0.0;
    };

    for (const auto* obj : objs) {
        uint64_t l1_access = 0, l1_miss = 0;
        uint64_t l2_access = 0, l2_miss = 0;
        for (int t = 0; t < NUM_ACCESS_TYPE; t++) {
            if (t == TRANSLATION) continue;
            l1_access += obj->access[L1D][t];
            l1_miss += obj->miss[L1D][t];
            l2_access += obj->access[L2C][t];
            l2_miss += obj->miss[L2C][t];
        }

        uint64_t total_access = l1_access + l2_access;
        uint64_t lifetime = compute_lifetime(*obj);
        uint64_t total_miss = l1_miss + l2_miss;

        double mpki = 0.0, avg_lat = 0.0;
        double l1_mr = 0.0, l2_mr = 0.0, peak = 0.0;
        if (total_instr > 0) mpki = 1000.0 * total_miss / total_instr;
        if (l1_access > 0)  l1_mr = 100.0 * l1_miss / l1_access;
        if (l2_access > 0)  l2_mr = 100.0 * l2_miss / l2_access;
        if (obj->latency_event_count > 0) avg_lat = (double)obj->total_miss_latency / obj->latency_event_count;
        if (peak_live_bytes > 0) peak = 100.0 * obj->size / peak_live_bytes;
        double dpkb = obj->size ? (double)total_access / (obj->size / 1024.0) : 0.0;

        // L1D
        uint64_t l1d_ld_acc  = obj->access[L1D][LOAD];
        uint64_t l1d_ld_miss = obj->miss[L1D][LOAD];
        double   l1d_ld_mr   = mr(l1d_ld_acc, l1d_ld_miss);

        uint64_t l1d_rfo_acc  = obj->access[L1D][RFO];
        uint64_t l1d_rfo_miss = obj->miss[L1D][RFO];
        double   l1d_rfo_mr   = mr(l1d_rfo_acc, l1d_rfo_miss);

        uint64_t l1d_pf_acc  = obj->access[L1D][PREFETCH];
        uint64_t l1d_pf_miss = obj->miss[L1D][PREFETCH];
        double   l1d_pf_mr   = mr(l1d_pf_acc, l1d_pf_miss);

        uint64_t l1d_wr_acc  = obj->access[L1D][WRITE];
        uint64_t l1d_wr_miss = obj->miss[L1D][WRITE];
        double   l1d_wr_mr   = mr(l1d_wr_acc, l1d_wr_miss);

        // L2C
        uint64_t l2c_ld_acc  = obj->access[L2C][LOAD];
        uint64_t l2c_ld_miss = obj->miss[L2C][LOAD];
        double   l2c_ld_mr   = mr(l2c_ld_acc, l2c_ld_miss);

        uint64_t l2c_rfo_acc  = obj->access[L2C][RFO];
        uint64_t l2c_rfo_miss = obj->miss[L2C][RFO];
        double   l2c_rfo_mr   = mr(l2c_rfo_acc, l2c_rfo_miss);

        uint64_t l2c_pf_acc  = obj->access[L2C][PREFETCH];
        uint64_t l2c_pf_miss = obj->miss[L2C][PREFETCH];
        double   l2c_pf_mr   = mr(l2c_pf_acc, l2c_pf_miss);

        uint64_t l2c_wr_acc  = obj->access[L2C][WRITE];
        uint64_t l2c_wr_miss = obj->miss[L2C][WRITE];
        double   l2c_wr_mr   = mr(l2c_wr_acc, l2c_wr_miss);

        // 数值行严格等宽对齐
        auto row = fmt::format(
            "{:<6x} {:<10} {:<12} {:<10} {:<12.2f} {:<10.2f} {:<10.2f} {:<8.2f}% {:<8.2f}% {:<8.2f}% "
            "{:<11} {:<11} {:<8.2f} "
            "{:<11} {:<11} {:<8.2f} "
            "{:<11} {:<11} {:<8.2f} "
            "{:<11} {:<11} {:<8.2f} "
            "{:<11} {:<11} {:<8.2f} "
            "{:<11} {:<11} {:<8.2f} "
            "{:<11} {:<11} {:<8.2f} "
            "{:<11} {:<11} {:<8.2f}",

            obj->object_id, obj->size, lifetime, total_access, dpkb,
            mpki, avg_lat, l1_mr, l2_mr, peak,

            l1d_ld_acc, l1d_ld_miss, l1d_ld_mr,
            l1d_rfo_acc, l1d_rfo_miss, l1d_rfo_mr,
            l1d_pf_acc,  l1d_pf_miss,  l1d_pf_mr,
            l1d_wr_acc,  l1d_wr_miss,  l1d_wr_mr,

            l2c_ld_acc,  l2c_ld_miss,  l2c_ld_mr,
            l2c_rfo_acc, l2c_rfo_miss, l2c_rfo_mr,
            l2c_pf_acc,  l2c_pf_miss,  l2c_pf_mr,
            l2c_wr_acc,  l2c_wr_miss,  l2c_wr_mr
        );
        lines.push_back(row);

        // JSON 保留完整三维结构
        json o;
        o["object_id"] = fmt::format("{:x}", obj->object_id);
        o["size"] = obj->size;
        o["lifetime"] = lifetime;
        o["total_access"] = total_access;
        o["dpkb"] = dpkb;
        o["mpki"] = mpki;
        o["avg_miss_latency"] = avg_lat;
        o["l1_miss_rate_pct"] = l1_mr;
        o["l2_miss_rate_pct"] = l2_mr;
        o["peak_memory_pct"] = peak;

        o["L1D"]["LOAD"]      = {{"access", l1d_ld_acc}, {"miss", l1d_ld_miss}, {"miss_rate_pct", l1d_ld_mr}};
        o["L1D"]["RFO"]       = {{"access", l1d_rfo_acc}, {"miss", l1d_rfo_miss}, {"miss_rate_pct", l1d_rfo_mr}};
        o["L1D"]["PREFETCH"]  = {{"access", l1d_pf_acc}, {"miss", l1d_pf_miss}, {"miss_rate_pct", l1d_pf_mr}};
        o["L1D"]["WRITE"]     = {{"access", l1d_wr_acc}, {"miss", l1d_wr_miss}, {"miss_rate_pct", l1d_wr_mr}};

        o["L2C"]["LOAD"]      = {{"access", l2c_ld_acc}, {"miss", l2c_ld_miss}, {"miss_rate_pct", l2c_ld_mr}};
        o["L2C"]["RFO"]       = {{"access", l2c_rfo_acc}, {"miss", l2c_rfo_miss}, {"miss_rate_pct", l2c_rfo_mr}};
        o["L2C"]["PREFETCH"]  = {{"access", l2c_pf_acc}, {"miss", l2c_pf_miss}, {"miss_rate_pct", l2c_pf_mr}};
        o["L2C"]["WRITE"]     = {{"access", l2c_wr_acc}, {"miss", l2c_wr_miss}, {"miss_rate_pct", l2c_wr_mr}};

        objects_json.push_back(o);
    }

    json_data["objects"] = objects_json;
    try {
        if (!fs::exists("result")) fs::create_directory("result");
        std::ofstream f("result/object_stats.json");
        if (f) { f << json_data.dump(4); f.close(); }
    } catch (...) {}
}

void champsim::plain_printer::print(champsim::phase_stats& stats)
{
  auto lines = format(stats);
  std::copy(std::begin(lines), std::end(lines), std::ostream_iterator<std::string>(stream, "\n"));
}

std::vector<std::string> champsim::plain_printer::format(champsim::phase_stats& stats)
{
  std::vector<std::string> lines{};
  lines.push_back(fmt::format("=== {} ===", stats.name));

  int i = 0;
  for (auto tn : stats.trace_names) {
    lines.push_back(fmt::format("CPU {} runs {}", i++, tn));
  }

  if (NUM_CPUS > 1) {
    lines.emplace_back("");
    lines.emplace_back("Total Simulation Statistics (not including warmup)");

    for (const auto& stat : stats.sim_cpu_stats) {
      auto sublines = format(stat);
      lines.emplace_back("");
      std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
      lines.emplace_back("");
    }

    for (const auto& stat : stats.sim_cache_stats) {
      auto sublines = format(stat);
      std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
    }
  }

  lines.emplace_back("");
  lines.emplace_back("Region of Interest Statistics");

  for (const auto& stat : stats.roi_cpu_stats) {
    auto sublines = format(stat);
    lines.emplace_back("");
    std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
    lines.emplace_back("");
  }

  for (const auto& stat : stats.roi_cache_stats) {
    auto sublines = format(stat);
    std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
  }

  lines.emplace_back("");
  lines.emplace_back("DRAM Statistics");
  for (const auto& stat : stats.roi_dram_stats) {
    auto sublines = format(stat);
    lines.emplace_back("");
    std::move(std::begin(sublines), std::end(sublines), std::back_inserter(lines));
  }

  uint64_t total_instr = 0;

  for (const auto& cpu_stat : stats.roi_cpu_stats)
    total_instr += cpu_stat.instrs();

  print_object_stats(
    lines,
    history_table,
    clock(),
    total_instr
  );

  // printf("pa2va map size: %d\n", pa_to_va_map.size());

  return lines;
}

void champsim::plain_printer::print(std::vector<phase_stats>& stats)
{
  for (auto p : stats) {
    print(p);
  }
}
