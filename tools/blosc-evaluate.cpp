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

#include <chrono>
#include <iostream>
#include <type_traits>

#include <arrow/array.h>
#include <arrow/compute/api_vector.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>

#include <blosc2.h>
#include <blosc2/filters-registry.h>

namespace {
  enum class Target {
    Record,
    RecordSort,
    RecordBatch,
    RecordBatchSort,
  };

  std::string
  target_name(Target target)
  {
    switch (target) {
    case Target::Record:
      return "Record";
    case Target::RecordSort:
      return "RecordSort";
    case Target::RecordBatch:
      return "RecordBatch";
    case Target::RecordBatchSort:
      return "RecordBatchSort";
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
}

int
main(int argc, const char**argv)
{
  bool verbose = false;
  // verbose = true;
  auto input_path = argv[1];
  auto nth_column = std::stoi(argv[2]);
  blosc2_init();
  std::vector<Target> targets = {Target::Record,
                                 Target::RecordSort,
                                 Target::RecordBatch,
                                 Target::RecordBatchSort};
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
          for (const auto &record_batch : record_batches) {
            const auto &array = std::static_pointer_cast<arrow::ListArray>(
              record_batch->column(nth_column));
            for (int64_t i = 0; i < array->length(); ++i) {
              auto values = std::static_pointer_cast<arrow::FloatArray>(
                array->value_slice(i));
              if (target == Target::RecordSort) {
                auto sorted_values_indices =
                  *arrow::compute::SortIndices(*values);
                values = std::static_pointer_cast<arrow::FloatArray>(
                  *arrow::compute::Take(*values, *sorted_values_indices));
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
          for (const auto &record_batch : record_batches) {
            const auto &array = std::static_pointer_cast<arrow::ListArray>(
              record_batch->column(nth_column));
            auto values =
              std::static_pointer_cast<arrow::FloatArray>(array->values());
            if (target == Target::RecordBatchSort) {
              auto sorted_values_indices =
                *arrow::compute::SortIndices(*values);
              values = std::static_pointer_cast<arrow::FloatArray>(
                *arrow::compute::Take(*values, *sorted_values_indices));
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
