#pragma once

#include <mutex>
#include <shared_mutex>

namespace Ignis
{
using Mutex       = std::mutex;
using SharedMutex = std::shared_mutex;

template <typename T>
using UniqueLock = std::unique_lock<T>;
template <typename T>
using SharedLock = std::shared_lock<T>;
template <typename T>
using LockGuard = std::lock_guard<T>;
} // namespace Ignis
