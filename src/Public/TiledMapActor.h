// This source code is licensed under the MIT license found in the LICENSE file in the root directory of this source tree.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"

#include "TiledMapActor.generated.h"

class UStaticMesh;
class UBillboardComponent;

USTRUCT()
struct EGG_API FTiledMapMeshTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMesh* StaticMesh;
};

UCLASS(hidecategories=(Movement, Advanced, Collision, Display, Actor, Attachment, Input))
class EGG_API ATiledMapActor : public AActor
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
private:
	// Reference to actor sprite
	UBillboardComponent* SpriteComponent;
#endif

public:

	ATiledMapActor();

	/** Path of the JSON format map relative to the project's Content folder. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tiled Map")
	FString MapPath;

	/** Table mapping tile indices to static meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tiled Map")
	UDataTable* MeshTable;

	/** Size of static meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tiled Map")
	FVector2D TileSize;

	/** Allow meshes to be flipped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tiled Map")
	bool bAllowFlipping;

	/** Returns SpriteComponent subobject **/
	UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }

#if WITH_EDITOR
	/** Read the map and spawn actors. Children of this actor will be destroyed. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Tiled Map")
	void ImportMap();
#endif

protected:

	bool InternalImportMap(const FString& Filepath, FString& ErrorMessage);
};
