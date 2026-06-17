#pragma once

#include <future>
#include <mutex>
#include <shared_mutex>

namespace Ignis
{
using Mutex        = std::mutex;
using SharedMutex  = std::shared_mutex;
using FutureStatus = std::future_status;

template <typename T>
using UniqueLock = std::unique_lock<T>;
template <typename T>
using SharedLock = std::shared_lock<T>;
template <typename T>
using LockGuard = std::lock_guard<T>;
template <typename T>
using Future = std::future<T>;
} // namespace Ignis
