#pragma once
#include "Memory.h"
#include <algorithm>

namespace Sailor::Memory
{
	template<typename TGlobalAllocator = GlobalHeapAllocator,
		uint32_t blockSize = 1024,
		uint32_t averageElementSize = 64,
		typename TPtrType = void*>

		class TBlockAllocator;

	class TDataBase
	{
	public:

		TDataBase() = default;

		TDataBase(size_t offset, size_t alignmentOffset, size_t size, uint32_t blockIndex) :
			m_offset(offset),
			m_size(size),
			m_alignmentOffset(alignmentOffset),
			m_blockIndex(blockIndex)
		{}

		size_t m_offset{};
		size_t m_alignmentOffset{};
		size_t m_size{};
		uint32_t m_blockIndex;

		void ShiftPointer(size_t offset)
		{
			m_offset += offset;
		}

		size_t Offset(const TDataBase& from, const TDataBase& to)
		{
			return (size_t)(to.m_offset - from.m_offset - from.m_size - from.m_alignmentOffset);
		}

		virtual void Clear()
		{
			m_offset = 0;
			m_size = 0;
			m_blockIndex = 0;
			m_alignmentOffset = 0;
		}

	protected:

		friend class TBlockAllocator<>;
	};

	template<typename TGlobalAllocator, uint32_t blockSize, uint32_t averageElementSize, typename TPtrType>
	class TBlockAllocator
	{
	public:

		class MemoryBlock;

		class TData : public TDataBase
		{
		public:

			TData() : TDataBase(), m_allocator{ nullptr }, m_ptr{} {}

			TData(size_t offset, size_t alignmentOffset, size_t size, TPtrType ptr, TBlockAllocator* allocator, uint32_t blockIndex) :
				TDataBase(offset, alignmentOffset, size, blockIndex),
				m_ptr(ptr),
				m_allocator(allocator)
			{}

			const TPtrType operator*() const { return Memory::GetAlignedPointer<TData, TPtrType>(*this); }
			TPtrType operator*() { return Memory::GetAlignedPointer<TData, TPtrType>(*this); }

			void Free() { m_allocator->Free(*this); }

			virtual void Clear() override
			{
				TDataBase::Clear();
				m_ptr = nullptr;
				m_allocator = nullptr;
			}

			TPtrType m_ptr{};
			TBlockAllocator* m_allocator;

		protected:

			friend class TBlockAllocator<>;
			friend class TBlockAllocator::MemoryBlock;

			friend void Sailor::Memory::Free<TData, TPtrType, TGlobalAllocator>(TData& ptr, TGlobalAllocator* allocator);
		};

		class MemoryBlock
		{
		public:

			MemoryBlock(size_t size, TBlockAllocator* owner) :
				m_blockSize(size),
				m_emptySpace(size),
				m_layout({ {0, size} }),
				m_blockIndex(InvalidIndex),
				m_owner(owner)
			{
				//std::cout << "Allocate Block " << (int32_t)size << std::endl;
				m_ptr = Sailor::Memory::Allocate<TData, TPtrType, TGlobalAllocator>(size, &m_owner->m_dataAllocator);
				m_layout.reserve(blockSize / averageElementSize);
			}

			~MemoryBlock()
			{
				//std::cout << "Free Block " << (int32_t)m_size << std::endl;
				if (m_blockIndex != InvalidIndex)
				{
					Sailor::Memory::Free<TData, TPtrType, TGlobalAllocator>(m_ptr, &m_owner->m_dataAllocator);
				}

				m_layout.clear();
			}

			MemoryBlock(const MemoryBlock& memoryBlock) = delete;
			MemoryBlock& operator=(const MemoryBlock& memoryBlock) = delete;

			MemoryBlock(MemoryBlock&& memoryBlock) noexcept
			{
				m_ptr = memoryBlock.m_ptr;
				m_blockSize = memoryBlock.m_blockSize;
				m_emptySpace = memoryBlock.m_emptySpace;
				m_blockIndex = memoryBlock.m_blockIndex;
				m_owner = memoryBlock.m_owner;
				m_layout = std::move(memoryBlock.m_layout);

				memoryBlock.m_owner = nullptr;
				memoryBlock.m_blockIndex = InvalidIndex;
				memoryBlock.m_emptySpace = 0;
				memoryBlock.m_blockSize = 0;
			}

			TData Allocate(uint32_t layoutIndex, size_t size, uint32_t alignmentOffset)
			{
				//std::cout << "Acquire " << (int32_t)size << std::endl;
				assert(layoutIndex != InvalidIndex);

				auto& emptySpace = m_layout[layoutIndex];

				size_t offset = emptySpace.first;
				emptySpace.first += size + alignmentOffset;
				emptySpace.second -= (size + alignmentOffset);

				if (emptySpace.second == 0)
				{
					std::iter_swap(m_layout.begin() + layoutIndex, m_layout.end() - 1);
					m_layout.pop_back();
				}

				RemoveSegmentation();

				m_emptySpace -= (size + alignmentOffset);
				return TData(offset, alignmentOffset, size, m_ptr.m_ptr, m_owner, m_blockIndex);
			}

