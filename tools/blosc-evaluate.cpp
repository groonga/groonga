// Copyright (C) 2023  Sutou Kouhei <kou@clear-code.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

// How to build:
//   $ c++ -O2 -o blosc-evaluate blosc-evaluate.cpp $(pkg-config --cflags --libs arrow blosc2)
// How to run:
//   $ ./blosc-evaluate input.arrow 2
//
//   input.arrow must have a ListArray at the 2nd (0-origin) column

// Sample result:
//
// zstd: Record: Total: Source: 287444400
// zstd: Record: Total: Compress: 261336432: 90.9172%: 0.599758s
// zstd: Record: Total: Decompress: 0.25487s
// zstd: RecordSort: Total: Source: 287444400
// zstd: RecordSort: Total: Compress: 220252587: 76.6244%: 9.33714s
// zstd: RecordSort: Total: Decompress: 0.284323s
// zstd: RecordMinShift: Total: Source: 287444400
// zstd: RecordMinShift: Total: Compress: 245690871: 85.4742%: 0.933335s
// zstd: RecordMinShift: Total: Decompress: 0.253858s
// zstd: RecordNormalize: Total: Source: 287444400
// zstd: RecordNormalize: Total: Compress: 246824309: 85.8685%: 0.950742s
// zstd: RecordNormalize: Total: Decompress: 0.253937s
// zstd: RecordStandardize: Total: Source: 287444400
// zstd: RecordStandardize: Total: Compress: 272578560: 94.8283%: 0.895418s
// zstd: RecordStandardize: Total: Decompress: 0.256032s
// zstd: RecordBatch: Total: Source: 287444400
// zstd: RecordBatch: Total: Compress: 258786000: 90.0299%: 0.455626s
// zstd: RecordBatch: Total: Decompress: 0.247945s
// zstd: RecordBatchSort: Total: Source: 287444400
// zstd: RecordBatchSort: Total: Compress: 45094890: 15.6882%: 1.91584s
// zstd: RecordBatchSort: Total: Decompress: 0.256293s
// zstd: RecordBatchMinShift: Total: Source: 287444400
// zstd: RecordBatchMinShift: Total: Compress: 230630363: 80.2348%: 0.578625s
// zstd: RecordBatchMinShift: Total: Decompress: 0.208425s
// zstd: RecordBatchNormalize: Total: Source: 287444400
// zstd: RecordBatchNormalize: Total: Compress: 238442111: 82.9524%: 0.562892s
// zstd: RecordBatchNormalize: Total: Decompress: 0.231286s
// zstd: RecordBatchStandardize: Total: Source: 287444400
// zstd: RecordBatchStandardize: Total: Compress: 267859285: 93.1865%: 0.395662s
// zstd: RecordBatchStandardize: Total: Decompress: 0.247745s
// zstd + shuffle: Record: Total: Source: 287444400
// zstd + shuffle: Record: Total: Compress: 249624818: 86.8428%: 2.09633s
// zstd + shuffle: Record: Total: Decompress: 0.177167s
// zstd + shuffle: RecordSort: Total: Source: 287444400
// zstd + shuffle: RecordSort: Total: Compress: 138016447: 48.015%: 8.19193s
// zstd + shuffle: RecordSort: Total: Decompress: 0.101493s
// zstd + shuffle: RecordMinShift: Total: Source: 287444400
// zstd + shuffle: RecordMinShift: Total: Compress: 221706358: 77.1302%: 2.66121s
// zstd + shuffle: RecordMinShift: Total: Decompress: 0.0840233s
// zstd + shuffle: RecordNormalize: Total: Source: 287444400
// zstd + shuffle: RecordNormalize: Total: Compress: 223060008: 77.6011%: 2.82881s
// zstd + shuffle: RecordNormalize: Total: Decompress: 0.088614s
// zstd + shuffle: RecordStandardize: Total: Source: 287444400
// zstd + shuffle: RecordStandardize: Total: Compress: 260332140: 90.5678%: 1.65743s
// zstd + shuffle: RecordStandardize: Total: Decompress: 0.065551s
// zstd + shuffle: RecordBatch: Total: Source: 287444400
// zstd + shuffle: RecordBatch: Total: Compress: 239263258: 83.2381%: 2.24826s
// zstd + shuffle: RecordBatch: Total: Decompress: 0.144454s
// zstd + shuffle: RecordBatchSort: Total: Source: 287444400
// zstd + shuffle: RecordBatchSort: Total: Compress: 36563946: 12.7204%: 1.67761s
// zstd + shuffle: RecordBatchSort: Total: Decompress: 0.138202s
// zstd + shuffle: RecordBatchMinShift: Total: Source: 287444400
// zstd + shuffle: RecordBatchMinShift: Total: Compress: 203745066: 70.8816%: 0.569811s
// zstd + shuffle: RecordBatchMinShift: Total: Decompress: 0.211793s
// zstd + shuffle: RecordBatchNormalize: Total: Source: 287444400
// zstd + shuffle: RecordBatchNormalize: Total: Compress: 212290902: 73.8546%: 1.95976s
// zstd + shuffle: RecordBatchNormalize: Total: Decompress: 0.245326s
// zstd + shuffle: RecordBatchStandardize: Total: Source: 287444400
// zstd + shuffle: RecordBatchStandardize: Total: Compress: 249115033: 86.6655%: 1.11073s
// zstd + shuffle: RecordBatchStandardize: Total: Decompress: 0.148155s
// zstd + shuffle + bytedelta: Record: Total: Source: 287444400
// zstd + shuffle + bytedelta: Record: Total: Compress: 255816514: 88.9969%: 1.8307s
// zstd + shuffle + bytedelta: Record: Total: Decompress: 0.232332s
// zstd + shuffle + bytedelta: RecordSort: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordSort: Total: Compress: 116441454: 40.5092%: 9.1973s
// zstd + shuffle + bytedelta: RecordSort: Total: Decompress: 0.223703s
// zstd + shuffle + bytedelta: RecordMinShift: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordMinShift: Total: Compress: 223187491: 77.6454%: 2.50365s
// zstd + shuffle + bytedelta: RecordMinShift: Total: Decompress: 0.103321s
// zstd + shuffle + bytedelta: RecordNormalize: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordNormalize: Total: Compress: 224754048: 78.1904%: 2.66798s
// zstd + shuffle + bytedelta: RecordNormalize: Total: Decompress: 0.108945s
// zstd + shuffle + bytedelta: RecordStandardize: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordStandardize: Total: Compress: 261608098: 91.0117%: 1.67885s
// zstd + shuffle + bytedelta: RecordStandardize: Total: Decompress: 0.0888164s
// zstd + shuffle + bytedelta: RecordBatch: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordBatch: Total: Compress: 243955753: 84.8706%: 1.99933s
// zstd + shuffle + bytedelta: RecordBatch: Total: Decompress: 0.171276s
// zstd + shuffle + bytedelta: RecordBatchSort: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordBatchSort: Total: Compress: 23772029: 8.27013%: 2.82653s
// zstd + shuffle + bytedelta: RecordBatchSort: Total: Decompress: 0.156963s
// zstd + shuffle + bytedelta: RecordBatchMinShift: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordBatchMinShift: Total: Compress: 208135913: 72.4091%: 0.559196s
// zstd + shuffle + bytedelta: RecordBatchMinShift: Total: Decompress: 0.107472s
// zstd + shuffle + bytedelta: RecordBatchNormalize: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordBatchNormalize: Total: Compress: 216503548: 75.3201%: 1.82629s
// zstd + shuffle + bytedelta: RecordBatchNormalize: Total: Decompress: 0.175968s
// zstd + shuffle + bytedelta: RecordBatchStandardize: Total: Source: 287444400
// zstd + shuffle + bytedelta: RecordBatchStandardize: Total: Compress: 251630502: 87.5406%: 1.15669s
// zstd + shuffle + bytedelta: RecordBatchStandardize: Total: Decompress: 0.10183s
// lz4: Record: Total: Source: 287444400
// lz4: Record: Total: Compress: 280947604: 97.7398%: 0.0652128s
// lz4: Record: Total: Decompress: 0.0130985s
// lz4: RecordSort: Total: Source: 287444400
// lz4: RecordSort: Total: Compress: 272433013: 94.7776%: 6.94601s
// lz4: RecordSort: Total: Decompress: 0.0370148s
// lz4: RecordMinShift: Total: Source: 287444400
// lz4: RecordMinShift: Total: Compress: 280928606: 97.7332%: 0.352483s
// lz4: RecordMinShift: Total: Decompress: 0.0142529s
// lz4: RecordNormalize: Total: Source: 287444400
// lz4: RecordNormalize: Total: Compress: 280926818: 97.7326%: 0.377359s
// lz4: RecordNormalize: Total: Decompress: 0.0148921s
// lz4: RecordStandardize: Total: Source: 287444400
// lz4: RecordStandardize: Total: Compress: 283819190: 98.7388%: 0.389299s
// lz4: RecordStandardize: Total: Decompress: 0.0155727s
// lz4: RecordBatch: Total: Source: 287444400
// lz4: RecordBatch: Total: Compress: 277523371: 96.5485%: 0.0869983s
// lz4: RecordBatch: Total: Decompress: 0.0389423s
// lz4: RecordBatchSort: Total: Source: 287444400
// lz4: RecordBatchSort: Total: Compress: 88499039: 30.7882%: 0.315801s
// lz4: RecordBatchSort: Total: Decompress: 0.126656s
// lz4: RecordBatchMinShift: Total: Source: 287444400
// lz4: RecordBatchMinShift: Total: Compress: 277469331: 96.5297%: 0.0650974s
// lz4: RecordBatchMinShift: Total: Decompress: 0.0393505s
// lz4: RecordBatchNormalize: Total: Source: 287444400
// lz4: RecordBatchNormalize: Total: Compress: 277437323: 96.5186%: 0.0636185s
// lz4: RecordBatchNormalize: Total: Decompress: 0.0393701s
// lz4: RecordBatchStandardize: Total: Source: 287444400
// lz4: RecordBatchStandardize: Total: Compress: 278429291: 96.8637%: 0.0597975s
// lz4: RecordBatchStandardize: Total: Decompress: 0.0395462s
// lz4 + shuffle: Record: Total: Source: 287444400
// lz4 + shuffle: Record: Total: Compress: 262250060: 91.2351%: 0.289145s
// lz4 + shuffle: Record: Total: Decompress: 0.0458152s
// lz4 + shuffle: RecordSort: Total: Source: 287444400
// lz4 + shuffle: RecordSort: Total: Compress: 152258928: 52.9699%: 6.95159s
// lz4 + shuffle: RecordSort: Total: Decompress: 0.0498535s
// lz4 + shuffle: RecordMinShift: Total: Source: 287444400
// lz4 + shuffle: RecordMinShift: Total: Compress: 247844448: 86.2234%: 0.490865s
// lz4 + shuffle: RecordMinShift: Total: Decompress: 0.073709s
// lz4 + shuffle: RecordNormalize: Total: Source: 287444400
// lz4 + shuffle: RecordNormalize: Total: Compress: 252542551: 87.8579%: 0.522459s
// lz4 + shuffle: RecordNormalize: Total: Decompress: 0.0759528s
// lz4 + shuffle: RecordStandardize: Total: Source: 287444400
// lz4 + shuffle: RecordStandardize: Total: Compress: 269846089: 93.8777%: 0.506312s
// lz4 + shuffle: RecordStandardize: Total: Decompress: 0.0441582s
// lz4 + shuffle: RecordBatch: Total: Source: 287444400
// lz4 + shuffle: RecordBatch: Total: Compress: 250664784: 87.2046%: 0.232639s
// lz4 + shuffle: RecordBatch: Total: Decompress: 0.0745392s
// lz4 + shuffle: RecordBatchSort: Total: Source: 287444400
// lz4 + shuffle: RecordBatchSort: Total: Compress: 56705666: 19.7275%: 0.206601s
// lz4 + shuffle: RecordBatchSort: Total: Decompress: 0.0841206s
// lz4 + shuffle: RecordBatchMinShift: Total: Source: 287444400
// lz4 + shuffle: RecordBatchMinShift: Total: Compress: 205740023: 71.5756%: 0.0889608s
// lz4 + shuffle: RecordBatchMinShift: Total: Decompress: 0.0486303s
// lz4 + shuffle: RecordBatchNormalize: Total: Source: 287444400
// lz4 + shuffle: RecordBatchNormalize: Total: Compress: 242201437: 84.2603%: 0.188966s
// lz4 + shuffle: RecordBatchNormalize: Total: Decompress: 0.102779s
// lz4 + shuffle: RecordBatchStandardize: Total: Source: 287444400
// lz4 + shuffle: RecordBatchStandardize: Total: Compress: 260956807: 90.7851%: 0.138437s
// lz4 + shuffle: RecordBatchStandardize: Total: Decompress: 0.0707157s
// lz4 + shuffle + bytedelta: Record: Total: Source: 287444400
// lz4 + shuffle + bytedelta: Record: Total: Compress: 265450398: 92.3484%: 0.281276s
// lz4 + shuffle + bytedelta: Record: Total: Decompress: 0.0668538s
// lz4 + shuffle + bytedelta: RecordSort: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordSort: Total: Compress: 143742615: 50.0071%: 6.8342s
// lz4 + shuffle + bytedelta: RecordSort: Total: Decompress: 0.0954882s
// lz4 + shuffle + bytedelta: RecordMinShift: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordMinShift: Total: Compress: 246543652: 85.7709%: 0.480066s
// lz4 + shuffle + bytedelta: RecordMinShift: Total: Decompress: 0.0855211s
// lz4 + shuffle + bytedelta: RecordNormalize: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordNormalize: Total: Compress: 250827025: 87.2611%: 0.511373s
// lz4 + shuffle + bytedelta: RecordNormalize: Total: Decompress: 0.0867664s
// lz4 + shuffle + bytedelta: RecordStandardize: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordStandardize: Total: Compress: 269889703: 93.8928%: 0.507261s
// lz4 + shuffle + bytedelta: RecordStandardize: Total: Decompress: 0.0698385s
// lz4 + shuffle + bytedelta: RecordBatch: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordBatch: Total: Compress: 251727790: 87.5744%: 0.264335s
// lz4 + shuffle + bytedelta: RecordBatch: Total: Decompress: 0.0979651s
// lz4 + shuffle + bytedelta: RecordBatchSort: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordBatchSort: Total: Compress: 51162190: 17.799%: 0.161024s
// lz4 + shuffle + bytedelta: RecordBatchSort: Total: Decompress: 0.124016s
// lz4 + shuffle + bytedelta: RecordBatchMinShift: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordBatchMinShift: Total: Compress: 206212553: 71.74%: 0.0949428s
// lz4 + shuffle + bytedelta: RecordBatchMinShift: Total: Decompress: 0.0868394s
// lz4 + shuffle + bytedelta: RecordBatchNormalize: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordBatchNormalize: Total: Compress: 240906825: 83.8099%: 0.176405s
// lz4 + shuffle + bytedelta: RecordBatchNormalize: Total: Decompress: 0.11371s
// lz4 + shuffle + bytedelta: RecordBatchStandardize: Total: Source: 287444400
// lz4 + shuffle + bytedelta: RecordBatchStandardize: Total: Compress: 260847731: 90.7472%: 0.130564s
// lz4 + shuffle + bytedelta: RecordBatchStandardize: Total: Decompress: 0.0914545s

