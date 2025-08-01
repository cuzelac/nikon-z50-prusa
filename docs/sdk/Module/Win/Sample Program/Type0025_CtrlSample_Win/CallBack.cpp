//================================================================================================
// Copyright Nikon Corporation - All rights reserved
//
// View this file in a non-proportional font, tabs = 3
//================================================================================================

#include <stdlib.h>
#include <stdio.h>
#if defined( _WIN32 )
	#include <windows.h>
	#include <mmsystem.h>
#elif defined(__APPLE__)
    #include <sys/times.h>
#endif
#include "Maid3.h"
#include "Maid3d1.h"
#include "CtrlSample.h"

#if defined( _WIN32 )
ULONG g_ulProgressValue;// used in only ProgressProc
#elif defined(__APPLE__)
unsigned long g_ulProgressValue;// used in only ProgressProc
#endif
BOOL	g_bFirstCall = TRUE;// used in ProgressProc, and DoDeleteDramImage

//------------------------------------------------------------------------------------------------------------------------------------

void CALLPASCAL CALLBACK ModEventProc( NKREF refProc, ULONG ulEvent, NKPARAM data )
{
	BOOL bRet;
	LPRefObj pRefParent = (LPRefObj)refProc, pRefChild = NULL;

	switch(ulEvent){
		case kNkMAIDEvent_AddChild:
			bRet = AddChild( pRefParent, (SLONG)data );
			if ( bRet == FALSE ) return;
			pRefChild = GetRefChildPtr_ID( pRefParent, (SLONG)data );
			// Enumerate children(Item and Data Objects) and open them.
			bRet = EnumChildrten( pRefChild->pObject );
			if ( bRet == FALSE ) return;
			break;
		case kNkMAIDEvent_RemoveChild:
			bRet = RemoveChild( pRefParent, (SLONG)data );
			if ( bRet == FALSE ) return;
			break;
		case kNkMAIDEvent_WarmingUp:
			// The Type0025 Module does not use this event.
			puts( "Event_WarmingUp to Module object is not supported.\n" );
			break;
		case kNkMAIDEvent_WarmedUp:
			// The Type0025 Module does not use this event.
			puts( "Event_WarmedUp to Module object is not supported.\n" );
			break;
		case kNkMAIDEvent_CapChange:
		case kNkMAIDEvent_CapChangeOperationOnly:
			// re-enumerate the capabilities
			if ( pRefParent->pCapArray != NULL ) {
				free( pRefParent->pCapArray );
				pRefParent->ulCapCount = 0;
				pRefParent->pCapArray = NULL;
			}
			bRet = EnumCapabilities( pRefParent->pObject, &(pRefParent->ulCapCount), &(pRefParent->pCapArray), NULL, NULL );
			if ( bRet == FALSE ) return;
			// ToDo: Execute a process when the property of a capability was changed.
			break;
		case kNkMAIDEvent_CapChangeValueOnly:
			// ToDo: Execute a process when the value of a capability was changed.
			printf( "The value of Capability(CapID=0x%X) was changed.\n", (ULONG)data );
			break;
		case kNkMAIDEvent_OrphanedChildren:
			// ToDo: Close children(Source Objects).
			break;
		default:
			puts( "Detected unknown Event to the Module object.\n" );
		}
}
//------------------------------------------------------------------------------------------------------------------------------------

