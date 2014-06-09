// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MultiBox/MultiBoxExtender.h"
#include "ITextEditorWidget.h"

/** An editable text widget that supports multiple lines and soft word-wrapping. */
class SLATE_API SMultiLineEditableText : public SWidget, public ITextEditorWidget
{
public:
	SLATE_BEGIN_ARGS( SMultiLineEditableText )
		: _Text()
		, _WrapTextAt( 0.0f )
		, _AutoWrapText(false)
		, _TextStyle( &FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>( "NormalText" ) )
		, _Font()
		, _Margin( FMargin() )
		, _LineHeightPercentage( 1.0f )
		, _Justification( ETextJustify::Left )
		, _IsReadOnly(false)
		, _OnTextChanged()
		, _OnTextCommitted()
		, _ContextMenuExtender()
	{}
		/** The initial text that will appear in the widget. */
		SLATE_ATTRIBUTE(FText, Text)

		/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
		SLATE_ATTRIBUTE(float, WrapTextAt)

		/** Whether to wrap text automatically based on the widget's computed horizontal space.  IMPORTANT: Using automatic wrapping can result
		    in visual artifacts, as the the wrapped size will computed be at least one frame late!  Consider using WrapTextAt instead.  The initial 
			desired size will not be clamped.  This works best in cases where the text block's size is not affecting other widget's layout. */
		SLATE_ATTRIBUTE(bool, AutoWrapText)

		/** Pointer to a style of the text block, which dictates the font, color, and shadow options. */
		SLATE_STYLE_ARGUMENT(FTextBlockStyle, TextStyle)

		/** Font color and opacity (overrides Style) */
		SLATE_ATTRIBUTE(FSlateFontInfo, Font)

		/** The amount of blank space left around the edges of text area. */
		SLATE_ATTRIBUTE(FMargin, Margin)

		/** The amount to scale each lines height by. */
		SLATE_ATTRIBUTE(float, LineHeightPercentage)

		/** How the text should be aligned with the margin. */
		SLATE_ATTRIBUTE(ETextJustify::Type, Justification)

		/** Sets whether this text box can actually be modified interactively by the user */
		SLATE_ATTRIBUTE(bool, IsReadOnly)

		/** Called whenever the text is changed interactively by the user */
		SLATE_EVENT(FOnTextChanged, OnTextChanged)

		/** Called whenever the text is committed.  This happens when the user presses enter or the text box loses focus. */
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)

		/** Menu extender for the right-click context menu */
		SLATE_EVENT(FMenuExtensionDelegate, ContextMenuExtender)

	SLATE_END_ARGS()

	SMultiLineEditableText();

	void Construct( const FArguments& InArgs );

	/**
	 * Gets the text assigned to this text block
	 */
	const FText& GetText() const
	{
		return BoundText.Get();
	}

	/**
	 * Sets the text for this text block
	 */
	void SetText(const TAttribute< FText >& InText);

	virtual bool GetIsReadOnly() const OVERRIDE;
	virtual void ClearSelection() OVERRIDE;

