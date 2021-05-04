/* Copyright (C) 2021, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <benchmark/benchmark.h>

#include <base/string_utils.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/algorithm.hpp>

static void BMStringSplit(benchmark::State &state) {
  std::string const kInputString = "Hello, world";
  for (auto _ : state) {
    const auto v = rigel::strings::split(kInputString, ',');
    benchmark::DoNotOptimize(v);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(BMStringSplit);

static void BMBoostStringSplit(benchmark::State &state) {
  const std::string kInputString = "Hello, world";
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<std::string> v;
    state.ResumeTiming();
    boost::split(v, kInputString, boost::is_any_of(","));
    benchmark::DoNotOptimize(v);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(BMBoostStringSplit);

static void BMStartsWithTrueCase(benchmark::State &state) {
  const std::string kInputString = "12341234";
  for (auto _ : state) {
    benchmark::DoNotOptimize(
      rigel::strings::startsWith(kInputString, "1234"));
  }
}

BENCHMARK(BMStartsWithTrueCase);


static void BMStartsWithTrueCaseLongString(benchmark::State &state) {
  const std::string kInputString = "Iid7tUoNzQaGQjb9QqmuvqVQU9XbPmOvVbOI5ozuKdQN9bdHeP";
  for (auto _ : state) {
    benchmark::DoNotOptimize(
      rigel::strings::startsWith(kInputString, "Iid7tUoNzQaGQjb9QqmuvqVQU9Xb"));
  }
}

BENCHMARK(BMStartsWithTrueCaseLongString);

static void BMStartsWithFalseCase(benchmark::State &state) {
  const std::string kInputString = "12341234";
  for (auto _ : state) {
    benchmark::DoNotOptimize(
      rigel::strings::startsWith(kInputString, "234"));
  }
}

BENCHMARK(BMStartsWithFalseCase);


static void BMBoostStartsWithTrueCase(benchmark::State &state) {
  const std::string kInputString = "12341234";
  for (auto _ : state) {
    benchmark::DoNotOptimize(
      boost::starts_with(kInputString, "1234"));
  }
}

BENCHMARK(BMBoostStartsWithTrueCase);

static void BMBoostStartsWithFalseCase(benchmark::State &state) {
  const std::string kInputString = "12341234";
  for (auto _ : state) {
    benchmark::DoNotOptimize(
      boost::starts_with(kInputString, "234"));
  }
}

BENCHMARK(BMBoostStartsWithFalseCase);

static void BMTrimLeft(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::string kInputString = "  1234  ";
    state.ResumeTiming();
    benchmark::DoNotOptimize(
      rigel::strings::trimLeft(kInputString));
  }
}

BENCHMARK(BMTrimLeft);

static void BMTrimRight(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::string kInputString = "  1234  ";
    state.ResumeTiming();
    benchmark::DoNotOptimize(
      rigel::strings::trimRight(kInputString));
  }
}

BENCHMARK(BMTrimRight);

static void BMTrim(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::string kInputString = "  1234  ";
    state.ResumeTiming();
    benchmark::DoNotOptimize(
      rigel::strings::trim(kInputString));
  }
}

BENCHMARK(BMTrim);

static void BMBoostTrim(benchmark::State &state) {
  for (auto _ : state) {
    state.PauseTiming();
    std::string kInputString = "  1234  ";
    state.ResumeTiming();
    boost::trim(kInputString);
  }
}

BENCHMARK(BMBoostTrim);
