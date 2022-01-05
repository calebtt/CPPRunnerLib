#include <algorithm>
#include <iostream>
#include <vector>

#include "CPPLambdaRunner.hpp"

/* works in at least C++14 with VS2022 and surely C++20 and beyond. */
int main()
{
	using namespace std;
	//The lambda for use in the class CPPLambdaRunner
	//it has three arguments, an atomic<bool> stopCondition, a std::mutex, and the template type InternalData&
	auto MyThreadFunc = [](auto& stopCondition, auto& mut, auto& protectedData)
	{
		while (!stopCondition)
		{
			//lock the mutex to protect access, in this running thread.
			//the scoped lock will be destructed and release the mutex every iteration
			//of this while() loop, ensuring another operation can acquire a mutex lock
			//and perform some other operation on the data (such as getting a copy of it, below).
			sds::CPPLambdaRunner<std::vector<int>>::ScopedLockType tempLock(mut);
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
		while (!stopCondition)
		{
			sds::CPPLambdaRunner<std::vector<int>>::ScopedLockType tempLock(mut);
			protectedData.clear();
			for (int i = 0; i < 1000; i++)
			{
				protectedData.push_back(i);
			}
		}
	};
	//Instantiate with the type you want mutex protected access to, and lambda you wish to run.
	sds::CPPLambdaRunner<vector<int>> myRunner(MyThreadFunc);
	sds::CPPLambdaRunner<vector<int>> myRunner2(MyThreadFunc2);
	//now we can start running the lambda concurrently with this thread.
	cout << "Start thread returns: " << myRunner.StartThread() << endl;
	cout << "Start thread2 returns: " << myRunner2.StartThread() << endl;
	//make this thread wait for 1 second or so while the lambda is running in a background thread.
	std::this_thread::sleep_for(chrono::milliseconds(1000));
	//get a copy of the protected internal data with mutex protected access
	auto&& retVec = myRunner.GetCurrentState();
	auto&& retVec2 = myRunner2.GetCurrentState();
	//print to screen
	std::for_each(retVec.begin(), retVec.end(), [](auto elem) { std::cout << elem << " "; });
	std::for_each(retVec2.begin(), retVec2.end(), [](auto elem) { std::cout << elem << " "; });
	//pause and wait for [enter] before stopping the lambda thread and finally ending the main thread.
	cout << "ENTER to exit." << endl;
	cin.get();
	//myRunner.StopThread(); //CPPLambdaRunner destructor will also stop the thread.
}