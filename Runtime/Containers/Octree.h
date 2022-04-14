#pragma once
#include <cassert>
#include <memory>
#include <functional>
#include <concepts>
#include <type_traits>
#include <iterator>
#include <algorithm>
#include "Core/Defines.h"
#include "Math/Math.h"
#include "Memory/Memory.h"
#include "Containers/Concepts.h"
#include "Containers/Map.h"
#include "Containers/Vector.h"

namespace Sailor
{
	template<typename TElementType, typename TAllocator = Memory::DefaultGlobalAllocator>
	class SAILOR_API TOctree final
	{
		static constexpr uint32_t NumElementsInNode = 4u;

	protected:

		struct TNodeElement
		{
			TNodeElement(const TElementType& data, const glm::ivec3& pos, const glm::ivec3& extents) : m_element(data), m_extents(extents), m_position(pos)
			{
				m_volume = m_extents.x * m_extents.y * m_extents.z;
			}

			__forceinline bool operator<(const TNodeElement& rhs) const { return this->m_volume < rhs.m_volume; }

			TElementType m_element{};
			glm::ivec3 m_position{};
			glm::ivec3 m_extents{};
			uint32_t m_volume{};
		};

		struct TNode
		{
			constexpr uint32_t GetIndex(int32_t x, int32_t y, int32_t z) { return (x == -1 ? 0 : 1) + (z == -1 ? 1 : 0) * 2 + (y == -1 ? 0 : 1) * 4; }

			__forceinline bool IsLeaf() const { return m_internal[0] == nullptr; }
			__forceinline bool Contains(const glm::ivec3& pos, const glm::ivec3& extents) const
			{
				const int32_t halfSize = m_size / 2;
				return m_center.x - halfSize < pos.x - extents.x &&
					m_center.y - halfSize < pos.y - extents.y &&
					m_center.z - halfSize < pos.z - extents.z &&
					m_center.x + halfSize > pos.x + extents.x &&
					m_center.y + halfSize > pos.y + extents.y &&
					m_center.z + halfSize > pos.z + extents.z;
			}

			__forceinline bool Overlaps(const glm::ivec3& pos, const glm::ivec3& extents) const
			{
				const int32_t halfSize = m_size / 2;
				return
					(m_center.x - halfSize < pos.x + extents.x) && (m_center.x + halfSize > pos.x - extents.x) &&
					(m_center.y - halfSize < pos.y + extents.y) && (m_center.y + halfSize > pos.y - extents.y) &&
					(m_center.z - halfSize < pos.z + extents.z) && (m_center.z + halfSize > pos.z - extents.z);
			}

			__forceinline bool CanCollapse() const
			{
				if (IsLeaf())
				{
					return false;
				}

				for (uint32_t i = 0; i < 8; i++)
				{
					if (!m_internal[i]->IsLeaf() || m_internal[i]->m_elements.Num() > 0)
					{
						return false;
					}
				}

				return true;
			}

			__forceinline void Insert(const TElementType& element, const glm::ivec3& pos, const glm::ivec3& extents)
			{
				size_t index = std::distance(m_elements.begin(),
					std::upper_bound(m_elements.begin(), m_elements.end(), TNodeElement(element, pos, extents)));

				m_elements.Insert(TNodeElement(element, pos, extents), index);
			}

			__forceinline bool Remove(const glm::ivec3& pos, const glm::ivec3& extents, const TElementType& element)
			{
				auto it = std::lower_bound(m_elements.begin(), m_elements.end(), TNodeElement(element, pos, extents));
				if (it != m_elements.end())
				{
					const size_t index = std::distance(m_elements.begin(), it);
					for (size_t i = index; i < m_elements.Num(); i++)
					{
						if (m_elements[i].m_element == element)
						{
							m_elements.RemoveAt(index);
							return true;
						}
					}
				}

				return false;
			}

			uint32_t m_size = 0;
			glm::ivec3 m_center{};
			TNode* m_internal[8]{};
			TVector<TNodeElement, Memory::TInlineAllocator<NumElementsInNode * 8u + 64u, TAllocator>> m_elements{};
		};

	public:

		// Constructors & Destructor
		TOctree(glm::ivec3 center, uint32_t size = 16536u, uint32_t minSize = 4) : m_map(size)
		{
			m_minSize = minSize;
			m_root = Memory::New<TNode>(m_allocator);
			m_root->m_size = size;
			m_root->m_center = center;
		}

		virtual ~TOctree()
		{
			Clear();

			Memory::Delete<TNode>(m_allocator, m_root);
			m_root = nullptr;
		}

		TOctree(const TOctree& octree) {}
		TOctree& operator= (const TOctree& octree) {}

		TOctree(TOctree&& octree) { Swap(std::move(*this), octree); }
		TOctree& operator= (TOctree&& octree)
		{
			Swap(std::move(*this), octree);
			return this;
		}

		static void Swap(TOctree&& lhs, TOctree&& rhs)
		{
			std::swap(rhs->m_root, lhs->m_root);
			std::swap(rhs->m_allocator, lhs->m_allocator);
			std::swap(rhs->m_num, lhs->m_num);
			std::swap(rhs->m_minSize, lhs->m_minSize);
			std::swap(rhs->m_map, lhs->m_map);
		}

