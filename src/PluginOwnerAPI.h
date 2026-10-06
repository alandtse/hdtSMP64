#pragma once

#include <cstdint>

namespace RE
{
	class Actor;
	class NiNode;
	class NiAVObject;
}

namespace hdt
{
	// @brief Which kind of mesh an IMeshClaimContext describes.
	enum class MeshKind : std::uint8_t
	{
		Armor,
		HeadPart
	};

	// @brief Read-only description of one mesh hdtSMP64 is about to simulate, handed to IMeshOwner::claim.
	// @details The context and every pointer obtained from it are valid only for the duration of the claim call.
	// Copy any string, and take your own reference to any node, that you need to keep.
	class IMeshClaimContext
	{
	public:
		virtual ~IMeshClaimContext() = default;

		virtual MeshKind getKind() const = 0;

		// @brief SMP XML path of the mesh.
		virtual const char* getPhysicsFile() const = 0;

		// @brief May be null.
		virtual RE::Actor* getActor() const = 0;

		// @brief The NPC node the bones were merged into.
		virtual RE::NiNode* getSkeleton() const = 0;

		virtual RE::NiAVObject* getModel() const = 0;

		// @brief Nullptr if the bone was not renamed.
		virtual const char* getMergedBoneName(const char* originalName) const = 0;
	};

	// @brief Implemented by a plugin that takes over simulation of specific hdtSMP64 meshes.
	// @details claim and release run on the game main thread or the Papyrus VM thread, serialized by hdtSMP64's
	// actor manager lock. Implementations must not throw and must not call back into hdtSMP64 except to register
	// or unregister owners.
	class IMeshOwner
	{
	public:
		virtual ~IMeshOwner() = default;

		// @brief Returning true takes ownership of the described mesh; hdtSMP64 then creates no system for it.
		// @param claimToken Identifies this claim and is passed to the matching release.
		virtual bool claim(const IMeshClaimContext& context, std::uint64_t claimToken) = 0;
		virtual void release(std::uint64_t claimToken) = 0;
	};
}
