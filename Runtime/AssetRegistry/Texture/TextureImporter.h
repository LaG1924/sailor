#pragma once
#include "Core/Defines.h"
#include <string>
#include <vector>
#include <nlohmann_json/include/nlohmann/json.hpp>
#include "Core/Submodule.h"
#include "Memory/SharedPtr.hpp"
#include "Memory/WeakPtr.hpp"
#include "AssetRegistry/AssetInfo.h"
#include "TextureAssetInfo.h"
#include "RHI/Renderer.h"
#include "Framework/Object.h"

namespace Sailor
{
	class Texture : public Object
	{
	public:

		Texture(UID uid) : Object(uid) {}

		virtual bool IsReady() const override;

		const RHI::TexturePtr& GetRHI() const { return m_rhiTexture; }
		RHI::TexturePtr& GetRHI() { return m_rhiTexture; }

	protected:

		RHI::TexturePtr m_rhiTexture;

		friend class TextureImporter;
	};

	using TexturePtr = TWeakPtr<Texture>;

	class TextureImporter final : public TSubmodule<TextureImporter>, public IAssetInfoHandlerListener
	{
	public:
		using ByteCode = std::vector<uint8_t>;

		SAILOR_API TextureImporter(TextureAssetInfoHandler* infoHandler);
		virtual SAILOR_API ~TextureImporter() override;

		virtual SAILOR_API void OnImportAsset(AssetInfoPtr assetInfo) override;
		virtual SAILOR_API void OnUpdateAssetInfo(AssetInfoPtr assetInfo, bool bWasExpired) override;

		SAILOR_API bool LoadTexture_Immediate(UID uid, TexturePtr& outTexture);
		SAILOR_API bool LoadTexture(UID uid, TexturePtr& outTexture, JobSystem::TaskPtr& outLoadingJob);

	private:

		SAILOR_API bool ImportTexture(UID uid, ByteCode& decodedData, int32_t& width, int32_t& height, uint32_t& mipLevels);
		
		SAILOR_API bool IsTextureLoaded(UID uid) const;
		std::unordered_map<UID, TSharedPtr<Texture>> m_loadedTextures;
		std::mutex m_mutex;
	};
}
