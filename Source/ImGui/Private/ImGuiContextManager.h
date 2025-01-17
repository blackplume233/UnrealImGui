// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include "ImGuiContextProxy.h"
#include "VersionCompatibility.h"


class FImGuiModuleSettings;
struct FImGuiDPIScaleInfo;

// TODO: It might be useful to broadcast FContextProxyCreatedDelegate to users, to support similar cases to our ImGui
// demo, but we would need to remove from that interface internal classes.

// Delegate called when new context proxy is created.
// @param ContextIndex - Index for that world
// @param ContextProxy - Created context proxy
DECLARE_MULTICAST_DELEGATE_TwoParams(FContextProxyCreatedDelegate, FImguiViewHandle, FImGuiContextProxy&);

// Manages ImGui context proxies.
class FImGuiContextManager
{
public:

	FImGuiContextManager(FImGuiModuleSettings& InSettings);

	FImGuiContextManager(const FImGuiContextManager&) = delete;
	FImGuiContextManager& operator=(const FImGuiContextManager&) = delete;

	FImGuiContextManager(FImGuiContextManager&&) = delete;
	FImGuiContextManager& operator=(FImGuiContextManager&&) = delete;

	~FImGuiContextManager();

	ImFontAtlas& GetFontAtlas() { return FontAtlas; }
	const ImFontAtlas& GetFontAtlas() const { return FontAtlas; }

#if WITH_EDITOR
	// Get or create editor ImGui context proxy.
	FORCEINLINE FImGuiContextProxy& GetEditorContextProxy() { return *GetEditorContextData().ContextProxy; }
#endif

#if !WITH_EDITOR
	// Get or create standalone game ImGui context proxy.
	FORCEINLINE FImGuiContextProxy& GetWorldContextProxy() { return *GetStandaloneWorldContextData().ContextProxy; }
#endif

	// Get or create ImGui context proxy for given world.
	FORCEINLINE FImGuiContextProxy& GetWorldContextProxy(const UWorld& World) { return *GetWorldContextData(World).ContextProxy; }

	// Get or create ImGui context proxy for given world. Additionally get context index for that proxy.
	FORCEINLINE FImGuiContextProxy& GetWorldContextProxy(const UWorld& World, FName& OutContextIndex) { return *GetWorldContextData(World, &OutContextIndex).ContextProxy; }

	// Get context proxy by index, or null if context with that index doesn't exist.
	FORCEINLINE FImGuiContextProxy* GetContextProxy(FImguiViewHandle ContextIndex)
	{
#if WITH_EDITOR
		if(ContextIndex.GetContextName() == Utilities::EDITOR_CONTEXT_INDEX)
		{
			return &GetEditorContextProxy();
		}
#endif
		
		FContextData* Data = Contexts.Find(ContextIndex.GetContextName());
		return Data ? Data->ContextProxy.Get() : nullptr;
	}
	
	IMGUI_API FORCEINLINE FImGuiContextProxy* GetOrCreateContextProxy(FImguiViewHandle ContextIndex);
	
	// Delegate called when a new context proxy is created.
	FContextProxyCreatedDelegate OnContextProxyCreated;

	// Delegate called after font atlas is built.
	FSimpleMulticastDelegate OnFontAtlasBuilt;

	void Tick(float DeltaSeconds);

private:

	struct FContextData
	{
		FContextData(const FString& ContextName, FName ContextIndex, ImFontAtlas& FontAtlas, float DPIScale, int32 InPIEInstance = -1)
			: PIEInstance(InPIEInstance)
			, ContextProxy(new FImGuiContextProxy(ContextName, ContextIndex, &FontAtlas, DPIScale))
		{
		}

		FORCEINLINE bool CanTick() const { return PIEInstance < 0 || GEngine->GetWorldContextFromPIEInstance(PIEInstance); }

		int32 PIEInstance = -1;
		TUniquePtr<FImGuiContextProxy> ContextProxy;
	};

#if ENGINE_COMPATIBILITY_LEGACY_WORLD_ACTOR_TICK
	void OnWorldTickStart(ELevelTick TickType, float DeltaSeconds);
#endif
	void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaSeconds);

#if ENGINE_COMPATIBILITY_WITH_WORLD_POST_ACTOR_TICK
	void OnWorldPostActorTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
#endif

#if WITH_EDITOR
	FContextData& GetEditorContextData();
#endif

#if !WITH_EDITOR
	FContextData& GetStandaloneWorldContextData();
#endif

	FContextData& GetWorldContextData(const UWorld& World, FName* OutContextIndex = nullptr);

	void SetDPIScale(const FImGuiDPIScaleInfo& ScaleInfo);
	void BuildFontAtlas();
	void RebuildFontAtlas();

	TMap<FName, FContextData> Contexts;

	ImFontAtlas FontAtlas;
	TArray<TUniquePtr<ImFontAtlas>> FontResourcesToRelease;

	FImGuiModuleSettings& Settings;

	float DPIScale = -1.f;
	int32 FontResourcesReleaseCountdown = 0;
};