#include <chrono>
#include <iostream>
#include <limits>
#include <type_traits>

#include <arrow/array.h>
#include <arrow/compute/api_aggregate.h>
#include <arrow/compute/api_scalar.h>
#include <arrow/compute/api_vector.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>

#include <blosc2.h>
#include <blosc2/filters-registry.h>

namespace {
  enum class Target {
    Record,
    RecordSort,
    RecordMinShift,
    RecordNormalize,
    RecordStandardize,
    RecordBatch,
    RecordBatchSort,
    RecordBatchMinShift,
    RecordBatchNormalize,
    RecordBatchStandardize,
  };

  std::string
  target_name(Target target)
  {
    switch (target) {
    case Target::Record:
      return "Record";
    case Target::RecordSort:
      return "RecordSort";
    case Target::RecordMinShift:
      return "RecordMinShift";
    case Target::RecordNormalize:
      return "RecordNormalize";
    case Target::RecordStandardize:
      return "RecordStandardize";
    case Target::RecordBatch:
      return "RecordBatch";
    case Target::RecordBatchSort:
      return "RecordBatchSort";
    case Target::RecordBatchMinShift:
      return "RecordBatchMinShift";
    case Target::RecordBatchNormalize:
      return "RecordBatchNormalize";
    case Target::RecordBatchStandardize:
      return "RecordBatchStandardize";
    default:
      return "Unknown";
    }
  }

