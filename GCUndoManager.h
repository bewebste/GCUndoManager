//
//  GCUndoManager.h
//  GCDrawKit
//
//  Created by graham on 4/12/09.
//  Copyright 2009-2011 Apptree.net. All rights reserved.
//

// VERSION HISTORY:
// 2009/12/09 - first release
// 2010/01/01 - fixes for Core Data, optional retaining of targets, and prevention of reentrancy when removing tasks
// 2011/01/11 - fix to ensure submitting tasks in response to a checkpoint notification is correctly handled
// 2011/07/08 - added NSUndoManagerDidCloseUndoGroupNotification for 10.7 (Lion) compatibility

#import <Cocoa/Cocoa.h>
#import <BWViewControllers/BWForwardingProxy.h>

// internal undo manager state is one of these constants

typedef enum
{
	kGCUndoCollectingTasks		= 0,
	kGCUndoIsUndoing			= 1,
	kGCUndoIsRedoing			= 2
}
GCUndoManagerState;

typedef enum
{
	kGCCoalesceLastTask			= 0,
	kGCCoalesceAllMatchingTasks	= 1
}
GCUndoTaskCoalescingKind;


@class GCUndoGroup, GCUndoManagerProxy, GCConcreteUndoTask;

// the undo manager is a public-API compatible replacement for NSUndoManager but features a simpler internal implementation, some bug fixes and less
// fragility than NSUndoManager. It can be used with NSDocument's -setUndoManager: method (cast to id or NSUndoManager). However its compatibility with
// Core Data is unknown and untested at this time. See further notes at the end of this file.


@interface GCUndoManager : NSObject <BWForwardingProxyDelegate>
{
@private
	NSMutableArray*		mUndoStack;				// list of groups making up the undo stack
	NSMutableArray*		mRedoStack;				// list of groups making up the redo stack
	NSArray*			mRunLoopModes;			// current run loop modes, used by automatic grouping by event
	id					mNextTarget;			// next prepared target
	GCUndoGroup*		mOpenGroupRef;			// internal reference to current open group
	GCUndoManagerProxy*	mProxy;					// the proxy object returned by -prepareWithInvocationTarget: if proxying is used
	NSInteger			mGroupLevel;			// current grouping level, 0 = no groups open
	NSUInteger			mLevelsOfUndo;			// how many undo actions are added before old ones are discarded, 0 = unlimited
	NSInteger			mEnableLevel;			// enable ref count, 0 = enabled.
	NSInteger			mChangeCount;			// count of changes (submitting any task increments this)
	GCUndoManagerState	mState;					// current undo manager state
	GCUndoTaskCoalescingKind mCoalKind;			// coalescing behaviour - match on most recent task or all tasks in group
	id					mDelegate;
	NSMutableDictionary* mForwardingProxies;
	BOOL				mGroupsByEvent;			// YES if automatic grouping occurs for the main loop event cycle
	BOOL				mCoalescing;			// YES if consecutive tasks are coalesced
	BOOL				mAutoDeleteEmptyGroups;	// YES if empty groups are automatically removed from the stack
	BOOL				mRetainsTargets;		// YES if invocation targets are retained
	BOOL				mIsRemovingTargets;		// YES during stack clean-up to prevent re-entrancy
	BOOL				mIsInCheckpoint;		// YES during the notification processing for a checkpoint
}

// NSUndoManager compatible API
// undo groups

- (void)				beginUndoGrouping;
- (void)				endUndoGrouping;

- (NSInteger)			groupingLevel;
- (BOOL)				groupsByEvent;
- (void)				setGroupsByEvent:(BOOL) groupByEvent;

- (NSArray*)			runLoopModes;
- (void)				setRunLoopModes:(NSArray*) modes;

// enabling undo registration

- (void)				enableUndoRegistration;
- (void)				disableUndoRegistration;
- (BOOL)				isUndoRegistrationEnabled;

// setting the number of undos allowed before old ones are discarded

- (NSUInteger)			levelsOfUndo;
- (void)				setLevelsOfUndo:(NSUInteger) levels;

// performing the undo or redo

- (BOOL)				canUndo;
- (BOOL)				canRedo;

