//================================================================================================
// Copyright Nikon Corporation - All rights reserved
//
// View this file in a non-proportional font, tabs = 3
//================================================================================================

#if defined( _WIN32 )
	#include <io.h>
	#include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/stat.h>

#include "Maid3.h"
#include "Maid3d1.h"
#include "CtrlSample.h"

extern ULONG	g_ulCameraType;	// CameraType
#define VIDEO_SIZE_BLOCK  0x500000		// movie data read block size : 5MB

#define ObjectBitmapHandle_Format_MOV	11	//MOV
#define ObjectBitmapHandle_Format_MP4	12	//MP4

BOOL g_bCancel = FALSE;

#if defined( _WIN32 )
	BOOL WINAPI	cancelhandler(DWORD dwCtrlType);
#elif defined(__APPLE__)
	void	cancelhandler(int sig);
#endif

//------------------------------------------------------------------------------------------------
//
SLONG CallMAIDEntryPoint( 
		LPNkMAIDObject	pObject,				// module, source, item, or data object
		ULONG				ulCommand,			// Command, one of eNkMAIDCommand
		ULONG				ulParam,				// parameter for the command
		ULONG				ulDataType,			// Data type, one of eNkMAIDDataType
		NKPARAM			data,					// Pointer or long integer
		LPNKFUNC			pfnComplete,		// Completion function, may be NULL
		NKREF				refComplete )		// Value passed to pfnComplete
{
	return (*(LPMAIDEntryPointProc)g_pMAIDEntryPoint)( 
						pObject, ulCommand, ulParam, ulDataType, data, pfnComplete, refComplete );
}
//------------------------------------------------------------------------------------------------
//
BOOL Search_Module( void* Path )
{
#if defined( _WIN32 )
	char	TempPath[MAX_PATH];
	struct	_finddata_t c_file;
	intptr_t	hFile;

	// Search a module file in the current directory.
	GetCurrentDirectory( MAX_PATH - 11, TempPath );
	strcat( TempPath, "\\Type0025.md3" );
	if ( (hFile = _findfirst( TempPath, &c_file )) == -1L ) {
		return FALSE;
	}
	strcpy( (char*)Path, TempPath );
	return TRUE;
#elif defined(__APPLE__)
    CFStringRef modulePath = NULL;
    
    CFStringRef moduleName = CFSTR( "Type0025 Module.bundle" );
    CFBundleRef bundle = CFBundleGetMainBundle();
    if(bundle != NULL)
    {
        CFURLRef executableURL = CFBundleCopyExecutableURL( bundle );
        if(executableURL != NULL)
        {
            CFURLRef parentFolderURL = CFURLCreateCopyDeletingLastPathComponent( kCFAllocatorDefault, executableURL );
            if(parentFolderURL != NULL)
            {
                CFURLRef moduleURL = CFURLCreateCopyAppendingPathComponent( kCFAllocatorDefault, parentFolderURL, moduleName, FALSE );
                if(moduleURL != NULL)
                {
                    if(CFURLResourceIsReachable( moduleURL, NULL ))
                    {
                        modulePath = CFURLCopyFileSystemPath( moduleURL, kCFURLPOSIXPathStyle );
                    }
                    CFRelease( moduleURL );
                }
                CFRelease( parentFolderURL );
            }
            CFRelease( executableURL );
        }
    }
    
    Boolean result = FALSE;
    if(modulePath != NULL)
    {
        result = CFStringGetCString( modulePath, (char *)Path, PATH_MAX, kCFStringEncodingUTF8 );
        CFRelease( modulePath );
    }
    return result;
#endif
}
//------------------------------------------------------------------------------------------------
//
BOOL Load_Module( void* Path )
{
#if defined( _WIN32 )
	g_hInstModule = LoadLibrary( (LPCSTR)Path );

	if (g_hInstModule) {
		g_pMAIDEntryPoint = (LPMAIDEntryPointProc)GetProcAddress( g_hInstModule, "MAIDEntryPoint" );
		if ( g_pMAIDEntryPoint == NULL )
			puts( "MAIDEntryPoint cannot be found.\n" ); 
	} else {
		g_pMAIDEntryPoint = NULL;
		printf( "\"%s\" cannot be opened.\n", (LPCSTR)Path );
	}
	return (g_hInstModule != NULL) && (g_pMAIDEntryPoint != NULL);
#elif defined(__APPLE__)
    CFStringRef modulePath = CFStringCreateWithCString( kCFAllocatorDefault, (const char *)Path, kCFStringEncodingUTF8 );
    if(modulePath != NULL)
    {
        CFURLRef moduleURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault, modulePath, kCFURLPOSIXPathStyle, TRUE );
        if(moduleURL != NULL)
        {
            if(gBundle != NULL)
            {
                CFRelease( gBundle );
                gBundle = NULL;
            }
            gBundle = CFBundleCreate( kCFAllocatorDefault, moduleURL );
            CFRelease( moduleURL );
        }
        CFRelease( modulePath );
    }
    
    if(gBundle == NULL)
    {
        return FALSE;
    }
    
	// Load and link dynamic CFBundle object
	if(!CFBundleLoadExecutable(gBundle))
    {
		CFRelease( gBundle );
		gBundle = NULL;
		return FALSE;
	}
    
	// Get entry point from BundleRef
	// Set the pointer for Maid entry point LPMAIDEntryPointProc type variabl
	g_pMAIDEntryPoint = (LPMAIDEntryPointProc)CFBundleGetFunctionPointerForName( gBundle, CFSTR("MAIDEntryPoint") );
	return (g_pMAIDEntryPoint != NULL);
