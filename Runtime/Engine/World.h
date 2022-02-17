#pragma once
#include "Sailor.h"
#include "GameObject.h"
#include "Memory/SharedPtr.hpp"
#include "Memory/ObjectAllocator.hpp"

namespace Sailor
{
	using WorldPtr = TWeakPtr<class World>;

	class World
	{
	public:

		World(std::string name);

		virtual ~World() = default;

		World(const World&) = delete;
		World& operator=(const World&) = delete;

		World(World&&) = default;
		World& operator=(World&&) = default;

		template<typename TObject, typename... TArgs >
		TObjectPtr<TObject> Instantiate(TArgs&& ... args)
		{
			auto newObject = GameObjectPtr::Make(m_allocator, std::forward(args));
			assert(newObject);

			newObject->m_world = this;
			m_objects.Add(newObject);

			return newObject;
		}

		void Destroy(GameObjectPtr object)
		{
			if (object && !object->bPendingDestroy)
			{
				object->bPendingDestroy = true;
				m_pendingDestroyObjects.PushBack(std::move(object));
			}
		}

		void Tick(float deltaTime);

		Memory::ObjectAllocatorPtr GetAllocator() { return m_allocator; }
		const Memory::ObjectAllocatorPtr& GetAllocator() const { return m_allocator; }

	protected:

		std::string m_name;
		TVector<GameObjectPtr> m_objects;
		TList<GameObjectPtr, Memory::TInlineAllocator<sizeof(GameObjectPtr) * 32>> m_pendingDestroyObjects;
		Memory::ObjectAllocatorPtr m_allocator;
	};
}