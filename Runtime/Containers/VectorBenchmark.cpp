#include "Memory.h"
#include <cstdlib>
#include <vector>
#include <cassert>
#include <cctype>
#include "Core/Utils.h"
#include "Vector.h"
#include "Memory/Memory.h"
#include "Tasks/Tasks.h"
#include "Tasks/Scheduler.h"

using namespace Sailor;
using namespace Sailor::Memory;
using Timer = Utils::Timer;

class TestCase_VectorPerfromance
{
	struct TData
	{
		TData(uint32_t value)
		{
			m_value = value;
			m_data = new size_t[8];
			memset(m_data, value, 8 * sizeof(size_t));
			numInstances++;
		}

		TData(const TData& rhs)
		{
			m_data = new size_t[8];
			m_value = rhs.m_value;

			memcpy(m_data, rhs.m_data, 8 * sizeof(size_t));
			numInstances++;
		}

		~TData()
		{
			delete[] m_data;
			numInstances--;
		}

		bool operator==(const TData& rhs) const
		{
			return m_value == rhs.m_value && memcmp(m_data, rhs.m_data, 8 * sizeof(size_t)) == 0;
		}

		TData& operator=(const TData& rhs)
		{
			m_value = rhs.m_value;
			memcpy(m_data, rhs.m_data, 8 * sizeof(size_t));
			return *this;
		}

		size_t* m_data = nullptr;
		uint32_t m_value;
	};

public:

	static void RunTests()
	{
		RunSanityTests();
	}

	static void RunSanityTests()
	{
		printf("%s\n", "class Sailor::TVector");
		printf("Sanity check passed: %d\n", SanityCheck());
		PerformanceTests();
		printf("\n");
	}

	static void PerformanceTests()
	{
		Timer stdVector;
		Timer tVector;

		const size_t count = 163600;

		TVector<TData> container;
		std::vector<TData> ideal;

		srand(0);
		tVector.Start();
		for (size_t i = 0; i < count; i++)
		{
			const int32_t value = rand();

			if (i % 2 == 0)
			{
				container.Add(value);
			}
			else
			{
				container.Emplace(value);
			}
		}
		tVector.Stop();

		srand(0);
		stdVector.Start();
		for (size_t i = 0; i < count; i++)
		{
			const int32_t value = rand();

			if (i % 2 == 0)
			{
				ideal.push_back(value);
			}
			else
			{
				ideal.emplace_back(value);
			}
		}
		stdVector.Stop();

		SAILOR_LOG("Performance test add/emplace:\n\tstd::vector %llums\n\tTVector %llums", stdVector.ResultMs(), tVector.ResultMs());

		Timer stdVectorRemoveSwap;
		Timer tVectorRemoveSwap;

		uint32_t countToRemoveSwap = count / 2;

		srand(0);
		tVectorRemoveSwap.Start();
		for (size_t i = 0; i < countToRemoveSwap; i++)
		{
			const size_t pos = rand() % container.Num();
			const auto valueToRemove = container[pos];

			container.RemoveAtSwap(pos);
		}
		tVectorRemoveSwap.Stop();

		srand(0);
		stdVectorRemoveSwap.Start();
		for (size_t i = 0; i < countToRemoveSwap; i++)
		{
			const size_t pos = rand() % ideal.size();
			const auto valueToRemove = ideal[pos];

			std::iter_swap(ideal.begin() + pos, ideal.end() - 1);
			ideal.pop_back();
		}
		stdVectorRemoveSwap.Stop();

		SAILOR_LOG("Performance test RemoveAtSwap:\n\tstd::vector %llums\n\tTVector %llums", stdVectorRemoveSwap.ResultMs(), tVectorRemoveSwap.ResultMs());

		Timer stdVectorRemoveAt;
		Timer tVectorRemoveAt;

		srand(0);
		stdVectorRemoveAt.Start();
		uint32_t countToDelete = count / 128;
		for (size_t i = 0; i < countToDelete; i++)
		{
			const int32_t value = rand() % ideal.size();
			ideal.erase(ideal.begin() + value);
		}
		stdVectorRemoveAt.Stop();

		srand(0);
		tVectorRemoveAt.Start();
		for (size_t i = 0; i < countToDelete; i++)
		{
			const int32_t value = rand() % container.Num();
			container.RemoveAt(value);
		}
		tVectorRemoveAt.Stop();

		SAILOR_LOG("Performance test RemoveAt:\n\tstd::vector %llums\n\tTVector %llums", stdVectorRemoveAt.ResultMs(), tVectorRemoveAt.ResultMs());

		Timer stdVectorRemoveFirst;
		Timer tVectorRemoveFirst;

		uint32_t countToRemove = count / 128;

		srand(0);
		stdVectorRemoveFirst.Start();
		for (size_t i = 0; i < countToRemove; i++)
		{
			const size_t pos = rand() % ideal.size();
			const auto valueToRemove = ideal[pos];

			ideal.erase(std::find(ideal.begin(), ideal.end(), valueToRemove));
		}
		stdVectorRemoveFirst.Stop();

		srand(0);
		tVectorRemoveFirst.Start();
		for (size_t i = 0; i < countToRemove; i++)
		{
			const size_t pos = rand() % container.Num();
			const auto valueToRemove = container[pos];

			container.RemoveFirst(valueToRemove);
		}
		tVectorRemoveFirst.Stop();

		SAILOR_LOG("Performance test RemoveFirst:\n\tstd::vector %llums\n\tTVector %llums", stdVectorRemoveFirst.ResultMs(), tVectorRemoveFirst.ResultMs());

		Timer stdVectorInsert;
		Timer tVectorInsert;

		uint32_t countToAdd = count / 256;
		stdVectorInsert.Start();
		for (size_t i = 0; i < countToAdd; i++)
		{
			const size_t num = rand() % 16;
			const size_t pos = rand() % ideal.size();

			ideal.insert(ideal.begin() + pos, 1u, TData((uint32_t)i));
		}
		stdVectorInsert.Stop();

		tVectorInsert.Start();
		for (size_t i = 0; i < countToAdd; i++)
		{
			const size_t num = rand() % 16;
			const size_t pos = rand() % container.Num();
			container.Insert(TData((uint32_t)i), pos);
		}
		tVectorInsert.Stop();

		SAILOR_LOG("Performance test Insert:\n\tstd::vector %llums\n\tTVector %llums", stdVectorInsert.ResultMs(), tVectorInsert.ResultMs());
	}

