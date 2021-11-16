#pragma once
#include <nlohmann_json/include/nlohmann/json.hpp>
#include "Defines.h"
#include "Math/Math.h"

namespace Sailor
{
	class IJsonSerializable
	{
	public:

		virtual SAILOR_API void Serialize(nlohmann::json& outData) const = 0;
		virtual SAILOR_API void Deserialize(const nlohmann::json& inData) = 0;
	};
}

namespace ns
{
	SAILOR_API void to_json(nlohmann::json& j, const class Sailor::IJsonSerializable& p);
	SAILOR_API void from_json(const nlohmann::json& j, class Sailor::IJsonSerializable& p);
}

using json = nlohmann::json;

namespace glm
{
	void to_json(json& j, const glm::vec2& P);
	void from_json(const json& j, glm::vec2& P);
	void to_json(json& j, const glm::vec3& P);
	void from_json(const json& j, glm::vec3& P);
	void to_json(json& j, const glm::vec4& P);
	void from_json(const json& j, glm::vec4& P);
	void to_json(json& j, const glm::quat& P);
	void from_json(const json& j, glm::quat& P);
}