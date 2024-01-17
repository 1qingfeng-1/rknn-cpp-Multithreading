#ifndef RKNNPOOL_H
#define RKNNPOOL_H

#include "ThreadPool.hpp"
#include <vector>
#include <iostream>
#include <mutex>
#include <queue>
#include <memory>

#include "lock_queue.h"

// rknnModel模型类, inputType模型输入类型, outputType模型输出类型
template <typename rknnModel, typename inputType, typename outputType>
class rknnPool
{
private:
    int _threadNum;
    std::string _modelPath;

    size_t _id;

    std::mutex idMtx, queueMtx;
    std::unique_ptr<dpool::ThreadPool> pool;
    std::unique_ptr<LockQueue<std::future<outputType>>> futs_ptr;
    std::vector<std::shared_ptr<rknnModel>> models;

protected:
    int getModelId();

public:
    rknnPool(const std::string &modelPath, int threadNum);

    int init();

    // 模型推理/Model inference
    int put(inputType inputData);

    // 获取推理结果/Get the results of your inference
    int get(outputType &outputData);

    ~rknnPool();
};

template <typename rknnModel, typename inputType, typename outputType>
rknnPool<rknnModel, inputType, outputType>::rknnPool(const std::string &modelPath, int threadNum):_modelPath(modelPath), _threadNum(threadNum){}

template <typename rknnModel, typename inputType, typename outputType>
int rknnPool<rknnModel, inputType, outputType>::init()
{
    try
    {
        pool = std::make_unique<dpool::ThreadPool>(_threadNum);
        for (int i = 0; i < _threadNum; i++){
            models.push_back(std::make_shared<rknnModel>(_modelPath.c_str()));
        }
            
    }
    catch (const std::bad_alloc &e)
    {
        std::cout << "Out of memory: " << e.what() << std::endl;
        return -1;
    }

    // 初始化模型/Initialize the model
    for (int i = 0, ret = 0; i < _threadNum; i++)
    {
        ret = models[i]->init(models[0]->get_pctx(), i != 0);
        if (ret != 0)
            return ret;
    }

    futs_ptr = std::make_unique<LockQueue<std::future<outputType>>>(30);
    futs_ptr.get()->reset();

    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
int rknnPool<rknnModel, inputType, outputType>::getModelId()
{
    std::lock_guard<std::mutex> lock(idMtx);
    int modelId = _id % _threadNum;
    _id++;
    return modelId;
}

template <typename rknnModel, typename inputType, typename outputType>
int rknnPool<rknnModel, inputType, outputType>::put(inputType inputData)
{
    auto feture_ret = pool->submit(&rknnModel::infer, models[getModelId()], inputData);
    futs_ptr->push(std::move(feture_ret));
    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
int rknnPool<rknnModel, inputType, outputType>::get(outputType &outputData)
{
    // std::lock_guard<std::mutex> lock(queueMtx);
    // if(futs.empty())
    //     return 1;
    // outputData = futs.front().get();
    // futs.pop();
    // return 0;
    std::future<outputType> future_output;
    futs_ptr->pop(future_output);
    outputData = std::move(future_output.get());
    return 0;
}

template <typename rknnModel, typename inputType, typename outputType>
rknnPool<rknnModel, inputType, outputType>::~rknnPool()
{
   futs_ptr->quit();
   futs_ptr->finished();
}

#endif