#endif
}
//------------------------------------------------------------------------------------------------
//
BOOL Command_Open( NkMAIDObject* pParentObj, NkMAIDObject* pChildObj, ULONG ulChildID )
{
	SLONG lResult = CallMAIDEntryPoint( pParentObj, kNkMAIDCommand_Open, ulChildID, 
									kNkMAIDDataType_ObjectPtr, (NKPARAM)pChildObj, NULL, NULL );
	return lResult == kNkMAIDResult_NoError;
}
//------------------------------------------------------------------------------------------------
//
BOOL Command_Close( LPNkMAIDObject pObject )
{
	SLONG nResult = CallMAIDEntryPoint( pObject, kNkMAIDCommand_Close, 0, 0, 0, NULL, NULL );

	return nResult == kNkMAIDResult_NoError;
}
//------------------------------------------------------------------------------------------------
//
BOOL Close_Module( LPRefObj pRefMod )
{
	BOOL bRet;
	LPRefObj pRefSrc, pRefItm, pRefDat;
	ULONG i, j, k;

	if(pRefMod->pObject != NULL)
	{
		for(i = 0; i < pRefMod->ulChildCount; i ++)
		{
			pRefSrc = GetRefChildPtr_Index( pRefMod, i );
			for(j = 0; j < pRefSrc->ulChildCount; j ++)
			{
				pRefItm = GetRefChildPtr_Index( pRefSrc, j );
				for(k = 0; k < pRefItm->ulChildCount; k ++)
				{
					pRefDat = GetRefChildPtr_Index( pRefItm, k );
					bRet = ResetProc( pRefDat );
					if ( bRet == FALSE )	return FALSE;
					bRet = Command_Close( pRefDat->pObject );
					if ( bRet == FALSE )	return FALSE;
					free(pRefDat->pObject);
					free(pRefDat->pCapArray);
					free(pRefDat);//
					pRefDat = NULL;//
				}
				bRet = ResetProc( pRefItm );
				if ( bRet == FALSE )	return FALSE;
				bRet = Command_Close( pRefItm->pObject );
				if ( bRet == FALSE )	return FALSE;
				free(pRefItm->pObject);
				free(pRefItm->pRefChildArray);
				free(pRefItm->pCapArray);
				free(pRefItm);//
				pRefItm = NULL;//
			}
			bRet = ResetProc( pRefSrc );
			if ( bRet == FALSE )	return FALSE;
			bRet = Command_Close( pRefSrc->pObject );
			if ( bRet == FALSE )	return FALSE;
			free(pRefSrc->pObject);
			free(pRefSrc->pRefChildArray);
			free(pRefSrc->pCapArray);
			free(pRefSrc);//
			pRefSrc = NULL;//
		}
		bRet = ResetProc( pRefMod );
		if ( bRet == FALSE )	return FALSE;
		bRet = Command_Close( pRefMod->pObject );
		if ( bRet == FALSE )	return FALSE;
		free(pRefMod->pObject);
		pRefMod->pObject = NULL;

		free(pRefMod->pRefChildArray);
		pRefMod->pRefChildArray = NULL;
		pRefMod->ulChildCount = 0;
	
		free(pRefMod->pCapArray);
		pRefMod->pCapArray = NULL;
		pRefMod->ulCapCount = 0;
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------
//
void InitRefObj( LPRefObj pRef )
{
	pRef->pObject = NULL;
	pRef->lMyID = 0x8000;
	pRef->pRefParent = NULL;
	pRef->ulChildCount = 0;
	pRef->pRefChildArray = NULL;
	pRef->ulCapCount = 0;
	pRef->pCapArray = NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------
// issue async command while wait for the CompletionProc called.
BOOL IdleLoop( LPNkMAIDObject pObject, ULONG* pulCount, ULONG ulEndCount )
{
	BOOL bRet = TRUE;
	while( *pulCount < ulEndCount && bRet == TRUE ) {
		bRet = Command_Async( pObject );
	#if defined( _WIN32 )
		Sleep(10);
	#elif defined(__APPLE__)
		struct timespec t;
        t.tv_sec = 0;
        t.tv_nsec = 10 * 1000;// 10 msec == 10 * 1000 nsec
        nanosleep(&t, NULL);
	#endif
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Wait for Apple event. On MacOSX, the event from camera is an Apple event. 
void WaitEvent()
{
	#if defined( _WIN32 )
		// Do nothing
	#elif defined(__APPLE__)
		// Do nothing
	#endif
}
//------------------------------------------------------------------------------------------------------------------------------------
// enumerate capabilities belong to the object that 'pObject' points to.
BOOL EnumCapabilities( LPNkMAIDObject pObject, ULONG* pulCapCount, LPNkMAIDCapInfo* ppCapArray, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;

	do {
 		// call the module to get the number of the capabilities.
		ULONG	ulCount = 0L;
		LPRefCompletionProc pRefCompletion;
		// This memory block is freed in the CompletionProc.
		pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
		pRefCompletion->pulCount = &ulCount;
		pRefCompletion->pRef = NULL;
		nResult = CallMAIDEntryPoint(	pObject,
												kNkMAIDCommand_GetCapCount,
												0,
												kNkMAIDDataType_UnsignedPtr,
												(NKPARAM)pulCapCount,
												(LPNKFUNC)CompletionProc,
												(NKREF)pRefCompletion );
		IdleLoop( pObject, &ulCount, 1 );
 
 		if ( nResult == kNkMAIDResult_NoError )
 		{
 			// allocate memory for the capability array
 			*ppCapArray = (LPNkMAIDCapInfo)malloc( *pulCapCount * sizeof( NkMAIDCapInfo ) );
  
 			if ( *ppCapArray != NULL )
 			{
 				// call the module to get the capability array
   				ulCount = 0L;
				// This memory block is freed in the CompletionProc.
				pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
				pRefCompletion->pulCount = &ulCount;
				pRefCompletion->pRef = NULL;
				nResult = CallMAIDEntryPoint(	pObject,
														kNkMAIDCommand_GetCapInfo,
														*pulCapCount,
														kNkMAIDDataType_CapInfoPtr,
														(NKPARAM)*ppCapArray,
														(LPNKFUNC)CompletionProc,
														(NKREF)pRefCompletion );
				IdleLoop( pObject, &ulCount, 1 );

 				if (nResult == kNkMAIDResult_BufferSize)
 				{
					free( *ppCapArray );
					*ppCapArray = NULL;
				}
			}
		}
	}
	// repeat the process if the number of capabilites changed between the two calls to the module
	while (nResult == kNkMAIDResult_BufferSize);

	// return TRUE if the capabilities were successfully enumerated
	return (nResult == kNkMAIDResult_NoError || nResult == kNkMAIDResult_Pending);
}
//------------------------------------------------------------------------------------------------------------------------------------
// enumerate child object
BOOL EnumChildrten(LPNkMAIDObject pobject)
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_EnumChildren, 
											0,
											kNkMAIDDataType_Null,
											(NKPARAM)NULL,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return ( nResult == kNkMAIDResult_NoError );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_CapGetArray(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_CapGetArray,
											ulParam,
											ulDataType,
											pData,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return (nResult == kNkMAIDResult_NoError);
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_CapGetDefault(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_CapGetDefault,
											ulParam,
											ulDataType,
											pData,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return (nResult == kNkMAIDResult_NoError);
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_CapGet(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete )
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_CapGet, 
											ulParam,
											ulDataType,
											pData,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	return ( nResult == kNkMAIDResult_NoError );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_CapSet(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete )
{
	BOOL bSuccess = FALSE;
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(	pobject,
											kNkMAIDCommand_CapSet, 
											ulParam,
											ulDataType,
											pData,
											(LPNKFUNC)CompletionProc,
											(NKREF)pRefCompletion );
	IdleLoop( pobject, &ulCount, 1 );

	// MovRecInCardStatusの場合、Result Codesがゼロ(No Error)以外にも成功コードが存在する
	if (ulParam == kNkMAIDCapability_MovRecInCardStatus)
	{
		switch (nResult)
		{
		case kNkMAIDResult_NoError:
			bSuccess = TRUE;
			break;
		case kNkMAIDResult_RecInCard:
			printf("Card Recording\n");
			bSuccess = TRUE;
			break;
		case kNkMAIDResult_RecInExternalDevice:
			printf("ExternalDevice Recording\n");
			bSuccess = TRUE;
			break;
		case kNkMAIDResult_RecInCardAndExternalDevice:
			printf("Card and ExternalDevice Recording\n");
			bSuccess = TRUE;
			break;
		default:
			bSuccess = FALSE;
			break;
		}
	}
	else
	{
		bSuccess = (nResult == kNkMAIDResult_NoError) ? TRUE : FALSE;
	}

	return (bSuccess);
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_CapGetSB(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete, SLONG* pnResult)
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc(sizeof(RefCompletionProc));
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(pobject,
		kNkMAIDCommand_CapGet,
		ulParam,
		ulDataType,
		pData,
		(LPNKFUNC)CompletionProc,
		(NKREF)pRefCompletion);
	IdleLoop(pobject, &ulCount, 1);

	*pnResult = nResult;
	return (nResult == kNkMAIDResult_NoError);
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_CapSetSB(LPNkMAIDObject pobject, ULONG ulParam, ULONG ulDataType, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete, SLONG* pnResult)
{
	SLONG nResult;
	ULONG	ulCount = 0L;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc(sizeof(RefCompletionProc));
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;
	nResult = CallMAIDEntryPoint(pobject,
		kNkMAIDCommand_CapSet,
		ulParam,
		ulDataType,
		pData,
		(LPNKFUNC)CompletionProc,
		(NKREF)pRefCompletion);
	IdleLoop(pobject, &ulCount, 1);

	*pnResult = nResult;
	return (nResult == kNkMAIDResult_NoError);
}

//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_CapStart(LPNkMAIDObject pobject, ULONG ulParam, LPNKFUNC pfnComplete, NKREF refComplete, SLONG* pnResult)
{
	SLONG nResult = CallMAIDEntryPoint( pobject,
													kNkMAIDCommand_CapStart, 
													ulParam,
													kNkMAIDDataType_Null,
													(NKPARAM)NULL,
													pfnComplete,
													refComplete );
	if ( pnResult != NULL ) *pnResult = nResult;

	return ( nResult == kNkMAIDResult_NoError || nResult == kNkMAIDResult_Pending || nResult == kNkMAIDResult_BulbReleaseBusy || 
		     nResult == kNkMAIDResult_SilentReleaseBusy || nResult == kNkMAIDResult_MovieFrameReleaseBusy ||
			 nResult == kNkMAIDResult_Waiting_2ndRelease );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_CapStartGeneric( LPNkMAIDObject pObject, ULONG ulParam, NKPARAM pData, LPNKFUNC pfnComplete, NKREF refComplete, SLONG* pnResult )
{
	SLONG nResult = CallMAIDEntryPoint( pObject,
													kNkMAIDCommand_CapStart, 
													ulParam,
													kNkMAIDDataType_GenericPtr,
													pData,
													pfnComplete,
													refComplete );
	if ( pnResult != NULL ) *pnResult = nResult;

	return ( nResult == kNkMAIDResult_NoError || nResult == kNkMAIDResult_Pending );
}

//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_Abort(LPNkMAIDObject pobject, LPNKFUNC pfnComplete, NKREF refComplete)
{
	SLONG lResult = CallMAIDEntryPoint( pobject,
													kNkMAIDCommand_Abort, 
													(ULONG)NULL,
													kNkMAIDDataType_Null,
													(NKPARAM)NULL,
													pfnComplete,
													refComplete );
	return lResult == kNkMAIDResult_NoError;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL Command_Async(LPNkMAIDObject pobject)
{
	SLONG lResult = CallMAIDEntryPoint( pobject,
										kNkMAIDCommand_Async,
										0,
										kNkMAIDDataType_Null,
										(NKPARAM)NULL,
										(LPNKFUNC)NULL,
										(NKREF)NULL );
	return( lResult == kNkMAIDResult_NoError || lResult == kNkMAIDResult_Pending );
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL SelectSource( LPRefObj pRefObj, ULONG *pulSrcID )
{
	BOOL	bRet;
	NkMAIDEnum	stEnum;
	char	buf[256];
	UWORD	wSel;
	ULONG	i;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_Children );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Enum ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_Children, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	// check the data of the capability.
	if ( stEnum.wPhysicalBytes != 4 ) return FALSE;

	if ( stEnum.ulElements == 0 ) {
		printf( "There is no Source object.\n0. Exit\n>" );
		scanf( "%s", buf );
		return TRUE;
	}

	// allocate memory for array data
	stEnum.pData = malloc( stEnum.ulElements * stEnum.wPhysicalBytes );
	if ( stEnum.pData == NULL ) return FALSE;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == FALSE ) {
		free( stEnum.pData );
		return FALSE;
	}

	// show the list of selectable Sources
	for ( i = 0; i < stEnum.ulElements; i++ )
		printf( "%d. ID = %d\n", i + 1, ((ULONG*)stEnum.pData)[i] );

	if ( stEnum.ulElements == 1 )
		printf( "0. Exit\nSelect (1, 0)\n>" );
	else
		printf( "0. Exit\nSelect (1-%d, 0)\n>", stEnum.ulElements );

	scanf( "%s", buf );
	wSel = atoi( buf );

	if ( wSel > 0 && wSel <= stEnum.ulElements ) {
		*pulSrcID = ((ULONG*)stEnum.pData)[wSel - 1];
		free( stEnum.pData );
	} else {
		free( stEnum.pData );
		if ( wSel != 0 ) return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL SelectItem( LPRefObj pRefObj, ULONG *pulItemID )
{
	BOOL	bRet;
	NkMAIDEnum	stEnum;
	char	buf[256];
	UWORD	wSel;
	ULONG	i;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_Children );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Enum ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_Children, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	// check the data of the capability.
	if ( stEnum.ulElements == 0 ) {
		printf( "There is no item.\n" );
		return TRUE;
	}

	// check the data of the capability.
	if ( stEnum.wPhysicalBytes != 4 ) return FALSE;

	// allocate memory for array data
	stEnum.pData = malloc( stEnum.ulElements * stEnum.wPhysicalBytes );
	if ( stEnum.pData == NULL ) return FALSE;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == FALSE ) {
		free( stEnum.pData );
		return FALSE;
	}

	// show the list of selectable Items
	for ( i = 0; i < stEnum.ulElements; i++ )
		printf( "%d. Internal ID = %08X\n", i + 1, ((ULONG*)stEnum.pData)[i] );

	if ( stEnum.ulElements == 0 )
		printf( "There is no Item object.\n0. Exit\n>" );
	else if ( stEnum.ulElements == 1 )
		printf( "0. Exit\nSelect (1, 0)\n>" );
	else
		printf( "0. Exit\nSelect (1-%d, 0)\n>", stEnum.ulElements );

	scanf( "%s", buf );
	wSel = atoi( buf );

	if ( wSel > 0 && wSel <= stEnum.ulElements ) {
		*pulItemID = ((ULONG*)stEnum.pData)[wSel - 1];
		free( stEnum.pData );
	} else {
		free( stEnum.pData );
		if ( wSel != 0 ) return FALSE;
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL SelectData( LPRefObj pRefObj, ULONG *pulDataType )
{
	BOOL	bRet;
	char	buf[256];
	UWORD	wSel;
	ULONG	ulDataTypes, i = 0, DataTypes[8];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_DataTypes );
	if ( pCapInfo == NULL ) return FALSE;

	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_DataTypes, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_DataTypes, kNkMAIDDataType_UnsignedPtr, (NKPARAM)&ulDataTypes, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	// show the list of selectable Data type object.
	if ( ulDataTypes & kNkMAIDDataObjType_Image ) {
	
		DataTypes[i++] = kNkMAIDDataObjType_Image;
		printf( "%d. Image\n", i );
	}
	if ( ulDataTypes & kNkMAIDDataObjType_Video ) {
	
		DataTypes[i++] = kNkMAIDDataObjType_Video;
		printf( "%d. Movie\n", i );
	}
	if ( ulDataTypes & kNkMAIDDataObjType_Thumbnail ) {
		DataTypes[i++] = kNkMAIDDataObjType_Thumbnail;
		printf( "%d. Thumbnail\n", i );
	}

	if ( i == 0 )
		printf( "There is no Data object.\n0. Exit\n>" );
	else if ( i == 1 )
		printf( "0. Exit\nSelect (1, 0)\n>" );
	else
		printf( "0. Exit\nSelect (1-%d, 0)\n>", i );

	scanf( "%s", buf );
	wSel = atoi( buf );

	if ( wSel > 0 && wSel <= i )
		*pulDataType = DataTypes[wSel - 1];

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL CheckDataType( LPRefObj pRefObj, ULONG *pulDataType )
{
	BOOL	bRet;
	ULONG	ulDataTypes = 0;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_DataTypes );
	if ( pCapInfo == NULL ) return FALSE;

	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_DataTypes, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_DataTypes, kNkMAIDDataType_UnsignedPtr, (NKPARAM)&ulDataTypes, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	// show the list of selectable Data type object.
	if ( ulDataTypes & kNkMAIDDataObjType_Video )
	{
		return FALSE;
	}

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
char* GetEnumString( ULONG ulCapID, ULONG ulValue, char *psString )
{
	switch ( ulCapID ) {
		case kNkMAIDCapability_FlashMode:
			switch( ulValue ){
				case kNkMAIDFlashMode_FillFlash:
					strcpy( psString, "Normal" );
					break;
				case kNkMAIDFlashMode_RearCurtainSync:
					strcpy( psString, "Rear-sync" );
					break;
				case kNkMAIDFlashMode_SlowSyncFillFlash:
					strcpy( psString, "Slow-sync" );
					break;
				case kNkMAIDFlashMode_RedEyeReductionLighting:
					strcpy( psString, "Red Eye Reduction" );
					break;
				case kNkMAIDFlashMode_SlowSyncRedEye:
					strcpy( psString, "Slow-sync Red Eye Reduction" );
					break;
				case kNkMAIDFlashMode_Off:
					strcpy( psString, "flash off" );
					break;
				default:
					sprintf( psString, "FlashMode %u", ulValue );
			}
			break;
		case kNkMAIDCapability_ExposureMode:
			switch( ulValue ){
				case kNkMAIDExposureMode_Program:
					strcpy(psString, "Program");
					break;
				case kNkMAIDExposureMode_AperturePriority:
					strcpy(psString, "Aperture");
					break;
				case kNkMAIDExposureMode_SpeedPriority:
					strcpy(psString, "Speed");
					break;
				case kNkMAIDExposureMode_Manual:
					strcpy(psString, "Manual");
					break;
				case kNkMAIDExposureMode_Auto:
					strcpy(psString, "Auto");
					break;
				case kNkMAIDExposureMode_Scene:
					strcpy(psString, "Scene");
					break;
				case kNkMAIDExposureMode_UserMode1:
					strcpy(psString, "UserMode1");
					break;
				case kNkMAIDExposureMode_UserMode2:
					strcpy(psString, "UserMode2");
					break;
				case kNkMAIDExposureMode_Effects:
					strcpy(psString, "Effects");
					break;
				default:
					sprintf( psString, "ExposureMode %u", ulValue );
			}
			break;
		case kNkMAIDCapability_PictureControl:
			switch( ulValue ){
				case kNkMAIDPictureControl_Undefined:
					strcpy( psString, "Undefined" );
					break;
				case kNkMAIDPictureControl_Standard:
					strcpy( psString, "Standard" );
					break;
				case kNkMAIDPictureControl_Neutral:
					strcpy( psString, "Neutral" );
					break;
				case kNkMAIDPictureControl_Vivid:
					strcpy( psString, "Vivid" );
					break;
				case kNkMAIDPictureControl_Monochrome:
					strcpy( psString, "Monochrome" );
					break;
				case kNkMAIDPictureControl_Portrait:
					strcpy( psString, "Portrait" );
					break;
				case kNkMAIDPictureControl_Landscape:
					strcpy( psString, "Landscape" );
					break;
				case kNkMAIDPictureControl_Flat:
					strcpy( psString, "Flat" );
					break;
				case kNkMAIDPictureControl_Auto:
					strcpy(psString, "Auto");
					break;
				case kNkMAIDPictureControl_Dream:
					strcpy(psString, "Dream");
					break;
				case kNkMAIDPictureControl_Morning:
					strcpy(psString, "Morning");
					break;
				case kNkMAIDPictureControl_Pop:
					strcpy(psString, "Pop");
					break;
				case kNkMAIDPictureControl_Sunday:
					strcpy(psString, "Sunday");
					break;
				case kNkMAIDPictureControl_Somber:
					strcpy(psString, "Somber");
					break;
				case kNkMAIDPictureControl_Dramatic:
					strcpy(psString, "Dramatic");
					break;
				case kNkMAIDPictureControl_Silence:
					strcpy(psString, "Silence");
					break;
				case kNkMAIDPictureControl_Breached:
					strcpy(psString, "Breached");
					break;
				case kNkMAIDPictureControl_Melancholic:
					strcpy(psString, "Melancholic");
					break;
				case kNkMAIDPictureControl_Pure:
					strcpy(psString, "Pure");
					break;
				case kNkMAIDPictureControl_Denim:
					strcpy(psString, "Denim");
					break;
				case kNkMAIDPictureControl_Toy:
					strcpy(psString, "Toy");
					break;
				case kNkMAIDPictureControl_Sepia:
					strcpy(psString, "Sepia");
					break;
				case kNkMAIDPictureControl_Blue:
					strcpy(psString, "Blue");
					break;
				case kNkMAIDPictureControl_Red:
					strcpy(psString, "Red");
					break;
				case kNkMAIDPictureControl_Pink:
					strcpy(psString, "Pink");
					break;
				case kNkMAIDPictureControl_Charcoal:
					strcpy(psString, "Charcoal");
					break;
				case kNkMAIDPictureControl_Graphite:
					strcpy(psString, "Graphite");
					break;
				case kNkMAIDPictureControl_Binary:
					strcpy(psString, "Binary");
					break;
				case kNkMAIDPictureControl_Carbon:
					strcpy(psString, "Carbon");
					break;
				case kNkMAIDPictureControl_Custom1:
				case kNkMAIDPictureControl_Custom2:
				case kNkMAIDPictureControl_Custom3:
				case kNkMAIDPictureControl_Custom4:
				case kNkMAIDPictureControl_Custom5:
				case kNkMAIDPictureControl_Custom6:
				case kNkMAIDPictureControl_Custom7:
				case kNkMAIDPictureControl_Custom8:
				case kNkMAIDPictureControl_Custom9:
					sprintf( psString, "Custom Picture Contol %d", ulValue-200 );
					break;
				default:
					sprintf( psString, "Picture Control %u", ulValue );
			}
			break;
		case kNkMAIDCapability_MoviePictureControl:
			switch (ulValue){
			case kNkMAIDMoviePictureControl_Undefined:
				strcpy(psString, "Undefined");
				break;
			case kNkMAIDMoviePictureControl_Standard:
				strcpy(psString, "Standard");
				break;
			case kNkMAIDMoviePictureControl_Neutral:
				strcpy(psString, "Neutral");
				break;
			case kNkMAIDMoviePictureControl_Vivid:
				strcpy(psString, "Vivid");
				break;
			case kNkMAIDMoviePictureControl_Monochrome:
				strcpy(psString, "Monochrome");
				break;
			case kNkMAIDMoviePictureControl_Portrait:
				strcpy(psString, "Portrait");
				break;
			case kNkMAIDMoviePictureControl_Landscape:
				strcpy(psString, "Landscape");
				break;
			case kNkMAIDMoviePictureControl_Flat:
				strcpy(psString, "Flat");
				break;
			case kNkMAIDMoviePictureControl_Auto:
				strcpy(psString, "Auto");
				break;
			case kNkMAIDMoviePictureControl_SamePhoto:
				strcpy(psString, "SamePhoto");
				break;
			case kNkMAIDMoviePictureControl_Dream:
				strcpy(psString, "Dream");
				break;
			case kNkMAIDMoviePictureControl_Morning:
				strcpy(psString, "Morning");
				break;
			case kNkMAIDMoviePictureControl_Pop:
				strcpy(psString, "Pop");
				break;
			case kNkMAIDMoviePictureControl_Sunday:
				strcpy(psString, "Sunday");
				break;
			case kNkMAIDMoviePictureControl_Somber:
				strcpy(psString, "Somber");
				break;
			case kNkMAIDMoviePictureControl_Dramatic:
				strcpy(psString, "Dramatic");
				break;
			case kNkMAIDMoviePictureControl_Silence:
				strcpy(psString, "Silence");
				break;
			case kNkMAIDMoviePictureControl_Breached:
				strcpy(psString, "Breached");
				break;
			case kNkMAIDMoviePictureControl_Melancholic:
				strcpy(psString, "Melancholic");
				break;
			case kNkMAIDMoviePictureControl_Pure:
				strcpy(psString, "Pure");
				break;
			case kNkMAIDMoviePictureControl_Denim:
				strcpy(psString, "Denim");
				break;
			case kNkMAIDMoviePictureControl_Toy:
				strcpy(psString, "Toy");
				break;
			case kNkMAIDMoviePictureControl_Sepia:
				strcpy(psString, "Sepia");
				break;
			case kNkMAIDMoviePictureControl_Blue:
				strcpy(psString, "Blue");
				break;
			case kNkMAIDMoviePictureControl_Red:
				strcpy(psString, "Red");
				break;
			case kNkMAIDMoviePictureControl_Pink:
				strcpy(psString, "Pink");
				break;
			case kNkMAIDMoviePictureControl_Charcoal:
				strcpy(psString, "Charcoal");
				break;
			case kNkMAIDMoviePictureControl_Graphite:
				strcpy(psString, "Graphite");
				break;
			case kNkMAIDMoviePictureControl_Binary:
				strcpy(psString, "Binary");
				break;
			case kNkMAIDMoviePictureControl_Carbon:
				strcpy(psString, "Carbon");
				break;
			case kNkMAIDMoviePictureControl_Custom1:
			case kNkMAIDMoviePictureControl_Custom2:
			case kNkMAIDMoviePictureControl_Custom3:
			case kNkMAIDMoviePictureControl_Custom4:
			case kNkMAIDMoviePictureControl_Custom5:
			case kNkMAIDMoviePictureControl_Custom6:
			case kNkMAIDMoviePictureControl_Custom7:
			case kNkMAIDMoviePictureControl_Custom8:
			case kNkMAIDMoviePictureControl_Custom9:
				sprintf(psString, "Custom Picture Contol %d", ulValue - 200);
				break;
			default:
				sprintf(psString, "Picture Control %u", ulValue);
			}
			break;
		case kNkMAIDCapability_LiveViewImageSize:
			switch( ulValue ){
				case kNkMAIDLiveViewImageSize_QVGA:
					strcpy( psString, "QVGA" );
					break;
				case kNkMAIDLiveViewImageSize_VGA:
					strcpy( psString, "VGA" );
					break;
				case kNkMAIDLiveViewImageSize_XGA:
					strcpy(psString, "XGA");
					break;
				default:
					sprintf( psString, "LiveViewImageSize %u", ulValue );
			}
			break;
		case kNkMAIDCapability_SBIntegrationFlashReady:
			switch (ulValue){
			case kNkMAIDSBIntegrationFlashReady_NotReady:
				strcpy(psString, "NotReady");
				break;
			case kNkMAIDSBIntegrationFlashReady_Ready:
				strcpy(psString, "Ready");
				break;
			default:
				sprintf(psString, "SBIntegrationFlashReady %u", ulValue);
			}
			break;
		case kNkMAIDCapability_HighlightBrightness:
			switch (ulValue){
			case kNkMAIDHighlightBrightness_180:
				strcpy(psString, "180");
				break;
			case kNkMAIDHighlightBrightness_191:
				strcpy(psString, "191");
				break;
			case kNkMAIDHighlightBrightness_202:
				strcpy(psString, "202");
				break;
			case kNkMAIDHighlightBrightness_213:
				strcpy(psString, "213");
				break;
			case kNkMAIDHighlightBrightness_224:
				strcpy(psString, "224");
				break;
			case kNkMAIDHighlightBrightness_235:
				strcpy(psString, "235");
				break;
			case kNkMAIDHighlightBrightness_248:
				strcpy(psString, "248");
				break;
			case kNkMAIDHighlightBrightness_255:
				strcpy(psString, "255");
				break;
			default:
				sprintf(psString, "HighlightBrightness %u", ulValue);
			}
			break;
		case kNkMAIDCapability_DiffractionCompensation:
			switch (ulValue){
			case kNkMAIDDiffractionCompensation_Off:
				strcpy(psString, "Off");
				break;
			case kNkMAIDDiffractionCompensation_On:
				strcpy(psString, "On");
				break;
			default:
				sprintf(psString, "DiffractionCompensation %u", ulValue);
			}
			break;
		case kNkMAIDCapability_ShootingMode:
			switch (ulValue){
			case eNkMAIDShootingMode_S:
				strcpy(psString, "Single frame");
				break;
			case eNkMAIDShootingMode_C:
				strcpy(psString, "Continuous L");
				break;
			case eNkMAIDShootingMode_CH:
				strcpy(psString, "Continuous H");
				break;
			case eNkMAIDShootingMode_SelfTimer:
				strcpy(psString, "Self-timer");
				break;
			case eNkMAIDShootingMode_CHPriorityFrame:
				strcpy(psString, "Continuous H(extended)");
				break;
			default:
				sprintf(psString, "ShootingMode %u", ulValue);
			}
			break;
		case kNkMAIDCapability_VibrationReduction:
			switch (ulValue){
			case kNkMAIDVibrationReduction_Off:
				strcpy(psString, "Off");
				break;
			case kNkMAIDVibrationReduction_Normal:
				strcpy(psString, "Normal");
				break;
			case kNkMAIDVibrationReduction_Sport:
				strcpy(psString, "Sport");
				break;
			default:
				sprintf(psString, "VibrationReduction %u", ulValue);
			}
			break;
		case kNkMAIDCapability_HDRSaveIndividualImages:
			switch (ulValue){
			case kNkMAIDHDRSaveIndividualImages_Off:
				strcpy(psString, "Off");
				break;
			case kNkMAIDHDRSaveIndividualImages_On:
				strcpy(psString, "On");
				break;
			default:
				sprintf(psString, "HDRSaveIndividualImages %u", ulValue);
			}
			break;
		case kNkMAIDCapability_MovieAttenuator:
			switch (ulValue){
			case kNkMAIDMovieAttenuator_disable:
				strcpy(psString, "disable");
				break;
			case kNkMAIDMovieAttenuator_enable:
				strcpy(psString, "enable");
				break;
			default:
				sprintf(psString, "MovieAttenuator %u", ulValue);
			}
			break;
		case kNkMAIDCapability_MovieAutoDistortion:
			switch (ulValue){
			case kNkMAIDMovieAutoDistortion_Off:
				strcpy(psString, "Off");
				break;
			case kNkMAIDMovieAutoDistortion_On:
				strcpy(psString, "On");
				break;
			default:
				sprintf(psString, "MovieAutoDistortion %u", ulValue);
			}
			break;
		case kNkMAIDCapability_MovieDiffractionCompensation:
			switch (ulValue){
			case kNkMAIDMovieDiffractionCompensation_Off:
				strcpy(psString, "Off");
				break;
			case kNkMAIDMovieDiffractionCompensation_On:
				strcpy(psString, "On");
				break;
			default:
				sprintf(psString, "MovieDiffractionCompensation %u", ulValue);
			}
			break;
		case kNkMAIDCapability_MovieFileType:
			switch (ulValue){
			case kNkMAIDMovieFileType_MOV:
				strcpy(psString, "MOV");
				break;
			case kNkMAIDMovieFileType_MP4:
				strcpy(psString, "MP4");
				break;
			default:
				sprintf(psString, "MovieFileType %u", ulValue);
			}
			break;
		case kNkMAIDCapability_MovieFocusMode:
			switch (ulValue){
			case kNkMAIDMovieFocusMode_AFs:
				strcpy(psString, "AF-S");
				break;
			case kNkMAIDMovieFocusMode_AFc:
				strcpy(psString, "AF-C");
				break;
			case kNkMAIDMovieFocusMode_AFf:
				strcpy(psString, "AF-F");
				break;
			case kNkMAIDMovieFocusMode_MF_SEL:
				strcpy(psString, "MF");
				break;
			default:
				sprintf(psString, "MovieFocusMode %u", ulValue);
			}
			break;
		case kNkMAIDCapability_MovieVibrationReduction:
			switch (ulValue){
			case kNkMAIDMovieVibrationReduction_Off:
				strcpy(psString, "Off");
				break;
			case kNkMAIDMovieVibrationReduction_Normal:
				strcpy(psString, "Normal");
				break;
			case kNkMAIDMovieVibrationReduction_Sport:
				strcpy(psString, "Sport");
				break;
			case kNkMAIDMovieVibrationReduction_SamePhoto:
				strcpy(psString, "SamePhoto");
				break;
			default:
				sprintf(psString, "MovieVibrationReduction %u", ulValue);
			}
			break;
		case kNkMAIDCapability_MovieVignetteControl:
			switch (ulValue){
			case kNkMAIDMovieVignetteControl_Off:
				strcpy(psString, "Off");
				break;
			case kNkMAIDMovieVignetteControl_Low:
				strcpy(psString, "Low");
				break;
			case kNkMAIDMovieVignetteControl_Normal:
				strcpy(psString, "Normal");
				break;
			case kNkMAIDMovieVignetteControl_High:
				strcpy(psString, "High");
				break;
			case kNkMAIDMovieVignetteControl_SamePhoto:
				strcpy(psString, "SamePhoto");
				break;
			default:
				sprintf(psString, "MovieVignetteControl %u", ulValue);
			}
			break;
		case kNkMAIDCapability_LiveViewZoomArea:
			switch (ulValue){
			case kNkMAIDLiveViewZoomArea_0:
				strcpy(psString, "0");
				break;
			case kNkMAIDLiveViewZoomArea_360:
				strcpy(psString, "360 x 240");
				break;
			case kNkMAIDLiveViewZoomArea_512:
				strcpy(psString, "512 x 342");
				break;
			case kNkMAIDLiveViewZoomArea_720:
				strcpy(psString, "720 x 480");
				break;
			case kNkMAIDLiveViewZoomArea_1024:
				strcpy(psString, "1024 x 682");
				break;
			case kNkMAIDLiveViewZoomArea_1448:
				strcpy(psString, "1448 x 964");
				break;
			default:
				sprintf(psString, "LiveViewZoomArea %u", ulValue);
			}
			break;
		case kNkMAIDCapability_LiveViewImageStatus:
			switch (ulValue){
			case kNkMAIDLiveViewImageStatus_CannotAcquire:
				strcpy(psString, "CannotAcquire");
				break;
			case kNkMAIDLiveViewImageStatus_CanAcquire:
				strcpy(psString, "CanAcquire");
				break;
			default:
				sprintf(psString, "LiveViewImageStatus %u", ulValue);
			}
			break;

		case kNkMAIDCapability_StillCaptureModeSaveFrame:
			switch (ulValue) {
			case eNkMAIDStillCaptureModeSaveFrame_S:
				strcpy(psString, "Single frame");
				break;
			case eNkMAIDStillCaptureModeSaveFrame_CH:
				strcpy(psString, "Continuous H");
				break;
			default:
				sprintf(psString, "StillCaptureModeSaveFrame %u", ulValue);
			}
			break;

		default:
			strcpy( psString, "Undefined String" ); 
	}
	return psString;
}
//------------------------------------------------------------------------------------------------------------------------------------
char*	GetUnsignedString( ULONG ulCapID, ULONG ulValue, char *psString )
{
	char buff[256];

	switch ( ulCapID )
	{
	case kNkMAIDCapability_MeteringMode:
		sprintf( buff, "%d : Matrix\n", kNkMAIDMeteringMode_Matrix );
		strcpy( psString, buff );
		sprintf( buff, "%d : CenterWeighted\n", kNkMAIDMeteringMode_CenterWeighted );
		strcat( psString, buff );
		sprintf( buff, "%d : Spot\n", kNkMAIDMeteringMode_Spot );
		strcat( psString, buff );
		sprintf(buff, "%d : HighLight\n", kNkMAIDMeteringMode_HighLight);
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_FocusMode:
		sprintf( buff, "%d : MF\n", kNkMAIDFocusMode_MF );
		strcpy( psString, buff );
		sprintf( buff, "%d : AF-S\n", kNkMAIDFocusMode_AFs );
		strcat( psString, buff );
		sprintf( buff, "%d : AF-C\n", kNkMAIDFocusMode_AFc );
		strcat( psString, buff );
		sprintf( buff, "%d : AF-A\n", kNkMAIDFocusMode_AFa );
		strcat( psString, buff );
		sprintf( buff, "%d : AF-F\n", kNkMAIDFocusMode_AFf );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_RawJpegImageStatus:
		sprintf( buff, "%d : Single\n", eNkMAIDRawJpegImageStatus_Single );
		strcpy( psString, buff );
		sprintf( buff, "%d : Raw + Jpeg\n", eNkMAIDRawJpegImageStatus_RawJpeg );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_DataTypes:
		strcpy( psString, "\0" );
		if ( ulValue & kNkMAIDDataObjType_Image )
		{
			strcat( psString, "Image, " );
		}
		if ( ulValue & kNkMAIDDataObjType_Sound )
		{
			strcat( psString, "Sound, " );
		}
		if ( ulValue & kNkMAIDDataObjType_Video )
		{
			strcat( psString, "Video, " );
		}
		if ( ulValue & kNkMAIDDataObjType_Thumbnail )
		{
			strcat( psString, "Thumbnail, " );
		}
		if ( ulValue & kNkMAIDDataObjType_File )
		{
			strcat( psString, "File " );
		}
		strcat( psString, "\n" );
		break;
	case kNkMAIDCapability_ModuleType:
		sprintf( buff, "%d : Scanner\n", kNkMAIDModuleType_Scanner );
		strcpy( psString, buff );
		sprintf( buff, "%d : Camera\n", kNkMAIDModuleType_Camera );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_WBFluorescentType:
		sprintf( buff, "%d : Sodium-vapor lamps\n", kNkWBFluorescentType_SodiumVapor );
		strcpy( psString, buff );
		sprintf( buff, "%d : Warm-white fluorescent\n", kNkWBFluorescentType_WarmWhite );
		strcat( psString, buff );
		sprintf( buff, "%d : White fluorescent\n", kNkWBFluorescentType_White );
		strcat( psString, buff );
		sprintf( buff, "%d : Cool-white fluorescent\n", kNkWBFluorescentType_CoolWhite );
		strcat( psString, buff );
		sprintf( buff, "%d : Day white fluorescent\n", kNkWBFluorescentType_DayWhite );
		strcat( psString, buff );
		sprintf( buff, "%d : Daylight fluorescent\n", kNkWBFluorescentType_Daylight );
		strcat( psString, buff );
		sprintf( buff, "%d : High temp. mercury-vapor\n", kNkWBFluorescentType_HiTempMercuryVapor );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_LiveViewProhibit:
		if (ulValue & kNkMAIDLiveViewProhibit_TempRise)
		{
			strcat( psString, "High Temperature," );
		}
		if (ulValue & kNkMAIDLiveViewProhibit_Battery)
		{
			strcat( psString, "Battery, " );
		}
		if (ulValue & kNkMAIDLiveViewProhibit_Sequence)
		{
			strcat(psString, "Sequence, ");
		}
		strcat(psString, "\n");
		break;
	case kNkMAIDCapability_LiveViewStatus:
		sprintf( buff, "%d : OFF\n", kNkMAIDLiveViewStatus_OFF );
		strcpy( psString, buff );
		sprintf(buff, "%d : ON_RemoteLV\n", kNkMAIDLiveViewStatus_ON_RemoteLV);
		strcat(psString, buff);
		sprintf(buff, "%d : ON_CameraLV\n", kNkMAIDLiveViewStatus_ON_CameraLV);
		strcat(psString, buff);
		break;
	case kNkMAIDCapability_MovRecInCardStatus:
		sprintf( buff, "%d : OFF\n", kNkMAIDMovRecInCardStatus_Off );
		strcpy( psString, buff );
		sprintf( buff, "%d : ON\n", kNkMAIDMovRecInCardStatus_On );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_MovRecInCardProhibit:
		strcpy(psString, "\0");
		if (ulValue & kNkMAIDMovRecInCardProhibit_LVPhoto)
		{
			strcat(psString, "LiveViewPhoto, ");
		}
		if (ulValue & kNkMAIDMovRecInCardProhibit_LVImageZoom)
		{
			strcat(psString, "LiveViewZoom, ");
		}
		if (ulValue & kNkMAIDMovRecInCardProhibit_CardProtect)
		{
			strcat(psString, "Card protect, ");
		}
		if (ulValue & kNkMAIDMovRecInCardProhibit_RecMov)
		{
			strcat(psString, "Recording movie, ");
		}
		if (ulValue & kNkMAIDMovRecInCardProhibit_MovInBuf)
		{
			strcat(psString, "Movie in buffer, ");
		}
		if (ulValue & kNkMAIDMovRecInCardProhibit_CardFull)
		{
			strcat(psString, "Card full, ");
		}
		if (ulValue & kNkMAIDMovRecInCardProhibit_NoFormat)
		{
			strcat(psString, "Card unformatted, ");
		}
		if (ulValue & kNkMAIDMovRecInCardProhibit_CardErr)
		{
			strcat(psString, "Card error, ");
		}
		if (ulValue & kNkMAIDMovRecInCardProhibit_NoCard)
		{
			strcat(psString, "No card, ");
		}
		strcat(psString, "\n");
		break;
	case kNkMAIDCapability_SaveMedia:
		sprintf( buff, "%d : Card\n", kNkMAIDSaveMedia_Card );
		strcpy( psString, buff );
		sprintf( buff, "%d : SDRAM\n", kNkMAIDSaveMedia_SDRAM );
		strcat( psString, buff );
		sprintf( buff, "%d : Card + SDRAM\n", kNkMAIDSaveMedia_Card_SDRAM );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_MovieMeteringMode:
		sprintf(buff, "%d : Matrix\n", kNkMAIDMovieMeteringMode_Matrix);
		strcpy(psString, buff);
		sprintf(buff, "%d : CenterWeighted\n", kNkMAIDMovieMeteringMode_CenterWeighted);
		strcat(psString, buff);
		sprintf(buff, "%d : HighLight\n", kNkMAIDMovieMeteringMode_HighLight);
		strcat(psString, buff);
		break;
	case kNkMAIDCapability_FlickerReductionSetting:
		sprintf( buff, "%d : Off\n", kNkMAIDFlickerReductionSetting_Off );
		strcpy( psString, buff );
		sprintf( buff, "%d : On\n", kNkMAIDFlickerReductionSetting_On );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_MovieActive_D_Lighting:
		sprintf( buff, "%d : Off\n", kNkMAIDMovieActive_D_Lighting_Off );
		strcpy( psString, buff );
		sprintf( buff, "%d : Low\n", kNkMAIDMovieActive_D_Lighting_Low );
		strcat( psString, buff );
		sprintf( buff, "%d : Normal\n", kNkMAIDMovieActive_D_Lighting_Normal );
		strcat( psString, buff );
		sprintf( buff, "%d : High\n", kNkMAIDMovieActive_D_Lighting_High );
		strcat( psString, buff );
		sprintf( buff, "%d : Extra high\n", kNkMAIDMovieActive_D_Lighting_ExtraHigh );
		strcat( psString, buff );
		sprintf( buff, "%d : Same as photo\n", kNkMAIDMovieActive_D_Lighting_SamePhoto );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_ElectronicVR:
		sprintf( buff, "%d : Off\n", kMAIDElectronicVR_OFF );
		strcpy( psString, buff );
		sprintf( buff, "%d : On\n", kMAIDElectronicVR_ON );
		strcat( psString, buff );
		break;
	case kNkMAIDCapability_WBAutoType:
		sprintf(buff, "%d : Keep warm loghting colors\n", kNkWBAutoType_WarmWhite);
		strcpy(psString, buff);
		sprintf(buff, "%d : Keep white\n", kNkWBAutoType_KeepWhite);
		strcat(psString, buff);
		sprintf(buff, "%d : Keep overall atmosphere\n", kNkWBAutoType_KeepAtmosphere);
		strcat(psString, buff);
		break;
	case kNkMAIDCapability_MovieScreenSize:
		sprintf(buff, "%d : 3840x2160 30p\n", kNkMAIDMovieScreenSize8_QFHD_30p);
		strcpy(psString, buff);
		sprintf(buff, "%d : 3840x2160 25p\n", kNkMAIDMovieScreenSize8_QFHD_25p);
		strcat(psString, buff);
		sprintf(buff, "%d : 3840x2160 24p\n", kNkMAIDMovieScreenSize8_QFHD_24p);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 120p\n", kNkMAIDMovieScreenSize8_FullHD_120p);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 100p\n", kNkMAIDMovieScreenSize8_FullHD_100p);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 60p\n", kNkMAIDMovieScreenSize8_FullHD_60p);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 50p\n", kNkMAIDMovieScreenSize8_FullHD_50p);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 30p\n", kNkMAIDMovieScreenSize8_FullHD_30p);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 25p\n", kNkMAIDMovieScreenSize8_FullHD_25p);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 24p\n", kNkMAIDMovieScreenSize8_FullHD_24p);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 30p x4 Slow\n", kNkMAIDMovieScreenSize8_FullHD_30p_x4_Slow);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 25p x4 Slow\n", kNkMAIDMovieScreenSize8_FullHD_25p_x4_Slow);
		strcat(psString, buff);
		sprintf(buff, "%d : 1920x1080 24p x5 Slow\n", kNkMAIDMovieScreenSize8_FullHD_24p_x5_Slow);
		strcat(psString, buff);
		break;
	case kNkMAIDCapability_LensType:
		if (ulValue & kNkMAIDLensType_D)
		{
			strcat(psString, "D Type, ");
		}
		if (ulValue & kNkMAIDLensType_G)
		{
			strcat(psString, "G Type, ");
		}
		if (ulValue & kNkMAIDLensType_VR)
		{
			strcat(psString, "VR, ");
		}
		if (ulValue & kNkMAIDLensType_DX)
		{
			strcat(psString, "DX, ");
		}
		if (ulValue & kNkMAIDLensType_AFS)
		{
			strcat(psString, "AF-S Lens, ");
		}
		if (ulValue & kNkMAIDLensType_AD)
		{
			strcat(psString, "Auto Distortion, ");
		}
		if (ulValue & kNkMAIDLensType_RET)
		{
			strcat(psString, "Retractable lens, ");
		}
		if (ulValue & kNkMAIDLensType_E)
		{
			strcat(psString, "E type, ");
		}
		if (ulValue & kNkMAIDLensType_STM)
		{
			strcat(psString, "STM, ");
		}
		if (ulValue & kNkMAIDLensType_CD)
		{
			strcat(psString, "Constantly Distortion, ");
		}
		strcat(psString, "\n");
		break;
	case kNkMAIDCapability_CameraType:
		if (ulValue == kNkMAIDCameraType_Z_50)
		{
			strcat(psString, "Z 50");
		}
		strcat(psString, "\n");
		break;

	case kNkMAIDCapability_UserMode1:
	case kNkMAIDCapability_UserMode2:
		sprintf(buff, "%d : Night landscape\n", kNkMAIDUserMode_NightLandscape);
		strcpy(psString, buff);
		sprintf(buff, "%d : Party/indoor\n", kNkMAIDUserMode_PartyIndoor);
		strcat(psString, buff);
		sprintf(buff, "%d : Beach/snow\n", kNkMAIDUserMode_BeachSnow);
		strcat(psString, buff);
		sprintf(buff, "%d : Sunset\n", kNkMAIDUserMode_Sunset);
		strcat(psString, buff);
		sprintf(buff, "%d : Dusk/dawn\n", kNkMAIDUserMode_Duskdawn);
		strcat(psString, buff);
		sprintf(buff, "%d : Pet portrait\n", kNkMAIDUserMode_Petportrait);
		strcat(psString, buff);
		sprintf(buff, "%d : Candlelight\n", kNkMAIDUserMode_Candlelight);
		strcat(psString, buff);
		sprintf(buff, "%d : Blossom\n", kNkMAIDUserMode_Blossom);
		strcat(psString, buff);
		sprintf(buff, "%d : Autumn colors\n", kNkMAIDUserMode_AutumnColors);
		strcat(psString, buff);
		sprintf(buff, "%d : Food\n", kNkMAIDUserMode_Food);
		strcat(psString, buff);
		sprintf(buff, "%d : Silhouette\n", kNkMAIDUserMode_Silhouette);
		strcat(psString, buff);
		sprintf(buff, "%d : High key\n", kNkMAIDUserMode_Highkey);
		strcat(psString, buff);
		sprintf(buff, "%d : Low key\n", kNkMAIDUserMode_Lowkey);
		strcat(psString, buff);
		sprintf(buff, "%d : Portrait\n", kNkMAIDUserMode_Portrait);
		strcat(psString, buff);
		sprintf(buff, "%d : Landscape\n", kNkMAIDUserMode_Landscape);
		strcat(psString, buff);
		sprintf(buff, "%d : Child\n", kNkMAIDUserMode_Child);
		strcat(psString, buff);
		sprintf(buff, "%d : Sports\n", kNkMAIDUserMode_Sports);
		strcat(psString, buff);
		sprintf(buff, "%d : Close up\n", kNkMAIDUserMode_Closeup);
		strcat(psString, buff);
		sprintf(buff, "%d : Night portrait\n", kNkMAIDUserMode_NightPortrait);
		strcat(psString, buff);
		sprintf(buff, "%d : Program auto\n", kNkMAIDUserMode_Program);
		strcat(psString, buff);
		sprintf(buff, "%d : Shutter speed priority\n", kNkMAIDUserMode_SpeedPriority);
		strcat(psString, buff);
		sprintf(buff, "%d : Aperture priority\n", kNkMAIDUserMode_AperturePriority);
		strcat(psString, buff);
		sprintf(buff, "%d : Manual\n", kNkMAIDUserMode_Manual);
		strcat(psString, buff);
		sprintf(buff, "%d : Auto\n", kNkMAIDUserMode_Auto);
		strcat(psString, buff);
		sprintf(buff, "%d : Night vision\n", kNkMAIDUserMode_NightVision);
		strcat(psString, buff);
		sprintf(buff, "%d : Miniature effect\n", kNkMAIDUserMode_Miniature);
		strcat(psString, buff);
		sprintf(buff, "%d : Selective color\n", kNkMAIDUserMode_SelectColor);
		strcat(psString, buff);
		sprintf(buff, "%d : Toy camera\n", kNkMAIDUserMode_ToyCamera);
		strcat(psString, buff);
		sprintf(buff, "%d : Super vivid\n", kNkMAIDUserMode_SuperVivid);
		strcat(psString, buff);
		sprintf(buff, "%d : Pop\n", kNkMAIDUserMode_Pop);
		strcat(psString, buff);
		sprintf(buff, "%d : Photo Illustration\n", kNkMAIDUserMode_PhotoIllustration);
		strcat(psString, buff);
		break;
	case kNkMAIDCapability_EffectMode:
		sprintf(buff, "%d : Night vision\n", kNkMAIDEffectMode_NightVision);
		strcpy(psString, buff);
		sprintf(buff, "%d : Miniature effect\n", kNkMAIDEffectMode_Miniature);
		strcat(psString, buff);
		sprintf(buff, "%d : Selective color\n", kNkMAIDEffectMode_SelectColor);
		strcat(psString, buff);
		sprintf(buff, "%d : Silhouette\n", kNkMAIDEffectMode_Silhouette);
		strcat(psString, buff);
		sprintf(buff, "%d : High key\n", kNkMAIDEffectMode_Highkey);
		strcat(psString, buff);
		sprintf(buff, "%d : Low key\n", kNkMAIDEffectMode_Lowkey);
		strcat(psString, buff);
		sprintf(buff, "%d : Toy camera\n", kNkMAIDEffectMode_ToyCamera);
		strcat(psString, buff);
		sprintf(buff, "%d : Super vivid\n", kNkMAIDEffectMode_SuperVivid);
		strcat(psString, buff);
		sprintf(buff, "%d : Pop\n", kNkMAIDEffectMode_Pop);
		strcat(psString, buff);
		sprintf(buff, "%d : Photo Illustration\n", kNkMAIDEffectMode_PhotoIllustration);
		strcat(psString, buff);
		break;
	case kNkMAIDCapability_SceneMode:
		sprintf(buff, "%d : Night landscape\n", kNkMAIDSceneMode_NightLandscape);
		strcpy(psString, buff);
		sprintf(buff, "%d : Party/indoor\n", kNkMAIDSceneMode_PartyIndoor);
		strcat(psString, buff);
		sprintf(buff, "%d : Beach/snow\n", kNkMAIDSceneMode_BeachSnow);
		strcat(psString, buff);
		sprintf(buff, "%d : Sunset\n", kNkMAIDSceneMode_Sunset);
		strcat(psString, buff);
		sprintf(buff, "%d : Dusk/dawn\n", kNkMAIDSceneMode_Duskdawn);
		strcat(psString, buff);
		sprintf(buff, "%d : Pet portrait\n", kNkMAIDSceneMode_Petportrait);
		strcat(psString, buff);
		sprintf(buff, "%d : Candlelight\n", kNkMAIDSceneMode_Candlelight);
		strcat(psString, buff);
		sprintf(buff, "%d : Blossom\n", kNkMAIDSceneMode_Blossom);
		strcat(psString, buff);
		sprintf(buff, "%d : Autumn colors\n", kNkMAIDSceneMode_AutumnColors);
		strcat(psString, buff);
		sprintf(buff, "%d : Food\n", kNkMAIDSceneMode_Food);
		strcat(psString, buff);
		sprintf(buff, "%d : Portrait\n", kNkMAIDSceneMode_Portrait);
		strcat(psString, buff);
		sprintf(buff, "%d : Landscape\n", kNkMAIDSceneMode_Landscape);
		strcat(psString, buff);
		sprintf(buff, "%d : Child\n", kNkMAIDSceneMode_Child);
		strcat(psString, buff);
		sprintf(buff, "%d : Sports\n", kNkMAIDSceneMode_Sports);
		strcat(psString, buff);
		sprintf(buff, "%d : Close up\n", kNkMAIDSceneMode_Closeup);
		strcat(psString, buff);
		sprintf(buff, "%d : Night portrait\n", kNkMAIDSceneMode_NightPortrait);
		strcat(psString, buff);
		break;

	default:
		psString[0] = '\0';
		break; 
	}
	return psString;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Distribute the function according to array type.
BOOL SetEnumCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	NkMAIDEnum	stEnum;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Enum ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	switch ( stEnum.ulType ) {
		case kNkMAIDArrayType_Unsigned:
			return SetEnumUnsignedCapability( pRefObj, ulCapID, &stEnum );
			break;
		case kNkMAIDArrayType_PackedString:
			return SetEnumPackedStringCapability( pRefObj, ulCapID, &stEnum );
			break;
		case kNkMAIDArrayType_String:
			return SetEnumStringCapability( pRefObj, ulCapID, &stEnum );
			break;
		default:
			return FALSE;
	}
}

//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Enum(Unsigned Integer) type capability and set a value for it.
BOOL SetEnumUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum )
{
	BOOL	bRet;
	char	psString[64], buf[256];
	UWORD	wSel;
	ULONG	i;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check the data of the capability.
	if ( pstEnum->wPhysicalBytes != 4 ) return FALSE;

	// check if this capability has elements.
	if( pstEnum->ulElements == 0 )
	{
		// This capablity has no element and is not available.
		printf( "There is no element in this capability. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
		return TRUE;
	}

	// allocate memory for array data
	pstEnum->pData = malloc( pstEnum->ulElements * pstEnum->wPhysicalBytes );
	if ( pstEnum->pData == NULL ) return FALSE;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
	if( bRet == FALSE ) {
		free( pstEnum->pData );
		return FALSE;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );
	
	for ( i = 0; i < pstEnum->ulElements; i++ )
		printf( "%2d. %s\n", i + 1, GetEnumString( ulCapID, ((ULONG*)pstEnum->pData)[i], psString ) );
	printf( "Current Setting: %d\n", pstEnum->ulValue + 1 );

	// check if this capability supports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
		if ( wSel > 0 && wSel <= pstEnum->ulElements ) {
			pstEnum->ulValue = wSel - 1;
			// send the selected number
			bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
			// This statement can be changed as follows.
			//bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)pstEnum->ulValue, NULL, NULL );
			if( bRet == FALSE ) {
				free( pstEnum->pData );
				return FALSE;
			}
		}
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	free( pstEnum->pData );
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Enum(Packed String) type capability and set a value for it.
BOOL SetEnumPackedStringCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum )
{
	BOOL	bRet;
	char	*psStr, buf[256];
	UWORD	wSel;
	size_t  i;
	ULONG	ulCount = 0;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check the data of the capability.
	if ( pstEnum->wPhysicalBytes != 1 ) return FALSE;

	// check if this capability has elements.
	if( pstEnum->ulElements == 0 )
	{
		// This capablity has no element and is not available.
		printf( "There is no element in this capability. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
		return TRUE;
	}

	// allocate memory for array data
	pstEnum->pData = malloc( pstEnum->ulElements * pstEnum->wPhysicalBytes );
	if ( pstEnum->pData == NULL ) return FALSE;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
	if( bRet == FALSE ) {
		free( pstEnum->pData );
		return FALSE;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );
	for ( i = 0; i < pstEnum->ulElements; ) {
		psStr = (char*)((char*)pstEnum->pData + i);
		printf( "%2d. %s\n", ++ulCount, psStr );
		i += strlen( psStr ) + 1;
	}
	printf( "Current Setting: %d\n", pstEnum->ulValue + 1 );

	// check if this capability supports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
		if ( wSel > 0 && wSel <= pstEnum->ulElements ) {
			pstEnum->ulValue = wSel - 1;
			// send the selected number
			bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
			// This statement can be changed as follows.
			//bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)pstEnum->ulValue, NULL, NULL );
			if( bRet == FALSE ) {
				free( pstEnum->pData );
				return FALSE;
			}
		}
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	free( pstEnum->pData );
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Enum(String Integer) type capability and set a value for it.
BOOL SetEnumStringCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDEnum pstEnum )
{
	BOOL	bRet;
	char	buf[256];
	UWORD	wSel;
	ULONG	i;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check the data of the capability.
	if ( pstEnum->wPhysicalBytes != 256 ) return FALSE;

	// check if this capability has elements.
	if( pstEnum->ulElements == 0 )
	{
		// This capablity has no element and is not available.
		printf( "There is no element in this capability. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
		return TRUE;
	}

	// allocate memory for array data
	pstEnum->pData = malloc( pstEnum->ulElements * pstEnum->wPhysicalBytes );
	if ( pstEnum->pData == NULL ) return FALSE;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
	if( bRet == FALSE ) {
		free( pstEnum->pData );
		return FALSE;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );
	for ( i = 0; i < pstEnum->ulElements; i++ )
		printf( "%2d. %s\n", i + 1, ((NkMAIDString*)pstEnum->pData)[i].str );
	printf( "Current Setting: %2d\n", pstEnum->ulValue + 1 );

	// check if this capability supports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
		if ( wSel > 0 && wSel <= pstEnum->ulElements ) {
			pstEnum->ulValue = wSel - 1;
			// send the selected number
			bRet =Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_EnumPtr, (NKPARAM)pstEnum, NULL, NULL );
			// This statement can be changed as follows.
			//bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)pstEnum->ulValue, NULL, NULL );
			if( bRet == FALSE ) {
				free( pstEnum->pData );
				return FALSE;
			}
		}
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	free( pstEnum->pData );
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Integer type capability and set a value for it.
BOOL SetIntegerCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	SLONG	lValue;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Integer ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_IntegerPtr, (NKPARAM)&lValue, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current Value: %d\n", lValue );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		lValue = (SLONG)atol( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Integer, (NKPARAM)lValue, NULL, NULL );
		if( bRet == FALSE ) return FALSE;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Unsigned Integer type capability and set a value for it.
BOOL SetUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	ULONG	ulValue;
	char	buf[1024];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Unsigned ) return FALSE;
	// check if this capability suports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	ulValue = 0;
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_UnsignedPtr, (NKPARAM)&ulValue, NULL, NULL );
	if( bRet == FALSE ) return FALSE;
	// show current value of this capability
	memset( buf, 0x00, sizeof(buf) );
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "%s", GetUnsignedString( ulCapID, ulValue, buf ) );
	printf("Current Value: %d\n", ulValue);

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		ulValue = (ULONG)atol( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)ulValue, NULL, NULL );
		if( bRet == FALSE ) return FALSE;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Get the current setting of a Unsigned Integer type capability.
BOOL GetUnsignedCapability( LPRefObj pRefObj, ULONG ulCapID, ULONG* pulValue )
{
    LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
    if( pCapInfo == NULL ) return FALSE;

    // check data type of the capability
    if( pCapInfo->ulType != kNkMAIDCapType_Unsigned ) return FALSE;
    // check if this capability suports CapGet operation.
    if( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

    return Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_UnsignedPtr, ( NKPARAM )pulValue, NULL, NULL );
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Float type capability and set a value for it.
BOOL SetFloatCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	double	lfValue;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Float ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_FloatPtr, (NKPARAM)&lfValue, NULL, NULL );
	if( bRet == FALSE ) return FALSE;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current Value: %f\n", lfValue );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new value\n>" );
		scanf( "%s", buf );
		lfValue = atof( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_FloatPtr, (NKPARAM)&lfValue, NULL, NULL );
		if( bRet == FALSE ) return FALSE;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a String type capability and set a value for it.
BOOL SetStringCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	NkMAIDString	stString;
	char szBuf[256] = { 0 };
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_String ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_StringPtr, (NKPARAM)&stString, NULL, NULL );
	if( bRet == FALSE ) return FALSE;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current String: %s\n", stString.str );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input new string\n>" );
#if defined( _WIN32 )
		rewind(stdin);		// clear stdin
#elif defined(__APPLE__)
		fgets( szBuf, sizeof(szBuf), stdin );
#endif
		fgets( szBuf, sizeof(szBuf), stdin );
		sscanf( szBuf, "%s", (char*)stString.str );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_StringPtr, (NKPARAM)&stString, NULL, NULL );
		if( bRet == FALSE ) return FALSE;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", stString.str );
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Size type capability and set a value for it.
BOOL SetSizeCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	NkMAIDSize	stSize;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Size ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_SizePtr, (NKPARAM)&stSize, NULL, NULL );
	if( bRet == FALSE ) return FALSE;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current Size: Width = %d    Height = %d\n", stSize.w, stSize.h );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input Width\n>" );
		scanf( "%s", buf );
		stSize.w = (ULONG)atol( buf );
		printf( "Input Height\n>" );
		scanf( "%s", buf );
		stSize.h = (ULONG)atol( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_SizePtr, (NKPARAM)&stSize, NULL, NULL );
		if( bRet == FALSE ) return FALSE;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a DateTime type capability and set a value for it.
