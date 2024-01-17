#ifndef __LOCK_QUEUE_H__
#define __LOCK_QUEUE_H__

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>

#include "opencv2/core.hpp"


template <class T>
class LockQueue {
protected:
	// Data
	std::queue<T> queue_;
	typename std::queue<T>::size_type size_max_;

	// Thread gubbins
	std::mutex mutex_;
	std::condition_variable full_;
	std::condition_variable empty_;

	// Exit
	std::atomic_bool quit_{false};
	std::atomic_bool finished_{false};

public:
	LockQueue(const size_t size_max);

	bool push(T &&data);
	bool pop(T &data);

	// The queue has finished accepting input
	void finished();
	// The queue will cannot be pushed or popped
	void quit();

	void reset();

	int q_size(){
		return queue_.size();
	}

};

using MatQueue = LockQueue<cv::Mat>;

template <class T>
LockQueue<T>::LockQueue(size_t size_max) :
		size_max_{size_max} {
}

template <class T>
bool LockQueue<T>::push(T &&data) {
	std::unique_lock<std::mutex> lock(mutex_);

	while (!quit_.load() && !finished_.load()) {

		if (queue_.size() < size_max_) {
			queue_.push(std::move(data));

			empty_.notify_one();
			return true;
		} else {
			full_.wait(lock);
			continue;
		}
	}

	return false;
}

template <class T>
bool LockQueue<T>::pop(T &data) {
	std::unique_lock<std::mutex> lock(mutex_);

	while (!quit_.load()) {

		if (!queue_.empty()) {
			data = std::move(queue_.front());
			queue_.pop();

			full_.notify_one();
			return true;
		} else if (queue_.empty() && finished_.load()) {
			return false;
		} else {
			empty_.wait(lock);
		}
	}

	return false;
}

template <class T>
void LockQueue<T>::finished() {
	std::unique_lock<std::mutex> lock(mutex_);
	finished_.store(true);
	empty_.notify_all();
}

template <class T>
void LockQueue<T>::quit() {
	std::unique_lock<std::mutex> lock(mutex_);
	quit_.store(true);
	empty_.notify_all();
	full_.notify_all();
}


template <class T>
void LockQueue<T>::reset() {
	std::unique_lock<std::mutex> lock(mutex_);
	quit_.store(false);
	finished_.store(false);
	empty_.notify_all();
	full_.notify_all();
}

#endif