#include <doctest/doctest.h>

#include "ExternalOwner.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace
{
	class FakeContext : public hdt::IMeshClaimContext
	{
	public:
		hdt::MeshKind getKind() const override { return hdt::MeshKind::Armor; }
		const char* getPhysicsFile() const override { return "test.xml"; }
		RE::Actor* getActor() const override { return nullptr; }
		RE::NiNode* getSkeleton() const override { return nullptr; }
		RE::NiAVObject* getModel() const override { return nullptr; }
		const char* getMergedBoneName(const char*) const override { return nullptr; }
	};

	class FakeOwner : public hdt::IMeshOwner
	{
	public:
		explicit FakeOwner(bool a_claims) :
			claims(a_claims)
		{}

		bool claim(const hdt::IMeshClaimContext&, std::uint64_t a_token) override
		{
			++claimCalls;
			lastClaimToken = a_token;

			if (claims && onClaim) {
				onClaim();
			}

			return claims;
		}

		void release(std::uint64_t a_token) override
		{
			++releaseCalls;
			lastReleaseToken = a_token;
		}

		bool claims = false;
		int claimCalls = 0;
		int releaseCalls = 0;
		std::uint64_t lastClaimToken = 0;
		std::uint64_t lastReleaseToken = 0;
		std::function<void()> onClaim;
	};

	// Unregisters on scope exit so a failing CHECK cannot leak a registration into the next case.
	class OwnerScope
	{
	public:
		~OwnerScope()
		{
			auto& registry = hdt::ExternalOwnerRegistry::instance();
			for (const auto& owner : m_owners) {
				registry.removeOwner(owner.get());
			}
		}

		FakeOwner& create(bool a_claims)
		{
			m_owners.push_back(std::make_unique<FakeOwner>(a_claims));
			return *m_owners.back();
		}

		FakeOwner& add(bool a_claims)
		{
			FakeOwner& owner = create(a_claims);
			hdt::ExternalOwnerRegistry::instance().addOwner(&owner);
			return owner;
		}

	private:
		std::vector<std::unique_ptr<FakeOwner>> m_owners;
	};
}

TEST_CASE("no owners registered gives an inactive claim")
{
	OwnerScope scope;

	const hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
	CHECK_FALSE(claim.active());
	CHECK(claim.token() == 0);
}

TEST_CASE("add ignores null and duplicate owners")
{
	OwnerScope scope;
	auto& registry = hdt::ExternalOwnerRegistry::instance();

	registry.addOwner(nullptr);
	CHECK_FALSE(registry.tryClaim(FakeContext()).active());

	FakeOwner& owner = scope.add(true);
	registry.addOwner(&owner);
	registry.addOwner(&owner);

	const hdt::ExternalClaim claim = registry.tryClaim(FakeContext());
	CHECK(claim.active());
	CHECK(owner.claimCalls == 1);
}

TEST_CASE("the first owner in registration order that claims wins")
{
	OwnerScope scope;
	FakeOwner& first = scope.add(true);
	FakeOwner& second = scope.add(true);

	const hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
	CHECK(claim.active());
	CHECK(first.claimCalls == 1);
	CHECK(second.claimCalls == 0);
}

TEST_CASE("an owner that declines is skipped and the next one is asked")
{
	OwnerScope scope;
	FakeOwner& declining = scope.add(false);
	FakeOwner& accepting = scope.add(true);

	const hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
	CHECK(claim.active());
	CHECK(declining.claimCalls == 1);
	CHECK(accepting.claimCalls == 1);
}

