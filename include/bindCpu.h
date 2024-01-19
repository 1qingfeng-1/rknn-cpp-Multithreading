//
// Created by 13736 on 2022/8/9.
//

#ifndef TESTQYTRACK_BINDCPU_H
#define TESTQYTRACK_BINDCPU_H

#include <limits.h>
#include <stdio.h>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#endif
#include <stdint.h>
#include <string.h>




int get_cpucount();

int get_max_freq_khz(int cpuid);

void bindBigCore();

void bindLittleCore();


#endif //TESTQYTRACK_BINDCPU_H
