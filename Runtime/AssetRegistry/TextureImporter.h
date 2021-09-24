#pragma once
#include "Defines.h"
#include <string>
#include <vector>
#include <nlohmann_json/include/nlohmann/json.hpp>
#include "Core/Singleton.hpp"
#include "Core/SharedPtr.hpp"
#include "Core/WeakPtr.hpp"
#include "AssetInfo.h"

namespace Sailor
{
	class TextureImporter final : public TSingleton<TextureImporter>, public IAssetInfoHandlerListener
	{
	public:
		using ByteCode = std::vector<uint32_t>;

		static SAILOR_API void Initialize();

		virtual SAILOR_API ~TextureImporter() override;
		virtual SAILOR_API void OnAssetInfoUpdated(AssetInfo* assetInfo) override;

	private:
		
	};
}
