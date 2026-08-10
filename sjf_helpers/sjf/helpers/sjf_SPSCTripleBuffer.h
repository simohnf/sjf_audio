/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 07/08/2026.
//

#pragma once
#include <JuceHeader.h>


namespace sjf::helpers
{
	/**
	 * A simple single producer, single consumer triple buffer.
	 * Usage
	 *	Thread 1:
	 *			auto data = tripleBuffer.getWrite();
	 *			doSomethingWithData(data);
	 *			tripleBuffer.updateWriteIndex();
	 *
	 *			(alternatively Thread 1 can call tripleBuffer.editData() )
	 *
	 *	Thread 2:
	 *			auto data = tripleBuffer.getRead();
	 *
	 * @tparam T the object to be shared between threads
	 * @tparam CacheLineSize set to ensure there is no false sharing
	 *
	 */
	template <typename T, size_t CacheLineSize = 256>
	class SPSCTripleBuffer
	{
		public:
			T& getWrite()
			{
				return buffers[writeIndex].data;
			}

			void updateWriteIndex()
			{
				const auto next = (writeIndex << 1) | 1; // set index, shift, set flag
				writeIndex = nextIndex.exchange(next) >> 1; // shift back again
			}

			// callable should accept T& as argument
			template<typename F>
			void editData(F&& f)
			{
				std::forward<F>(f)(getWrite());
				updateWriteIndex();
			}

			const T& getRead()
			{

				if (nextIndex.load() & 1)
				{
					const auto next = (readIndex << 1); // we only shift, that way flag is automatically 0
					readIndex = nextIndex.exchange(next) >> 1;
				}
				return buffers[readIndex].data;
			}
		private:
			// Force each buffer slot to occupy its own isolated cache line
			struct alignas(CacheLineSize) PaddedBuffer
			{
				T data;
			};

			std::array<PaddedBuffer, 3> buffers{};
			alignas(CacheLineSize) size_t readIndex{0};
			alignas(CacheLineSize) size_t writeIndex{2};
			alignas(CacheLineSize) std::atomic<size_t> nextIndex{(1 << 1)};
	};
}



//DUMMY_PLUGIN_SJF_SPSCTRIPLEBUFFER_H
