/* *  main.c *  OpenGL Image * *  Created by ggs on Fri May 11 2001.	Copyright:	Copyright � 2001 Apple Computer, Inc., All Rights Reserved	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.				("Apple") in consideration of your agreement to the following terms, and your				use, installation, modification or redistribution of this Apple software				constitutes acceptance of these terms.  If you do not agree with these terms,				please do not use, install, modify or redistribute this Apple software.				In consideration of your agreement to abide by the following terms, and subject				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s				copyrights in this original Apple software (the "Apple Software"), to use,				reproduce, modify and redistribute the Apple Software, with or without				modifications, in source and/or binary forms; provided that if you redistribute				the Apple Software in its entirety and without modifications, you must retain				this notice and the following text and disclaimers in all such redistributions of				the Apple Software.  Neither the name, trademarks, service marks or logos of				Apple Computer, Inc. may be used to endorse or promote products derived from the				Apple Software without specific prior written permission from Apple.  Except as				expressly stated in this notice, no other rights or licenses, express or implied,				are granted by Apple herein, including but not limited to any patent rights that				may be infringed by your derivative works or by other works in which the Apple				Software may be incorporated.				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN				COMBINATION WITH YOUR PRODUCTS.				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. * */  // Note:  The interface code in this file, while very reasonable, is not designed to be production quality.  It is design to support this particular sample. //        The code of interest for this particular sample is contained in the files: OpenGL_Image_Utilities.h and OpenGL_Image_Utilities.c #ifdef __APPLE_CC__	#include <Carbon/Carbon.h>#else	#include <CarbonEvents.h> 	#include <IBCarbonRuntime.h>	#include <Debugging.h>	#include <Gestalt.h>#endif#include "OpenGL_Image.h"#include "Carbon_Error_Handler.h"// ==================================enum{	kFileMenu = 400,	kCloseItem = 2,	kImageMenu = 501,	kRotateitem = 9,	kInfoItem = 11,	kLineItem,	kGridItem};EventHandlerUPP gEvtHandler;			// main event handlerEventHandlerUPP gWinEvtHandler;			// window event handlerfloat gZoomFactor = 1.1f;long gWindowCount = 0;// ==================================	static Boolean IsMacOSX (void);    static void HandleWindowUpdate(WindowRef window);    static pascal OSStatus myWindowEvtHndlr (EventHandlerCallRef myHandler, EventRef event, void* userData);    pascal void IdleTimer (EventLoopTimerRef inTimer, void* userData);    EventLoopTimerUPP GetTimerUPP (void);    static pascal OSStatus myEvtHndlr (EventHandlerCallRef myHandler, EventRef event, void* userData);// ==================================// determine via gestalt if we are running on Mac OS Xstatic Boolean IsMacOSX (void){	UInt32 response;	if ((Gestalt (gestaltSystemVersion, (SInt32 *) &response) == noErr) && (response >= 0x01000))		return true;	else		return false;}// ---------------------------------// sets up ports fro all window update functions passingthe window ref on to the draing functionstatic void HandleWindowUpdate(WindowRef window){    DrawGL (window); // draw content}// ---------------------------------// per window event handler, userData will contain WindowRef for window that event is forstatic pascal OSStatus myWindowEvtHndlr (EventHandlerCallRef myHandler, EventRef event, void* userData){#pragma unused (myHandler)    WindowRef			window = (WindowRef) userData;    OSStatus			result = eventNotHandledErr;    if (GetEventKind(event) == kEventWindowDrawContent)    {        GetEventParameter (event, kEventParamDirectObject, typeWindowRef, NULL, sizeof(WindowRef), NULL, &window);        HandleWindowUpdate(window);        result = noErr;    }    else if (GetEventKind(event) == kEventWindowClose)    {		pRecImage pWindowInfo = NULL;		if (window)		{			pWindowInfo = (pRecImage) GetWRefCon (window);			if ((pWindowInfo) && (pWindowInfo->spinning))				StopRotation (window);			DisposeGLForWindow (window);			DisposeWindow (window);			window = NULL;			gWindowCount--;			if (gWindowCount <= 0)			{				DisableMenuItem (GetMenuHandle (kImageMenu), 0);				DisableMenuItem (GetMenuHandle (kFileMenu), kCloseItem);				InvalMenuBar ();			}		}		result = noErr;    }    else if (GetEventKind(event) == kEventWindowShowing)    {        result = BuildGLForWindow (window);    }    else if ((GetEventKind(event) == kEventWindowResizeCompleted) || (GetEventKind(event) == kEventWindowDragCompleted))    {        result = ResizeMoveGLWindow (window);    }    else if (GetEventKind(event) == kEventMouseUp)    {        Point dragPoint;        GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &dragPoint);        MouseUpGLWindow ();    }    else if (GetEventKind(event) == kEventMouseDown)    {        Point dragPoint;        UInt32 modifiers;        GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &dragPoint);        GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);        MouseDownGLWindow (window, dragPoint, modifiers);    }    else if (GetEventKind(event) == kEventMouseDragged)    {        Point dragPoint;        GetEventParameter(event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &dragPoint);        DragGLWindow (dragPoint);    }    else if (GetEventKind(event) == kEventWindowZoomed)    {        ResizeMoveGLWindow (window);    }    return result;}// ---------------------------------// application event handler, handles menu items for the most partstatic pascal OSStatus myEvtHndlr (EventHandlerCallRef myHandler, EventRef event, void* userData){#pragma unused (myHandler, userData)    OSStatus result = eventNotHandledErr;    EventHandlerRef	ref;    EventTypeSpec	list[] = { { kEventClassWindow, kEventWindowShowing },                               { kEventClassWindow, kEventWindowClose },                               { kEventClassWindow, kEventWindowDrawContent },                               { kEventClassMouse, kEventMouseUp },                               { kEventClassMouse, kEventMouseDown },                               { kEventClassMouse, kEventMouseDragged },                               { kEventClassWindow, kEventWindowResizeCompleted },                               { kEventClassWindow, kEventWindowDragCompleted },                               { kEventClassWindow, kEventWindowZoomed} };    if (GetEventKind(event) == kEventMenuOpening)    {        pRecImage pWindowInfo = NULL;        WindowRef window = FrontWindow ();        if (window)        {            pWindowInfo = (pRecImage) GetWRefCon (window);            if (pWindowInfo)            {                CheckMenuItem(GetMenuHandle (kImageMenu), kRotateitem, pWindowInfo->spinning);                CheckMenuItem(GetMenuHandle (kImageMenu), kLineItem, pWindowInfo->lines);                CheckMenuItem(GetMenuHandle (kImageMenu), kInfoItem, pWindowInfo->info);                CheckMenuItem(GetMenuHandle (kImageMenu), kGridItem, pWindowInfo->grid);            }        }    }    else if (GetEventKind(event) == kEventProcessCommand)    {        HICommand command;        Rect rectPort;        pRecImage pWindowInfo = NULL;        WindowRef window = FrontWindow ();        if (window)        {            GetWindowPortBounds (window, &rectPort);            pWindowInfo = (pRecImage) GetWRefCon (window);        }        GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command);                if (command.commandID == 'opnf')        {            IBNibRef nibRef;            pWindowInfo = (recImage *) NewPtrClear (sizeof (recImage));            if (LoadImageForRecImage (pWindowInfo))            {            	if (IsMacOSX ())                	result = CreateNibReference (CFSTR ("main X"), &nibRef);                else                	result = CreateNibReference (CFSTR ("main 9"), &nibRef);                if (noErr == result)                    result = CreateWindowFromNib (nibRef, CFSTR ("MainWindow"), &window);                if ((window) && (noErr == result))                {                    SetWRefCon (window, (long) pWindowInfo);                    InstallWindowEventHandler (window, gWinEvtHandler, GetEventTypeCount (list), list, (void*) window, &ref);        			if (noErr != BuildGLForWindow (window))           			{        				DisposeGLForWindow (window);        				DisposeWindow (window);					}					else					{						SetWTitle (window, pWindowInfo->name);						ShowWindow (window);						gWindowCount++;						if (gWindowCount > 0)						{							EnableMenuItem (GetMenuHandle (kImageMenu), 0);							EnableMenuItem (GetMenuHandle (kFileMenu), kCloseItem);							InvalMenuBar ();						}					}				}            }        }        else if (command.commandID == 'clsf')        {			if (window)			{				pWindowInfo = (pRecImage) GetWRefCon (window);				if ((pWindowInfo) && (pWindowInfo->spinning))					StopRotation (window);				DisposeGLForWindow (window);				DisposeWindow (window);				window = NULL;				gWindowCount--;				if (gWindowCount <= 0)				{					DisableMenuItem (GetMenuHandle (kImageMenu), 0);					DisableMenuItem (GetMenuHandle (kFileMenu), kCloseItem);					InvalMenuBar ();				}			}        }        else if (pWindowInfo)        {            switch (command.commandID)            {                case 'zmin':                    pWindowInfo->zoom *= gZoomFactor;                    pWindowInfo->centerX *= gZoomFactor;                    pWindowInfo->centerY *= gZoomFactor;                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'zout':                    pWindowInfo->zoom /= gZoomFactor;                    pWindowInfo->centerX /= gZoomFactor;                    pWindowInfo->centerY /= gZoomFactor;                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'zwin':					{						float ztemp2 = (float) (rectPort.right - rectPort.left) / (float) pWindowInfo->imageWidth;						float ztemp1 = (float) (rectPort.bottom - rectPort.top) / (float) pWindowInfo->imageHeight;						if (ztemp2 < ztemp1)							pWindowInfo->zoom = ztemp2;						else							pWindowInfo->zoom = ztemp1;					}					pWindowInfo->centerX = (float) 0.0;					pWindowInfo->centerY = (float) 0.0;					pWindowInfo->rotation = (float) 0.0;					GetWindowPortBounds (window, &rectPort);					InvalWindowRect (window, &rectPort);					result = noErr;                    break;                case 'zt11':                    pWindowInfo->centerX /= pWindowInfo->zoom;                    pWindowInfo->centerY /= pWindowInfo->zoom;                    pWindowInfo->zoom = (float) 1.0;                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'tlin':                    pWindowInfo->lines = (unsigned char) (1 - pWindowInfo->lines);                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'tinf':                    pWindowInfo->info = (unsigned char) (1 - pWindowInfo->info);                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'grid':                    pWindowInfo->grid = (unsigned char) (1 - pWindowInfo->grid);                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'rocw':                    pWindowInfo->rotation += 90.0;                    while (pWindowInfo->rotation >= 360.0) pWindowInfo->rotation -= 360.0; // ensure within -360 to 360                    while (pWindowInfo->rotation <= -360.0) pWindowInfo->rotation += 360.0;                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'rccw':                    pWindowInfo->rotation -= 90.0;                    while (pWindowInfo->rotation >= 360.0) pWindowInfo->rotation -= 360.0; // ensure within -360 to 360                    while (pWindowInfo->rotation <= -360.0) pWindowInfo->rotation += 360.0;                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'rot0':                    pWindowInfo->rotation = (float) 0.0;                    GetWindowPortBounds (window, &rectPort);                    InvalWindowRect (window, &rectPort);                    result = noErr;                    break;                case 'spin':                    pWindowInfo->spinning = (unsigned char) (1 - pWindowInfo->spinning);                    if (pWindowInfo->spinning)						StartRotation (window);					else 						StopRotation (window);                    result = noErr;                    break;            }        }    }    return result;}// ==================================// application mainint main(int argc, char* argv[]){#pragma unused (argc, argv)    IBNibRef 		nibRef;        OSStatus		err;    EventHandlerRef	ref;    EventTypeSpec	list[] = { { kEventClassCommand,  kEventProcessCommand },                               { kEventClassMenu,  kEventMenuOpening } };    // Create a Nib reference passing the name of the nib file (without the .nib extension)    // CreateNibReference only searches into the application bundle.	if (IsMacOSX ())    	err = CreateNibReference (CFSTR("main X"), &nibRef);    else    	err = CreateNibReference (CFSTR("main 9"), &nibRef);    require_noerr( err, CantGetNibRef );        // Once the nib reference is created, set the menu bar. "MainMenu" is the name of the menu bar    // object. This name is set in InterfaceBuilder when the nib is created.    err = SetMenuBarFromNib(nibRef, CFSTR("MainMenu"));    require_noerr( err, CantSetMenuBar );	DisableMenuItem (GetMenuHandle (kImageMenu), 0);	DisableMenuItem (GetMenuHandle (kFileMenu), kCloseItem);	InvalMenuBar ();        gEvtHandler = NewEventHandlerUPP(myEvtHndlr);    InstallApplicationEventHandler (gEvtHandler, GetEventTypeCount (list), list, 0, &ref);    gWinEvtHandler = NewEventHandlerUPP(myWindowEvtHndlr);    // We don't need the nib reference anymore.    DisposeNibReference(nibRef);        // Call the event loop    RunApplicationEventLoop();CantSetMenuBar:CantGetNibRef:	return err;}