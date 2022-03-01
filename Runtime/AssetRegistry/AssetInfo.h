#pragma once
#include <string>
#include <ctime>
#include "AssetRegistry/AssetRegistry.h"
#include "AssetRegistry/UID.h"
#include "Core/Singleton.hpp"
#include "Containers/Vector.h"
#include "Core/JsonSerializable.h"
#include "nlohmann_json/include/nlohmann/json.hpp"

namespace Sailor
{
	class AssetInfo : IJsonSerializable
	{

	public:

		SAILOR_API AssetInfo();
		SAILOR_API virtual ~AssetInfo() = default;

		SAILOR_API const UID& GetUID() const { return m_UID; }

		SAILOR_API std::string GetRelativeAssetFilepath() const;
		SAILOR_API std::string GetRelativeMetaFilepath() const;

		// That includes "../Content/" in the beginning
		SAILOR_API std::string GetAssetFilepath() const { return m_folder + m_assetFilename; }
		
		// That includes "../Content/" in the beginning
		SAILOR_API std::string GetMetaFilepath() const;

		SAILOR_API std::time_t GetAssetLastModificationTime() const;
		SAILOR_API std::time_t GetMetaLastModificationTime() const;

		SAILOR_API bool IsExpired() const;

		SAILOR_API virtual void Serialize(nlohmann::json& outData) const override;
		SAILOR_API virtual void Deserialize(const nlohmann::json& inData) override;

	protected:

		std::time_t m_loadTime;
		std::string m_folder;
		std::string m_assetFilename;
		UID m_UID;

		friend class IAssetInfoHandler;
	};

	using AssetInfoPtr = AssetInfo*;

	class SAILOR_API IAssetInfoHandlerListener
	{
	public:
		virtual void OnUpdateAssetInfo(AssetInfoPtr assetInfo, bool bWasExpired) = 0;
		virtual void OnImportAsset(AssetInfoPtr assetInfo) = 0;

	};

	class SAILOR_API IAssetInfoHandler
	{

	public:

		void Subscribe(IAssetInfoHandlerListener* listener) { m_listeners.Add(listener); }
		void Unsubscribe(IAssetInfoHandlerListener* listener)
		{
			m_listeners.Remove(listener);
		}

		virtual void GetDefaultMetaJson(nlohmann::json& outDefaultJson) const = 0;

		virtual AssetInfoPtr LoadAssetInfo(const std::string& metaFilepath) const;
		virtual AssetInfoPtr ImportAsset(const std::string& assetFilepath) const;
		virtual void ReloadAssetInfo(AssetInfoPtr assetInfo) const;

		virtual ~IAssetInfoHandler() = default;

	protected:

		virtual AssetInfoPtr CreateAssetInfo() const = 0;
		TVector<std::string> m_supportedExtensions;

		TVector<IAssetInfoHandlerListener*> m_listeners;
	};

	class SAILOR_API DefaultAssetInfoHandler final : public TSubmodule<DefaultAssetInfoHandler>, public IAssetInfoHandler
	{

	public:

		DefaultAssetInfoHandler(AssetRegistry* assetRegistry);

		virtual void GetDefaultMetaJson(nlohmann::json& outDefaultJson) const override;
		virtual AssetInfoPtr CreateAssetInfo() const override;

		virtual ~DefaultAssetInfoHandler() override = default;
	};
}