TEST_CASE("a claim releases its owner exactly once")
{
	OwnerScope scope;
	FakeOwner& owner = scope.add(true);

	SUBCASE("on destruction")
	{
		{
			const hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
			CHECK(claim.active());
			CHECK(owner.releaseCalls == 0);
		}

		CHECK(owner.releaseCalls == 1);
	}

	SUBCASE("on reset")
	{
		{
			hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
			REQUIRE(claim.active());

			claim.reset();
			CHECK_FALSE(claim.active());
			CHECK(owner.releaseCalls == 1);

			claim.reset();
			CHECK(owner.releaseCalls == 1);
		}

		CHECK(owner.releaseCalls == 1);
	}

	SUBCASE("on move-assignment over an active claim")
	{
		{
			hdt::ExternalClaim first = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
			hdt::ExternalClaim second = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
			REQUIRE(first.active());
			REQUIRE(second.active());

			first = std::move(second);
			CHECK(owner.releaseCalls == 1);
			CHECK(first.active());
			CHECK_FALSE(second.active());
		}

		CHECK(owner.releaseCalls == 2);
	}
}

TEST_CASE("a moved-from claim does not release")
{
	OwnerScope scope;
	FakeOwner& owner = scope.add(true);

	{
		hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
		REQUIRE(claim.active());

		hdt::ExternalClaim moved = std::move(claim);
		CHECK_FALSE(claim.active());
		CHECK(claim.token() == 0);
		CHECK(moved.active());
	}

	CHECK(owner.releaseCalls == 1);
}

TEST_CASE("release is not delivered to an owner that was removed after claiming")
{
	OwnerScope scope;
	FakeOwner& owner = scope.add(true);

	{
		hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
		REQUIRE(claim.active());

		hdt::ExternalOwnerRegistry::instance().removeOwner(&owner);
	}

	CHECK(owner.releaseCalls == 0);
}

TEST_CASE("an owner may register an owner from inside claim")
{
	OwnerScope scope;
	FakeOwner& added = scope.create(true);
	FakeOwner& registering = scope.add(true);
	registering.onClaim = [&] { hdt::ExternalOwnerRegistry::instance().addOwner(&added); };

	auto& registry = hdt::ExternalOwnerRegistry::instance();

	{
		const hdt::ExternalClaim claim = registry.tryClaim(FakeContext());
		CHECK(claim.active());
		CHECK(registering.claimCalls == 1);
		CHECK(added.claimCalls == 0);
	}

	registry.removeOwner(&registering);

	{
		const hdt::ExternalClaim claim = registry.tryClaim(FakeContext());
		CHECK(claim.active());
		CHECK(added.claimCalls == 1);
	}
}

TEST_CASE("an owner may unregister an owner from inside claim")
{
	OwnerScope scope;
	FakeOwner& removing = scope.add(true);
	FakeOwner& victim = scope.add(true);
	removing.onClaim = [&] { hdt::ExternalOwnerRegistry::instance().removeOwner(&victim); };

	auto& registry = hdt::ExternalOwnerRegistry::instance();

	{
		const hdt::ExternalClaim claim = registry.tryClaim(FakeContext());
		CHECK(claim.active());
		CHECK(removing.claimCalls == 1);
		CHECK(victim.claimCalls == 0);
	}

	{
		const hdt::ExternalClaim claim = registry.tryClaim(FakeContext());
		CHECK(claim.active());
		CHECK(victim.claimCalls == 0);
	}
}

TEST_CASE("tokens are unique and nonzero across claims")
{
	OwnerScope scope;
	scope.add(true);

	std::vector<std::uint64_t> tokens;
	for (int i = 0; i < 8; ++i) {
		const hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
		REQUIRE(claim.active());
		CHECK(claim.token() != 0);
		tokens.push_back(claim.token());
	}

	std::sort(tokens.begin(), tokens.end());
	CHECK(std::adjacent_find(tokens.begin(), tokens.end()) == tokens.end());
}

TEST_CASE("the token passed to claim equals the token passed to the matching release")
{
	OwnerScope scope;
	FakeOwner& owner = scope.add(true);

	{
		const hdt::ExternalClaim claim = hdt::ExternalOwnerRegistry::instance().tryClaim(FakeContext());
		REQUIRE(claim.active());
		CHECK(owner.lastClaimToken == claim.token());
	}

	CHECK(owner.releaseCalls == 1);
	CHECK(owner.lastReleaseToken == owner.lastClaimToken);
}
