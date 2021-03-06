// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionDDX.generated.h"


UCLASS()
class UMaterialExpressionDDX : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** The value we want to compute ddx/ddy from */
	UPROPERTY()
	FExpressionInput Value;


	//~ Begin UMaterialExpression Interface
#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif
	//~ End UMaterialExpression Interface
};