void CALLPASCAL CALLBACK SrcEventProc( NKREF refProc, ULONG ulEvent, NKPARAM data )
{
	BOOL bRet;
	LPRefObj pRefParent = (LPRefObj)refProc, pRefChild = NULL;
	NkMAIDEventParam* pParam = NULL;

	switch(ulEvent){
		case kNkMAIDEvent_AddChild:
			bRet = AddChild( pRefParent, (SLONG)data );
			if ( bRet == FALSE ) return;
			pRefChild = GetRefChildPtr_ID( pRefParent, (SLONG)data );
			// Enumerate children(Data Objects) and open them.
			bRet = EnumChildrten( pRefChild->pObject );
			if ( bRet == FALSE ) return;
			break;
		case kNkMAIDEvent_RemoveChild:
			bRet = RemoveChild( pRefParent, (SLONG)data );
			if ( bRet == FALSE ) return;
			break;
		case kNkMAIDEvent_WarmingUp:
			// The Type0025 Module does not use this event.
			puts( "Event_WarmingUp to Source object is not supported.\n" );
			break;
		case kNkMAIDEvent_WarmedUp:
			// The Type0025 Module does not use this event.
			puts( "Event_WarmedUp to Source object is not supported.\n" );
			break;
		case kNkMAIDEvent_CapChange:
		case kNkMAIDEvent_CapChangeOperationOnly:
			// re-enumerate the capabilities
			if ( pRefParent->pCapArray != NULL ) {
				free( pRefParent->pCapArray );
				pRefParent->ulCapCount = 0;
				pRefParent->pCapArray = NULL;
			}
			bRet = EnumCapabilities( pRefParent->pObject, &(pRefParent->ulCapCount), &(pRefParent->pCapArray), NULL, NULL );
			if ( bRet == FALSE ) return;
			// ToDo: Execute a process when the property of a capability was changed.
			break;
		case kNkMAIDEvent_CapChangeValueOnly:
			// ToDo: Execute a process when the value of a capability was changed.
			printf( "The value of Capability(CapID=0x%X) was changed.\n", (ULONG)data );
			break;
		case kNkMAIDEvent_OrphanedChildren:
			// ToDo: Close children(Item Objects).
			break;
		case kNkMAIDEvent_AddPreviewImage:
			// The Type0025 Module does not use this event.
			puts( "Event_AddPreviewImage to Item object is not supported.\n" );
			break;
		case kNkMAIDEvent_CaptureComplete:
			// ToDo: Show the image transfer finished.
			break;
		case kNkMAIDEvent_AddChildInCard:
			printf( "a Video object(ID=0x%X) added in card.\n", (ULONG)data );
			break;
		case kNkMAIDEvent_RecordingInterrupted:
			pParam = (NkMAIDEventParam*)data;
			printf("Recording was Interrupted (ErrorID=0x%X, RecordType=0x%X).\n", pParam->ulParam[0], pParam->ulParam[1]);
			break;
		case kNkMAIDEvent_MovieRecordComplete:
			{
			char strParam[64] = "";
			switch ((ULONG)data){
				case 0:
					 strcpy(strParam, "Card Recording");
					 break;
				case 1:
					 strcpy(strParam, "ExternalDevice Recording");
					 break;
				case 2:
					 strcpy(strParam, "Card and ExternalDevice Recording");
					 break;
				default:
					 strcpy(strParam, "Unknown parameter");
					 break;
				}
			printf("MovieRecordComplete Event(%s)\r\n", strParam);
			break;
			}
		case kNkMAIDEvent_PictureControlAdjustChanged:
			pParam = (NkMAIDEventParam*)data;
			printf("PictureControl Adjust Changed (PicCtrlItemID=0x%X, ShootingMode=0x%X).\n", pParam->ulParam[0], pParam->ulParam[1]);
			break;
		case kNkMAIDEvent_StartMovieRecord:
		{
			char strParam[64] = "";
			switch ((ULONG)data){
			case 0:
				strcpy(strParam, "Recording to Card");
				break;
			case 1:
				strcpy(strParam, "Recording to External Recording Device");
				break;
			case 2:
				strcpy(strParam, "Recording to Card and External Recording Device");
				break;
			default:
				strcpy(strParam, "Unknown parameter");
				break;
			}
			printf("MovieRecordComplete Event(%s)\r\n", strParam);
			break;
		}

		default:
			puts( "Detected unknown Event to the Source object.\n" );
		}
}
//------------------------------------------------------------------------------------------------------------------------------------

