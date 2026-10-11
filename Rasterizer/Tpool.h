#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(int n) { resize(n); }
    ~ThreadPool() { stop(); }

    void resize(int n) {
        stop();
        if (n < 1) n = 1;
        stopping = false;
        unfinished = 0;

        for (int i = 0; i < n; ++i) {
            workers.emplace_back([this] { workerLoop(); });
        }
    }

    int size() const { return (int)workers.size(); }

    void submit(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lk(mtx);
            tasks.push(std::move(job));
            ++unfinished;
        }
        cv.notify_one();
    }

    // 等到当前所有 submit 的任务都完成
    void wait() {
        std::unique_lock<std::mutex> lk(doneMtx);
        doneCv.wait(lk, [this] { return unfinished.load() == 0; });
    }

private:
    void workerLoop() {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lk(mtx);
                cv.wait(lk, [this] { return stopping || !tasks.empty(); });
                if (stopping && tasks.empty()) return;
                job = std::move(tasks.front());
                tasks.pop();
            }

            job();

            if (--unfinished == 0) {
                std::lock_guard<std::mutex> lk(doneMtx);
                doneCv.notify_all();
            }
        }
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(mtx);
            stopping = true;
        }
        cv.notify_all();
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
        workers.clear();

        // 清队列（防止 resize 时残留）
        while (!tasks.empty()) tasks.pop();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex mtx;
    std::condition_variable cv;

    std::atomic<bool> stopping{ false };

    std::atomic<int> unfinished{ 0 };
    std::mutex doneMtx;
    std::condition_variable doneCv;
};
