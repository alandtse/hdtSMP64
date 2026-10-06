#pragma once

#include "PluginOwnerAPI.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace hdt
{
	class ExternalOwnerRegistry;

	// @brief Move-only RAII handle for one mesh claim; releases its owner when destroyed, reset or reassigned.
	class ExternalClaim
	{
	public:
		ExternalClaim() = default;
		ExternalClaim(ExternalOwnerRegistry* registry, IMeshOwner* owner, std::uint64_t token);
		ExternalClaim(ExternalClaim&& a_rhs) noexcept;

		// Releases the claim this object currently holds before taking over the other's.
		ExternalClaim& operator=(ExternalClaim&& a_rhs) noexcept;

		ExternalClaim(const ExternalClaim&) = delete;
		ExternalClaim& operator=(const ExternalClaim&) = delete;

		~ExternalClaim();

		bool active() const { return m_owner != nullptr; }
		std::uint64_t token() const { return m_token; }

		// Releases if active, then becomes inactive.
		void reset();

	private:
		ExternalOwnerRegistry* m_registry = nullptr;
		IMeshOwner* m_owner = nullptr;
		std::uint64_t m_token = 0;
	};

	// @brief Process-wide list of mesh owners, asked in registration order whenever hdtSMP64 is about to
	// create a system for an armor or head part.
	class ExternalOwnerRegistry
	{
	public:
		static ExternalOwnerRegistry& instance();

		// Ignores null and already registered owners.
		void addOwner(IMeshOwner* owner);
		void removeOwner(IMeshOwner* owner);

		// The first registered owner returning true from claim wins; inactive when none does.
		ExternalClaim tryClaim(const IMeshClaimContext& context);

		// Calls owner->release only while that owner is still registered.
		void release(IMeshOwner* owner, std::uint64_t token);

	private:
		ExternalOwnerRegistry() = default;

		// Owner callbacks run outside m_mutex so an owner may register or unregister owners from inside claim.
		std::mutex m_mutex;
		std::vector<IMeshOwner*> m_owners;
		std::atomic<std::uint64_t> m_nextToken{ 1 };
	};
}
