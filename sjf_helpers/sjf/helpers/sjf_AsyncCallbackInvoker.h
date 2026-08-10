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



//DUMMY_PLUGIN_SJF_ASYNCCALLBACKINVOKER_H
