#ifndef _CellTaskServer_hpp_
#define _CellTaskServer_hpp_

#include<thread>
#include<mutex>
#include<list>

//��������-����
class CellTask
{
public:
	CellTask()
	{

	}

	//������
	virtual ~CellTask()
	{

	}
	//ִ������
	virtual void doTask()
	{

	}
private:

};



//ִ������ķ�������
class CellTaskServer
{
private:
	//��������
	std::list<CellTask*> _tasks;
	//�������ݻ�����
	std::list<CellTask*> _tasksBuf;
	//�ı����ݻ�����ʱ��Ҫ����
	std::mutex _mutex;
public:
	//��������
	void addTask(CellTask* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}

	//��������
	void OnRun()
	{
		while (true)
		{
			//�ӻ�����ȡ������
			if (!_tasksBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf)
				{
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}
			//���û������
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//��������
			for (auto pTask : _tasks)
			{
				pTask->doTask();
				delete pTask;
			}
			//�������
			_tasks.clear();
		}

	}
protected:
	
};
#endif // !_CELL_TASK_H_