BOOL SetDateTimeCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	NkMAIDDateTime	stDateTime;
	char	buf[256];
	UWORD	wValue;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_DateTime ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_DateTimePtr, (NKPARAM)&stDateTime, NULL, NULL );
	if( bRet == FALSE ) return FALSE;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "Current DateTime: %d/%02d/%4d %d:%02d:%02d\n",
		stDateTime.nMonth + 1, stDateTime.nDay, stDateTime.nYear, stDateTime.nHour, stDateTime.nMinute, stDateTime.nSecond );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input Month(1-12) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return TRUE;
		wValue = atoi( buf );
		if ( wValue >= 1 && wValue <= 12 )
			stDateTime.nMonth = wValue - 1;

		printf( "Input Day(1-31) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return TRUE;
		wValue = atoi( buf );
		if ( wValue >= 1 && wValue <= 31 )
			stDateTime.nDay = wValue;

		printf( "Input Year(4 digits) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return TRUE;
		wValue = atoi( buf );
		if ( wValue > 0 )
			stDateTime.nYear = wValue;

		printf( "Input Hour(0-23) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return TRUE;
		wValue = atoi( buf );
		if ( wValue >= 0 && wValue <= 23 )
			stDateTime.nHour = wValue;

		printf( "Input Minute(0-59) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return TRUE;
		wValue = atoi( buf );
		if ( wValue >= 0 && wValue <= 59 )
			stDateTime.nMinute = wValue;

		printf( "Input Second(0-59) or Cancel:'c'\n>" );
		scanf( "%s", buf );
		if ( *buf == 'c' || *buf == 'C') return TRUE;
		wValue = atoi( buf );
		if ( wValue >= 0 && wValue <= 59 )
			stDateTime.nSecond = wValue;

		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_DateTimePtr, (NKPARAM)&stDateTime, NULL, NULL );
		if( bRet == FALSE ) return FALSE;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Boolean type capability and set a value for it.
BOOL SetBoolCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	BYTE	bFlag;
	char	buf[256];
	UWORD	wSel;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Boolean ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_BooleanPtr, (NKPARAM)&bFlag, NULL, NULL );
	if( bRet == FALSE ) return FALSE;
	// show current setting of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	printf( "1. On      2. Off\n" );
	printf( "Current Setting: %d\n", bFlag ? 1 : 2 );

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input '1' or '2'\n>" );
		scanf( "%s", buf );
		wSel = atoi( buf );
		if ( (wSel == 1) || (wSel == 2) ) {
			bFlag = (wSel == 1) ? TRUE : FALSE;
			bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_Boolean, (NKPARAM)bFlag, NULL, NULL );
			if( bRet == FALSE ) return FALSE;
		}
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Show the current setting of a Range type capability and set a value for it.
BOOL SetRangeCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	NkMAIDRange	stRange;
	double	lfValue;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Range ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;

	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_RangePtr, (NKPARAM)&stRange, NULL, NULL );
	if( bRet == FALSE ) return FALSE;
	// show current value of this capability
	printf( "[%s]\n", pCapInfo->szDescription );
	
	// check if this capability supports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		if ( stRange.ulSteps == 0 ) {
			// the value of this capability is set to 'lfValue' directly
			printf( "Current Value: %f  (Max: %f  Min: %f)\n", stRange.lfValue, stRange.lfUpper, stRange.lfLower );
			printf( "Input new value.\n>" );
			scanf( "%s", buf );
			stRange.lfValue = atof( buf );
		} else {
			// the value of this capability is calculated from 'ulValueIndex'
			lfValue = stRange.lfLower + stRange.ulValueIndex * (stRange.lfUpper - stRange.lfLower) / (stRange.ulSteps - 1);
			printf( "Current Value: %f  (Max: %f  Min: %f)\n", lfValue, stRange.lfUpper, stRange.lfLower );
			printf( "Input new value.\n>" );
			scanf( "%s", buf );
			lfValue = atof( buf );
			stRange.ulValueIndex = (ULONG)((lfValue - stRange.lfLower) * (stRange.ulSteps - 1) / (stRange.lfUpper - stRange.lfLower));
		}
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_RangePtr, (NKPARAM)&stRange, NULL, NULL );
		if( bRet == FALSE ) return FALSE;
	} else {
		// This capablity is read-only.
		if ( stRange.ulSteps == 0 ) {
			// the value of this capability is set to 'lfValue' directly
			lfValue = stRange.lfValue;
		} else {
			// the value of this capability is calculated from 'ulValueIndex'
			lfValue = stRange.lfLower + stRange.ulValueIndex * (stRange.lfUpper - stRange.lfLower) / (stRange.ulSteps - 1);
		}
		printf( "Current Value: %f  (Max: %f  Min: %f)\n", lfValue, stRange.lfUpper, stRange.lfLower );
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return TRUE;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Distribute the function according to Point type.
BOOL SetPointCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet;
	NkMAIDPoint	stPoint;
	char	buf[256];
	LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Point ) return FALSE;

	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// This capablity can be set.
		printf( "Input x\n>" );
		scanf( "%s", buf );
		stPoint.x = atoi( buf );
		printf( "Input y\n>" );
		scanf( "%s", buf );
		stPoint.y = atoi( buf );
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_PointPtr, (NKPARAM)&stPoint, NULL, NULL );
		if( bRet == FALSE ) return FALSE;
	} else {
		// This capablity is read-only.
		printf( "This value cannot be changed. Enter '0' to exit.\n>" );
		scanf( "%s", buf );
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Set/Get PictureControlDataCapability
BOOL PictureControlDataCapability(LPRefObj pRefSrc, ULONG ulCapID)
{
	char buf[256], filename[256];
	NkMAIDPicCtrlData stPicCtrlData;
	ULONG	ulSel, ulPicConNo, ulTemp = 0;
	BOOL	bRet = TRUE;

	memset(filename, 0x00, sizeof(filename));

	switch (ulCapID)
	{
	case kNkMAIDCapability_PictureControlDataEx2:
		strcpy(filename, "PicCtrlData.dat");
		break;
	case kNkMAIDCapability_MoviePictureControlDataEx2:
		strcpy(filename, "MovPicCtrlData.dat");
		break;
	}
	// sub command loop
	do {
		memset(&stPicCtrlData, 0, sizeof(NkMAIDPicCtrlData));

		printf("\nSelect (1-2, 0)\n");
		printf(" 1. Set Picture ControlEx2 data the file named \"%s\"\n", filename);
		printf(" 2. Get Picture ControlEx2 data\n");
		printf(" 0. Exit\n>");
		scanf("%s", buf);
		ulSel = (ULONG)atol(buf);
		switch (ulSel)
		{
		case 1://Set Picture Control data 
			printf("\nSelect Picture Control(1-37, 0)\n");
			printf(" 1. Standard                    2. Neutral\n");
			printf(" 3. Vivid                       4. Monochrome\n");
			printf(" 5. Portrait                    6. Landscape\n");
			printf(" 7. Flat                        8. Auto\n");
			printf(" 9. Dream                       10. Morning\n");
			printf(" 11. Pop                        12. Sunday\n");
			printf(" 13. Somber                     14. Dramatic\n");
			printf(" 15. Silence                    16. Breached\n");
			printf(" 17. Melancholic                18. Pure\n");
			printf(" 19. Denim                      20. Toy\n");
			printf(" 21. Sepia                      22. Blue\n");
			printf(" 23. Red                        24. Pink\n");
			printf(" 25. Charcoal                   26. Graphite\n");
			printf(" 27. Binary                     28. Carbon\n");
			printf(" 29. Custom Picture Contol 1    30. Custom Picture Contol 2 \n");
			printf(" 31. Custom Picture Contol 3    32. Custom Picture Contol 4 \n");
			printf(" 33. Custom Picture Contol 5    34. Custom Picture Contol 6 \n");
			printf(" 35. Custom Picture Contol 7    36. Custom Picture Contol 8 \n");
			printf(" 37. Custom Picture Contol 9\n");
			printf(" 0. Exit\n>");
			scanf("%s", buf);
			ulPicConNo = atoi(buf);
			if (ulPicConNo == 0) break; //Exit
			if (ulPicConNo < 1 || ulPicConNo > 37)
			{
				printf("Invalid Picture Control\n");
				return FALSE;
			}

			if (9 <= ulPicConNo && ulPicConNo <= 28)
			{
				ulPicConNo += 92; // eNkMAIDPictureControl 101:Dream - 120:Carbon 
			}
			else if (ulPicConNo >= 29)
			{
				ulPicConNo += 172; // Custom 201 - 209
			}

			// set target Picture Control
			stPicCtrlData.ulPicCtrlItem = ulPicConNo;

			// initial registration is not supported about 1-28
			if ((stPicCtrlData.ulPicCtrlItem >= 1 && stPicCtrlData.ulPicCtrlItem <= 28))
			{
				printf("\nSelect ModifiedFlag (1, 0)\n");
				printf(" 1. edit\n");
				printf(" 0. Exit\n>");
				scanf("%s", buf);
				ulTemp = atoi(buf);
				if (ulTemp == 0) break; // Exit
				if (ulTemp < 1 || ulTemp > 1)
				{
					printf("Invalid ModifiedFlag\n");
					break;
				}
				// set Modification flas
				stPicCtrlData.bModifiedFlag = TRUE;
			}
			else
			{
				printf("\nSelect ModifiedFlag (1-2, 0)\n");
				printf(" 1. initial registration          2. edit\n");
				printf(" 0. Exit\n>");
				scanf("%s", buf);
				ulTemp = atoi(buf);
				if (ulTemp == 0) break; // Exit
				if (ulTemp < 1 || ulTemp > 2)
				{
					printf("Invalid ModifiedFlag\n");
					break;
				}
				// set Modification flas
				stPicCtrlData.bModifiedFlag = (ulTemp == 1) ? FALSE : TRUE;
			}

			bRet = SetPictureControlDataCapability(pRefSrc, &stPicCtrlData, filename, ulCapID);
			break;

		case 2://Get Picture Control data
			printf("\nSelect Picture Control(1-37, 0)\n");
			printf(" 1. Standard                    2. Neutral\n");
			printf(" 3. Vivid                       4. Monochrome\n");
			printf(" 5. Portrait                    6. Landscape\n");
			printf(" 7. Flat                        8. Auto\n");
			printf(" 9. Dream                       10. Morning\n");
			printf(" 11. Pop                        12. Sunday\n");
			printf(" 13. Somber                     14. Dramatic\n");
			printf(" 15. Silence                    16. Breached\n");
			printf(" 17. Melancholic                18. Pure\n");
			printf(" 19. Denim                      20. Toy\n");
			printf(" 21. Sepia                      22. Blue\n");
			printf(" 23. Red                        24. Pink\n");
			printf(" 25. Charcoal                   26. Graphite\n");
			printf(" 27. Binary                     28. Carbon\n");
			printf(" 29. Custom Picture Contol 1    30. Custom Picture Contol 2 \n");
			printf(" 31. Custom Picture Contol 3    32. Custom Picture Contol 4 \n");
			printf(" 33. Custom Picture Contol 5    34. Custom Picture Contol 6 \n");
			printf(" 35. Custom Picture Contol 7    36. Custom Picture Contol 8 \n");
			printf(" 37. Custom Picture Contol 9\n");
			printf(" 0. Exit\n>");
			scanf("%s", buf);
			ulPicConNo = atoi(buf);
			if (ulPicConNo == 0) break; //Exit
			if (ulPicConNo < 1 || ulPicConNo > 37)
			{
				printf("Invalid Picture Control\n");
				return FALSE;
			}

			if (9 <= ulPicConNo && ulPicConNo <= 28)
			{
				ulPicConNo += 92; // eNkMAIDPictureControl 101:Dream - 120:Carbon 
			}
			else if (ulPicConNo >= 29)
			{
				ulPicConNo += 172; // Custom 201 - 209
			}

			// set target Picture Control
			stPicCtrlData.ulPicCtrlItem = ulPicConNo;

			bRet = GetPictureControlDataCapability(pRefSrc, &stPicCtrlData, ulCapID);
			break;
		default:
			ulSel = 0;
		}

		if (bRet == FALSE)
		{
			printf("An Error occured. \n");
		}
	} while (ulSel > 0);

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Set PictureControlDataCapability
BOOL SetPictureControlDataCapability(LPRefObj pRefObj, NkMAIDPicCtrlData* pPicCtrlData, char* filename, ULONG ulCapID)
{
	BOOL	bRet = TRUE;
	FILE	*stream;
	ULONG	count = 0;
	ULONG   ulTotal = 0;
	char	*ptr = NULL;

	if (ulCapID == kNkMAIDCapability_PictureControlDataEx2 ||
		ulCapID == kNkMAIDCapability_MoviePictureControlDataEx2)
	{
		LPNkMAIDCapInfo	pCapInfo = GetCapInfo(pRefObj, ulCapID);
		if (pCapInfo == NULL) return FALSE;

		// check data type of the capability
		if (pCapInfo->ulType != kNkMAIDCapType_Generic) return FALSE;
		// check if this capability supports CapSet operation.
		if (!CheckCapabilityOperation(pRefObj, ulCapID, kNkMAIDCapOperation_Set)) return FALSE;
	}

	// Read preset data from file.
	if ((stream = fopen(filename, "rb")) == NULL)
	{
		printf("\nfile open error.\n");
		return FALSE;
	}

	if (ulCapID == kNkMAIDCapability_PictureControlDataEx2 ||
		ulCapID == kNkMAIDCapability_MoviePictureControlDataEx2)
	{
		// Max Picture Control data size is 614
		// Allocate memory for Picture Control data.
		pPicCtrlData->pData = (char*)malloc(614);
		ptr = (char*)pPicCtrlData->pData;
		while (!feof(stream))
		{
			// Read file until eof.
			count = (ULONG)fread(ptr, sizeof(char), 100, stream);
			if (ferror(stream))
			{
				printf("\nfile read error.\n");
				fclose(stream);
				free(pPicCtrlData->pData);
				return FALSE;
			}
			/* Total count up actual bytes read */
			ulTotal += count;
			ptr += count;
			if (614 < ulTotal)
			{

				printf("\nThe size of File is over 614 byte.\n");
				fclose(stream);
				free(pPicCtrlData->pData);
				return FALSE;
			}
		}
	}
	pPicCtrlData->ulSize = ulTotal;

	if (ulCapID == kNkMAIDCapability_PictureControlDataEx2 ||
		ulCapID == kNkMAIDCapability_MoviePictureControlDataEx2)
	{
		// Set Picture Control data.
		bRet = Command_CapSet(pRefObj->pObject, ulCapID, kNkMAIDDataType_GenericPtr, (NKPARAM)pPicCtrlData, NULL, NULL);
		if (bRet == FALSE)
		{
			printf("\nFailed in setting Picture Contol Data.\n");
		}
	}
	else
	{
		// Set Picture Control data.
		bRet = Command_CapSet(pRefObj->pObject, ulCapID, kNkMAIDDataType_GenericPtr, (NKPARAM)pPicCtrlData, NULL, NULL);
		if (bRet == FALSE)
		{
			printf("\nFailed in setting Picture Contol DataEx.\n");
		}
	}

	fclose(stream);
	free(pPicCtrlData->pData);

	return bRet;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Get PictureControlDataCapability
BOOL GetPictureControlDataCapability(LPRefObj pRefObj, NkMAIDPicCtrlData* pPicCtrlData, ULONG ulCapID)
{
	BOOL	bRet = TRUE;
	FILE	*stream = NULL;
	unsigned char* pucData = NULL;	// Picture Control Data pointer

	if (ulCapID == kNkMAIDCapability_PictureControlDataEx2 ||
		ulCapID == kNkMAIDCapability_MoviePictureControlDataEx2)
	{
		LPNkMAIDCapInfo	pCapInfo = GetCapInfo(pRefObj, ulCapID);
		if (pCapInfo == NULL) return FALSE;

		// check data type of the capability
		if (pCapInfo->ulType != kNkMAIDCapType_Generic) return FALSE;
		// check if this capability supports CapGet operation.
		if (!CheckCapabilityOperation(pRefObj, ulCapID, kNkMAIDCapOperation_Get)) return FALSE;
	}

	// Max Picture Control data size is 614
	// Allocate memory for Picture Control data.
	pPicCtrlData->ulSize = 614;
	pPicCtrlData->pData = (char*)malloc(614);

	if (ulCapID == kNkMAIDCapability_PictureControlDataEx2 ||
		ulCapID == kNkMAIDCapability_MoviePictureControlDataEx2)
	{
		// Get Picture Control data.
		bRet = Command_CapGet(pRefObj->pObject, ulCapID, kNkMAIDDataType_GenericPtr, (NKPARAM)pPicCtrlData, NULL, NULL);
		if (bRet == FALSE)
		{
			printf("\nFailed in getting Picture Control Data.\n");
			free(pPicCtrlData->pData);
			return FALSE;
		}
	}

	// Save to file
	// open file
	switch (ulCapID)
	{
	case kNkMAIDCapability_PictureControlDataEx2:
		stream = fopen("PicCtrlData.dat", "wb");
		break;
	case kNkMAIDCapability_MoviePictureControlDataEx2:
		stream = fopen("MovPicCtrlData.dat", "wb");
		break;
	}

	if (stream == NULL)
	{
		printf("\nfile open error.\n");
		free(pPicCtrlData->pData);
		return FALSE;
	}

	// Get data pointer
	pucData = (unsigned char*)pPicCtrlData->pData;

	// write file
	fwrite(pucData, 1, pPicCtrlData->ulSize, stream);
	switch (ulCapID)
	{
	case kNkMAIDCapability_PictureControlDataEx2:
		printf("\nPicCtrlData.dat was saved.\n");
		break;
	case kNkMAIDCapability_MoviePictureControlDataEx2:
		printf("\nMovPicCtrlData.dat was saved.\n");
		break;
	}

	// close file
	fclose(stream);
	free(pPicCtrlData->pData);

	return TRUE;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Convert LensDataString.
BOOL ConvertLensDataString(ULONG ulLensID, LPGetLensDataInfo pGetLensDataInfo)
{
	UCHAR	i = 0;

	for (i = 0; i < (pGetLensDataInfo->ulCount); i++){

		if (ulLensID == 0xFFFFFFFF){
			printf("No.%2d ", i + 1);
		}
		else{
			printf("No.%2d ", ulLensID + 1);
		}
		switch (pGetLensDataInfo->pSettingLensData[i].ucFmmManualSetting)
		{
		case kNkMAIDFmmManualSetting_NoSettting:
			printf("Focal length : NoSetting ");
			break;
		case kNkMAIDFmmManualSetting_6mm:
			printf("Focal length : 6  mm ");
			break;
		case kNkMAIDFmmManualSetting_8mm:
			printf("Focal length : 8  mm ");
			break;
		case kNkMAIDFmmManualSetting_13mm:
			printf("Focal length : 13 mm ");
			break;
		case kNkMAIDFmmManualSetting_15mm:
			printf("Focal length : 15 mm ");
			break;
		case kNkMAIDFmmManualSetting_16mm:
			printf("Focal length : 16 mm ");
			break;
		case kNkMAIDFmmManualSetting_18mm:
			printf("Focal length : 18 mm ");
			break;
		case kNkMAIDFmmManualSetting_20mm:
			printf("Focal length : 20 mm ");
			break;
		case kNkMAIDFmmManualSetting_24mm:
			printf("Focal length : 24 mm ");
			break;
		case kNkMAIDFmmManualSetting_25mm:
			printf("Focal length : 25 mm ");
			break;
		case kNkMAIDFmmManualSetting_28mm:
			printf("Focal length : 28 mm ");
			break;
		case kNkMAIDFmmManualSetting_35mm:
			printf("Focal length : 35 mm ");
			break;
		case kNkMAIDFmmManualSetting_43mm:
			printf("Focal length : 43 mm ");
			break;
		case kNkMAIDFmmManualSetting_45mm:
			printf("Focal length : 45 mm ");
			break;
		case kNkMAIDFmmManualSetting_50mm:
			printf("Focal length : 50 mm ");
			break;
		case kNkMAIDFmmManualSetting_55mm:
			printf("Focal length : 55 mm ");
			break;
		case kNkMAIDFmmManualSetting_58mm:
			printf("Focal length : 58 mm ");
			break;
		case kNkMAIDFmmManualSetting_70mm:
			printf("Focal length : 70 mm ");
			break;
		case kNkMAIDFmmManualSetting_80mm:
			printf("Focal length : 80 mm ");
			break;
		case kNkMAIDFmmManualSetting_85mm:
			printf("Focal length : 85 mm ");
			break;
		case kNkMAIDFmmManualSetting_86mm:
			printf("Focal length : 86 mm ");
			break;
		case kNkMAIDFmmManualSetting_100mm:
			printf("Focal length : 100mm ");
			break;
		case kNkMAIDFmmManualSetting_105mm:
			printf("Focal length : 105mm ");
			break;
		case kNkMAIDFmmManualSetting_135mm:
			printf("Focal length : 135mm ");
			break;
		case kNkMAIDFmmManualSetting_180mm:
			printf("Focal length : 180mm ");
			break;
		case kNkMAIDFmmManualSetting_200mm:
			printf("Focal length : 200mm ");
			break;
		case kNkMAIDFmmManualSetting_300mm:
			printf("Focal length : 300mm ");
			break;
		case kNkMAIDFmmManualSetting_360mm:
			printf("Focal length : 360mm ");
			break;
		case kNkMAIDFmmManualSetting_400mm:
			printf("Focal length : 400mm ");
			break;
		case kNkMAIDFmmManualSetting_500mm:
			printf("Focal length : 500mm ");
			break;
		case kNkMAIDFmmManualSetting_600mm:
			printf("Focal length : 600mm ");
			break;
		case kNkMAIDFmmManualSetting_800mm:
			printf("Focal length : 800mm ");
			break;
		case kNkMAIDFmmManualSetting_1000mm:
			printf("Focal length : 1000mm ");
			break;
		case kNkMAIDFmmManualSetting_1200mm:
			printf("Focal length : 1200mm ");
			break;
		case kNkMAIDFmmManualSetting_1400mm:
			printf("Focal length : 1400mm ");
			break;
		case kNkMAIDFmmManualSetting_1600mm:
			printf("Focal length : 1600mm ");
			break;
		case kNkMAIDFmmManualSetting_2000mm:
			printf("Focal length : 2000mm ");
			break;
		case kNkMAIDFmmManualSetting_2400mm:
			printf("Focal length : 2400mm ");
			break;
		case kNkMAIDFmmManualSetting_2800mm:
			printf("Focal length : 2800mm ");
			break;
		case kNkMAIDFmmManualSetting_3200mm:
			printf("Focal length : 3200mm ");
			break;
		case kNkMAIDFmmManualSetting_4000mm:
			printf("Focal length : 4000mm ");
			break;
		default:
			printf("Focal length : Out Of Range ");
			break;
		}

		switch (pGetLensDataInfo->pSettingLensData[i].ucF0ManualSetting)
		{
		case kNkMAIDF0ManualSetting_NoSettting:
			printf("F Number : NoSetting\n");
			break;
		case kNkMAIDF0ManualSetting_F1_2:
			printf("F Number : F1.2\n");
			break;
		case kNkMAIDF0ManualSetting_F1_4:
			printf("F Number : F1.4\n");
			break;
		case kNkMAIDF0ManualSetting_F1_8:
			printf("F Number : F1.8\n");
			break;
		case kNkMAIDF0ManualSetting_F2_0:
			printf("F Number : F2.0\n");
			break;
		case kNkMAIDF0ManualSetting_F2_5:
			printf("F Number : F2.5\n");
			break;
		case kNkMAIDF0ManualSetting_F2_8:
			printf("F Number : F2.8\n");
			break;
		case kNkMAIDF0ManualSetting_F3_3:
			printf("F Number : F3.3\n");
			break;
		case kNkMAIDF0ManualSetting_F3_5:
			printf("F Number : F3.5\n");
			break;
		case kNkMAIDF0ManualSetting_F4_0:
			printf("F Number : F4.0\n");
			break;
		case kNkMAIDF0ManualSetting_F4_5:
			printf("F Number : F4.5\n");
			break;
		case kNkMAIDF0ManualSetting_F5_0:
			printf("F Number : F5.0\n");
			break;
		case kNkMAIDF0ManualSetting_F5_6:
			printf("F Number : F5.6\n");
			break;
		case kNkMAIDF0ManualSetting_F6_3:
			printf("F Number : F6.3\n");
			break;
		case kNkMAIDF0ManualSetting_F7_1:
			printf("F Number : F7.1\n");
			break;
		case kNkMAIDF0ManualSetting_F8_0:
			printf("F Number : F8.0\n");
			break;
		case kNkMAIDF0ManualSetting_F9_5:
			printf("F Number : F9.5\n");
			break;
		case kNkMAIDF0ManualSetting_F11:
			printf("F Number : F11\n");
			break;
		case kNkMAIDF0ManualSetting_F13:
			printf("F Number : F13\n");
			break;
		case kNkMAIDF0ManualSetting_F15:
			printf("F Number : F15\n");
			break;
		case kNkMAIDF0ManualSetting_F16:
			printf("F Number : F16\n");
			break;
		case kNkMAIDF0ManualSetting_F19:
			printf("F Number : F19\n");
			break;
		case kNkMAIDF0ManualSetting_F22:
			printf("F Number : F22\n");
			break;
		default:
			printf("F Number : Out Of Range\n");
			break;
		}
	}
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Set TrackingAFArea.
BOOL SetTrackingAFAreaCapability(LPRefObj pRefSrc)
{
	BOOL	bRet;
	NkMAIDTrackingAFArea stTrackingAFArea;
	char	buf[256];
	UCHAR	ucValue;

	memset(&stTrackingAFArea, 0, sizeof(NkMAIDTrackingAFArea));

	//Check Operation
	LPNkMAIDCapInfo pCapInfo = GetCapInfo(pRefSrc, kNkMAIDCapability_TrackingAFArea);
	if (pCapInfo == NULL) return FALSE;

	// check data type of the capability
	if (pCapInfo->ulType != kNkMAIDCapType_Generic) return FALSE;

	if (!CheckCapabilityOperation(pRefSrc, kNkMAIDCapability_TrackingAFArea, kNkMAIDCapOperation_Set)) {
		// This capablity is read-only.
		printf("This value cannot be changed. Enter '0' to exit.\n>");
		scanf("%s", buf);
		return FALSE;
	}

	printf("1. Stop Tracking \n");
	printf("2. Start Tracking \n");
	printf("Input Number \n>");
	scanf("%s", buf);
	ucValue = (UCHAR)(atoi(buf));
	if (ucValue == 1)
	{
		stTrackingAFArea.ulTrackingStatus = 0;
	}
	else
	if (ucValue == 2)
	{
		stTrackingAFArea.ulTrackingStatus = 1;
		printf("Input XY coordinates \n");
		printf("X = \n");
		scanf("%s", buf);
		stTrackingAFArea.stAfPoint.x = atoi(buf);
		printf("Y = \n");
		scanf("%s", buf);
		stTrackingAFArea.stAfPoint.y = atoi(buf);
	}
	else
	{
		printf("Invalid Number\n");
		return FALSE;
	}


	// This capablity can be set.
	bRet = Command_CapSet(pRefSrc->pObject, kNkMAIDCapability_TrackingAFArea, kNkMAIDDataType_GenericPtr, (NKPARAM)&stTrackingAFArea, NULL, NULL);
	if (bRet == FALSE) return FALSE;

	return TRUE;

}


//------------------------------------------------------------------------------------------------------------------------------------
//Delete Dram Image
BOOL DeleteDramCapability( LPRefObj pRefItem, ULONG ulItmID )
{
	LPRefObj	pRefSrc = (LPRefObj)pRefItem->pRefParent;
	LPRefObj	pRefDat = NULL;
	BOOL	bRet = TRUE;
	NkMAIDCallback	stProc;
	LPRefDataProc	pRefDeliver;
	LPRefCompletionProc	pRefCompletion;
	ULONG	ulCount = 0L;
	SLONG nResult;


	// 1. Open ImageObject
	pRefDat = GetRefChildPtr_ID( pRefItem, kNkMAIDDataObjType_Image );
	if ( pRefDat == NULL )
	{
		// Create Image object and RefSrc structure.
		if ( AddChild( pRefItem, kNkMAIDDataObjType_Image ) == FALSE )
		{
			printf("Image object can't be opened.\n");
			return FALSE;
		}
		pRefDat = GetRefChildPtr_ID( pRefItem, kNkMAIDDataObjType_Image );
	}

	// 2. Set DataProc function
	// 2-1. set reference from DataProc
	pRefDeliver = (LPRefDataProc)malloc( sizeof(RefDataProc) );// this block will be freed in CompletionProc.
	pRefDeliver->pBuffer = NULL;
	pRefDeliver->ulOffset = 0L;
	pRefDeliver->ulTotalLines = 0L;
	pRefDeliver->lID = pRefItem->lMyID;
	// 2-2. set reference from CompletionProc
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );// this block will be freed in CompletionProc.
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = pRefDeliver;
	// 2-3. set reference from DataProc
	stProc.pProc = (LPNKFUNC)DataProc;
	stProc.refProc = (NKREF)pRefDeliver;
	// 2-4. set DataProc as data delivery callback function
	if( CheckCapabilityOperation( pRefDat, kNkMAIDCapability_DataProc, kNkMAIDCapOperation_Set ) )
	{
		bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;
	} 
	else
	{
		return FALSE;
	}
		
	// 3. Acquire image
	bRet = Command_CapStart( pRefDat->pObject, kNkMAIDCapability_Acquire, (LPNKFUNC)CompletionProc, (NKREF)pRefCompletion, &nResult );
	if ( bRet == FALSE ) return FALSE;

	if (nResult == kNkMAIDResult_NoError)
	{
		// image had read before issuing delete command.
		printf("\nInternal ID [0x%08X] had read before issuing delete command.\n", ulItmID );
	}
	else
	if (nResult == kNkMAIDResult_Pending)
	{
		// 4. Async command
		bRet = Command_Async( pRefDat->pObject );
		if ( bRet == FALSE ) return FALSE;

		// 5. Abort
		bRet = Command_Abort( pRefDat->pObject, NULL, NULL);
		if ( bRet == FALSE ) return FALSE;
		
		// 6. Set Item ID
		bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_CurrentItemID, kNkMAIDDataType_Unsigned, (NKPARAM)ulItmID, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;

		// 7. Delete DRAM (Delete timing No.2)
		bRet = Command_CapStart( pRefSrc->pObject, kNkMAIDCapability_DeleteDramImage, NULL, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;

		// 8. Reset DataProc
		bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;

		printf("\nInternal ID [0x%08X] was deleted.\n", ulItmID );
	}
	
	// Upper function to close ItemObject. 
	g_bFileRemoved = TRUE;
	// progress proc flag reset 
	g_bFirstCall = TRUE;

	// 9. Close ImageObject
	bRet = RemoveChild( pRefItem, kNkMAIDDataObjType_Image );


	return bRet;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Get Live view image
BOOL GetLiveViewImageCapability( LPRefObj pRefSrc )
{
	char	HeaderFileName[256], ImageFileName[256];
	FILE*	hFileHeader = NULL;		// LiveView Image file name
	FILE*	hFileImage = NULL;		// LiveView header file name
	ULONG	ulHeaderSize = 0;		//The header size of LiveView
	NkMAIDArray	stArray;
	int i = 0;
	unsigned char* pucData = NULL;	// LiveView data pointer
	BOOL	bRet = TRUE;


	// Set header size of LiveView
	if ( g_ulCameraType == kNkMAIDCameraType_Z_50 )
	{
		ulHeaderSize = 512;
	}

	memset( &stArray, 0, sizeof(NkMAIDArray) );		
	
	bRet = GetArrayCapability( pRefSrc, kNkMAIDCapability_GetLiveViewImage, &stArray );
	if ( bRet == FALSE ) return FALSE;
		
	// create file name
	while( TRUE )
	{
		sprintf( HeaderFileName, "LiveView%03d_H.%s", ++i, "dat" );
		sprintf( ImageFileName, "LiveView%03d.%s", i, "jpg" );
		if ( (hFileHeader = fopen(HeaderFileName, "r") ) != NULL ||
				(hFileImage  = fopen(ImageFileName, "r") )  != NULL )
		{
			// this file name is already used.
			if (hFileHeader)
			{
				fclose( hFileHeader );
				hFileHeader = NULL;
			}
			if (hFileImage)
			{
				fclose( hFileImage );
				hFileImage = NULL;		
			}
		}	 
		else
		{
			break;
		}
	}
		
	// open file
	hFileHeader = fopen( HeaderFileName, "wb" );
	if ( hFileHeader == NULL )
	{
		printf("file open error.\n");
		return FALSE;
	}				
	hFileImage = fopen( ImageFileName, "wb" );
	if ( hFileImage == NULL )
	{
		fclose( hFileHeader );
		printf("file open error.\n");
		return FALSE;
	}
	
	// Get data pointer
	pucData = (unsigned char*)stArray.pData;

	// write file
	fwrite( pucData, 1, ulHeaderSize, hFileHeader );
	fwrite( pucData+ulHeaderSize, 1, (stArray.ulElements-ulHeaderSize), hFileImage );
	printf("\n%s was saved.\n", HeaderFileName);
	printf("%s was saved.\n", ImageFileName);
		
	// close file
	fclose( hFileHeader );
	fclose( hFileImage );
	free( stArray.pData );

	return TRUE;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Delete Custom Picture Control
BOOL DeleteCustomPictureControlCapability(LPRefObj pRefSrc, ULONG ulCapID)
{
	ULONG	ulSel = 0;
	BOOL	bRet;
	ULONG	ulValue;
	char	buf[256];

	printf( "\nSelect Custom Picture Control to delete.(1-9, 0)\n");

	printf( "1. Custom Picture Contol 1\n");
	printf( "2. Custom Picture Contol 2\n");
	printf( "3. Custom Picture Contol 3\n");
	printf( "4. Custom Picture Contol 4\n");
	printf( "5. Custom Picture Contol 5\n");
	printf( "6. Custom Picture Contol 6\n");
	printf( "7. Custom Picture Contol 7\n");
	printf( "8. Custom Picture Contol 8\n");
	printf( "9. Custom Picture Contol 9\n");
	printf( "0. Exit\n>" );
	scanf( "%s", buf );

	ulSel = atoi( buf );
	if ( ulSel == 0 ) return TRUE; // Exit
	if ( ulSel < 1 || ulSel > 9 ) 
	{
		printf("Invalid Custom Picture Control\n");
		return FALSE;
	}
	ulSel += 200;		// Custom 201 - 209
	ulValue = ulSel;	// Set Custom Picture Control to delete
	if (CheckCapabilityOperation(pRefSrc, ulCapID, kNkMAIDCapOperation_Set)){
		bRet = Command_CapSet(pRefSrc->pObject, ulCapID, kNkMAIDDataType_Unsigned, (NKPARAM)ulValue, NULL, NULL);
		return bRet;
	}
	else{
		return FALSE;
	}
}
//------------------------------------------------------------------------------------------------------------------------------------
// read the array data from the camera and display it on the screen
BOOL ShowArrayCapability( LPRefObj pRefObj, ULONG ulCapID )
{
	BOOL	bRet = TRUE;
	NkMAIDArray	stArray;
	ULONG	ulSize, i, j;
	LPNkMAIDCapInfo	pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Array ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	ulSize = stArray.ulElements * stArray.wPhysicalBytes;
	// allocate memory for array data
	stArray.pData = malloc( ulSize );
	if ( stArray.pData == NULL ) return FALSE;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
	if( bRet == FALSE ) {
		free( stArray.pData );
		return FALSE;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );
	for ( i = 0, j = 0; i*16+j < ulSize; i++ ) {
		for ( ; j < 16 && i*16+j < ulSize; j++ ) {
			printf( " %02X", ((UCHAR*)stArray.pData)[i*16+j] );
		}
		j = 0;
		printf( "\n" );
	}

	if ( stArray.pData != NULL )
		free( stArray.pData );
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// read the array data from the camera and save it on a storage(hard drive)
//  for kNkMAIDCapability_GetLiveViewImage
BOOL GetArrayCapability( LPRefObj pRefObj, ULONG ulCapID, LPNkMAIDArray pstArray )
{
	BOOL	bRet = TRUE;
	LPNkMAIDCapInfo	pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Array ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)pstArray, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	// check if this capability supports CapGetArray operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_GetArray ) ) return FALSE;
	// allocate memory for array data
	pstArray->pData = malloc( pstArray->ulElements * pstArray->wPhysicalBytes );
	if ( pstArray->pData == NULL ) return FALSE;
	// get array data
	bRet = Command_CapGetArray( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)pstArray, NULL, NULL );
	if( bRet == FALSE ) {
		free( pstArray->pData );
		pstArray->pData = NULL;
		return FALSE;
	}

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );

	// Do not free( pstArray->pData )
	// Upper class use pstArray->pData to save file.
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// load the array data from a storage and send it to the camera
BOOL LoadArrayCapability( LPRefObj pRefObj, ULONG ulCapID, char* filename )
{
	BOOL	bRet = TRUE;
	NkMAIDArray	stArray;
	FILE *stream;
	LPNkMAIDCapInfo	pCapInfo = GetCapInfo( pRefObj, ulCapID );
	if ( pCapInfo == NULL ) return FALSE;

	// check data type of the capability
	if ( pCapInfo->ulType != kNkMAIDCapType_Array ) return FALSE;
	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Get ) ) return FALSE;
	bRet = Command_CapGet( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
	if( bRet == FALSE ) return FALSE;

	// allocate memory for array data
	stArray.pData = malloc( stArray.ulElements * stArray.wPhysicalBytes );
	if ( stArray.pData == NULL ) return FALSE;

	// show selectable items for this capability and current setting
	printf( "[%s]\n", pCapInfo->szDescription );

	if ( (stream = fopen( filename, "rb" ) ) == NULL) {
		printf( "file not found\n" );
		if ( stArray.pData != NULL )
			free( stArray.pData );
		return FALSE;
	}
	fread( stArray.pData, 1, stArray.ulElements * stArray.wPhysicalBytes, stream );
	fclose( stream );

	// check if this capability supports CapSet operation.
	if ( CheckCapabilityOperation( pRefObj, ulCapID, kNkMAIDCapOperation_Set ) ) {
		// set array data
		bRet = Command_CapSet( pRefObj->pObject, ulCapID, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
		if( bRet == FALSE ) {
			free( stArray.pData );
			return FALSE;
		}
	}
	if ( stArray.pData != NULL )
		free( stArray.pData );
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// make look up table and send it to module.
BOOL SetNewLut( LPRefObj pRefSrc )
{
	BOOL	bRet;
	NkMAIDArray stArray;
	double	lfGamma, dfMaxvalue;
	ULONG	i, ulLUTDimSize, ulPlaneCount;
	char	buf[256];

	printf( "Gamma >");
	scanf( "%s", buf );
	lfGamma = atof( buf );

	bRet = Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_Lut, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
	if( bRet == FALSE ) return FALSE;
	stArray.pData = malloc( stArray.ulElements * stArray.wPhysicalBytes );
	if( stArray.pData == NULL ) return FALSE;

	ulLUTDimSize = stArray.ulDimSize1;
	ulPlaneCount = stArray.ulDimSize2;
	// If the array is one dimension, ulDimSize2 is 0. So the ulPlaneCount should be set 1.
	if ( ulPlaneCount == 0 ) ulPlaneCount = 1;

	dfMaxvalue = (double)( pow( 2.0, stArray.wLogicalBits ) - 1.0);

	// Make first plane of LookUp Table
	if(stArray.wPhysicalBytes == 1) {
		for( i = 0; i < ulLUTDimSize; i++)
			((UCHAR*)stArray.pData)[i] = (UCHAR)( pow( ((double)i / ulLUTDimSize), 1.0 / lfGamma ) * dfMaxvalue + 0.5 ); 
	} else if(stArray.wPhysicalBytes == 2) {
		for( i = 0; i < ulLUTDimSize; i++)
			((UWORD*)stArray.pData)[i] = (UWORD)( pow( ((double)i / ulLUTDimSize), 1.0 / lfGamma ) * dfMaxvalue + 0.5 ); 
	} else {
		free(stArray.pData);
		return FALSE;
	}
	// Copy from first plane to second and third... plane.
	for( i = 1; i < ulPlaneCount; i++)
		memcpy( (LPVOID)((char*)stArray.pData + i * ulLUTDimSize * stArray.wPhysicalBytes), stArray.pData, ulLUTDimSize * stArray.wPhysicalBytes );

	// check if this capability supports CapSet operation.
	if ( CheckCapabilityOperation( pRefSrc, kNkMAIDCapability_Lut, kNkMAIDCapOperation_Set ) ) {
		// Send look up table
		bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_Lut, kNkMAIDDataType_ArrayPtr, (NKPARAM)&stArray, NULL, NULL );
		if( bRet == FALSE ) {
			free( stArray.pData );
			return FALSE;
		}
	}
	free(stArray.pData);
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL IssueProcess( LPRefObj pRefSrc, ULONG ulCapID )
{
	LPNkMAIDObject pSourceObject = pRefSrc->pObject;
	LPNkMAIDCapInfo pCapInfo;
	ULONG	ulCount = 0L;
	BOOL bRet;
	LPRefCompletionProc pRefCompletion;
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;

	// Confirm whether this capability is supported or not.
	pCapInfo =	GetCapInfo( pRefSrc, ulCapID );
	// check if the CapInfo is available.
	if ( pCapInfo == NULL ) return FALSE;

	printf( "[%s]\n", pCapInfo->szDescription );

	// Start the process
	bRet = Command_CapStart( pSourceObject, ulCapID, (LPNKFUNC)CompletionProc, (NKREF)pRefCompletion, NULL );
	if ( bRet == FALSE ) return FALSE;
	// Wait for end of the process and issue Command_Async.
	IdleLoop( pSourceObject, &ulCount, 1 );

	return TRUE;
}

//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL TerminateCaptureCapability( LPRefObj pRefSrc )
{
	LPNkMAIDObject pSourceObject = pRefSrc->pObject;
	LPNkMAIDCapInfo pCapInfo;
	ULONG	ulCount = 0L;
	BOOL bRet;
	NkMAIDTerminateCapture Param;
	LPRefCompletionProc pRefCompletion;

	Param.ulParameter1 = 0;
	Param.ulParameter2 = 0;

	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = NULL;

	// Confirm whether this capability is supported or not.
	pCapInfo =	GetCapInfo( pRefSrc, kNkMAIDCapability_TerminateCapture );
	// check if the CapInfo is available.
	if ( pCapInfo == NULL ) return FALSE;

	printf( "[%s]\n", pCapInfo->szDescription );

	// Start the process
	bRet = Command_CapStartGeneric( pSourceObject, kNkMAIDCapability_TerminateCapture, (NKPARAM)&Param,(LPNKFUNC)CompletionProc, (NKREF)pRefCompletion, NULL );
	if ( bRet == FALSE ) return FALSE;

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
BOOL GetRecordingInfoCapability(LPRefObj pRefObj)
{
	BOOL bRet = TRUE;
	NkMAIDGetRecordingInfo stGetRecordingInfo;
	LPNkMAIDCapInfo pCapInfo = NULL;

	pCapInfo = GetCapInfo(pRefObj, kNkMAIDCapability_GetRecordingInfo);
	if (pCapInfo == NULL)
	{
		return FALSE;
	}

	// check data type of the capability
	if (pCapInfo->ulType != kNkMAIDCapType_Generic)
	{
		return FALSE;
	}

	// check if this capability supports CapGet operation.
	if (!CheckCapabilityOperation(pRefObj, kNkMAIDCapability_GetRecordingInfo, kNkMAIDCapOperation_Get))
	{
		return FALSE;
	}

	bRet = Command_CapGet(pRefObj->pObject, kNkMAIDCapability_GetRecordingInfo, kNkMAIDDataType_GenericPtr, (NKPARAM)&stGetRecordingInfo, NULL, NULL);
	if (bRet == FALSE)
	{
		// abnomal.
		printf("\nFailed in getting RecordingInfo.\n");

		return FALSE;
	}

	printf("File Index                  %d\n", stGetRecordingInfo.ulIndexOfMov);
	printf("The number of divided files %d\n", stGetRecordingInfo.ulTotalMovCount);
	printf("Total Size                  %llu[byte]\n", stGetRecordingInfo.ullTotalMovSize);

	return TRUE;
}

//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL IssueProcessSync( LPRefObj pRefSrc, ULONG ulCapID )
{
	LPNkMAIDObject pSourceObject = pRefSrc->pObject;
	LPNkMAIDCapInfo pCapInfo;
	BOOL bRet;
	// Confirm whether this capability is supported or not.
	pCapInfo =	GetCapInfo( pRefSrc, ulCapID );
	// check if the CapInfo is available.
	if ( pCapInfo == NULL ) return FALSE;

	printf( "[%s]\n", pCapInfo->szDescription );

	// Start the process
	bRet = Command_CapStart( pSourceObject, ulCapID, NULL, NULL, NULL );
	if ( bRet == FALSE ) return FALSE;

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL IssueAcquire( LPRefObj pRefDat )
{
	BOOL	bRet;
	LPRefObj	pRefItm = (LPRefObj)pRefDat->pRefParent;
	NkMAIDCallback	stProc;
	LPRefDataProc	pRefDeliver;
	LPRefCompletionProc	pRefCompletion;
	ULONG	ulCount = 0L;

	// set reference from DataProc
	pRefDeliver = (LPRefDataProc)malloc( sizeof(RefDataProc) );// this block will be freed in CompletionProc.
	pRefDeliver->pBuffer = NULL;
	pRefDeliver->ulOffset = 0L;
	pRefDeliver->ulTotalLines = 0L;
	pRefDeliver->lID = pRefItm->lMyID;
	// set reference from CompletionProc
	pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );// this block will be freed in CompletionProc.
	pRefCompletion->pulCount = &ulCount;
	pRefCompletion->pRef = pRefDeliver;
	// set reference from DataProc
	stProc.pProc = (LPNKFUNC)DataProc;
	stProc.refProc = (NKREF)pRefDeliver;

	// set DataProc as data delivery callback function
	if( CheckCapabilityOperation( pRefDat, kNkMAIDCapability_DataProc, kNkMAIDCapOperation_Set ) ) {
		bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;
	} else
		return FALSE;

	// start getting an image
	bRet = Command_CapStart( pRefDat->pObject, kNkMAIDCapability_Acquire, (LPNKFUNC)CompletionProc, (NKREF)pRefCompletion, NULL );
	if ( bRet == FALSE ) return FALSE;
	IdleLoop( pRefDat->pObject, &ulCount, 1 );

	// reset DataProc
	bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
	if ( bRet == FALSE ) return FALSE;

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Get Video image
BOOL GetVideoImageExCapability(LPRefObj pRefDat, ULONG ulCapID)
{
	BOOL	bRet = TRUE;
	char	MovieFileName[256];
	FILE*	hFileMovie = NULL;		// Movie file name
	unsigned char* pucData = NULL;	// Movie data pointer
	NK_UINT_64	ullTotalSize = 0;
	int i = 0;
	NkMAIDGetVideoImageEx	stVideoImage;
	NkMAIDEnum	stEnum;
	LPRefObj pobject;

#if defined( _WIN32 )
	SetConsoleCtrlHandler(cancelhandler, TRUE);
#elif defined(__APPLE__)
	struct sigaction action, oldaction;
	memset(&action, 0, sizeof(action));
	action.sa_handler = cancelhandler;
	action.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &action, &oldaction);
#endif

	memset(&stVideoImage, 0, sizeof(NkMAIDGetVideoImageEx));
	memset(&pobject, 0, sizeof(LPRefObj));

	// get total size
	stVideoImage.ullDataSize = 0;
	bRet = Command_CapGet(pRefDat->pObject, ulCapID, kNkMAIDDataType_GenericPtr, (NKPARAM)&stVideoImage, NULL, NULL);
	if (bRet == FALSE) return FALSE;

	ullTotalSize = stVideoImage.ullDataSize;
	if (ullTotalSize == 0) return FALSE;

	// get movie data
	stVideoImage.ullDataSize = VIDEO_SIZE_BLOCK;		// read block size : 5MB
	stVideoImage.ullReadSize = 0;
	stVideoImage.ullOffset = 0;

	// allocate memory for array data
	stVideoImage.pData = malloc(VIDEO_SIZE_BLOCK);
	if (stVideoImage.pData == NULL) return FALSE;

	// get movie file type
	LPRefObj pRefItem = (LPRefObj)pRefDat->pRefParent;
	if (!pRefItem) return FALSE;
	LPRefObj pRefSource = (LPRefObj)pRefItem->pRefParent;
	if (!pRefSource) return FALSE;
	bRet = Command_CapGet(pRefSource->pObject, kNkMAIDCapability_MovieFileType, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL);
	if (bRet == FALSE) return FALSE;

	// create file name
	while (TRUE)
	{
		if (stEnum.ulValue == 0){
			sprintf(MovieFileName, "MovieData%03d.%s", ++i, "mov");
		}
		else if (stEnum.ulValue == 1){
			sprintf(MovieFileName, "MovieData%03d.%s", ++i, "mp4");
		}
		if ((hFileMovie = fopen(MovieFileName, "r")) != NULL)
		{
			// this file name is already used.
			fclose(hFileMovie);
			hFileMovie = NULL;
		}
		else
		{
			break;
		}
	}

	// open file
	hFileMovie = fopen(MovieFileName, "wb");
	if (hFileMovie == NULL)
	{
		fclose(hFileMovie);
		printf("file open error.\n");
		return FALSE;
	}

	// Get data pointer
	pucData = (unsigned char*)stVideoImage.pData;

	printf("Please press the Ctrl+C to cancel.\n");

	// write file
	while ((stVideoImage.ullOffset < ullTotalSize) && (bRet == TRUE))
	{
		if (TRUE == g_bCancel)
		{
			stVideoImage.ullDataSize = 0;
			bRet = Command_CapGetArray(pRefDat->pObject, ulCapID, kNkMAIDDataType_GenericPtr, (NKPARAM)&stVideoImage, NULL, NULL);
			break;
		}

		bRet = Command_CapGetArray(pRefDat->pObject, ulCapID, kNkMAIDDataType_GenericPtr, (NKPARAM)&stVideoImage, NULL, NULL);

		stVideoImage.ullOffset += (stVideoImage.ullReadSize);

		fwrite(pucData, (ULONG)stVideoImage.ullReadSize, 1, hFileMovie);

		if (bRet == FALSE) {
			free(stVideoImage.pData);
			return FALSE;
		}

	}
#if defined( _WIN32 )
	SetConsoleCtrlHandler(cancelhandler, FALSE);
#elif defined(__APPLE__)
	sigaction(SIGINT, &oldaction, NULL);
#endif

	if (stVideoImage.ullOffset < ullTotalSize && TRUE == g_bCancel)
	{
		printf("Get Video image was canceled.\n");
	}
	else {
		printf("%s was saved.\n", MovieFileName);
	}
	g_bCancel = FALSE;

	// close file
	fclose(hFileMovie);
	free(stVideoImage.pData);

	return TRUE;
}

//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL IssueThumbnail( LPRefObj pRefSrc )
{
	BOOL	bRet;
	LPRefObj	pRefItm, pRefDat;
	NkMAIDCallback	stProc;
	LPRefDataProc	pRefDeliver;
	LPRefCompletionProc	pRefCompletion;
	ULONG	ulItemID, ulFinishCount = 0L;
	ULONG	i, j;
	NkMAIDEnum	stEnum;
	LPNkMAIDCapInfo	pCapInfo;
	
	pCapInfo = GetCapInfo( pRefSrc, kNkMAIDCapability_Children );
	// check if the CapInfo is available.
	if ( pCapInfo == NULL )	return FALSE;

	// check if this capability supports CapGet operation.
	if ( !CheckCapabilityOperation( pRefSrc, kNkMAIDCapability_Children, kNkMAIDCapOperation_Get ) ) return FALSE;
	bRet = Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if ( bRet == FALSE ) return FALSE;

	// If the source object has no item, it does nothing and returns soon.
	if ( stEnum.ulElements == 0 )	return TRUE;

	// allocate memory for array data
	stEnum.pData = malloc( stEnum.ulElements * stEnum.wPhysicalBytes );
	if ( stEnum.pData == NULL ) return FALSE;
	// get array data
	bRet = Command_CapGetArray( pRefSrc->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
	if ( bRet == FALSE ) {
		free( stEnum.pData );
		return FALSE;
	}

	// Open all thumbnail objects in the current directory.
	for ( i = 0; i < stEnum.ulElements; i++ ) {
		ulItemID = ((ULONG*)stEnum.pData)[i];
		pRefItm = GetRefChildPtr_ID( pRefSrc, ulItemID );
		if ( pRefItm == NULL ) {
			// open the item object
			bRet = AddChild( pRefSrc, ulItemID );
			if ( bRet == FALSE ) {
				free( stEnum.pData );
				return FALSE;
			}
			pRefItm = GetRefChildPtr_ID( pRefSrc, ulItemID );
		}
		if ( pRefItm != NULL ) {
			pRefDat = GetRefChildPtr_ID( pRefItm, kNkMAIDDataObjType_Thumbnail );
			if ( pRefDat == NULL ) {
				// open the thumbnail object
				bRet = AddChild( pRefItm, kNkMAIDDataObjType_Thumbnail );
				if ( bRet == FALSE ) {
					free( stEnum.pData );
					return FALSE;
				}
				pRefDat = GetRefChildPtr_ID( pRefItm, kNkMAIDDataObjType_Thumbnail );
			}
		}
	}
	free ( stEnum.pData );

	// set NkMAIDCallback structure for DataProc
	stProc.pProc = (LPNKFUNC)DataProc;

	// acquire all thumbnail images.
	for ( i = 0; i < pRefSrc->ulChildCount; i++ ) {
		pRefItm = GetRefChildPtr_Index( pRefSrc, i );
		pRefDat = GetRefChildPtr_ID( pRefItm, kNkMAIDDataObjType_Thumbnail );

		if ( pRefDat != NULL ) {
			// set RefDeliver structure refered in DataProc
			pRefDeliver = (LPRefDataProc)malloc( sizeof(RefDataProc) );// this block will be freed in CompletionProc.
			pRefDeliver->pBuffer = NULL;
			pRefDeliver->ulOffset = 0L;
			pRefDeliver->ulTotalLines = 0L;
			pRefDeliver->lID = pRefItm->lMyID;

			// set DataProc as data delivery callback function
			stProc.refProc = (NKREF)pRefDeliver;
			if( CheckCapabilityOperation( pRefDat, kNkMAIDCapability_DataProc, kNkMAIDCapOperation_Set ) ) {
				bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == FALSE ) return FALSE;
			} else
				return FALSE;

			pRefCompletion = (LPRefCompletionProc)malloc( sizeof(RefCompletionProc) );// this block will be freed in CompletionProc.

			// Set RefCompletion structure refered from CompletionProc.
			pRefCompletion->pulCount = &ulFinishCount;
			pRefCompletion->pRef = pRefDeliver;

			// Starting Acquire Thumbnail
			bRet = Command_CapStart( pRefDat->pObject, kNkMAIDCapability_Acquire, (LPNKFUNC)CompletionProc, (NKREF)pRefCompletion, NULL );
			if ( bRet == FALSE ) return FALSE;
		} else {
			// This item doesn't have a thumbnail, so we count up ulFinishCount.
			ulFinishCount++;
		}

		// Send Async command to all DataObjects that have started acquire command.
		for ( j = 0; j <= i; j++ ) {
			bRet = Command_Async( GetRefChildPtr_ID(GetRefChildPtr_Index(pRefSrc, j ), kNkMAIDDataObjType_Thumbnail)->pObject );
			if ( bRet == FALSE ) return FALSE;
		}
	}

	// Send Async command to all DataObjects, untill all scanning complete.
	while ( ulFinishCount < pRefSrc->ulChildCount ) {
		for ( j = 0; j < pRefSrc->ulChildCount; j++ ) {
			bRet = Command_Async( GetRefChildPtr_ID(GetRefChildPtr_Index( pRefSrc, j), kNkMAIDDataObjType_Thumbnail )->pObject );
			if ( bRet == FALSE ) return FALSE;
		}
	}

	// Close all item objects(include image and thumbnail object).
	while ( pRefSrc->ulChildCount > 0 ) {
		pRefItm = GetRefChildPtr_Index( pRefSrc, 0 );
		ulItemID = pRefItm->lMyID;
		// reset DataProc
		bRet = Command_CapSet( pRefDat->pObject, kNkMAIDCapability_DataProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;
		bRet = RemoveChild( pRefSrc, ulItemID );
		if ( bRet == FALSE ) return FALSE;
	}

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// get pointer to CapInfo, the capability ID of that is 'ulID'
LPNkMAIDCapInfo GetCapInfo(LPRefObj pRef, ULONG ulID)
{
	ULONG i;
	LPNkMAIDCapInfo pCapInfo;

	if (pRef == NULL)
		return NULL;
	for ( i = 0; i < pRef->ulCapCount; i++ ){
		pCapInfo = (LPNkMAIDCapInfo)( (char*)pRef->pCapArray + i * sizeof(NkMAIDCapInfo) );
		if ( pCapInfo->ulID == ulID )
			break;
	}
	if ( i < pRef->ulCapCount )
		return pCapInfo;
	else
		return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL CheckCapabilityOperation(LPRefObj pRef, ULONG ulID, ULONG ulOperations)
{
	SLONG nResult;
	LPNkMAIDCapInfo pCapInfo = GetCapInfo(pRef, ulID);

	if(pCapInfo != NULL){
		if(pCapInfo->ulOperations & ulOperations){
			nResult = kNkMAIDResult_NoError;
		}else{
			nResult = kNkMAIDResult_NotSupported;
		}
	}else{
		nResult = kNkMAIDResult_NotSupported;
	}

	return (nResult == kNkMAIDResult_NoError);
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL AddChild( LPRefObj pRefParent, SLONG lIDChild )
{
	SLONG lResult;
	ULONG ulCount = pRefParent->ulChildCount;
	LPVOID pNewMemblock = realloc( pRefParent->pRefChildArray, (ulCount + 1) * sizeof(LPRefObj) );
	LPRefObj pRefChild = (LPRefObj)malloc( sizeof(RefObj) );

	if(pNewMemblock == NULL || pRefChild == NULL) {
		puts( "There is not enough memory" );
		return FALSE;
	}
	pRefParent->pRefChildArray = pNewMemblock;
	((LPRefObj*)pRefParent->pRefChildArray)[ulCount] = pRefChild;
	InitRefObj(pRefChild);
	pRefChild->lMyID = lIDChild;
	pRefChild->pRefParent = pRefParent;
	pRefChild->pObject = (LPNkMAIDObject)malloc(sizeof(NkMAIDObject));
	if(pRefChild->pObject == NULL){
		puts( "There is not enough memory" );
		pRefParent->pRefChildArray = realloc( pRefParent->pRefChildArray, ulCount * sizeof(LPRefObj) );
		return FALSE;
	}

	pRefChild->pObject->refClient = (NKREF)pRefChild;
	lResult = Command_Open( pRefParent->pObject, pRefChild->pObject, lIDChild );
	if(lResult == TRUE)
		pRefParent->ulChildCount ++;
	else {
		puts( "Failed in Opening an object." );
		pRefParent->pRefChildArray = realloc( pRefParent->pRefChildArray, ulCount * sizeof(LPRefObj) );
		free(pRefChild->pObject);
		free(pRefChild);
		return FALSE;
	}

	lResult = EnumCapabilities( pRefChild->pObject, &(pRefChild->ulCapCount), &(pRefChild->pCapArray), NULL, NULL );
	
	// set callback functions to child object.
	SetProc( pRefChild );

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL RemoveChild( LPRefObj pRefParent, SLONG lIDChild )
{
	LPRefObj pRefChild = NULL, *pOldRefChildArray, *pNewRefChildArray;
	ULONG i, n;
	pRefChild = GetRefChildPtr_ID( pRefParent, lIDChild );
	if ( pRefChild == NULL ) return FALSE;

	while ( pRefChild->ulChildCount > 0 )
		RemoveChild( pRefChild, ((LPRefObj*)pRefChild->pRefChildArray)[0]->lMyID );

	if ( ResetProc( pRefChild ) == FALSE ) return FALSE;
	if ( Command_Close( pRefChild->pObject ) == FALSE ) return FALSE;
	pOldRefChildArray = (LPRefObj*)pRefParent->pRefChildArray;
	pNewRefChildArray = NULL;
	if( pRefParent->ulChildCount > 1 ){
		pNewRefChildArray = (LPRefObj*)malloc( (pRefParent->ulChildCount - 1) * sizeof(LPRefObj) );
		for( n = 0, i = 0; i < pRefParent->ulChildCount; i++ ){
			if( ((LPRefObj)pOldRefChildArray[i])->lMyID != lIDChild )
				memmove( &pNewRefChildArray[n++], &pOldRefChildArray[i], sizeof(LPRefObj) );
		}
	}
	pRefParent->pRefChildArray = pNewRefChildArray;
	pRefParent->ulChildCount--;
	if ( pRefChild->pObject != NULL )
		free( pRefChild->pObject );
	if ( pRefChild->pCapArray != NULL )
		free( pRefChild->pCapArray );
	if ( pRefChild->pRefChildArray != NULL )
		free( pRefChild->pRefChildArray );
	if ( pRefChild != NULL )
		free( pRefChild );
	if ( pOldRefChildArray != NULL )
		free( pOldRefChildArray );
	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL SetProc( LPRefObj pRefObj )
{
	BOOL bRet;
	NkMAIDCallback	stProc;
	stProc.refProc = (NKREF)pRefObj;

	if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_ProgressProc, kNkMAIDCapOperation_Set ) ){
		stProc.pProc = (LPNKFUNC)ProgressProc;
		bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_ProgressProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;
	}

	switch ( pRefObj->pObject->ulType  ) {
		case kNkMAIDObjectType_Module:
			// If Module object supports Cap_EventProc, set ModEventProc. 
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)ModEventProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == FALSE ) return FALSE;
			}
			// UIRequestProc is supported by Module object only.
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_UIRequestProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)UIRequestProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_UIRequestProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == FALSE ) return FALSE;
			}
			break;
		case kNkMAIDObjectType_Source:
			// If Source object supports Cap_EventProc, set SrcEventProc. 
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)SrcEventProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == FALSE ) return FALSE;
			}
			break;
		case kNkMAIDObjectType_Item:
			// If Item object supports Cap_EventProc, set ItmEventProc. 
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)ItmEventProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == FALSE ) return FALSE;
			}
			break;
		case kNkMAIDObjectType_DataObj:
			// if Data object supports Cap_EventProc, set DatEventProc. 
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
				stProc.pProc = (LPNKFUNC)DatEventProc;
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_CallbackPtr, (NKPARAM)&stProc, NULL, NULL );
				if ( bRet == FALSE ) return FALSE;
			}
			break;
	}

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
//
BOOL ResetProc( LPRefObj pRefObj )
{
	BOOL bRet;

	if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_ProgressProc, kNkMAIDCapOperation_Set ) ) {
		bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_ProgressProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;
	}
	if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_EventProc, kNkMAIDCapOperation_Set ) ) {
		bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_EventProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
		if ( bRet == FALSE ) return FALSE;
	}

	if ( pRefObj->pObject->ulType == kNkMAIDObjectType_Module ) {
			// UIRequestProc is supported by Module object only.
			if( CheckCapabilityOperation( pRefObj, kNkMAIDCapability_UIRequestProc, kNkMAIDCapOperation_Set ) ) {
				bRet = Command_CapSet( pRefObj->pObject, kNkMAIDCapability_UIRequestProc, kNkMAIDDataType_Null, (NKPARAM)NULL, NULL, NULL );
				if ( bRet == FALSE ) return FALSE;
			}
	}

	return TRUE;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Get pointer to reference of child object by child's ID
