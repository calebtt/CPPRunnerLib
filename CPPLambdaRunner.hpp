#pragma once
#include <thread>
#include <mutex>
#include <functional>

namespace sds
{
	/// <summary>
	/// Convenience class, contains using typedefs for first two args of the
	///	user-supplied lambda function.
	/// </summary>
	struct LambdaArgs
	{
		using LambdaArg1 = std::atomic<bool>;
		using LambdaArg2 = std::mutex;
	};
	/// <summary>
	/// class for modifying data concurrently
	/// Instantiate with the type you would like to have mutex protected access to
	/// within a lambda on it's own thread.
	///	Template type "InternalData" must be default constructable!
	///	Instantiation requires a lambda of a certain form: function{void(atomic{bool}&, mutex&, InternalData&)}
	/// </summary>
	template <class InternalData>
	class CPPLambdaRunner
	{
	public:
		using LambdaType = std::function<void(std::atomic<bool>&, std::mutex&, InternalData&)>;
		using ScopedLockType = std::lock_guard<std::mutex>; //Interestingly, accessibility modifiers (public/private/etc.) work on "using" typedefs!
		CPPLambdaRunner(LambdaType lambdaToRun) : m_lambda(std::move(lambdaToRun)) { }
		CPPLambdaRunner(const CPPLambdaRunner& other) = delete;
		CPPLambdaRunner(CPPLambdaRunner&& other) = delete;
		CPPLambdaRunner& operator=(const CPPLambdaRunner& other) = delete;
		CPPLambdaRunner& operator=(CPPLambdaRunner&& other) = delete;
		~CPPLambdaRunner()
		{
			StopThread();
		}
	private:
		LambdaType m_lambda;
		InternalData m_local_state{}; // default constructed type InternalData
		std::atomic<bool> m_is_stop_requested = false;
		std::unique_ptr<std::thread> m_local_thread;
		std::mutex m_state_mutex;
	public:
		/// <summary>
		/// Starts running a new thread for the lambda.
		/// </summary>
		///	<returns>true on success, false on failure.</returns>
		bool StartThread()
		{
			if (m_local_thread != nullptr)
			{
				return false;
			}
			m_is_stop_requested = false;
			m_local_thread = std::make_unique<std::thread>(m_lambda, std::ref(m_is_stop_requested), std::ref(m_state_mutex), std::ref(m_local_state));
			return m_local_thread->joinable();
		}
		/// <summary>
		/// Non-blocking way to stop a running thread.
		/// </summary>
		void RequestStop()
		{
			if (this->m_local_thread != nullptr)
			{
				this->m_is_stop_requested = true;
			}
		}
		/// <summary>
		/// Blocking way to stop a running thread, joins to current thread and waits.
		/// </summary>
		void StopThread()
		{
			//Get this setting out of the way.
			this->m_is_stop_requested = true;
			//If there is a thread obj..
			if (this->m_local_thread != nullptr)
			{
				if (this->m_local_thread->joinable())
				{
					//join to wait for thread to stop, then reset to a nullptr.
					this->m_local_thread->join();
					this->m_local_thread.reset();
				}
				else
				{
					//if it is not joinable, set to nullptr
					this->m_local_thread.reset();
				}
			}
		}
		/// <summary>
		/// Utility function to update the InternalData with mutex locking thread safety.
		/// </summary>
		/// <param name="state">InternalData obj to be copied to the internal one.</param>
		void UpdateState(const InternalData& state)
		{
			ScopedLockType tempLock(m_state_mutex);
			m_local_state = state;
		}
		/// <summary>
		/// Returns a copy of the internal InternalData obj with mutex locking thread safety.
		/// </summary>
		InternalData GetCurrentState()
		{
			ScopedLockType tempLock(m_state_mutex);
			return m_local_state;
		}
	};
}