  struct Pattern {
    std::string name;
    using PrepareFunc = std::add_pointer<void(blosc2_cparams& cparams)>::type;
    PrepareFunc prepare;
  };

  struct Timer {
    template <typename ProcessFunc>
    static std::chrono::steady_clock::duration measure(ProcessFunc func) {
      auto start = std::chrono::steady_clock::now();
      func();
      auto end = std::chrono::steady_clock::now();
      return end - start;
    }
  };

  std::shared_ptr<arrow::Array>
  sort(std::shared_ptr<arrow::Array> array)
  {
    auto sorted_values_indices = *arrow::compute::SortIndices(*array);
    return *arrow::compute::Take(*array, *sorted_values_indices);
  }

  std::shared_ptr<arrow::Array>
  min_shift(std::shared_ptr<arrow::Array> array)
  {
    auto min_max_datum = *arrow::compute::MinMax(*array);
    auto min_max_struct = min_max_datum.scalar_as<arrow::StructScalar>();
    auto min = min_max_struct.value[0];
    return (*arrow::compute::Subtract(*array, min)).make_array();
  }

  std::shared_ptr<arrow::Array>
  normalize(std::shared_ptr<arrow::Array> array)
  {
    auto min_max_datum = *arrow::compute::MinMax(*array);
    auto min_max_struct = min_max_datum.scalar_as<arrow::StructScalar>();
    auto min = min_max_struct.value[0];
    auto min_raw = std::static_pointer_cast<arrow::FloatScalar>(min)->value;
    auto max = min_max_struct.value[1];
    auto max_raw = std::static_pointer_cast<arrow::FloatScalar>(max)->value;
    auto max_diff_raw = max_raw - min_raw;
    auto max_diff = std::make_shared<arrow::FloatScalar>(max_diff_raw);
    if (max_diff_raw < std::numeric_limits<float>::epsilon()) {
      return array;
    } else {
      return (*arrow::compute::Divide(*arrow::compute::Subtract(*array, min),
                                      max_diff))
        .make_array();
    }
  }