		void Clear()
		{
			if (m_root)
			{
				Clear_Internal(m_root);
			}
			m_map.Clear();
		}

		size_t GetNum() const { return m_num; }
		size_t GetNumNodes() const { return m_numNodes; }

		//TVector<TElementType, TAllocator> GetElements(glm::ivec3 extents) const { return TVector(); }

		bool Insert(const glm::ivec3& pos, const glm::ivec3& extents, const TElementType& element)
		{
			if (Insert_Internal(m_root, pos, extents, element))
			{
				m_num++;
				return true;
			}
			return false;
		}

		bool Remove(const glm::ivec3& pos, const glm::ivec3& extents, const TElementType& element)
		{
			TNode** node;
			if (m_map.Find(element, node))
			{
				if ((*node)->Remove(pos, extents, element))
				{
					Resolve_Internal(*node);
					return true;
				}
			}

			return false;
		}

		__forceinline void Resolve()
		{
			Resolve_Internal(m_root);
		}

	protected:

		void Resolve_Internal(TNode* node)
		{
			if (node->m_elements.Num())
			{
				return;
			}

			for (uint32_t i = 0; i < 8; i++)
			{
				if (!node->IsLeaf())
				{
					Resolve_Internal(node->m_internal[i]);
				}
			}

			if (node->CanCollapse())
			{
				Collapse(node);
			}
		}

		void Clear_Internal(TNode* node)
		{
			if (node->IsLeaf())
			{
				return;
			}

			for (uint32_t i = 0; i < 8; i++)
			{
				Clear_Internal(node->m_internal[i]);
			}

			Collapse(node);
		}

		__forceinline void Subdivide(TNode* node)
		{
			assert(node->IsLeaf());

			// Bottom   Top    
			// |0|1|    |4|5|
			// |2|3|    |6|7|
			const glm::ivec3 offset[] = { glm::ivec3(1, -1, -1), glm::ivec3(1, -1, 1), glm::ivec3(-1, -1, -1), glm::ivec3(-1, -1, 1),
										  glm::ivec3(1, 1, -1), glm::ivec3(1, 1, 1), glm::ivec3(-1, 1, -1), glm::ivec3(-1, 1, 1) };

			const int32_t quarterSize = node->m_size / 4;

			for (uint32_t i = 0; i < 8; i++)
			{
				node->m_internal[i] = Memory::New<TNode>(m_allocator);
				node->m_internal[i]->m_size = quarterSize * 2;
				node->m_internal[i]->m_center = offset[i] * quarterSize + node->m_center;
			}

			m_numNodes += 8;
		}

		__forceinline void Collapse(TNode* node)
		{
			for (uint32_t i = 0; i < 8; i++)
			{
				Memory::Delete<TNode>(m_allocator, node->m_internal[i]);
				node->m_internal[i] = nullptr;
			}

			m_numNodes -= 8;
		}

		// Not used
		bool Remove_Internal(TNode* node, const glm::ivec3& pos, const glm::ivec3& extents, const TElementType& element)
		{
			const bool bIsLeaf = node->IsLeaf();
			const bool bOverlaps = node->Overlaps(pos, extents);

			if (!bOverlaps)
			{
				return false;
			}

			const size_t index = node->m_elements.FindIf([&](const auto& lhs) { return lhs.m_element == element; });
			if (index != -1)
			{
				node->m_elements.RemoveAtSwap(index);
				Resolve_Internal(node);
				return true;
			}

			if (!bIsLeaf)
			{
				for (uint32_t i = 0; i < 8; i++)
				{
					if (Remove_Internal(node->m_internal[i], pos, extents, element))
					{
						return true;
					}
				}
			}
			return false;
		}

		bool Insert_Internal(TNode* node, const glm::ivec3& pos, const glm::ivec3& extents, const TElementType& element)
		{
			const bool bIsLeaf = node->IsLeaf();
			const bool bContains = node->Contains(pos, extents);

			if (!bContains)
			{
				return false;
			}

			if (bIsLeaf)
			{
				if (node->m_elements.Num() == NumElementsInNode && node->m_size > m_minSize)
				{
					auto elements = std::move(node->m_elements);
					node->m_elements.Clear();

					elements.Add(TNodeElement(element, pos, extents));

					Subdivide(node);

					for (auto& el : elements)
					{
						assert(Insert_Internal(node, el.m_position, el.m_extents, el.m_element));
					}

					return true;
				}

				node->Insert(element, pos, extents);
				m_map[element] = node;

				return true;
			}
			else
			{
				for (uint32_t i = 0; i < 8; i++)
				{
					if (Insert_Internal(node->m_internal[i], pos, extents, element))
					{
						return true;
					}
				}

				node->Insert(element, pos, extents);
				m_map[element] = node;
			}

			return true;
		}

		TNode* m_root{};
		size_t m_num = 0u;
		uint32_t m_minSize = 1;
		size_t m_numNodes = 1u;
		TAllocator m_allocator{};
		TMap<TElementType, TNode*> m_map{};
	};

	SAILOR_API void RunOctreeBenchmark();
}