include RequiredIncs.inc

memset PROTO :DWORD, :BYTE, :DWORD
memcpy PROTO :DWORD, :DWORD, :DWORD 
Swap PROTO :DWORD, :DWORD
GetOSVersion PROTO :DWORD
GetHardDriveSerialNumber PROTO :DWORD

StrM MACRO data:VARARG                    
    LOCAL Buff
.data
    Buff db data, 0h
.code
    exitm <addr Buff>
ENDM

TVersionInfo struct
    dwOSVersionInfoSize dd ?
    dwMajorVersion 		dd ?   
    dwMinorVersion 		dd ?
    dwBuildNumber 		dd ?
    dwPlatformId 		dd ?
    szCSDVersion 		db 128 dup (?)  
TVersionInfo ends

GETVERSIONOUTPARAMS struct
   bVersion			db	?
   bRevision		db	?
   bReserved		db	?
   bIDEDeviceMap	db	?
   dwCapabilities	dd	?
   dwReserved		dd	4 dup (?)
GETVERSIONOUTPARAMS ends
   
TIDERegs struct 
  bFeaturesReg     	db		? 
  bSectorCountReg 	db		?
  bSectorNumberReg 	db		?
  bCylLowReg       	db		?
  bCylHighReg      	db		?
  bDriveHeadReg    	db		?
  bCommandReg     	db		?
  bReserved       	db		?
TIDERegs ends 
  
TSendCmdInParams struct     
  dwBufferSize  	dd		?  
  IRDriveRegs  		TIDERegs <?> 
  bDriveNumber 		db		? 
  bReserved    		db 	4 	dup (?) 
  dwReserved   		dd 	4 	dup (?) 
  bBuffer      		db 	1 	dup (?) 
TSendCmdInParams ends
  
TIdSector struct 
  wGenConfig					dw		?	  	            
  wNumCyls        				dw 		?	
  wReserved       				dw 		?	
  wNumHeads       				dw 		?
  wBytesPerTrack  				dw 		?
  wBytesPerSector            	dw		?
  wSectorsPerTrack           	dw 		?
  wVendorUnique              	dw 		3 dup (?) 
  sSerialNumber              	db 		20 dup (?) 
  wBufferType                	dw		? 
  wBufferSize                	dw 		?
  wECCSize                   	dw 		?
  sFirmwareRev               	db 		8 dup (?) 
  sModelNumber               	db 		40 dup (?) 
  wMoreVendorUnique          	dw 		?
  wDoubleWordIO              	dw 		?
  wCapabilities              	dw 		?
  wReserved1                 	dw 		?
  wPIOTiming                 	dw 		?
  wDMATiming                 	dw 		?
  wBS                        	dw 		?
  wNumCurrentCyls            	dw 		?
  wNumCurrentHeads           	dw 		?
  wNumCurrentSectorsPerTrack 	dw 		?
  ulCurrentSectorCapacity    	dd 		?
  wMultSectorStuff           	dw 		?
  ulTotalAddressableSectors  	dd 		?
  wSingleWordDMA             	dw 		?
  wMultiWordDMA              	dw 		?
  bReserved                  	db 		128 dup (?) 
TIdSector ends
     
TDriverStatus struct 
  bDriverError 	db		? 
  bIDEStatus   	db		? 
  bReserved    	db 		2	dup	(?) 
  dwReserved   	dd		2	dup	(?) 
TDriverStatus ends
   
TSendCmdOutParams struct 
  dwBufferSize  	dd		? 
  DriverStatus 		TDriverStatus	<> 
  bBuffer      		db		1	dup	(?) 
TSendCmdOutParams ends 

.const
	DFP_GET_VERSION        			EQU 00074080h
	DFP_SEND_DRIVE_COMMAND 			EQU 0007C084h
	DFP_RECEIVE_DRIVE_DATA 			EQU 0007C088h
	
	CAP_IDE_ID_FUNCTION    			EQU 1  
	CAP_IDE_ATAPI_ID       			EQU 2 
	CAP_IDE_EXECUTE_SMART_FUNCTION  EQU	4  
	
	IDENTIFY_BUFFER_SIZE 			EQU		512
	
.data
	Platform98 DB "98", 0
	PlatformXP DB "XP", 0
	Platform2000NT DB "2000/NT", 0
	PlatformNT DB "NT", 0

.code