			void RemoveSegmentation()
			{
				std::sort(m_layout.begin(), m_layout.end(), [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
				for (int32_t i = (int32_t)m_layout.size() - 1; i > 0; i--)
				{
					if (m_layout[i].first == m_layout[i - 1].first + m_layout[i - 1].second)
					{
						m_layout[i - 1].second += m_layout[i].second;

						std::iter_swap(m_layout.begin() + i, m_layout.end() - 1);
						m_layout.pop_back();
					}
				}
			}

			void Free(TData& ptr)
			{
				//std::cout << "Release " << (int32_t)ptr.m_size << std::endl;

				if (!ptr.m_ptr)
				{
					return;
				}

				m_emptySpace += ptr.m_size + ptr.m_alignmentOffset;

				m_layout.push_back({ ptr.m_offset, ptr.m_size + ptr.m_alignmentOffset });

				RemoveSegmentation();

				ptr.Clear();
			}

			bool FindLocationInLayout(size_t size, size_t alignment, uint32_t& layoutIndex, uint32_t& alignmentOffset)
			{
				if (size > m_emptySpace)
				{
					return false;
				}

				//std::sort(m_layout.begin(), m_layout.end(), [](auto& lhs, auto& rhs) { return lhs.second > rhs.second; });
				for (uint32_t i = 0; i != (uint32_t)m_layout.size(); i++)
				{
					if (Align(size, alignment, Memory::Shift(GetAlignedPointer<TData, TPtrType>(m_ptr), m_layout[i].first), m_layout[i].second, alignmentOffset))
					{
						layoutIndex = i;
						return true;
					}
				}

				return false;
			}

			size_t GetBlockSize() const { return m_blockSize; }
			float GetOccupation() const { return 1.0f - (float)m_emptySpace / m_blockSize; }

			bool IsFull() const { return m_emptySpace == 0; }
			bool IsEmpty() const { return m_emptySpace == m_blockSize; }

		private:

			TData m_ptr;
			size_t m_blockSize;
			size_t m_emptySpace;
			uint32_t m_blockIndex{ InvalidIndex };
			TBlockAllocator* m_owner;

			std::vector<std::pair<size_t, size_t>> m_layout;

			friend class TBlockAllocator;
		};

		TBlockAllocator() = default;
		TBlockAllocator(const TBlockAllocator&) = delete;
		TBlockAllocator& operator= (const TBlockAllocator&) = delete;

		template<typename TDataType>
		TData Allocate(uint32_t count)
		{
			size_t size = count * sizeof(TDataType);
			return Allocate(size, alignof(TDataType));
		}

		TData Allocate(size_t size, size_t alignment)
		{
			uint32_t layoutIndex;
			uint32_t blockLayoutIndex;
			uint32_t alignmentOffset;
			FindMemoryBlock(size, alignment, layoutIndex, blockLayoutIndex, alignmentOffset);

			const auto blockIndex = m_layout[layoutIndex];
			auto& block = m_blocks[blockIndex];
			auto res = block.Allocate(blockLayoutIndex, size, alignmentOffset);

			if (HeuristicToSkipBlocks(block.GetOccupation()))
			{
				std::iter_swap(m_layout.begin() + layoutIndex, m_layout.end() - 1);
				m_layout.pop_back();
			}
			return res;
		}

		void Free(TData& data)
		{
			if (data.m_ptr)
			{
				const uint32_t index = data.m_blockIndex;
				const float prevOccupation = m_blocks[index].GetOccupation();
				m_blocks[index].Free(data);
				const float currentOccupation = m_blocks[index].GetOccupation();

				if (HeuristicToSkipBlocks(prevOccupation) && !HeuristicToSkipBlocks(currentOccupation))
				{
					m_layout.push_back(index);
				}
			}
		}

		virtual ~TBlockAllocator()
		{
			m_blocks.clear();
			m_layout.clear();
		}

		MemoryBlock& GetMemoryBlock(uint32_t index) const { return m_blocks[index]; }

	private:

		static constexpr uint32_t InvalidIndex = (uint32_t)-1;

		bool HeuristicToSkipBlocks(float occupation) const
		{
			const float border = 1.0f - (float)averageElementSize / blockSize;
			return occupation > border;
		}

		void FindMemoryBlock(size_t size, size_t alignment, uint32_t& outLayoutIndex, uint32_t& outBlockLayoutIndex, uint32_t& outAlignedOffset)
		{
			for (int32_t index = (int32_t)(m_layout.size() - 1); index >= 0; index--)
			{
				auto& block = m_blocks[m_layout[index]];
				if (block.FindLocationInLayout(size, alignment, outBlockLayoutIndex, outAlignedOffset))
				{
					outLayoutIndex = index;
					return;
				}
			}

			// Create new block
			MemoryBlock block = MemoryBlock((size_t)max((uint32_t)size, (uint32_t)blockSize), this);
			block.m_blockIndex = (uint32_t)m_blocks.size();
			outLayoutIndex = (uint32_t)m_layout.size();
			block.FindLocationInLayout(size, alignment, outBlockLayoutIndex, outAlignedOffset);
			m_layout.push_back(block.m_blockIndex);
			m_usedDataSpace += block.GetBlockSize();
			m_blocks.push_back(std::move(block));
		}

		bool FreeBlock(MemoryBlock& block) const
		{
			if (block.IsEmpty())
			{
				m_usedDataSpace -= block.m_size;
				m_blocks.remove(block);
				return true;
			}
			return false;
		}

		TGlobalAllocator m_dataAllocator;

		size_t m_usedDataSpace = 0;
		std::vector<MemoryBlock> m_blocks;
		std::vector<uint32_t> m_layout;
	};
}