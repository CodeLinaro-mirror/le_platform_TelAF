/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ASYNC_CALLBACK_UTILS_HPP
#define ASYNC_CALLBACK_UTILS_HPP

#include <future>
#include <memory>

/**
 * @brief Template class to encapsulate asynchronous callback context.
 *
 * This class holds metadata of type Meta and a shared pointer to a promise object.
 * The promise is used to communicate the result of an asynchronous operation.
 *
 * @tparam Meta Type of metadata associated with the asynchronous operation.
 */
template<typename Meta>
class AsyncContext
{
public:
    ///< Metadata associated with the async operation.
    Meta metadata;

    ///< Shared promise for result signaling.
    std::shared_ptr<std::promise<le_result_t>> promisePtr;

    /**
     * @brief Constructor initializes metadata and creates a new promise.
     *
     * @param meta Metadata to associate with this context.
     */
    explicit AsyncContext(const Meta& meta) : metadata(meta)
    {
        promisePtr = std::make_shared<std::promise<le_result_t>>();
    }

    /**
     * @brief Retrieve the future associated with the promise.
     *
     * This allows consumers to wait for or poll the result of the asynchronous operation.
     *
     * @return std::future<le_result_t> Future object linked to the promise.
     */
    std::future<le_result_t> GetFuture()
    {
        return promisePtr->get_future();
    }
};

/**
 * @brief Utility to wrap a user callback with promise fulfillment and error handling.
 *
 * This function generates a lambda that, when invoked, executes the user callback,
 * sets the result on the associated promise, and handles exceptions robustly.
 *
 * @tparam Meta Type of metadata in the context.
 * @tparam Args Argument types for the callback.
 * @tparam UserCallback Type of the user callback function.
 * @param context Shared pointer to AsyncContext holding metadata and promise.
 * @param userCb User-provided callback to execute.
 * @return std::function<void(Args...)> Lambda function to be used as a callback.
 */
template <typename Meta, typename... Args, typename UserCallback>
auto MakeCallbackWrapper(std::shared_ptr<AsyncContext<Meta>> context, UserCallback userCb)
   -> std::function<void(Args...)>
{
    // Lambda captures context and user callback by value.
    auto lambda = [context, userCb](Args... args)
    {
        LE_FATAL_IF(context == nullptr, "Callback context is NULL");
        LE_FATAL_IF(context->promisePtr == nullptr, "Callback context promise is NULL");

        try
        {
            // Execute the user callback with forwarded arguments.
            le_result_t result = userCb(std::forward<Args>(args)...);

            try
            {
                context->promisePtr->set_value(result);
                LE_INFO("Promise set for context: [%p]", context.get());
            }
            catch (const std::future_error& fe)
            {
                LE_ERROR("Promise already satisfied for context [%p]: %s",
                    context.get(), fe.what());
            }
        }
        catch (const std::exception& e)
        {
            LE_ERROR("Exception in callback for context [%p]: %s",
                context.get(), e.what());
            try
            {
                context->promisePtr->set_value(LE_FAULT);
            }
            catch (const std::future_error& fe)
            {
                LE_ERROR("Promise already satisfied during error for context [%p]: %s",
                    context.get(), fe.what());
            }
        }
        catch (...)
        {
            LE_ERROR("Unknown exception in callback for context %p", context.get());
            try
            {
                context->promisePtr->set_value(LE_FAULT);
            }
            catch (const std::future_error& fe)
            {
                LE_ERROR("Promise already satisfied during unknown error for context [%p]: %s",
                    context.get(), fe.what());
            }
        }
    };
    return lambda;
}

#endif // ASYNC_CALLBACK_UTILS_HPP