- (void)				undo;
- (void)				redo;
- (void)				undoNestedGroup;

- (BOOL)				isUndoing;
- (BOOL)				isRedoing;

// undo menu management

- (void)				setActionName:(NSString*) actionName;
- (NSString*)			undoActionName;
- (NSString*)			redoActionName;
- (NSString*)			undoMenuItemTitle;
- (NSString*)			redoMenuItemTitle;
- (NSString*)			undoMenuTitleForUndoActionName:(NSString*) actionName;
- (NSString*)			redoMenuTitleForUndoActionName:(NSString*) actionName;

// for supporting the Versions Browser in Mac OS 10.7

- (void)                setActionIsDiscardable:(BOOL) discardable;

// registering actions with the undo manager

- (id)					prepareWithInvocationTarget:(id) target;
- (void)				forwardInvocation:(NSInvocation*) invocation;
- (void)				registerUndoWithTarget:(id) target selector:(SEL) selector object:(id) anObject;

// removing actions

- (void)				removeAllActions;
- (void)				removeAllActionsWithTarget:(id) target;

// private NSUndoManager API for compatibility

- (void)				_processEndOfEventNotification:(NSNotification*) note;

#pragma mark -
// additional API
// automatic empty group discarding (default = YES)

- (void)				setAutomaticallyDiscardsEmptyGroups:(BOOL) autoDiscard;
- (BOOL)				automaticallyDiscardsEmptyGroups;

// task coalescing (default = NO)

- (void)				enableUndoTaskCoalescing;
- (void)				disableUndoTaskCoalescing;
- (BOOL)				isUndoTaskCoalescingEnabled;

- (void)				setCoalescingKind:(GCUndoTaskCoalescingKind) kind;
- (GCUndoTaskCoalescingKind) coalescingKind;

// retaining targets (default = NO)

- (void)				setRetainsTargets:(BOOL) retainsTargets;
- (BOOL)				retainsTargets;
- (void)				setNextTarget:(id) target;

// getting/resetting change count

- (NSInteger)			changeCount;
- (void)				resetChangeCount;

- (id)					delegate;
- (void)				setDelegate:(id)newDelegate;

- (id)					forwardingProxyForKey:(NSString*)inKey create:(BOOL)createFlag;

#pragma mark -
// internal methods - public to permit overriding

- (GCUndoGroup*)		currentGroup;

- (NSArray*)			undoStack;
- (NSArray*)			redoStack;

- (GCUndoGroup*)		peekUndo;
- (GCUndoGroup*)		peekRedo;
- (NSUInteger)			numberOfUndoActions;
- (NSUInteger)			numberOfRedoActions;

- (void)				pushGroupOntoUndoStack:(GCUndoGroup*) aGroup;
- (void)				pushGroupOntoRedoStack:(GCUndoGroup*) aGroup;

- (BOOL)				submitUndoTask:(GCConcreteUndoTask*) aTask;

- (void)				popUndoAndPerformTasks;
- (void)				popRedoAndPerformTasks;
- (GCUndoGroup*)		popUndo;
- (GCUndoGroup*)		popRedo;

- (void)				clearRedoStack;
- (void)				checkpoint;

- (GCUndoManagerState)	undoManagerState;
- (void)				setUndoManagerState:(GCUndoManagerState) aState;
- (void)				reset;

- (void)				conditionallyBeginUndoGrouping;

// debugging utility:

- (void)				explodeTopUndoAction;

@end

@interface NSObject(GCUndoManagerDelegate)

- (id)undoManager:(GCUndoManager*)inUndoManager forwardingTargetForKey:(NSString*)inKey;

@end

#pragma mark -

// undo tasks (actions) come in two types - groups and concrete tasks. Both descend from the same semi-abstract base which
// provides the 'back pointer' to the parent group. The -perform method must be overridden by concrete subclasses.


@interface GCUndoTask : NSObject
{
@private
	GCUndoGroup*		mGroupRef;
}

- (GCUndoGroup*)		parentGroup;
- (void)				setParentGroup:(GCUndoGroup*) parent;
- (void)				perform;

@end


#pragma mark -

