// This source code is licensed under the MIT license found in the LICENSE file in the root directory of this source tree.

#include "TiledMapActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/CollisionProfile.h"
#include "Json.h"
#if WITH_EDITOR
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#endif

ATiledMapActor::ATiledMapActor()
	: MapPath("Maps/Map.json")
	, TileSize(100.f, 100.f)
{
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	RootComponent = SceneComponent;

#if WITH_EDITORONLY_DATA
	SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (!IsRunningCommandlet() && (SpriteComponent != NULL))
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> TiledMapActorSpriteObject;
			FConstructorStatics()
				: TiledMapActorSpriteObject(TEXT("/Engine/EditorResources/S_Terrain"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		SpriteComponent->Sprite = ConstructorStatics.TiledMapActorSpriteObject.Get();
		SpriteComponent->SetupAttachment(RootComponent);
		SpriteComponent->bIsScreenSizeScaled = true;
	}
#endif // WITH_EDITORONLY_DATA
}

bool ATiledMapActor::InternalImportMap(const FString& Filepath, FString& ErrorMessage)
{
	if (Filepath.IsEmpty())
	{
		ErrorMessage = FString::Printf(TEXT("Map path not set"));
		return false;
	}

	if (!MeshTable)
	{
		ErrorMessage = FString::Printf(TEXT("Mesh table is not set"));
		return false;
	}

	// Read json
	const FString FullFilepath = FPaths::ProjectContentDir() / Filepath;
	FString JsonStr;
	if (!FFileHelper::LoadFileToString(JsonStr, *FullFilepath))
	{
		ErrorMessage = FString::Printf(TEXT("Couldn't read %s"), *Filepath);
		return false;
	}

	TSharedRef<TJsonReader<TCHAR>> JsonReader = FJsonStringReader::Create(JsonStr);
	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject());
	if (!FJsonSerializer::Deserialize(JsonReader, Root))
	{
		ErrorMessage = FString::Printf(TEXT("Couldn't deserialize %s"), *Filepath);
		return false;
	}

	const int32 NumChildren = RootComponent->GetNumChildrenComponents();
	for (int32 ChildIdx = NumChildren - 1; ChildIdx >= 0; ChildIdx--)
	{
		RootComponent->GetChildComponent(ChildIdx)->DestroyComponent();
	}

	// Read tilesets
	TArray<TTuple<uint32, FString>> Tilesets;
	for (auto JsonTileset : Root->GetArrayField(TEXT("tilesets")))
	{
		const auto TilesetNode = JsonTileset->AsObject();
		const uint32 TilesetFirstGID = TilesetNode->GetIntegerField("firstgid");
		const FString TilesetName = TilesetNode->GetStringField("name");
		Tilesets.Emplace(TilesetFirstGID, TilesetName);
	}
	Tilesets.Sort([](const auto& A, const auto& B)
		{
			return A.Key > B.Key;
		});
	const uint32 FirstGID = Tilesets.Num() ? Tilesets[0].Key : 0u;

	// Read tile layers
	int LayerIdx = 0;
	for (auto JsonLayer : Root->GetArrayField(TEXT("layers")))
	{
		const auto LayerNode = JsonLayer->AsObject();
		const int32 LayerId = LayerNode->GetIntegerField("id");
		const int32 LayerWidth = LayerNode->GetIntegerField("width");
		const int32 LayerHeight = LayerNode->GetIntegerField("height");

		const TArray<TSharedPtr<FJsonValue>> TilesArray = LayerNode->GetArrayField(TEXT("data"));
		const auto TileGIDs = LayerNode->GetArrayField(TEXT("data"));
		for (int32 TileIdx = 0; TileIdx < TileGIDs.Num(); TileIdx++)
		{
			const int32 TileX = TileIdx % LayerWidth;
			const int32 TileY = TileIdx / LayerWidth;
			uint32 TileGID = TileGIDs[TileIdx]->AsNumber();

			const unsigned FLIPPED_DIAGONALLY_FLAG = 0x20000000;
			const unsigned FLIPPED_VERTICALLY_FLAG = 0x40000000;
			const unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
			const bool DiagonalFlip = TileGID & FLIPPED_DIAGONALLY_FLAG;
			const bool VerticalFlip = TileGID & FLIPPED_VERTICALLY_FLAG;
			const bool HorizontalFlip = TileGID & FLIPPED_HORIZONTALLY_FLAG;
			TileGID &= ~(FLIPPED_HORIZONTALLY_FLAG | FLIPPED_VERTICALLY_FLAG | FLIPPED_DIAGONALLY_FLAG);

			float Yaw = 0.f;
			FVector Scale = FVector::OneVector;
			if (DiagonalFlip && VerticalFlip)
			{
				Yaw = -90.f;
			}
			else if (HorizontalFlip && DiagonalFlip)
			{
				Yaw = 90.f;
			}
			else if (HorizontalFlip && VerticalFlip)
			{
				Yaw = 180.f;
			}
			else if (bAllowFlipping)
			{
				Scale.X = HorizontalFlip ? -1.f : 1.f;
				Scale.Y = VerticalFlip ? -1.f : 1.f;
			}

			if (TileGID < FirstGID)
			{
				// No tile
				continue;
			}

			for (auto Tileset : Tilesets)
			{
				if (TileGID >= Tileset.Key)
				{
					TileGID -= Tileset.Key;
					const FName TileRowKey = FName(*FString::Printf(TEXT("%s_%d"), *Tileset.Value, TileGID));
					const FTiledMapMeshTableRow* MeshTableRow = MeshTable->FindRow<FTiledMapMeshTableRow>(TileRowKey, TEXT(""), /*bWarnIfRowMissing=*/false);

					if (MeshTableRow && MeshTableRow->StaticMesh)
					{
						const FString ObjectName = FString::Printf(TEXT("%d_%d"), LayerId, TileIdx);
						UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(this, FName(*ObjectName));
						StaticMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
						StaticMeshComponent->Mobility = EComponentMobility::Static;
						StaticMeshComponent->SetGenerateOverlapEvents(false);
						StaticMeshComponent->bUseDefaultCollision = true;
						StaticMeshComponent->SetupAttachment(RootComponent);
						StaticMeshComponent->SetRelativeTransform(FTransform(FRotator(0.f, Yaw, 0.f), FVector(TileSize.X * TileX, TileSize.Y * TileY, 0.f), Scale));
						StaticMeshComponent->SetStaticMesh(MeshTableRow->StaticMesh);
						StaticMeshComponent->SetBoundsScale(4.0f);
						StaticMeshComponent->RegisterComponent();
					}

					break;
				}
			}
		}
	}

	return true;
}

#if WITH_EDITOR
void ATiledMapActor::ImportMap()
{
	FString ErrorMessage;
	if (!InternalImportMap(MapPath, ErrorMessage))
	{
		FNotificationInfo NotificationInfo(FText::FromString(ErrorMessage));
		NotificationInfo.ExpireDuration = 3.0f;
		NotificationInfo.bFireAndForget = true;

		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(SNotificationItem::CS_Fail);
		}
	}
}
#endif // WITH_EDITOR
