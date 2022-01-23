#include <algorithm>
#include <iostream>
#include <vector>
#include "CPPRunnerGeneric.hpp"

class ItThrows
{
	int elem = 0;
public:
	ItThrows()
	{
		if (1 == 0) throw std::exception();
	}
	bool operator()() const
	{
		return true;
	}
};

/* works in at least C++20 with VS2022 and surely beyond. */
int main()
{
	using namespace std;
	using RunnerType = sds::CPPRunnerGeneric<int>;
	using RunnerVectorType = sds::CPPRunnerGeneric<std::vector<int>>;
	using RunnerVectorThrowType = sds::CPPRunnerGeneric<std::vector<ItThrows>>;
	//The lambda for use in the class CPPLambdaRunner
	//it has three arguments, an atomic<bool> stopCondition, a std::mutex, and the template type InternalData&
	auto MyThreadFunc = [](auto& stopCondition, auto& mut, auto& protectedData)
	{
		using LockType = RunnerVectorType::ScopedLockType;
		while (!stopCondition)
		{
			//lock the mutex to protect access, in this running thread.
			//the scoped lock will be destructed and release the mutex every iteration
			//of this while() loop, ensuring another operation can acquire a mutex lock
			//and perform some other operation on the data (such as getting a copy of it, below).
			LockType tempLock(mut);
			//perform operations on the data in this "main loop"
			protectedData.clear();
			for (int i = 0; i < 1000; i++)
			{
				protectedData.push_back(i);
			}
		}
	};
	//Another way
	auto MyThreadFunc2 = [](sds::LambdaArgs::LambdaArg1& stopCondition, sds::LambdaArgs::LambdaArg2& mut, std::vector<int>& protectedData)
	{
		using LockType = RunnerVectorType::ScopedLockType;
		while (!stopCondition)
		{
			LockType tempLock(mut);
			protectedData.clear();
			for (int i = 0; i < 1000; i++)
			{
				protectedData.push_back(i);
			}
		}
	};
	//And a lambda for a single int value
	auto MyThreadFunc3 = [](auto& stopCondition, auto& mut, auto& protectedData)
	{
		using LockType = RunnerType::ScopedLockType;
		using mils = std::chrono::milliseconds;
		while (!stopCondition)
		{
			LockType l1(mut);
			for (int i = 0; i < 1000; i++)
				protectedData = i;
			std::this_thread::sleep_for(mils(10));
		}
	};
	auto MyThreadFunc4 = [](auto& stopCondition, auto& mut, auto& protectedData) { while (!stopCondition) std::this_thread::sleep_for(std::chrono::milliseconds(10)); };

	//Instantiate with the type you want mutex protected access to, and lambda you wish to run.
	RunnerVectorType myRunner(MyThreadFunc);
	RunnerVectorType myRunner2(MyThreadFunc2);
	RunnerType myRunner3(MyThreadFunc3);
	RunnerVectorThrowType myRunner4(MyThreadFunc4);

	//now we can start running the lambda concurrently with this thread.
	cout << "Start thread returns: " << myRunner.StartThread() << endl;
	cout << "Start thread2 returns: " << myRunner2.StartThread() << endl;
	cout << "Start thread3 returns: " << myRunner3.StartThread() << endl;
	cout << "Start thread4 returns: " << myRunner4.StartThread() << endl;
	//make this thread wait for 1 second or so while the lambda is running in a background thread.
	cout << "Waiting 1 second..." << endl;
	std::this_thread::sleep_for(chrono::milliseconds(1000));
	myRunner4.AddState(ItThrows());
	//get a copy of the protected internal data with mutex protected access
	auto retVec = myRunner.GetAndClearCurrentStates();
	auto retVec2 = myRunner2.GetCurrentState();
	auto retVal3 = myRunner3.GetCurrentState();
	auto retVec4 = myRunner4.GetAndClearCurrentStates();
	//print to screen
	cout << "Thread 1 last element: " << retVec.back() << endl;
	cout << "Thread 2 last element: " << retVec2.back() << endl;
	cout << "Thread 3 value: " << retVal3 << endl;
	cout << "Thread 4 last element: " << retVec4.back()() << endl;

	//std::for_each(retVec.begin(), retVec.end(), [](auto elem) { std::cout << elem << " "; });
	//std::for_each(retVec2.begin(), retVec2.end(), [](auto elem) { std::cout << elem << " "; });
	//pause and wait for [enter] before stopping the lambda thread and finally ending the main thread.
	cout << "ENTER to exit." << endl;
	cin.get();
	//myRunner.StopThread(); //destructor will also stop the thread.
}