// undo groups can contain any number of other groups or concrete tasks. The top level actions in the undo/redo stacks always consist
// of groups, even if they only contain a single concrete task. The group also provides the storage for the action name associated with
// the action, and whether or not the group is discardable. Groups own their tasks.

@interface GCUndoGroup	: GCUndoTask
{
@private
	NSString*			mActionName;
	NSMutableArray*		mTasks;
	BOOL                mActionIsDiscardable;
}

- (void)				addTask:(GCUndoTask*) aTask;
- (GCUndoTask*)			taskAtIndex:(NSUInteger) indx;
- (GCConcreteUndoTask*)	lastTaskIfConcrete;
- (NSArray*)			tasks;
- (NSArray*)			tasksWithTarget:(id) target selector:(SEL) selector;
- (BOOL)				isEmpty;

- (void)				removeTasksWithTarget:(id) aTarget undoManager:(GCUndoManager*) um;
- (void)				setActionName:(NSString*) name;
- (NSString*)			actionName;

//  set the latest undo action to discardable if it may be safely discarded when a document can not be saved for any reason. 
//  an example might be an undo action that changes the viewable area of a document.   To find out if an undo group contains only
//  discardable actions, look for the NSUndoManagerGroupIsDiscardableKey in the userInfo dictionary of the
//  NSUndoManagerDidCloseUndoGroupNotification.
- (void)                setActionIsDiscardable:(BOOL) discardable;
- (BOOL)                actionIsDiscardable;

@end


#pragma mark -

// concrete tasks wrap the NSInvocation which embodies the actual method call that is made when an action is undone or redone.
// Concrete tasks own the invocation, which is set to optionally retain its target and arguments.

@interface GCConcreteUndoTask : GCUndoTask
{
@private
	NSInvocation*		mInvocation;
	id					mTarget;
	BOOL				mTargetRetained;
}

- (id)					initWithInvocation:(NSInvocation*) inv;
- (id)					initWithTarget:(id) target selector:(SEL) selector object:(id) object;
- (void)				setTarget:(id) target retained:(BOOL) retainIt;
- (id)					target;
- (SEL)					selector;
- (BOOL)				targetRetained;

@end

// macros to throw exceptions (similar to NSAssert but always compiled in)

#ifndef THROW_IF_FALSE
#define THROW_IF_FALSE( condition, string )						if(!(condition)){[NSException raise:NSInternalInconsistencyException format:(string)];}
#define THROW_IF_FALSE1( condition, string, param1 )			if(!(condition)){[NSException raise:NSInternalInconsistencyException format:(string), (param1)];}
#define THROW_IF_FALSE2( condition, string, param1, param2 )	if(!(condition)){[NSException raise:NSInternalInconsistencyException format:(string), (param1), (param2)];}
#endif

/*
 
This class is a public API-compatible replacement for NSUndoManager. It can only be used with AppKit however, not with other types of executables.
 
The point of this is to provide an undo manager whose source is openly readable, available and debuggable. It also does not exhibit the
 NSUndoManager bug whereby opening and closing a group without adding any tasks creates an empty task. That substantially simplifies how
 it can be used in an interactive situation such as handling the mouse down/drag/up triplet of events.
 
 This also includes task coalescing whereby consecutive tasks having the same target and selector are only submitted to the stack once. This
 helps a lot with interactive tasks involving multiple events such as mouse dragging, so that undo does not replay all the intermediate steps.
 
 Instances of this can be used as well as NSUndoManager if required. This handles all of its own event loop observing and automatic open
 and close of groups independently of the standard mechanism.
 
 Otherwise this should behave identically to NSUndoManager when used in an application, except as noted below.
 
 The sending of notifications is not quite as it appears to be documented for NSUndoManager. If you implement as documented, the
 change count for NSDocument is not managed correctly. Instead, this sends notifications in a manner that appears to be what NSUndoManager
 actually does, and so NSDocument change counts work as they should. Also, the purpose and exact usage of NSCheckPointNotification is
 unclear so while this follows the documentation, any code relying on this vague notification might not work correctly.
 
 -undoNestedGroup only operates on top level groups in this implementation, and is thus functionally equivalent to -undo. In fact -undo simply
 calls -undoNestedGroup here.
 
 */
