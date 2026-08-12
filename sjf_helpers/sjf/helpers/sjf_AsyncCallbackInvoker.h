/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 08/08/2026.
//

#pragma once

#include <JuceHeader.h>
namespace sjf::helpers
{
	/**
	 * @brief Base class for thread-safe, rate-limited asynchronous callback invokers.
	 *
	 * Provides the shared timing infrastructure and lifetime safety mechanics for derived
	 * callback invokers (such as `AsyncCallbackInvoker`). It manages a global 60 Hz
	 * background timer instance via `juce::SharedResourcePointer` that safely polls
	 * registered invokers for pending updates.
	 *
	 * Lifetime protection is guaranteed across thread boundaries using an internal
	 * `std::weak_ptr` guard pattern, ensuring that pending callbacks are automatically
	 * pruned if the target invoker instance is destroyed before execution.
	 *
	 * @note This class is an implementation detail intended strictly as an abstract base
	 *       for `AsyncCallbackInvoker`. It should not be instantiated directly.
	 */
	class AsyncCallbackInvokerBase
	{
	public:

	protected:
		~AsyncCallbackInvokerBase() = default;

		virtual void invoke() = 0;

		class Timer : public juce::Timer
		{
		public:
			Timer()
			{
				startTimerHz(60);
			}

			~Timer() override
			{
				stopTimer();
			}

			void addCallbackHandler(AsyncCallbackInvokerBase* handler)
			{
				jassert(handler);
				if (handler)
					callbacks.push_back({handler, std::weak_ptr(handler->guard)});
			}

			void timerCallback() override
			{
				callbacks.erase(
							std::remove_if(callbacks.begin(), callbacks.end(),
										   [](const auto& cbAndG) { return std::get<1>(cbAndG).expired(); }),
							callbacks.end());

				for ( auto& cbAndGuard : callbacks)
				{
					const auto& invoker = std::get<0>(cbAndGuard);
					const auto& guard_ = std::get<1>(cbAndGuard);
					if (!guard_.expired())
					{
						invoker->invoke();
					}
				}

			}

		private:
			using CallbackAndGuard = std::tuple<AsyncCallbackInvokerBase*, std::weak_ptr<int>>;
			std::vector<CallbackAndGuard> callbacks;
		};

		std::shared_ptr<int> guard = std::make_shared<int>(42);

		SharedResourcePointer<Timer> timer;

		friend class AsyncCallbackInvokerBase::Timer;
	};

	/**
	 * @brief A lock-free, rate-limited asynchronous callback invoker designed for high-frequency state updates.
	 *
	 * Useful for coalescing rapid asynchronous state change events (such as lock-free triple buffer
	 * updates or `juce::ValueTree` modifications) originating on real-time threads into a single
	 * rate-limited execution on the message thread.
	 *
	 * Calls to `triggerUpdate()` safely flag an internal atomic boolean without locking or blocking.
	 * The underlying callback is then executed on the message thread at up to 60 Hz via the shared
	 * timer infrastructure. If multiple updates occur within a single timer tick, they are coalesced
	 * into a single callback invocation.
	 *
	 * Lifetime protection is fully automated; callbacks scheduled asynchronously via `juce::MessageManager`
	 * or the timer queue are safely ignored if the invoker instance goes out of scope.
	 *
	 * @tparam Callback Functional callable type (e.g., lambda or `std::function<void()>`) to invoke on updates.
	 */
	template<typename Callback>
	class AsyncCallbackInvoker : public AsyncCallbackInvokerBase
	{
	public:
		explicit AsyncCallbackInvoker(Callback&& c)
		: callback(std::forward<Callback>(c))
		{
			if (auto mm = MessageManager::getInstanceWithoutCreating())
			{
				mm->callAsync([this, guard_ = std::weak_ptr(guard)](){
				  if (!guard_.expired())
				  	timer->addCallbackHandler(this);
				 });
			}
		}

		virtual ~AsyncCallbackInvoker() = default;

		void triggerUpdate()
		{
			sendUpdate.store(true);
		}
	private:
		void invoke() override
		{
			if (sendUpdate.exchange(false))
				callback();
		}

		std::atomic<bool> sendUpdate{false};
		Callback callback;
	};
}


