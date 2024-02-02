/*
  Copyright (C) 2022-2024  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "grn.h"

#ifdef GRN_WITH_APACHE_ARROW
#  include "grn_arrow.hpp"
#  include <arrow/util/thread_pool.h>
#  include <mutex>
#  include <unordered_map>
#endif

namespace grn {
  class TaskExecutor {
  private:
    grn_ctx *ctx_;
    int32_t n_workers_;
#ifdef GRN_WITH_APACHE_ARROW
    std::shared_ptr<::arrow::internal::ThreadPool> thread_pool_;
    std::unordered_map<uintptr_t, ::arrow::Future<bool>> futures_;
    std::mutex futures_mutex_;
#endif

  public:
    TaskExecutor(grn_ctx *ctx, int32_t n_workers)
      : ctx_(ctx),
        n_workers_(n_workers)
#ifdef GRN_WITH_APACHE_ARROW
        ,
        thread_pool_(nullptr),
        futures_(),
        futures_mutex_()
#endif
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ < 0) {
        n_workers_ = ::arrow::internal::ThreadPool::DefaultCapacity();
      }
      if (n_workers_ > 1) {
        auto thread_pool_result =
          ::arrow::internal::ThreadPool::MakeEternal(n_workers_);
        if (thread_pool_result.ok()) {
          thread_pool_ = *thread_pool_result;
        } else {
          n_workers_ = 0;
        }
      }
#else
      n_workers_ = 0;
#endif
    }

    bool
    is_parallel()
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ > 1) {
        return true;
      }
#endif
      return false;
    }

    template <typename Function>
    bool
    execute(void *object, Function &&func, const char *tag)
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ > 1) {
        auto future_result = thread_pool_->Submit(func);
        if (!grnarrow::check(ctx_,
                             future_result,
                             tag,
                             " failed to submit a job")) {
          return false;
        }
        {
          std::unique_lock<std::mutex> lock(futures_mutex_);
          futures_.emplace(reinterpret_cast<uintptr_t>(object), *future_result);
        }
        return true;
      }
#endif
      return func();
    }

    bool
    wait(void *object, const char *tag)
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ > 1) {
        try {
          auto id = reinterpret_cast<uintptr_t>(object);
          std::unique_lock<std::mutex> lock(futures_mutex_);
          auto future = futures_.at(id);
          lock.unlock();
          auto status = future.status();
          lock.lock();
          futures_.erase(id);
          lock.unlock();
          return grnarrow::check(ctx_,
                                 status,
                                 tag,
                                 " failed to wait a job: ",
                                 id);
        } catch (std::out_of_range &) {
          return true;
        }
      }
#endif
      return true;
    }

    bool
    wait_all()
    {
#ifdef GRN_WITH_APACHE_ARROW
      if (n_workers_ > 1) {
        thread_pool_->WaitForIdle();
        return ctx_->rc == GRN_SUCCESS;
      }
#endif
      return true;
    }
  };
} // namespace grn