LPRefObj GetRefChildPtr_ID( LPRefObj pRefParent, SLONG lIDChild )
{
	LPRefObj pRefChild;
	ULONG ulCount;

	if(pRefParent == NULL)
		return NULL;

	for( ulCount = 0; ulCount < pRefParent->ulChildCount; ulCount++ ){
		if ( (pRefChild = GetRefChildPtr_Index(pRefParent, ulCount)) != NULL ) {
			if (pRefChild->lMyID == lIDChild)
				return pRefChild;
		}
	}

	return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------
// Get pointer to reference of child object by index
LPRefObj GetRefChildPtr_Index( LPRefObj pRefParent, ULONG ulIndex )
{
	if (pRefParent == NULL)
		return NULL;

	if( (pRefParent->pRefChildArray != NULL) && (ulIndex < pRefParent->ulChildCount) )
		return (LPRefObj)((LPRefObj*)pRefParent->pRefChildArray)[ulIndex];
	else
		return NULL;
}
//------------------------------------------------------------------------------------------------------------------------------------

#if defined( _WIN32 )
BOOL WINAPI	cancelhandler(DWORD dwCtrlType)
{
	if (dwCtrlType == CTRL_C_EVENT) {
		g_bCancel = TRUE;
		return TRUE;
	}
	return FALSE;
}
#elif defined(__APPLE__)
void	cancelhandler(int sig)
{
	g_bCancel = TRUE;
}
#endif
//------------------------------------------------------------------------------------------------------------------------------------
