#include "PluginInterfaceImpl.h"
#include "ExternalOwner.h"
#include "dhdtPapyrusFunctions.h"

hdt::PluginInterfaceImpl hdt::g_pluginInterface;

void hdt::PluginInterfaceImpl::addListener(IPreStepListener* l)
{
	if (l) {
		m_preStepDispatcher.AddEventSink(l);
	}
}

void hdt::PluginInterfaceImpl::removeListener(IPreStepListener* l)
{
	if (l) {
		m_preStepDispatcher.RemoveEventSink(l);
	}
}

void hdt::PluginInterfaceImpl::addListener(IPostStepListener* l)
{
	if (l) {
		m_postStepDispatcher.AddEventSink(l);
	}
}

void hdt::PluginInterfaceImpl::removeListener(IPostStepListener* l)
{
	if (l) {
		m_postStepDispatcher.RemoveEventSink(l);
	}
}

void hdt::PluginInterfaceImpl::addMeshOwner(IMeshOwner* owner)
{
	ExternalOwnerRegistry::instance().addOwner(owner);
}

void hdt::PluginInterfaceImpl::removeMeshOwner(IMeshOwner* owner)
{
	ExternalOwnerRegistry::instance().removeOwner(owner);
}

void hdt::PluginInterfaceImpl::reloadActorPhysics(RE::Actor* actor)
{
	if (!actor) {
		return;
	}

	SKSE::GetTaskInterface()->AddTask([handle = RE::ActorHandle(actor)] {
		if (auto a = handle.get()) {
			papyrus::impl::ResetPhysicsImpl(a.get(), false);
		}
	});
}

void hdt::PluginInterfaceImpl::addFrameListener(IFrameListener* l)
{
	if (l) {
		m_frameDispatcher.AddEventSink(l);
	}
}

void hdt::PluginInterfaceImpl::removeFrameListener(IFrameListener* l)
{
	if (l) {
		m_frameDispatcher.RemoveEventSink(l);
	}
}

void hdt::PluginInterfaceImpl::addFrameSyncListener(IFrameSyncListener* l)
{
	if (l) {
		m_frameSyncDispatcher.AddEventSink(l);
	}
}

void hdt::PluginInterfaceImpl::removeFrameSyncListener(IFrameSyncListener* l)
{
	if (l) {
		m_frameSyncDispatcher.RemoveEventSink(l);
	}
}

RE::BSEventNotifyControl hdt::PluginInterfaceImpl::ProcessEvent(const Events::FrameEvent* e, RE::BSTEventSource<Events::FrameEvent>*)
{
	if (e) {
		const PluginFrameEvent event{ e->gamePaused };
		m_frameDispatcher.SendEvent(std::addressof(event));
	}

	return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl hdt::PluginInterfaceImpl::ProcessEvent(const Events::FrameSyncEvent*, RE::BSTEventSource<Events::FrameSyncEvent>*)
{
	const PluginFrameSyncEvent event;
	m_frameSyncDispatcher.SendEvent(std::addressof(event));

	return RE::BSEventNotifyControl::kContinue;
}

void hdt::PluginInterfaceImpl::onPostPostLoad()
{
	// Send ourselves to any plugin that registered during the PostLoad event
	if (m_skseMessagingInterface) {
		m_skseMessagingInterface->Dispatch(PluginInterface::MSG_STARTUP, static_cast<PluginInterface*>(this), 0, nullptr);
	}
}

void hdt::PluginInterfaceImpl::init(const SKSE::LoadInterface* skse)
{
	//We need to have our SKSE plugin handle and the messaging interface in order to reach our plugins later
	if (skse) {
		m_sksePluginHandle = skse->GetPluginHandle();
		m_skseMessagingInterface = reinterpret_cast<SKSE::MessagingInterface*>(skse->QueryInterface(SKSE::LoadInterface::kMessaging));
	}
	if (!m_skseMessagingInterface) {
		logger::warn("Failed to get a messaging interface. Plugins will not work.");
	}
}