void CALLPASCAL CALLBACK ItmEventProc( NKREF refProc, ULONG ulEvent, NKPARAM data )
{
	BOOL bRet;
	LPRefObj pRefParent = (LPRefObj)refProc;

	switch(ulEvent){
		case kNkMAIDEvent_AddChild:
			bRet = AddChild( pRefParent, (SLONG)data );
			if ( bRet == FALSE ) return;
			break;
		case kNkMAIDEvent_RemoveChild:
			bRet = RemoveChild( pRefParent, (SLONG)data );
			if ( bRet == FALSE ) return;
			break;
		case kNkMAIDEvent_WarmingUp:
			// The Type0025 Module does not use this event.
			puts( "Event_WarmingUp to Item object is not supported.\n" );
			break;
		case kNkMAIDEvent_WarmedUp:
			// The Type0025 Module does not use this event.
			puts( "Event_WarmedUp to Item object is not supported.\n" );
			break;
		case kNkMAIDEvent_CapChange:
		case kNkMAIDEvent_CapChangeOperationOnly:
			// re-enumerate the capabilities
			if ( pRefParent->pCapArray != NULL ) {
				free( pRefParent->pCapArray );
				pRefParent->ulCapCount = 0;
				pRefParent->pCapArray = NULL;
			}
			bRet = EnumCapabilities( pRefParent->pObject, &(pRefParent->ulCapCount), &(pRefParent->pCapArray), NULL, NULL );
			if ( bRet == FALSE ) return;
			// ToDo: Execute a process when the property of a capability was changed.
			break;
		case kNkMAIDEvent_CapChangeValueOnly:
			// ToDo: Execute a process when the value of a capability was changed.
			printf( "The value of Capability(CapID=0x%X) was changed.\n", (ULONG)data );
			break;
		case kNkMAIDEvent_OrphanedChildren:
			// ToDo: Close children(Data Objects).
			break;
		default:
			puts( "Detected unknown Event to the Item object.\n" );
		}
	}
//------------------------------------------------------------------------------------------------------------------------------------

