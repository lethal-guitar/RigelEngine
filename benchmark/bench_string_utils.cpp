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


static void BMStringSplit(benchmark::State &state) {
  std::string const kInputString = "Hello, world";
  for (auto _ : state) {
    const auto v = rigel::strings::split(kInputString, ',');
    benchmark::DoNotOptimize(v);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(BMStringSplit);

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