  std::shared_ptr<arrow::Array>
  standardize(std::shared_ptr<arrow::Array> array)
  {
    auto mean = *arrow::compute::Mean(*array);
    auto standard = *arrow::compute::Stddev(*array);
    if (standard.scalar_as<arrow::DoubleScalar>().value <
        std::numeric_limits<double>::epsilon()) {
      return array;
    } else {
      return (*arrow::compute::Divide(*arrow::compute::Subtract(*array, mean),
                                      standard))
        .make_array();
    }
  }
}

int
main(int argc, const char**argv)
{
  bool verbose = false;
  // verbose = true;
  auto input_path = argv[1];
  auto nth_column = std::stoi(argv[2]);
  blosc2_init();
  std::vector<Target> targets = {
    Target::Record,
    Target::RecordSort,
    Target::RecordMinShift,
    Target::RecordNormalize,
    Target::RecordStandardize,
    Target::RecordBatch,
    Target::RecordBatchSort,
    Target::RecordBatchMinShift,
    Target::RecordBatchNormalize,
    Target::RecordBatchStandardize,
  };
  std::vector<Pattern> patterns;
  patterns.push_back({"zstd", [](blosc2_cparams &cparams) {
                        cparams.compcode = BLOSC_ZSTD;
                        cparams.filters[BLOSC2_MAX_FILTERS - 1] =
                          BLOSC_NOFILTER;
                      }});
  patterns.push_back({"zstd + shuffle", [](blosc2_cparams &cparams) {
                        cparams.compcode = BLOSC_ZSTD;
                      }});
  patterns.push_back(
    {"zstd + shuffle + bytedelta", [](blosc2_cparams &cparams) {
       cparams.compcode = BLOSC_ZSTD;
       cparams.filters[BLOSC2_MAX_FILTERS - 2] = BLOSC_SHUFFLE;
       cparams.filters[BLOSC2_MAX_FILTERS - 1] = BLOSC_FILTER_BYTEDELTA;
       cparams.filters_meta[BLOSC2_MAX_FILTERS - 1] = sizeof(float);
     }});
  patterns.push_back({"lz4", [](blosc2_cparams &cparams) {
                        cparams.compcode = BLOSC_LZ4;
                        cparams.filters[BLOSC2_MAX_FILTERS - 1] =
                          BLOSC_NOFILTER;
                      }});
  patterns.push_back({"lz4 + shuffle", [](blosc2_cparams &cparams) {
                        cparams.compcode = BLOSC_LZ4;
                      }});
  patterns.push_back(
    {"lz4 + shuffle + bytedelta", [](blosc2_cparams &cparams) {
       cparams.compcode = BLOSC_LZ4;
       cparams.filters[BLOSC2_MAX_FILTERS - 2] = BLOSC_SHUFFLE;
       cparams.filters[BLOSC2_MAX_FILTERS - 1] = BLOSC_FILTER_BYTEDELTA;
       cparams.filters_meta[BLOSC2_MAX_FILTERS - 1] = sizeof(float);
     }});
  for (const auto& pattern : patterns) {
    for (const auto& target : targets) {
      blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
      pattern.prepare(cparams);
      blosc2_context *cctx = blosc2_create_cctx(cparams);
      {
        auto input =
          *arrow::io::MemoryMappedFile::Open(input_path,
                                             arrow::io::FileMode::READ);
        auto reader = *arrow::ipc::RecordBatchFileReader::Open(input);
        auto record_batches = *reader->ToRecordBatches();
        size_t total_source_size = 0;
        size_t total_compressed_size = 0;
        std::chrono::steady_clock::duration total_compress_duration{0};
        std::chrono::steady_clock::duration total_decompress_duration{0};
        switch (target) {
        case Target::Record:
        case Target::RecordSort:
        case Target::RecordMinShift:
        case Target::RecordNormalize:
        case Target::RecordStandardize:
          for (const auto &record_batch : record_batches) {
            const auto &array = std::static_pointer_cast<arrow::ListArray>(
              record_batch->column(nth_column));
            for (int64_t i = 0; i < array->length(); ++i) {
              auto values = std::static_pointer_cast<arrow::FloatArray>(
                array->value_slice(i));
              switch (target) {
              case Target::RecordSort:
                total_compress_duration += Timer::measure([&]() {
                  values =
                    std::static_pointer_cast<arrow::FloatArray>(sort(values));
                });
                break;
              case Target::RecordMinShift:
                total_compress_duration += Timer::measure([&]() {
                  values = std::static_pointer_cast<arrow::FloatArray>(
                    min_shift(values));
                });
                break;
              case Target::RecordNormalize:
                total_compress_duration += Timer::measure([&]() {
                  values = std::static_pointer_cast<arrow::FloatArray>(
                    normalize(values));
                });
                break;
              case Target::RecordStandardize:
                total_compress_duration += Timer::measure([&]() {
                  values = std::static_pointer_cast<arrow::FloatArray>(
                    standardize(values));
                });
                break;
              default:
                break;
              }
              const auto source = values->raw_values();
              const auto source_size = sizeof(float) * values->length();
              std::vector<uint8_t> compressed;
              compressed.reserve(source_size * 2);
              int compressed_size;
              auto compress_duration = Timer::measure([&]() {
                compressed_size = blosc2_compress_ctx(cctx,
                                                      source,
                                                      source_size,
                                                      compressed.data(),
                                                      compressed.capacity());
              });
              if (compressed_size < 0) {
                std::cout << pattern.name << ": " << target_name(target) << ": "
                          << i << ": failed to compress: "
                          << print_error(compressed_size) << std::endl;
                break;
              }
              total_source_size += source_size;
              total_compressed_size += compressed_size;
              total_compress_duration += compress_duration;
              if (verbose) {
                std::cout << pattern.name << ": " << target_name(target) << ": "
                          << i << ": Source: " << source_size << std::endl;
                std::cout << pattern.name << ": " << target_name(target) << ": "
                          << i << ": Compressed: " << compressed_size << "("
                          << (static_cast<double>(compressed_size) /
                              source_size * 100)
                          << "%)" << std::endl;
              }

              auto dparams = BLOSC2_DPARAMS_DEFAULTS;
              auto dctx = blosc2_create_dctx(dparams);
              std::vector<uint8_t> decompressed;
              decompressed.reserve(source_size * 2);
              int decompressed_size;
              auto decompress_duration = Timer::measure([&]() {
                decompressed_size =
                  blosc2_decompress_ctx(dctx,
                                        compressed.data(),
                                        compressed_size,
                                        decompressed.data(),
                                        decompressed.capacity());
              });
              blosc2_free_ctx(dctx);
              if (decompressed_size < 0) {
                std::cout << pattern.name << ": " << target_name(target) << ": "
                          << i << ": failed to decompress: "
                          << print_error(decompressed_size) << std::endl;
                break;
              }
              if (verbose) {
                std::cout << pattern.name << ": " << target_name(target) << ": "
                          << i << ": Decompressed: " << decompressed_size
                          << std::endl;
              }
              if (decompressed_size != static_cast<int>(source_size)) {
                std::cout << pattern.name << ": " << target_name(target) << ": "
                          << i << ": failed to round trip: wrong size"
                          << std::endl;
              }
              if (memcmp(decompressed.data(), source, decompressed_size) != 0) {
                std::cout << pattern.name << ": " << target_name(target) << ": "
                          << i << ": failed to round trip: wrong content"
                          << std::endl;
              }
              total_decompress_duration += decompress_duration;
              // break;
            }
          }
          break;
        case Target::RecordBatch:
        case Target::RecordBatchSort:
        case Target::RecordBatchMinShift:
        case Target::RecordBatchNormalize:
        case Target::RecordBatchStandardize:
          for (const auto &record_batch : record_batches) {
            const auto &array = std::static_pointer_cast<arrow::ListArray>(
              record_batch->column(nth_column));
            auto values =
              std::static_pointer_cast<arrow::FloatArray>(array->values());
            switch (target) {
            case Target::RecordBatchSort:
              total_compress_duration += Timer::measure([&]() {
                values =
                  std::static_pointer_cast<arrow::FloatArray>(sort(values));
              });
              break;
            case Target::RecordBatchMinShift:
              total_compress_duration += Timer::measure([&]() {
                values = std::static_pointer_cast<arrow::FloatArray>(
                  min_shift(values));
              });
              break;
            case Target::RecordBatchNormalize:
              total_compress_duration += Timer::measure([&]() {
                values = std::static_pointer_cast<arrow::FloatArray>(
                  normalize(values));
              });
              break;
            case Target::RecordBatchStandardize:
              total_compress_duration += Timer::measure([&]() {
                values = std::static_pointer_cast<arrow::FloatArray>(
                  standardize(values));
              });
              break;
            default:
              break;
            }
            const auto source = values->raw_values();
            const auto source_size = sizeof(float) * values->length();
            std::vector<uint8_t> compressed;
            compressed.reserve(source_size * 2);
            int compressed_size;
            auto compress_duration = Timer::measure([&]() {
              compressed_size = blosc2_compress_ctx(cctx,
                                                    source,
                                                    source_size,
                                                    compressed.data(),
                                                    compressed.capacity());
            });
            if (compressed_size < 0) {
              std::cout << pattern.name << ": " << target_name(target)
                        << ": failed to compress: "
                        << print_error(compressed_size) << std::endl;
              break;
            }
            total_source_size += source_size;
            total_compressed_size += compressed_size;
            total_compress_duration += compress_duration;
            if (verbose) {
              std::cout << pattern.name << ": " << target_name(target)
                        << ": Source: " << source_size << std::endl;
              std::cout << pattern.name << ": " << target_name(target)
                        << ": Compressed: " << compressed_size
                        << "("
                        << (static_cast<double>(compressed_size) / source_size *
                            100)
                        << "%)" << std::endl;
            }

            auto dparams = BLOSC2_DPARAMS_DEFAULTS;
            auto dctx = blosc2_create_dctx(dparams);
            std::vector<uint8_t> decompressed;
            decompressed.reserve(source_size * 2);
            int decompressed_size;
            auto decompress_duration = Timer::measure([&]() {
              decompressed_size =
                blosc2_decompress_ctx(dctx,
                                      compressed.data(),
                                      compressed_size,
                                      decompressed.data(),
                                      decompressed.capacity());
            });
            blosc2_free_ctx(dctx);
            if (decompressed_size < 0) {
              std::cout << pattern.name << ": " << target_name(target)
                        << ": failed to decompress: "
                        << print_error(decompressed_size) << std::endl;
              break;
            }
            if (verbose) {
              std::cout << pattern.name << ": " << target_name(target)
                        << ": Decompressed: " << decompressed_size
                        << std::endl;
            }
            if (decompressed_size != static_cast<int>(source_size)) {
              std::cout << pattern.name << ": " << target_name(target)
                        << ": failed to round trip: wrong size"
                        << std::endl;
            }
            if (memcmp(decompressed.data(), source, decompressed_size) != 0) {
              std::cout << pattern.name << ": " << target_name(target)
                        << ": failed to round trip: wrong content"
                        << std::endl;
            }
            total_decompress_duration += decompress_duration;
          }
          break;
        }
        std::cout << pattern.name << ": " << target_name(target)
                  << ": Total: Source: " << total_source_size << std::endl;
        std::cout << pattern.name << ": " << target_name(target)
                  << ": Total: Compress: " << total_compressed_size << ": "
                  << (static_cast<double>(total_compressed_size) /
                      total_source_size * 100)
                  << "%: "
                  << (std::chrono::duration_cast<std::chrono::nanoseconds>(
                        total_compress_duration)
                        .count() /
                      static_cast<double>(std::nano::den))
                  << "s" << std::endl;
        std::cout << pattern.name << ": " << target_name(target)
                  << ": Total: Decompress: "
                  << (std::chrono::duration_cast<std::chrono::nanoseconds>(
                        total_decompress_duration)
                        .count() /
                      static_cast<double>(std::nano::den))
                  << "s" << std::endl;
      }
      blosc2_free_ctx(cctx);
    }
  }
  blosc2_destroy();
  return EXIT_SUCCESS;
}
