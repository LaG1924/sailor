#pragma once
#include <cstdio>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include "Sailor.h"
#include "Core/Submodule.h"
#include "Memory/UniquePtr.hpp"
#include "Tasks/Tasks.h"

// TODO: Implement ConcurrentList
#include <concurrent_queue.h>

#define SAILOR_ENQUEUE_TASK(Name, Lambda) Sailor::App::GetSubmodule<Tasks::Scheduler>()->Run(Sailor::Tasks::CreateTask(Name, Lambda))
#define SAILOR_ENQUEUE_TASK_RENDER_THREAD(Name, Lambda) Sailor::App::GetSubmodule<Tasks::Scheduler>()->Run(Sailor::Tasks::CreateTask(Name, Lambda, Sailor::Tasks::EThreadType::Render))
#define SAILOR_ENQUEUE_TASK_RHI_THREAD(Name, Lambda) Sailor::App::GetSubmodule<Tasks::Scheduler>()->Run(Sailor::Tasks::CreateTask(Name, Lambda, Sailor::Tasks::EThreadType::RHI))

namespace Sailor
{
	namespace Tasks
	{
		class WorkerThread;
		class Scheduler;

		class WorkerThread
		{
		public:

			SAILOR_API WorkerThread(
				std::string threadName,
				EThreadType threadType,
				std::condition_variable& refresh,
				std::mutex& mutex,
				TVector<ITaskPtr>& pJobsQueue);

			SAILOR_API virtual ~WorkerThread() = default;

			SAILOR_API WorkerThread(WorkerThread&& move) = delete;
			SAILOR_API WorkerThread(WorkerThread& copy) = delete;
			SAILOR_API WorkerThread& operator =(WorkerThread& rhs) = delete;

			SAILOR_API DWORD GetThreadId() const { return m_threadId; }
			SAILOR_API EThreadType GetThreadType() const { return m_threadType; }

			SAILOR_API void ForcelyPushJob(const ITaskPtr& pJob);

			SAILOR_API void Process();
			SAILOR_API void Join();
			SAILOR_API void WaitIdle();

		protected:

			SAILOR_API bool TryFetchJob(ITaskPtr& pOutJob);

			std::string m_threadName;
			TUniquePtr<std::thread> m_pThread;

			EThreadType m_threadType;
			DWORD m_threadId;

			std::atomic<bool> m_bIsBusy;

			// Specific jobs for this thread
			std::mutex m_queueMutex;
			TVector<ITaskPtr> m_pJobsQueue;

			// Assigned from scheduler
			std::condition_variable& m_refresh;
			std::mutex& m_commonQueueMutex;
			TVector<ITaskPtr>& m_pCommonJobsQueue;
		};

		class Scheduler final : public TSubmodule<Scheduler>
		{
			const uint8_t RHIThreadsNum = 2u;
			const size_t MaxTasksInPool = 16384;

		public:

			SAILOR_API void Initialize();

			SAILOR_API virtual ~Scheduler() override;

			// Lock thit thread until all jobs on thread type would be finished
			SAILOR_API void WaitIdle(EThreadType type);

			SAILOR_API uint32_t GetNumWorkerThreads() const;
			SAILOR_API uint32_t GetNumRenderingJobs() const;
			SAILOR_API uint32_t GetNumRHIThreads() const { return RHIThreadsNum; }

			SAILOR_API void Run(const ITaskPtr& pJob, bool bAutoRunChainedTasks = true);
			SAILOR_API void Run(const ITaskPtr& pJob, DWORD threadId, bool bAutoRunChainedTasks = true);
			SAILOR_API void ProcessJobsOnMainThread();

			SAILOR_API bool TryFetchNextAvailiableJob(ITaskPtr& pOutJob, EThreadType threadType);

			SAILOR_API void NotifyWorkerThread(EThreadType threadType, bool bNotifyAllThreads = false);

			SAILOR_API bool IsMainThread() const;
			SAILOR_API bool IsRendererThread() const;

			SAILOR_API DWORD GetMainThreadId() const { return m_mainThreadId; }
			SAILOR_API DWORD GetRendererThreadId() const;

			SAILOR_API Scheduler() = default;

			SAILOR_API void RunChainedTasks(const ITaskPtr& pJob);

			SAILOR_API TaskSyncBlock& GetTaskSyncBlock(const ITask& task) { return m_tasksPool[task.m_taskSyncBlockHandle]; }
			SAILOR_API uint16_t AcquireTaskSyncBlock();
			SAILOR_API void ReleaseTaskSyncBlock(const ITask& task);

		protected:

			SAILOR_API void RunChainedTasks_Internal(const ITaskPtr& pJob, const ITaskPtr& pJobToIgnore);

			SAILOR_API void GetThreadSyncVarsByThreadType(
				EThreadType threadType,
				std::mutex*& pOutMutex,
				TVector<ITaskPtr>*& pOutQueue,
				std::condition_variable*& pOutCondVar);

			std::mutex m_queueMutex[4];
			std::condition_variable m_refreshCondVar[4];
			TVector<ITaskPtr> m_pCommonJobsQueue[4];

			std::atomic<uint32_t> m_numBusyThreads;
			TVector<WorkerThread*> m_workerThreads;
			std::atomic_bool m_bIsTerminating;

			DWORD m_mainThreadId = -1;
			DWORD m_renderingThreadId = -1;

			// Task Synchronization primitives pool
			concurrency::concurrent_queue<uint16_t> m_freeList{};
			TVector<TaskSyncBlock> m_tasksPool{};

			friend class WorkerThread;

			template<typename TResult, typename TArgs>
			friend TaskPtr<TResult, TArgs> CreateTask(const std::string& name, typename TFunction<TResult, TArgs>::type lambda, EThreadType thread);
		};
	}
}