private:
	
	enum class ECursorAlignment : uint8
	{
		/** Visually align the cursor to the left of the character its placed at, and insert text before the character */
		Left,

		/** Visually align the cursor to the right of the character its placed at, and insert text after the character */
		Right,
	};

	enum class ECursorEOLMode : uint8
	{
		/** The cursor considers hard-line endings when looking for an EOL to align the cursor to (ignores line wrapping) */
		HardLines,

		/** The cursor considers soft-line endings when looking for an EOL to align the cursor to (checks for line wrapping) */
		SoftLines,
	};

	/** Store the information about the current cursor position */
	class FCursorInfo
	{
	public:
		FCursorInfo()
			: CursorPosition()
			, CursorAlignment(ECursorAlignment::Left)
			, LastCursorInteractionTime(0)
		{
		}

		/** Get the literal position of the cursor (note: this may not be the correct place to insert text to, use GetCursorInteractionLocation for that) */
		FORCEINLINE FTextLocation GetCursorLocation() const
		{
			return CursorPosition;
		}

		/** Get the alignment of the cursor */
		FORCEINLINE ECursorAlignment GetCursorAlignment() const
		{
			return CursorAlignment;
		}

		/** Get the interaction position of the cursor (where to insert, delete, etc, text from/to) */
		FORCEINLINE FTextLocation GetCursorInteractionLocation() const
		{
			// If the cursor is right aligned, we treat it as if it were one character further along for insert/delete purposes
			return FTextLocation(CursorPosition, (CursorAlignment == ECursorAlignment::Right) ? 1 : 0);
		}

		FORCEINLINE double GetLastCursorInteractionTime() const
		{
			return LastCursorInteractionTime;
		}

		/** Set the position of the cursor, and then work out the correct alignment based on the current text layout */
		void SetCursorLocationAndCalculateAlignment(const TSharedPtr<FTextLayout>& TextLayout, const FTextLocation& InCursorPosition, const ECursorEOLMode CursorEOLMode = ECursorEOLMode::HardLines);

		/** Set the literal position and alignment of the cursor */
		void SetCursorLocationAndAlignment(const FTextLocation& InCursorPosition, const ECursorAlignment InCursorAlignment);

		/** Create an undo for this cursor data */
		FCursorInfo CreateUndo() const;

		/** Restore this cursor data from an undo */
		void RestoreFromUndo(const FCursorInfo& UndoData);

	private:
		/** Current cursor position; there is always one. */
		FTextLocation CursorPosition;

		/** Cursor alignment (horizontal) within its position. This affects the rendering and behavior of the cursor when adding new characters */
		ECursorAlignment CursorAlignment;

		/** Last time the user did anything with the cursor.*/
		double LastCursorInteractionTime;
	};

	/**
	* Stores a single undo level for editable text
	*/
	class FUndoState
	{

	public:

		/** Text */
		FText Text;

		/** Selection state */
		TOptional<FTextLocation> SelectionStart;

		/** Cursor data */
		FCursorInfo CursorInfo;

	public:

		FUndoState()
			: Text()
			, SelectionStart()
			, CursorInfo()
		{
		}
	};

	class FSlateCursorRunHighlighter : public ISlateRunHighlighter
	{
	public:

		static TSharedRef< FSlateCursorRunHighlighter > Create(const FCursorInfo* InCursorInfo);

	public:

		enum class EMode : uint8
		{
			Cursor,
			Selection,
		};

	public:

		virtual ~FSlateCursorRunHighlighter() {}

		virtual int32 OnPaint( const FTextLayout::FLineView& Line, const TSharedRef< ISlateRun >& Run, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

		virtual FChildren* GetChildren() OVERRIDE;

		virtual void OnArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;

		void SetMode( EMode Mode );

	private:

		FSlateCursorRunHighlighter(const FCursorInfo* InCursorInfo);

	private:

		/** Cursor data that this highlighter is tracking */
		const FCursorInfo* CursorInfo;

		EMode Mode;

		static FNoChildren NoChildrenInstance;
	};

private:

	// BEGIN ITextEditorWidget interface
	virtual void StartChangingText() OVERRIDE;
	virtual void FinishChangingText() OVERRIDE;
	virtual void BackspaceChar() OVERRIDE;
	virtual void DeleteChar() OVERRIDE;
	virtual bool CanTypeCharacter(const TCHAR CharInQuestion) const OVERRIDE;
	virtual void TypeChar( const int32 Character ) OVERRIDE;
	virtual FReply MoveCursor( ECursorMoveMethod::Type Method, const FVector2D& Direction, ECursorAction::Type Action ) OVERRIDE;
	virtual void JumpTo(ETextLocation::Type JumpLocation, ECursorAction::Type Action) OVERRIDE;
	virtual void SelectAllText() OVERRIDE;
	virtual bool SelectAllTextWhenFocused() OVERRIDE;
	virtual void SelectWordAt(const FVector2D& LocalPosition) OVERRIDE;
	virtual void BeginDragSelection() OVERRIDE;
	virtual bool IsDragSelecting() const OVERRIDE;
	virtual void EndDragSelection() OVERRIDE;
	virtual bool AnyTextSelected() const OVERRIDE;
	virtual bool IsTextSelectedAt(const FVector2D& LocalPosition) const OVERRIDE;
	virtual void SetWasFocusedByLastMouseDown( bool Value ) OVERRIDE;
	virtual bool WasFocusedByLastMouseDown() const OVERRIDE;
	virtual void SetHasDragSelectedSinceFocused( bool Value ) OVERRIDE;
	virtual bool HasDragSelectedSinceFocused() const OVERRIDE;
	virtual FReply OnEscape() OVERRIDE;
	virtual void OnEnter() OVERRIDE;
	virtual bool CanExecuteCut() const OVERRIDE;
	virtual void CutSelectedTextToClipboard() OVERRIDE;
	virtual bool CanExecuteCopy() const OVERRIDE;
	virtual void CopySelectedTextToClipboard() OVERRIDE;
	virtual bool CanExecutePaste() const OVERRIDE;
	virtual void PasteTextFromClipboard() OVERRIDE;
	virtual bool CanExecuteUndo() const OVERRIDE;
	virtual void Undo() OVERRIDE;
	virtual void Redo() OVERRIDE;
	virtual TSharedRef< SWidget > GetWidget() OVERRIDE;
	virtual void SummonContextMenu( const FVector2D& InLocation ) OVERRIDE;
	virtual void LoadText() OVERRIDE;
	// END ITextEditorWidget interface


private:
	// BEGIN SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual void CacheDesiredSize() OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual FChildren* GetChildren() OVERRIDE;
	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
	virtual bool SupportsKeyboardFocus() const OVERRIDE;
	virtual FReply OnKeyChar( const FGeometry& MyGeometry,const FCharacterEvent& InCharacterEvent ) OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual FReply OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual void OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const OVERRIDE;
	// END SWidget interface

private:
	
	void OnWindowClosed(const TSharedRef<SWindow>&);

	/** Remember where the cursor was when we started selecting. */
	void BeginSelecting( const FTextLocation& Endpoint );

	void DeleteSelectedText();

	void ExecuteDeleteAction();
	bool CanExecuteDelete() const;

	bool CanExecuteSelectAll() const;

	bool DoesClipboardHaveAnyText() const;

	/**
	 * Given a location and a Direction to offset, return a new location.
	 *
	 * @param Location    Cursor location from which to offset
	 * @param Direction   Positive means right, negative means left.
	 */
	FTextLocation TranslatedLocation( const FTextLocation& CurrentLocation, int8 Direction ) const;

	/**
	 * Given a location and a Direction to offset, return a new location.
	 *
	 * @param Location    Cursor location from which to offset
	 * @param Direction   Positive means down, negative means up.
	 */
	FTextLocation TranslateLocationVertical( const FTextLocation& Location, int8 Direction ) const;

	/** Find the closest word boundary */
	FTextLocation ScanForWordBoundary( const FTextLocation& Location, int8 Direction ) const; 

	/** Get the character at Location */
	TCHAR GetCharacterAt( const FTextLocation& Location ) const;

	/** Are we at the beginning of all the text. */
	bool IsAtBeginningOfDocument( const FTextLocation& Location ) const;
	
	/** Are we at the end of all the text. */
	bool IsAtEndOfDocument( const FTextLocation& Location ) const;

	/** Is this location the beginning of a line */
	bool IsAtBeginningOfLine( const FTextLocation& Location ) const;

	/** Is this location the end of a line. */
	bool IsAtEndOfLine( const FTextLocation& Location ) const;

	/** Are we currently at the beginning of a word */
	bool IsAtWordStart( const FTextLocation& Location ) const;

	void UpdateCursorHighlight();

	void RemoveCursorHighlight();

	void PushUndoState(const SMultiLineEditableText::FUndoState& InUndoState);

	void ClearUndoStates();

	void MakeUndoState(SMultiLineEditableText::FUndoState& OutUndoState);

	void SaveText(const FText& TextToSave);

private:

	/** The text displayed in this text block */
	TAttribute<FText> BoundText;

	/** The text that is currently in TextLayout; used to avoid un-needed updates */
	TOptional<FText> LastSetText;

	/** In control of the layout and wrapping of the BoundText */
	TSharedPtr< FSlateTextLayout > TextLayout;

	/** Default style used by the TextLayout */
	FTextBlockStyle TextStyle;
	
	/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
	TAttribute< float > WrapTextAt;

	/** True if we're wrapping text automatically based on the computed horizontal space for this widget */
	TAttribute< bool > AutoWrapText;

	/** Cached auto-wrap width that this text is using. This is used when determining if the cached string size should be updated */
	mutable float CachedAutoWrapTextWidth;

	TAttribute< FMargin > Margin;
	TAttribute< ETextJustify::Type > Justification; 
	TAttribute< float > LineHeightPercentage;

	TSharedPtr< FSlateCursorRunHighlighter > CursorSelectionHighlighter;

	/** That start of the selection when there is a selection. The end is implicitly wherever the cursor happens to be. */
	TOptional<FTextLocation> SelectionStart;

	/** Current cursor data */
	FCursorInfo CursorInfo;

	/** The user probably wants the cursor where they last explicitly positioned it horizontally. */
	float PreferredCursorScreenOffsetInLine;

	/** Whether to select all text when the user clicks to give focus on the widget */
	TAttribute< bool > bSelectAllTextWhenFocused;

	/** True if we're currently selecting text by dragging the mouse cursor with the left button held down */
	bool bIsDragSelecting;

	/** True if the last mouse down caused us to receive keyboard focus */
	bool bWasFocusedByLastMouseDown;

	/** True if characters were selected by dragging since the last keyboard focus.  Used for text selection. */
	bool bHasDragSelectedSinceFocused;

	/** Undo states */
	TArray< FUndoState > UndoStates;

	/** Current undo state level that we've rolled back to, or INDEX_NONE if we haven't undone.  Used for 'Redo'. */
	int32 CurrentUndoLevel;

	/** Undo state that will be pushed if text is actually changed between calls to StartChangingText() and FinishChangingText() */
	FUndoState StateBeforeChangingText;

	/** True if we're currently (potentially) changing the text string */
	bool bIsChangingText;

	/** Sets whether this text box can actually be modified interactively by the user */
	TAttribute< bool > IsReadOnly;

	/** A list commands to execute if a user presses the corresponding keybinding in the text box */
	TSharedRef< FUICommandList > UICommandList;

	/** Called whenever the text is changed interactively by the user */
	FOnTextChanged OnTextChanged;

	/** Called whenever the text is committed.  This happens when the user presses enter or the text box loses focus. */
	FOnTextCommitted OnTextCommitted;

	/** Menu extender for right-click context menu */
	TSharedPtr<FExtender> MenuExtender;

	/** Weak pointer to context menu window that's currently open, if there is one */
	TWeakPtr< SWindow > ContextMenuWindow;
};
