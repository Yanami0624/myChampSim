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

struct PrintConfig {

    // 基本信息
    bool show_state      = true;
    bool show_base_addr  = true;
    bool show_size       = true;

    // 生命周期
    bool show_lifetime   = true;

    // cache 层
    bool show_l1         = true;
    bool show_l2         = true;
    bool show_llc        = true;

    bool show_hit_rate   = true;

    // 过滤
    bool show_only_alive = false;
};

static inline uint64_t compute_lifetime(
    const ObjectInfo& obj,
    uint64_t current_time)
{
    if (obj.alive) {
        return 0;
    }


    auto diff = obj.free_time - obj.alloc_time;
    return static_cast<uint64_t>(diff.count());
}

void print_object_stats(
    std::vector<std::string>& lines,
    const std::unordered_map<uint64_t, ObjectInfo>& history_table,
    const PrintConfig& cfg,
    uint64_t current_time)
{
    lines.emplace_back("");
    lines.emplace_back("OBJECT STATISTICS");

    lines.emplace_back(
"--------------------------------------------------------------------------------------------------------------");

    std::string header;

    header += fmt::format("{:<8}", "ID");

    if (cfg.show_state)
        header += fmt::format(" {:<6}", "STATE");

    if (cfg.show_base_addr)
        header += fmt::format(" {:<16}", "BASE_ADDR");

    if (cfg.show_size)
        header += fmt::format(" {:<10}", "SIZE");

    if (cfg.show_lifetime)
        header += fmt::format(" {:<10}", "LIFETIME");

    if (cfg.show_l1) {

        header += fmt::format(
            " {:<7} {:<7} {:<8}",
            "L1_ACC",
            "L1_HIT",
            "L1_MISS"
        );

        if (cfg.show_hit_rate)
            header += fmt::format(" {:<6}", "L1_HR");
    }

    if (cfg.show_l2) {

        header += fmt::format(
            " {:<7} {:<7} {:<8}",
            "L2_ACC",
            "L2_HIT",
            "L2_MISS"
        );
    }

    if (cfg.show_llc) {

        header += fmt::format(
            " {:<7} {:<7} {:<8}",
            "LLC_ACC",
            "LLC_HIT",
            "LLC_MISS"
        );
    }

    lines.push_back(header);

    lines.emplace_back(
"--------------------------------------------------------------------------------------------------------------");

    for (const auto& [id, obj] : history_table) {

        if (cfg.show_only_alive && !obj.alive)
            continue;

        uint64_t l1_acc  = obj.hit_count_l1  + obj.miss_count_l1;
        uint64_t l2_acc  = obj.hit_count_l2  + obj.miss_count_l2;
        uint64_t llc_acc = obj.hit_count_llc + obj.miss_count_llc;

        double l1_hr =
            (l1_acc == 0)
            ? 0.0
            : 100.0 * obj.hit_count_l1 / l1_acc;

        uint64_t lifetime =
            compute_lifetime(obj, current_time);

        std::string row;

        row += fmt::format("{:<8x}", id);

        if (cfg.show_state)
            row += fmt::format(
                " {:<6}",
                (obj.alive ? "alive" : "dead")
            );

        if (cfg.show_base_addr)
            row += fmt::format(
                " {:<16x}",
                obj.base_addr
            );

        if (cfg.show_size)
            row += fmt::format(
                " {:<10x}",
                obj.size
            );

        if (cfg.show_lifetime)
            row += fmt::format(
                " {:<10}",
                lifetime
            );

        if (cfg.show_l1) {

            row += fmt::format(
                " {:<7} {:<7} {:<8}",
                l1_acc,
                obj.hit_count_l1,
                obj.miss_count_l1
            );

            if (cfg.show_hit_rate)
                row += fmt::format(
                    " {:<5.1f}%",
                    l1_hr
                );
        }

        if (cfg.show_l2) {

            row += fmt::format(
                " {:<7} {:<7} {:<8}",
                l2_acc,
                obj.hit_count_l2,
                obj.miss_count_l2
            );
        }

        if (cfg.show_llc) {

            row += fmt::format(
                " {:<7} {:<7} {:<8}",
                llc_acc,
                obj.hit_count_llc,
                obj.miss_count_llc
            );
        }

        lines.push_back(row);
    }
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

  PrintConfig cfg;

cfg.show_state     = true;
cfg.show_base_addr = false;
cfg.show_size      = true;
cfg.show_lifetime  = true;

cfg.show_l1  = true;
cfg.show_l2  = true;
cfg.show_llc = false;

cfg.show_hit_rate = true;

print_object_stats(
    lines,
    history_table,
    cfg,
    clock()   // 或 global_clock
);

  // lines.emplace_back("");
  // lines.emplace_back("INSTR_TO_ADDR");
  // for(auto& [ip, addr] : instr_to_addr) {
  //   break;
  //   lines.push_back(fmt::format(
  //     "MAPPING {} {}",
  //     ip, addr
  //   ));
  // }

  return lines;
}

void champsim::plain_printer::print(std::vector<phase_stats>& stats)
{
  for (auto p : stats) {
    print(p);
  }
}