void CALLPASCAL CALLBACK DatEventProc( NKREF refProc, ULONG ulEvent, NKPARAM data )
{
	BOOL bRet;
	LPRefObj	pRefParent = (LPRefObj)refProc;

	switch(ulEvent){
		case kNkMAIDEvent_AddChild:
			// The Type0025 Module does not use this event.
			puts( "Event_AddChild to Data object is not supported.\n" );
			break;
		case kNkMAIDEvent_RemoveChild:
			// The Type0025 Module does not use this event.
			puts( "Event_RemoveChild to Data object is not supported.\n" );
			break;
		case kNkMAIDEvent_WarmingUp:
			// The Type0025 Module does not use this event.
			puts( "Event_WarmingUp to Data object is not supported.\n" );
			break;
		case kNkMAIDEvent_WarmedUp:
			// The Type0025 Module does not use this event.
			puts( "Event_WarmedUp to Data object is not supported.\n" );
			break;
		case kNkMAIDEvent_CapChange:// module notify that a capability is changed.
		case kNkMAIDEvent_CapChangeOperationOnly:
			// re-enumerate the capabilities
			if ( pRefParent->pCapArray != NULL ) {
				free( pRefParent->pCapArray );
				pRefParent->ulCapCount = 0;
				pRefParent->pCapArray = NULL;
			}
			bRet = EnumCapabilities( pRefParent->pObject, &(pRefParent->ulCapCount), &(pRefParent->pCapArray), NULL, NULL );
			if ( bRet == FALSE ) return;
			// ToDo: Execute a process when the property of a capability was changed.
		case kNkMAIDEvent_CapChangeValueOnly:
			// ToDo: Execute a process when the value of a capability was changed.
			printf( "The value of Capability(CapID=0x%X) was changed.\n", (ULONG)data );
			break;
		case kNkMAIDEvent_OrphanedChildren:
			// The Type0025 Module does not use this event.
			puts( "Event_OrphanedChildren to Data object is not supported.\n" );
			break;
		default:
			puts( "Detected unknown Event to the Data object.\n" );
	}
}
//------------------------------------------------------------------------------------------------------------------------------------
// copy the delivered data
NKERROR CALLPASCAL CALLBACK DataProc( NKREF ref, LPVOID pInfo, LPVOID pData )
{
	LPNkMAIDDataInfo pDataInfo = (LPNkMAIDDataInfo)pInfo;
	LPNkMAIDImageInfo pImageInfo = (LPNkMAIDImageInfo)pInfo;
	LPNkMAIDFileInfo pFileInfo = (LPNkMAIDFileInfo)pInfo;
	ULONG ullTotalSize, ulOffset;
	LPVOID pCurrentBuffer;
	ULONG ulByte;

	if ( pDataInfo->ulType & kNkMAIDDataObjType_File ) {
		if( ((LPRefDataProc)ref)->ulOffset == 0 && ((LPRefDataProc)ref)->pBuffer == NULL )
			((LPRefDataProc)ref)->pBuffer = malloc( pFileInfo->ulTotalLength );
		if ( ((LPRefDataProc)ref)->pBuffer == NULL ) {
			puts( "There is not enough memory." );
			return kNkMAIDResult_OutOfMemory;
		}
		ulOffset = ((LPRefDataProc)ref)->ulOffset;
		pCurrentBuffer = (LPVOID)((char*)((LPRefDataProc)ref)->pBuffer + ((LPRefDataProc)ref)->ulOffset);
		memmove( pCurrentBuffer, pData, pFileInfo->ulLength);
		ulOffset += pFileInfo->ulLength;

		if( ulOffset < pFileInfo->ulTotalLength ) {
			// We have not finished the delivery.
			((LPRefDataProc)ref)->ulOffset = ulOffset;
		} else {
			// We have finished the delivery. We will save this file.
			FILE *stream;
			char filename[256], Prefix[16], Ext[16];
			UWORD i = 0;
			if ( pDataInfo->ulType & kNkMAIDDataObjType_Image )
				strcpy(Prefix,"Image");
			else if ( pDataInfo->ulType & kNkMAIDDataObjType_Thumbnail )
				strcpy(Prefix,"Thumb");
			else
				strcpy(Prefix,"Unknown");
			switch( pFileInfo->ulFileDataType ) {
				case kNkMAIDFileDataType_JPEG:
					strcpy(Ext,".jpg");
					break;
				case kNkMAIDFileDataType_TIFF:
					strcpy(Ext,".tif");
					break;
				case kNkMAIDFileDataType_NIF:
					strcpy(Ext,".nef");
					break;
				case kNkMAIDFileDataType_NDF:
					strcpy(Ext,".ndf");
					break;
				default:
					strcpy(Ext,".dat");
			}
			while( TRUE ) {
				sprintf( filename, "%s%03d%s", Prefix, ++i, Ext );
				if ( (stream = fopen(filename, "r") ) != NULL )
					fclose(stream);
				else
					break;
			}
			if ( (stream = fopen(filename, "wb") ) == NULL)
				return kNkMAIDResult_UnexpectedError;
			fwrite(((LPRefDataProc)ref)->pBuffer, 1, pFileInfo->ulTotalLength, stream);
			fclose(stream);
			free(((LPRefDataProc)ref)->pBuffer);
			((LPRefDataProc)ref)->pBuffer = NULL;
			((LPRefDataProc)ref)->ulOffset = 0;
			// If the flag of fRemoveObject in NkMAIDFileInfo structure is TRUE, we should remove this item.
			if ( pFileInfo->fRemoveObject && (pDataInfo->ulType & kNkMAIDDataObjType_Image) )
				g_bFileRemoved = TRUE;
		}
	} else {
		ullTotalSize = pImageInfo->ulRowBytes * pImageInfo->szTotalPixels.h;
		if( ((LPRefDataProc)ref)->ulOffset == 0 && ((LPRefDataProc)ref)->pBuffer == NULL )
			((LPRefDataProc)ref)->pBuffer = malloc( ullTotalSize );
		if ( ((LPRefDataProc)ref)->pBuffer == NULL ) {
			puts( "There is not enough memory." );
			return kNkMAIDResult_OutOfMemory;
		}
		ulOffset = ((LPRefDataProc)ref)->ulOffset;
		pCurrentBuffer = (LPVOID)((char*)((LPRefDataProc)ref)->pBuffer + ulOffset);
		ulByte = pImageInfo->ulRowBytes * pImageInfo->rData.h;
		memmove( pCurrentBuffer, pData, ulByte );
		ulOffset += ulByte;

		if( ulOffset < ullTotalSize ) {
			// We have not finished the delivery.
			((LPRefDataProc)ref)->ulOffset = ulOffset;
		} else {
			// We have finished the delivery. We will save this file.
			FILE *stream;
			char filename[256], Prefix[16];
			UWORD i = 0;
			if ( pDataInfo->ulType & kNkMAIDDataObjType_Image )
				strcpy(Prefix,"Image");
			else if ( pDataInfo->ulType & kNkMAIDDataObjType_Thumbnail )
				strcpy(Prefix,"Thumb");
			else
				strcpy(Prefix,"Unknown");
			while( TRUE ) {
				sprintf( filename, "%s%03d.raw", Prefix, ++i );
				if ( (stream = fopen(filename, "r") ) != NULL )
					fclose(stream);
				else
					break;
			}
			if ( (stream = fopen(filename, "wb") ) == NULL)
				return kNkMAIDResult_UnexpectedError;
			fwrite(((LPRefDataProc)ref)->pBuffer, 1, ullTotalSize, stream);
			fclose(stream);
			free(((LPRefDataProc)ref)->pBuffer);
			((LPRefDataProc)ref)->pBuffer = NULL;
			((LPRefDataProc)ref)->ulOffset = 0;
			// If the flag of fRemoveObject in NkMAIDFileInfo structure is TRUE, we should remove this item.
			if ( pImageInfo->fRemoveObject && (pDataInfo->ulType & kNkMAIDDataObjType_Image) )
				g_bFileRemoved = TRUE;
		}
	}
	return kNkMAIDResult_NoError;
}
//------------------------------------------------------------------------------------------------------------------------------------