GetHardDriveSerialNumber proc uses esi edi ecx edx ebx lpBuff:DWORD
	LOCAL dwBytesReturned: DWORD 	
	LOCAL hDevice: DWORD     
    LOCAL pch: DWORD 
    LOCAL SCIP:	TSendCmdInParams 
    LOCAL IdOutCmd[(SizeOf(TSendCmdOutParams) + IDENTIFY_BUFFER_SIZE - 1)]: BYTE     
	LOCAL Platform[10]: BYTE
		
	invoke GetOSVersion, addr Platform
	invoke lstrcmp, addr Platform, addr Platform98 	 
	.if eax != 0 
		invoke CreateFile, StrM("\\.\PhysicalDrive0"), GENERIC_READ or GENERIC_WRITE, 
        			FILE_SHARE_READ or FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0
        mov hDevice, eax			 
    .else 
    	invoke CreateFile, StrM("\\.\SMARTVSD"), 0, 0, 0, CREATE_NEW, 0, 0
    	mov hDevice, eax 
    .endif	
    
 	.if hDevice == INVALID_HANDLE_VALUE 
 		mov eax, 0
 		ret
 	.endif	
	invoke memset, addr SCIP, 0, Sizeof(TSendCmdInParams)  
    invoke memset, addr IdOutCmd, 0, SizeOf(TSendCmdOutParams) + IDENTIFY_BUFFER_SIZE - 1 
    
    mov SCIP.dwBufferSize, IDENTIFY_BUFFER_SIZE 
	mov SCIP.bDriveNumber, 0 
    mov SCIP.IRDriveRegs.bSectorCountReg, 1 
    mov SCIP.IRDriveRegs.bSectorNumberReg, 1
    invoke lstrcmp, addr Platform, addr Platform98 
    .if eax != 0
    	xor ebx, ebx     	
    	mov bl, SCIP.bDriveNumber 
    	and bl, 1
    	shl bl, 4
    	or bl, 0A0h
	    mov SCIP.IRDriveRegs.bDriveHeadReg, bl
	.else
		mov SCIP.IRDriveRegs.bDriveHeadReg, 0A0h
	.endif	          
     mov SCIP.IRDriveRegs.bCommandReg, 0ECh       
    
    invoke DeviceIoControl, hDevice, DFP_RECEIVE_DRIVE_DATA, addr SCIP, SizeOf(TSendCmdInParams) - 1, 
      						addr IdOutCmd, SizeOf(TSendCmdOutParams) + IDENTIFY_BUFFER_SIZE - 1,
      						addr dwBytesReturned, 0
    .if eax == 0
    	mov eax, 0
    	ret
    .endif    
  	lea ebx, IdOutCmd
  	lea ebx, (TSendCmdOutParams ptr [ebx]).bBuffer   	 
  	lea ebx, (TIdSector ptr [ebx]).sSerialNumber  	
  	invoke memcpy, ebx, lpBuff, 20 
  	invoke Swap, lpBuff, 20
  	mov ebx, lpBuff  
  	mov byte ptr [ebx + 19], 0	
  	mov edx, 19  	
  @@:   
  	dec edx
  	cmp byte ptr [ebx + edx], 20h
  	jne @F
  	mov byte ptr [ebx + edx], 0 	  	
  	.if edx > 1
  		jmp @B
  	.else
  		jmp @F	
  	.endif  	
  @@:
  	mov eax, 1	 	  	                   
	ret
GetHardDriveSerialNumber endp 

Swap proc uses esi edi ecx edx ebx lpBuff:DWORD, dwLen:DWORD		
	mov ecx, dwLen
	mov edx, eax
	xor edx, edx
	mov esi, lpBuff 
@@:
	mov bl, byte ptr [esi]	
	inc esi 
	mov bh, byte ptr [esi]
	dec esi	
	mov byte ptr [esi], bh
	inc esi
	mov byte ptr [esi], bl
	add edx, 2
	.if edx < ecx 
		inc esi
		jmp @B
	.endif	
	ret
Swap endp

GetOSVersion PROC PlatformType:DWORD
    LOCAL VersionInfo:TVersionInfo
    mov eax, SIZEOF TVersionInfo
    mov VersionInfo.dwOSVersionInfoSize, eax 
    invoke GetVersionEx, ADDR VersionInfo
    .if VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
        invoke lstrcpy, PlatformType, ADDR Platform98
    .elseif VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT
        .if VersionInfo.dwMajorVersion == 5
            invoke lstrcpy, PlatformType, ADDR Platform2000NT 
        .else 
            invoke lstrcpy, PlatformType, ADDR PlatformNT
        .endif
        .if VersionInfo.dwBuildNumber >= 2500
            invoke lstrcpy, PlatformType, ADDR PlatformXP
        .endif
    .endif 
    xor eax, eax
    ret    
GetOSVersion ENDP

memcpy PROC ptrSrc:DWORD, ptrDest:DWORD, dwSize:DWORD     
	 mov  ecx, dwSize 
     mov  esi, ptrSrc       
     mov  edi, ptrDest
     cld
     rep  movsb
     ret
memcpy ENDP

memset PROC pMem:DWORD, dwVal:byte, dwSize:DWORD
    mov edi, pMem
    mov al, dwVal
    mov ecx, dwSize
    cld
    rep stosb 
    ret
memset ENDP
