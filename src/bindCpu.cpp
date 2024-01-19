//
// Created by 13736 on 2022/8/9.
//
#include "bindCpu.h"


int g_cpucount;
int max_freq_khz_medium = 0;
std::vector<int> cpu_max_freq_khz;
std::vector<int> cpu_idx;

static int set_sched_affinity(size_t thread_affinity_mask)
{
#ifdef __linux__

    #ifdef __GLIBC__
        pid_t pid = syscall(SYS_gettid);
    #else
    #ifdef PI3
        pid_t pid = getpid();
    #else
        pid_t pid = gettid();
    #endif
    #endif
        int syscallret = syscall(__NR_sched_setaffinity, pid, sizeof(thread_affinity_mask), &thread_affinity_mask);
        if (syscallret)
        {
            fprintf(stderr, "syscall error %d\n", syscallret);
            return -1;
        }
#endif    
    return 0;
}
// refer to: https://github.com/Tencent/ncnn/blob/ee41ef4a378ef662d24f137d97f7f6a57a5b0eba/src/cpu.cpp
int get_cpucount()
{
    int count = 0;
    // get cpu count from /proc/cpuinfo
    FILE* fp = fopen("/proc/cpuinfo", "rb");
    if (!fp)
        return 1;

    char line[1024];
    while (!feof(fp))
    {
        char* s = fgets(line, 1024, fp);
        if (!s)
            break;

        if (memcmp(line, "processor", 9) == 0)
        {
            count++;
        }
    }

    fclose(fp);


    if (count < 1)
        count = 1;

    if (count > (int)sizeof(size_t) * 8)
    {
        fprintf(stderr, "more than %d cpu detected, thread affinity may not work properly :(\n", (int)sizeof(size_t) * 8);
    }

    return count;
}
// refer to: https://github.com/Tencent/ncnn/blob/ee41ef4a378ef662d24f137d97f7f6a57a5b0eba/src/cpu.cpp

int get_max_freq_khz(int cpuid)
{
    // first try, for all possible cpu
    char path[256];
    sprintf(path, "/sys/devices/system/cpu/cpufreq/stats/cpu%d/time_in_state", cpuid);
    FILE* fp = fopen(path, "rb");
    if (!fp)
    {
        // second try, for online cpu
        sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/stats/time_in_state", cpuid);
        fp = fopen(path, "rb");
        if (fp)
        {
            int max_freq_khz = 0;
            while (!feof(fp))
            {
                int freq_khz = 0;
                int nscan = fscanf(fp, "%d %*d", &freq_khz);
                if (nscan != 1)
                    break;

                if (freq_khz > max_freq_khz)
                    max_freq_khz = freq_khz;
            }
            fclose(fp);
            if (max_freq_khz != 0)
                return max_freq_khz;
            fp = NULL;
        }

        if (!fp)
        {
            // third try, for online cpu
            sprintf(path, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", cpuid);
            fp = fopen(path, "rb");

            if (!fp)
                return -1;

            int max_freq_khz = -1;
            fscanf(fp, "%d", &max_freq_khz);

            fclose(fp);

            return max_freq_khz;
        }
    }

    int max_freq_khz = 0;
    while (!feof(fp))
    {
        int freq_khz = 0;
        int nscan = fscanf(fp, "%d %*d", &freq_khz);
        if (nscan != 1)
            break;

        if (freq_khz > max_freq_khz)
            max_freq_khz = freq_khz;
    }
    fclose(fp);
    return max_freq_khz;
}

static void swapSort(std::vector<int> &arr, std::vector<int> &idx, bool reverse = true) {
    if (reverse) {
        for (int i = 0; i < arr.size() - 1; ++i) {
            int maxVal = arr[i];
            int maxIdx = i;
            for (int j = i + 1; j < arr.size(); ++j) {
                if (arr[j] > maxVal) {
                    maxVal = arr[j];
                    maxIdx = j;
                }
            }
            // swap val
            int tmp = arr[maxIdx];
            arr[maxIdx] = arr[i];
            arr[i] = tmp;
            // swap idx
            tmp = idx[maxIdx];
            idx[maxIdx] = idx[i];
            idx[i] = tmp;
        }
    } else {
        for (int i = 0; i < arr.size() - 1; ++i) {
            int minVal = arr[i];
            int minIdx = i;
            for (int j = i + 1; j < arr.size(); ++j) {
                if (arr[j] < minVal) {
                    minVal = arr[j];
                    minIdx = j;
                }
            }
            // swap val
            int tmp = arr[minIdx];
            arr[minIdx] = arr[i];
            arr[i] = tmp;
            // swap idx
            tmp = idx[minIdx];
            idx[minIdx] = idx[i];
            idx[i] = tmp;
        }
    }
}

static void _cpuClsInit(){
    g_cpucount = get_cpucount();

    cpu_max_freq_khz = std::vector<int>(g_cpucount);
    cpu_idx = std::vector<int>(g_cpucount);

    for (int i=0; i<g_cpucount; i++)
    {
        int max_freq_khz = get_max_freq_khz(i);
        cpu_max_freq_khz[i] = max_freq_khz;
        cpu_idx[i] = i;
    }

    swapSort(cpu_max_freq_khz, cpu_idx);

    // distinguish big & little cores with ncnn strategy
    int max_freq_khz_min = INT_MAX;
    int max_freq_khz_max = 0;
    for (int i = 0; i < g_cpucount; i++) {
        if (cpu_max_freq_khz[i] > max_freq_khz_max)
            max_freq_khz_max = cpu_max_freq_khz[i];
        if (cpu_max_freq_khz[i] < max_freq_khz_min)
            max_freq_khz_min = cpu_max_freq_khz[i];
    }
    max_freq_khz_medium = (max_freq_khz_min + max_freq_khz_max) / 2;

}

void bindBigCore(){
    _cpuClsInit();
    size_t mask = 0;
    if(g_cpucount >4){
        g_cpucount = 4;
    }
    for (int i = 0; i < g_cpucount; ++i) {
        if (cpu_max_freq_khz[i] >= max_freq_khz_medium) {
            mask |= (1 << cpu_idx[i]);
        }
    }
    int ret = set_sched_affinity(mask);
}

void bindLittleCore(){
    _cpuClsInit();
    size_t mask = 0;
    for (int i = 0; i < g_cpucount; ++i) {
        if (cpu_max_freq_khz[i] < max_freq_khz_medium) {
            mask |= (1 << cpu_idx[i]);
        }
    }
    int ret = set_sched_affinity(mask);
}