	static bool SanityCheck()
	{
		const size_t count = 16571;

		TVector<TData> container;
		std::vector<TData> ideal;

		srand(0);
		for (size_t i = 0; i < count; i++)
		{
			const int32_t value = rand();

			if (i % 2 == 0)
			{
				ideal.push_back(TData(value));
				container.Add(TData(value));
			}
			else
			{
				ideal.emplace_back(TData(value));
				container.Emplace(TData(value));
			}
		}

		bool bRes = Compare(ideal, container);

		uint32_t countToDelete = count / 2;
		for (size_t i = 0; i < countToDelete; i++)
		{
			const int32_t value = rand() % ideal.size();
			ideal.erase(ideal.begin() + value);
			container.RemoveAt(value);
		}

		bRes &= Compare(ideal, container);

		uint32_t countToAdd = count / 4;
		for (size_t i = 0; i < countToAdd; i++)
		{
			const size_t num = rand() % 16;
			const size_t pos = rand() % ideal.size();

			ideal.insert(ideal.begin() + pos, 1u, TData((uint32_t)i));
			container.Insert(TData((uint32_t)i), pos);
		}

		bRes &= Compare(ideal, container);

		uint32_t countToRemove = count / 16;
		for (size_t i = 0; i < countToRemove; i++)
		{
			const size_t pos = rand() % ideal.size();
			const auto valueToRemove = ideal[pos];

			ideal.erase(std::find(ideal.begin(), ideal.end(), valueToRemove));
			container.RemoveFirst(valueToRemove);
		}

		bRes &= Compare(ideal, container);

		uint32_t countToRemoveSwap = count / 16;
		for (size_t i = 0; i < countToRemoveSwap; i++)
		{
			const size_t pos = rand() % ideal.size();
			const auto valueToRemove = ideal[pos];

			std::iter_swap(ideal.begin() + pos, ideal.end() - 1);
			ideal.pop_back();

			container.RemoveAtSwap(pos);
		}

		bRes &= Compare(ideal, container);

		container.Clear();
		ideal.clear();

		return bRes && numInstances == 0;
	}

	template<typename T>
	static bool Compare(const std::vector<T>& lhs, const TVector<T>& rhs)
	{
		if (lhs.size() != rhs.Num())
		{
			return false;
		}

		for (size_t i = 0; i < lhs.size(); i++)
		{
			if (rhs[i] != lhs[i])
			{
				return false;
			}
		}
		return true;
	}

	static size_t numInstances;
};

size_t TestCase_VectorPerfromance::numInstances = 0;

void Sailor::RunVectorBenchmark()
{
	printf("\nStarting vector benchmark...\n");

	TestCase_VectorPerfromance::RunTests();
}