void CALLPASCAL CALLBACK CompletionProc(
  			LPNkMAIDObject	pObject,			// module, source, item, or data object
			ULONG				ulCommand,		// Command, one of eNkMAIDCommand
			ULONG				ulParam,			// parameter for the command
			ULONG				ulDataType,		// Data type, one of eNkMAIDDataType
			NKPARAM			data,				// Pointer or long integer
			NKREF				refComplete,	// Reference set by client
			NKERROR			nResult )		// One of eNkMAIDResult)
{
	((LPRefCompletionProc)refComplete)->nResult = nResult;
	(*((LPRefCompletionProc)refComplete)->pulCount) ++;

	// if the Command is CapStart acquire, we terminate RefDeliver.
	if(ulCommand == kNkMAIDCommand_CapStart && ulParam == kNkMAIDCapability_Acquire) {
		LPRefDataProc pRefDeliver = (LPRefDataProc)((LPRefCompletionProc)refComplete)->pRef;
		if ( pRefDeliver != NULL ) {
			if ( pRefDeliver->pBuffer != NULL )
				free( pRefDeliver->pBuffer );
			free( pRefDeliver );
		}
	}
	// terminate refComplete.
	if ( refComplete != NULL )
		free( refComplete );

}
//------------------------------------------------------------------------------------------------------------------------------------

void CALLPASCAL CALLBACK ProgressProc(
		ULONG				ulCommand,			// Command, one of eNkMAIDCommand
		ULONG				ulParam,				// parameter for the command
		NKREF				refProc,				// Reference set by client
		ULONG				ulDone,				// Numerator
		ULONG				ulTotal )			// Denominator
{
#if defined( _WIN32 )
    ULONG ulNewProgressValue, ulCount;
#elif defined(__APPLE__)
    unsigned long ulNewProgressValue, ulCount;
#endif
	if ( ulTotal == 0 ) {
		// when we don't know how long this process is, we show such as barber's pole.
		if ( ulDone == 1 ) {
		#if defined( _WIN32 )
			ulNewProgressValue = timeGetTime();
			if( (ulNewProgressValue < g_ulProgressValue) || (ulNewProgressValue > g_ulProgressValue + 500) ) {
				printf( "c" );
				g_ulProgressValue = ulNewProgressValue;
			}
		#elif defined(__APPLE__)
            struct tms tm;
            ulNewProgressValue = times(&tm);
			if( (ulNewProgressValue < g_ulProgressValue) || (ulNewProgressValue > g_ulProgressValue + 30) ) {
				printf( "c" );
				g_ulProgressValue = ulNewProgressValue;
			}
		#endif
		} else if ( ulDone == 0 ) {
				printf( "o" );
		}
	} else {
		// when we know how long this process is, we show progress bar.
		if ( ulDone == 0 ) {
			if ( g_bFirstCall == TRUE ) {
				g_ulProgressValue = 0;
				g_bFirstCall = FALSE;
				printf("\n0       20        40        60        80        100");
				printf("\n---------+---------+---------+---------+---------+\n");
			}
		} else {
			// show progress bar
            NK_UINT_64 ullwork = 50 * ( NK_UINT_64 )ulDone;
            ulNewProgressValue = ( ULONG )( ( ullwork + ulTotal - 1 ) / ulTotal );
			ulCount = ulNewProgressValue - g_ulProgressValue;
			while ( ulCount-- )
				printf( "]" );
			g_ulProgressValue = ulNewProgressValue;
			if ( ulDone == ulTotal ) {
				printf( "\n" );
				g_bFirstCall = TRUE;
			}
		}
	}
}
//------------------------------------------------------------------------------------------------------------------------------------

