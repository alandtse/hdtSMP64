#include "ExternalOwner.h"

#include <algorithm>

namespace hdt
{
	ExternalClaim::ExternalClaim(ExternalOwnerRegistry* registry, IMeshOwner* owner, std::uint64_t token) :
		m_registry(registry),
		m_owner(owner),
		m_token(token)
	{}

	ExternalClaim::ExternalClaim(ExternalClaim&& a_rhs) noexcept :
		m_registry(a_rhs.m_registry),
		m_owner(a_rhs.m_owner),
		m_token(a_rhs.m_token)
	{
		a_rhs.m_registry = nullptr;
		a_rhs.m_owner = nullptr;
		a_rhs.m_token = 0;
	}

	ExternalClaim& ExternalClaim::operator=(ExternalClaim&& a_rhs) noexcept
	{
		if (this != &a_rhs) {
			reset();

			m_registry = a_rhs.m_registry;
			m_owner = a_rhs.m_owner;
			m_token = a_rhs.m_token;

			a_rhs.m_registry = nullptr;
			a_rhs.m_owner = nullptr;
			a_rhs.m_token = 0;
		}

		return *this;
	}

	ExternalClaim::~ExternalClaim()
	{
		reset();
	}

	void ExternalClaim::reset()
	{
		ExternalOwnerRegistry* const registry = m_registry;
		IMeshOwner* const owner = m_owner;
		const std::uint64_t token = m_token;

		m_registry = nullptr;
		m_owner = nullptr;
		m_token = 0;

		if (owner && registry) {
			registry->release(owner, token);
		}
	}

	ExternalOwnerRegistry& ExternalOwnerRegistry::instance()
	{
		static ExternalOwnerRegistry s_registry;
		return s_registry;
	}

	void ExternalOwnerRegistry::addOwner(IMeshOwner* owner)
	{
		if (!owner) {
			return;
		}

		std::lock_guard<std::mutex> lock(m_mutex);
		if (std::find(m_owners.begin(), m_owners.end(), owner) == m_owners.end()) {
			m_owners.push_back(owner);
		}
	}

	void ExternalOwnerRegistry::removeOwner(IMeshOwner* owner)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_owners.erase(std::remove(m_owners.begin(), m_owners.end(), owner), m_owners.end());
	}

	ExternalClaim ExternalOwnerRegistry::tryClaim(const IMeshClaimContext& context)
	{
		std::vector<IMeshOwner*> owners;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			owners = m_owners;
		}

		const std::uint64_t token = m_nextToken.fetch_add(1, std::memory_order_relaxed);
		for (IMeshOwner* owner : owners) {
			if (owner->claim(context, token)) {
				return ExternalClaim(this, owner, token);
			}
		}

		return ExternalClaim();
	}

	void ExternalOwnerRegistry::release(IMeshOwner* owner, std::uint64_t token)
	{
		if (!owner) {
			return;
		}

		bool registered = false;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			registered = std::find(m_owners.begin(), m_owners.end(), owner) != m_owners.end();
		}

		if (registered) {
			owner->release(token);
		}
	}
}
