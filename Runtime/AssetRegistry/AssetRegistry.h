#pragma once
#include <string>
#include <fstream>
#include "Containers/Containers.h"
#include "AssetRegistry/UID.h"
#include "Core/Submodule.h"
#include "AssetRegistry/AssetInfo.h"
#include "Core/Singleton.hpp"
#include "nlohmann_json/include/nlohmann/json.hpp"

namespace Sailor
{
	class AssetInfo;
	using AssetInfoPtr = AssetInfo*;

	enum class EAssetType;

	class AssetRegistry final : public TSubmodule<AssetRegistry>
	{
	public:

		static constexpr const char* ContentRootFolder = "../Content/";
		static constexpr const char* MetaFileExtension = "asset";

		SAILOR_API virtual ~AssetRegistry() override;

		template<typename TBinaryType, typename TFilepath>
		static bool ReadBinaryFile(const TFilepath& filename, TVector<TBinaryType>& buffer)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);
			file.unsetf(std::ios::skipws);

			if (!file.is_open())
			{
				return false;
			}

			size_t fileSize = (size_t)file.tellg();
			buffer.Clear();

			size_t mod = fileSize % sizeof(TBinaryType);
			size_t size = fileSize / sizeof(TBinaryType) + (mod ? 1 : 0);
			buffer.Resize(size);

			//buffer.resize(fileSize / sizeof(T));

			file.seekg(0, std::ios::beg);
			file.read(reinterpret_cast<char*>(buffer.GetData()), fileSize);

			file.close();
			return true;
		}

		SAILOR_API static bool ReadAllTextFile(const std::string& filename, std::string& text);

		SAILOR_API void ScanContentFolder();
		SAILOR_API void ScanFolder(const std::string& folderPath);
		SAILOR_API const UID& LoadAsset(const std::string& filepath);
		SAILOR_API const UID& GetOrLoadAsset(const std::string& filepath);

		template<typename TAssetInfoPtr = AssetInfoPtr>
		TAssetInfoPtr GetAssetInfoPtr(UID uid) const
		{
			return 	dynamic_cast<TAssetInfoPtr>(GetAssetInfoPtr_Internal(uid));
		}

		template<typename TAssetInfoPtr = AssetInfoPtr>
		TAssetInfoPtr GetAssetInfoPtr(const std::string& assetFilepath) const
		{
			return 	dynamic_cast<TAssetInfoPtr>(GetAssetInfoPtr_Internal(assetFilepath));
		}

		template<class TAssetInfo>
		void GetAllAssetInfos(TVector<UID>& outAssetInfos) const
		{
			outAssetInfos.Clear();
			for (const auto& assetInfo : m_loadedAssetInfo)
			{
				if (dynamic_cast<TAssetInfo*>(assetInfo.m_second))
				{
					outAssetInfos.Add(assetInfo.m_first);
				}
			}
		}

		SAILOR_API bool RegisterAssetInfoHandler(const TVector<std::string>& supportedExtensions, class IAssetInfoHandler* pAssetInfoHandler);
		SAILOR_API static std::string GetMetaFilePath(const std::string& assetFilePath);

	protected:

		SAILOR_API AssetInfoPtr GetAssetInfoPtr_Internal(UID uid) const;
		SAILOR_API AssetInfoPtr GetAssetInfoPtr_Internal(const std::string& assetFilepath) const;

		TMap<UID, AssetInfoPtr> m_loadedAssetInfo;
		TMap<std::string, UID> m_UIDs;
		TMap<std::string, class IAssetInfoHandler*> m_assetInfoHandlers;
	};
}