ULONG CALLPASCAL CALLBACK UIRequestProc( NKREF ref, LPNkMAIDUIRequestInfo pUIRequest )
{
	short	 nRet = kNkMAIDUIRequestResult_None;
	char	sAns[256];

	// display message
	if (pUIRequest->lpPrompt)
		printf( "\n%s\n", pUIRequest->lpPrompt );
	if (pUIRequest->lpDetail)
		printf( "\n%s\n", pUIRequest->lpDetail );

	// get an answer
	switch( pUIRequest->ulType ){
		case kNkMAIDUIRequestType_Ok:
			do {
				printf("\nPress 'O' key. ('O': OK)\n>");
				scanf( "%s", sAns );
			} while ( *sAns != 'o' && *sAns != 'O' );
			nRet = kNkMAIDUIRequestResult_Ok;
			break;
		case kNkMAIDUIRequestType_OkCancel:
			do {
				printf("\nPress 'O' or 'C' key. ('O': OK   'C': Cancel)\n>");
				scanf( "%s", sAns );
			} while ( *sAns != 'o' && *sAns != 'O' && *sAns != 'c' && *sAns != 'C' );
			if ( *sAns == 'o' || *sAns == 'O' )
				nRet = kNkMAIDUIRequestResult_Ok;
			else if ( *sAns == 'c' || *sAns == 'C' )
				nRet = kNkMAIDUIRequestResult_Cancel;
			break;
		case kNkMAIDUIRequestType_YesNo:
			do {
				printf("\nPress 'Y' or 'N' key. ('Y': Yes   'N': No)\n>");
				scanf( "%s", sAns );
			} while ( *sAns != 'y' && *sAns != 'Y' && *sAns != 'n' && *sAns != 'N' );
			if ( *sAns == 'y' || *sAns == 'Y' )
				nRet = kNkMAIDUIRequestResult_Yes;
			else if ( *sAns == 'n' || *sAns == 'N' )
				nRet = kNkMAIDUIRequestResult_No;
			break;
		case kNkMAIDUIRequestType_YesNoCancel:
			do {
				printf("\nPress 'Y' or 'N' or 'C' key. ('Y': Yes   'N': No   'C': Cancel)\n>");
				scanf( "%s", sAns );
			} while ( *sAns != 'y' && *sAns != 'Y' && *sAns != 'n' && *sAns != 'N' && *sAns != 'c' && *sAns != 'C' );
			if ( *sAns == 'y' || *sAns == 'Y' )
				nRet = kNkMAIDUIRequestResult_Yes;
			else if ( *sAns == 'n' || *sAns == 'N' )
				nRet = kNkMAIDUIRequestResult_No;
			else if ( *sAns == 'c' || *sAns == 'C' )
				nRet = kNkMAIDUIRequestResult_Cancel;
			break;
		default:
			nRet = kNkMAIDUIRequestResult_None;
	}

	return nRet;
}
//------------------------------------------------------------------------------------------------------------------------------------
