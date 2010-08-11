; NSIS Install Script 
; created by 
;     BBF, GlennMaynard, ChrisDanford
; I use the following command to create the installer:
; NOTE: this .NSI script is designed for NSIS v2.0+

SetCompressor /SOLID lzma
;--------------------------------
;Includes
	!define MUI_FINISHPAGE_NOAUTOCLOSE

	!include "MUI.nsh"	; Modern UI
	
	; Product info is in a separate file so that people will change
	; the version info for the installer and the program at the same time.
	; It's confusing when the installer and shortcut text doesn't match the 
	; title screen version text.
	!include "src\ProductInfo.inc"

	; needed by OpenCandy
	!include "nsdialogs.nsh"

;--------------------------------
;General

	!system "echo This may take a moment ..." ignore
	!system "utils\upx Program\*.exe" ignore

	; Request application privileges for Windows Vista
	RequestExecutionLevel admin

	Name "${PRODUCT_DISPLAY}"
	OutFile "${PRODUCT_FAMILY}-${PRODUCT_VER}.exe"

	Caption "${PRODUCT_DISPLAY}"
	UninstallCaption "${PRODUCT_DISPLAY}"

	; Some default compiler settings (uncomment and change at will):
!ifdef COMPRESS
	SetCompress auto
!else
	SetCompress off
!endif
	SetDatablockOptimize on ; (can be off)
!ifdef CRC_CHECK
	CRCCheck on
!else
	CRCCheck off
!endif
	
	ShowInstDetails show ; (can be show to have them shown, or nevershow to disable)
	ShowUninstDetails show ; (can be show to have them shown, or nevershow to disable)
	SetDateSave on ; (can be on to have files restored to their orginal date)
	InstallDir "$PROGRAMFILES\${PRODUCT_ID}"
	InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${PRODUCT_ID}" ""
	
;--------------------------------
;Interface Settings

	!define MUI_HEADERIMAGE
	!define MUI_HEADERIMAGE_BITMAP "Installer\header-${PRODUCT_BITMAP}.bmp"
	!define MUI_ABORTWARNING
	!define MUI_ICON "Installer\install.ico"
	!define MUI_UNICON "Installer\uninstall.ico"

;--------------------------------
;Language Selection Dialog Settings

	;Remember the installer language
	!define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
	!define MUI_LANGDLL_REGISTRY_KEY "Software\${PRODUCT_ID}" 
	!define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Pages

!ifdef SHOW_AUTORUN
	Page custom ShowAutorun LeaveAutorun
!endif

	!insertmacro MUI_PAGE_LICENSE "Installer_EULA.txt"

!ifdef ALLOW_OPENCANDY
	!include "OCSetupHlp.nsh"
	!include "opencandy.inc"

	PageEx custom
	 PageCallbacks OpenCandyPageStartFn OpenCandyPageLeaveFn 
	PageExEnd
!endif

	;!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES
	
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

	!insertmacro MUI_LANGUAGE "English" # first language is the default language
	!insertmacro MUI_LANGUAGE "French"
	!insertmacro MUI_LANGUAGE "German"
	!insertmacro MUI_LANGUAGE "Spanish"
	!insertmacro MUI_LANGUAGE "Italian"
	;!insertmacro MUI_LANGUAGE "SimpChinese"
	;!insertmacro MUI_LANGUAGE "TradChinese"
	;!insertmacro MUI_LANGUAGE "Japanese"
	;!insertmacro MUI_LANGUAGE "Korean"
	;!insertmacro MUI_LANGUAGE "Dutch"
	;!insertmacro MUI_LANGUAGE "Danish"
	;!insertmacro MUI_LANGUAGE "Swedish"
	;!insertmacro MUI_LANGUAGE "Norwegian"
	;!insertmacro MUI_LANGUAGE "Finnish"
	;!insertmacro MUI_LANGUAGE "Greek"
	;!insertmacro MUI_LANGUAGE "Russian"
	;!insertmacro MUI_LANGUAGE "Portuguese"
	;!insertmacro MUI_LANGUAGE "PortugueseBR"
	;!insertmacro MUI_LANGUAGE "Polish"
	;!insertmacro MUI_LANGUAGE "Ukrainian"
	;!insertmacro MUI_LANGUAGE "Czech"
	;!insertmacro MUI_LANGUAGE "Slovak"
	;!insertmacro MUI_LANGUAGE "Croatian"
	;!insertmacro MUI_LANGUAGE "Bulgarian"
	;!insertmacro MUI_LANGUAGE "Hungarian"
	;!insertmacro MUI_LANGUAGE "Thai"
	;!insertmacro MUI_LANGUAGE "Romanian"
	;!insertmacro MUI_LANGUAGE "Latvian"
	;!insertmacro MUI_LANGUAGE "Macedonian"
	;!insertmacro MUI_LANGUAGE "Estonian"
	;!insertmacro MUI_LANGUAGE "Turkish"
	;!insertmacro MUI_LANGUAGE "Lithuanian"
	;!insertmacro MUI_LANGUAGE "Catalan"
	;!insertmacro MUI_LANGUAGE "Slovenian"
	;!insertmacro MUI_LANGUAGE "Serbian"
	;!insertmacro MUI_LANGUAGE "SerbianLatin"
	;!insertmacro MUI_LANGUAGE "Arabic"
	;!insertmacro MUI_LANGUAGE "Farsi"
	;!insertmacro MUI_LANGUAGE "Hebrew"
	;!insertmacro MUI_LANGUAGE "Indonesian"
	;!insertmacro MUI_LANGUAGE "Mongolian"
	;!insertmacro MUI_LANGUAGE "Luxembourgish"
	;!insertmacro MUI_LANGUAGE "Albanian"
	;!insertmacro MUI_LANGUAGE "Breton"
	;!insertmacro MUI_LANGUAGE "Belarusian"
	;!insertmacro MUI_LANGUAGE "Icelandic"
	;!insertmacro MUI_LANGUAGE "Malay"
	;!insertmacro MUI_LANGUAGE "Bosnian"
	;!insertmacro MUI_LANGUAGE "Kurdish"

	; generate, then include installer strings
	;!delfile "nsis_strings_temp.inc"
	!system '"Program\${PRODUCT_FAMILY}.exe" --ExportNsisStrings'
	!include "nsis_strings_temp.inc"

;--------------------------------
;Reserve Files
  
  ;These files should be inserted before other files in the data block
  ;Keep these lines before any File command
  ;Only for solid compression (by default, solid compression is enabled for BZIP2 and LZMA)
  
  !insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
;Utility Functions
!ifdef ASSOCIATE_SMZIP
!define SHCNE_ASSOCCHANGED 0x08000000
!define SHCNF_IDLIST 0
 
Function RefreshShellIcons
  ; By jerome tremblay - april 2003
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd
Function un.RefreshShellIcons
  ; By jerome tremblay - april 2003
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd
!endif

;--------------------------------
;Installer Sections

Section "Main Section" SecMain

	; write out uninstaller
	SetOutPath "$INSTDIR"
	AllowSkipFiles off
	SetOverwrite on

!ifdef INSTALL_PROGRAM_LIBRARIES
	WriteUninstaller "$INSTDIR\uninstall.exe"

	; add registry entries
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\${PRODUCT_ID}" "" "$INSTDIR"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_ID}" "DisplayName" "$(TEXT_IO_REMOVE_ONLY)"
	WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_ID}" "UninstallString" '"$INSTDIR\uninstall.exe"'
!endif

!ifdef INSTALL_EXTERNAL_PCKS
	; Do this copy before anything else.  It's the most likely to fail.  
	; Possible failure reasons are: scratched CD, user tried to copy the installer but forgot the pcks.
	CreateDirectory $INSTDIR\pcks
	CopyFiles "${EXTERNAL_PCK_DIR}\*.pck" $INSTDIR\pcks 650000	; assume a CD full of data
	IfErrors do_error_pck do_no_error_pck
	do_error_pck:
	MessageBox MB_OK|MB_ICONSTOP "$(TEXT_IO_FATAL_ERROR_COPYING_PCK)"
	Quit
	do_no_error_pck:
!endif

!ifdef ASSOCIATE_SMZIP
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Classes\smzipfile" "" "$(TEXT_IO_SMZIP_PACKAGE)"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Classes\smzipfile\DefaultIcon" "" "$INSTDIR\Program\smpackage.ico"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Classes\smzipfile\shell\open\command" "" '"$INSTDIR\Program\${PRODUCT_FAMILY}.exe" "%1"'
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Classes\.smzip" "" "smzipfile"
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Classes\.smxml" "" "smzipfile"
!endif	

!ifdef INSTALL_NON_PCK_FILES
	CreateDirectory "$INSTDIR\Announcers"
	SetOutPath "$INSTDIR\Announcers"
	File "Announcers\instructions.txt"

	CreateDirectory "$INSTDIR\BGAnimations"
	SetOutPath "$INSTDIR\BGAnimations"

	CreateDirectory "$INSTDIR\CDTitles"
	SetOutPath "$INSTDIR\CDTitles"
	File "CDTitles\Instructions.txt"

	RMDir /r "$INSTDIR\Characters\default"
	CreateDirectory "$INSTDIR\Characters\default"
	SetOutPath "$INSTDIR\Characters"
	File /r /x CVS /x .svn "Characters\default"
	# File "Characters\instructions.txt"

	CreateDirectory "$INSTDIR\Courses"
	SetOutPath "$INSTDIR\Courses"
	File "Courses\instructions.txt"
	File /r /x CVS /x .svn "Courses\Samples"

	CreateDirectory "$INSTDIR\Packages"
	File "Packages\Instructions.txt"

	RMDir /r "$INSTDIR\NoteSkins\common\default"
	RMDir /r "$INSTDIR\NoteSkins\dance\default"
	RMDir /r "$INSTDIR\NoteSkins\dance\flat"
	SetOutPath "$INSTDIR\NoteSkins"
	File "NoteSkins\instructions.txt"

	SetOutPath "$INSTDIR\NoteSkins\common"
	File /r /x CVS /x .svn "NoteSkins\common\default"

	SetOutPath "$INSTDIR\NoteSkins\dance"
	File /r /x CVS /x .svn "NoteSkins\dance\default"
	File /r /x CVS /x .svn "NoteSkins\dance\flat"
	SetOutPath "$INSTDIR"

	CreateDirectory "$INSTDIR\BackgroundEffects"
	File /r /x CVS /x .svn "BackgroundEffects"

	CreateDirectory "$INSTDIR\BackgroundTransitions"
	File /r /x CVS /x .svn "BackgroundTransitions"

	CreateDirectory "$INSTDIR\RandomMovies"
	SetOutPath "$INSTDIR\RandomMovies"
	File "RandomMovies\instructions.txt"

	CreateDirectory "$INSTDIR\Songs"
	SetOutPath "$INSTDIR\Songs"
	File "Songs\Instructions.txt"

	RMDir /r "$INSTDIR\Themes\default"
	CreateDirectory "$INSTDIR\Themes"
	SetOutPath "$INSTDIR\Themes"
	File "Themes\instructions.txt"
	File /r /x CVS /x .svn "Themes\default"

	CreateDirectory "$INSTDIR\Data"
	SetOutPath "$INSTDIR\Data"
	File /r /x CVS /x .svn "Data\*"
!endif

!ifdef INSTALL_INTERNAL_PCKS
	CreateDirectory "$INSTDIR\pcks"
	SetOutPath "$INSTDIR\pcks"
	File /r "pcks\*.*"
!endif
	
	SetOutPath "$INSTDIR\Program"
!ifdef INSTALL_EXECUTABLES
	File "Program\${PRODUCT_FAMILY}.exe"
	File "Program\${PRODUCT_FAMILY}.exe.manifest"
	File "Program\${PRODUCT_FAMILY}.vdi"
	File "Program\Texture Font Generator.exe"
!endif
!ifdef ASSOCIATE_SMZIP
	File "Program\smpackage.ico"
	Call RefreshShellIcons
!endif
!ifdef INSTALL_PROGRAM_LIBRARIES
	File "Program\mfc71.dll"
	File "Program\msvcr71.dll"
	File "Program\msvcp71.dll"
	File "Program\jpeg.dll"
	File "Program\avcodec.dll"
	File "Program\avformat.dll"
	File "Program\dbghelp.dll"
	File "Program\zlib1.dll"

	SetOutPath "$INSTDIR"
	File "Docs\Licenses.txt"

	CreateDirectory "$INSTDIR\Manual"
	SetOutPath "$INSTDIR\Manual"
	File /r /x CVS /x .svn /x _* /x desktop.ini "Manual\*.*"

	; Create Start Menu icons
	SetShellVarContext All  # 	'all' doesn't work on Win9x
	CreateDirectory "$SMPROGRAMS\${PRODUCT_ID}\"
	CreateShortCut "$SMPROGRAMS\${PRODUCT_ID}\$(TEXT_IO_RUN).lnk" "$INSTDIR\Program\${PRODUCT_FAMILY}.exe"
	!ifdef MAKE_OPEN_PROGRAM_FOLDER_SHORTCUT
		CreateShortCut "$SMPROGRAMS\${PRODUCT_ID}\$(TEXT_IO_OPEN_PROGRAM_FOLDER).lnk" "$WINDIR\explorer.exe" "$INSTDIR\"
	!endif
		CreateShortCut "$SMPROGRAMS\${PRODUCT_ID}\$(TEXT_IO_UNINSTALL).lnk" "$INSTDIR\uninstall.exe"
		CreateShortCut "$SMPROGRAMS\${PRODUCT_ID}\$(TEXT_IO_WEB_SITE).lnk" "${PRODUCT_URL}"
	!ifdef MAKE_UPDATES_SHORTCUT
		CreateShortCut "$SMPROGRAMS\${PRODUCT_ID}\$(TEXT_IO_CHECK_FOR_UPDATES).lnk" "${UPDATES_URL}"
	!endif

	CreateShortCut "$INSTDIR\${PRODUCT_ID}.lnk" "$INSTDIR\Program\${PRODUCT_FAMILY}.exe"
!endif

	IfErrors do_error do_no_error
	do_error:
	MessageBox MB_OK|MB_ICONSTOP "$(TEXT_IO_FATAL_ERROR_INSTALL)"
	do_no_error:
	
SectionEnd

Section "${PRODUCT_DISPLAY}" SecCopyUI

; Refer to "All users" paths
  SetShellVarContext All

; Set output path to the installation directory.
  SetOutPath $INSTDIR

!ifdef ALLOW_OPENCANDY
  !insertmacro OpenCandyInstallDLL	
!endif
  
SectionEnd ; end the section

;--------------------------------
;Installer Functions

!ifdef SHOW_AUTORUN
Var hwnd ; Window handle of the custom page

Function ShowAutorun

	!insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
	
	InstallOptions::initDialog /NOUNLOAD "$PLUGINSDIR\custom.ini"
	; In this mode InstallOptions returns the window handle so we can use it
	Pop $hwnd
	
	GetDlgItem $1 $HWNDPARENT 1 ; Next button
	ShowWindow $1 0
	
!ifdef AUTORUN_SHOW_ONLY_INSTALL
	Goto show_only_install
!endif
	
	StrCpy $R1 "$INSTDIR\uninst.exe"
	StrCpy $R2 "_="
	IfFileExists "$R1" show_play_and_reinstall
	StrCpy $R1 "$INSTDIR\uninstall.exe"
	StrCpy $R2 "_?="
	IfFileExists "$R1" show_play_and_reinstall

	show_only_install:
	GetDlgItem $1 $hwnd 1201 ; Second cutom control
	ShowWindow $1 0
	GetDlgItem $1 $hwnd 1202 ; Third cutom control
	ShowWindow $1 0
	Goto done

	show_play_and_reinstall:
	GetDlgItem $1 $hwnd 1200 ; First cutom control
	ShowWindow $1 0

	done:

	; Now show the dialog and wait for it to finish
	InstallOptions::show
	
	; Finally fetch the InstallOptions status value (we don't care what it is though)
	Pop $0

FunctionEnd

Function LeaveAutorun

	; At this point the user has either pressed Next or one of our custom buttons
	; We find out which by reading from the INI file
	ReadINIStr $0 "$PLUGINSDIR\custom.ini" "Settings" "State"
	StrCmp $0 1 install
	StrCmp $0 2 play
	StrCmp $0 3 install
	Goto proceed

	install:
	Call PreInstall
	GoTo proceed
	
	play:
	Exec "$INSTDIR\Program\${PRODUCT_FAMILY}.exe"
	IfErrors play_error
	quit

	play_error:
	MessageBox MB_ICONEXCLAMATION "$(TEXT_IO_COULD_NOT_EXECUTE)"
	abort
	
	proceed:
	GetDlgItem $1 $HWNDPARENT 1 ; Next button
	ShowWindow $1 1

FunctionEnd
!endif

Function PreInstall

!ifdef INSTALL_PROGRAM_LIBRARIES
		; force uninstall of previous version using NSIS
		; We need to wait until the uninstaller finishes before continuing, since it's possible
		; to click the next button again before the uninstaller's window appears and takes focus.
		; This is tricky: we can't just ExecWait the uninstaller, since it copies off the uninstaller
		; EXE and exec's that (otherwise it couldn't delete itself), so it appears to exit immediately.
		; We need to copy it off ourself, run it with the hidden parameter to tell it it's a copy,
		; and then delete the copy ourself.  There's one more trick: the hidden parameter changed
		; between NSIS 1 and 2: in 1.x it was _=C:\Foo, in 2.x it's _?=C:\Foo.  Rename the installer
		; for newer versions, so we can tell the difference: "uninst.exe" is the old 1.x uninstaller,
		; "uninstall.exe" is 2.x.
		StrCpy $R1 "$INSTDIR\uninst.exe"
		StrCpy $R2 "_="
		IfFileExists "$R1" prompt_uninstall_nsis
		StrCpy $R1 "$INSTDIR\uninstall.exe"
		StrCpy $R2 "_?="
		IfFileExists "$R1" prompt_uninstall_nsis old_nsis_not_installed

		prompt_uninstall_nsis:
		MessageBox MB_YESNO|MB_ICONINFORMATION "$(TEXT_IO_UNINSTALL_PREVIOUS)" IDYES do_uninstall_nsis
		Abort

		do_uninstall_nsis:
		GetTempFileName $R3
		CopyFiles /SILENT $R1 $R3

		ExecWait '$R3 $R2$INSTDIR' $R4
		; Delete the copy of the installer.
		Delete $R3

		; $R4 is the exit value of the uninstaller.  0 means success, anything else is
		; failure (eg. aborted).
		IntCmp $R4 0 old_nsis_not_installed ; jump if 0

		MessageBox MB_YESNO|MB_DEFBUTTON2|MB_ICONINFORMATION "$(TEXT_IO_UNINSTALL_FAILED_INSTALL_ANYWAY)" IDYES old_nsis_not_installed
		Abort


		old_nsis_not_installed:



		; Check for DirectX 8.0 (to be moved to the right section later)
		; We only use this for sound.  Actually, I could probably make the sound
		; work with an earlier one; I'm not sure if that's needed or not.  For one
		; thing, forcing people to upgrade drivers is somewhat of a good thing;
		; but upgrading to DX8 if you really don't have to is also somewhat
		; annoying, too ... -g
		ReadRegStr $0 HKEY_LOCAL_MACHINE "Software\Microsoft\DirectX" "Version"
		StrCpy $1 $0 2 2 ;  8.1 is "4.08.01.0810"
		IntCmpU $1 8 check_subversion old_dx ok
		check_subversion:
		StrCpy $1 $0 2 5
		IntCmpU $1 0 ok old_dx ok

		; We can function without it (using WaveOut), so don't *require* this.
		old_dx:
	!ifdef DIRECTX_81_REDIST_PRESENT
		MessageBox MB_YESNO|MB_ICONINFORMATION "$(TEXT_IO_INSTALL_DIRECTX)" IDNO ok
		Exec "DirectX81\dxsetup.exe"
		quit
		ok:
	!else
		MessageBox MB_YESNO|MB_ICONINFORMATION "$(TEXT_IO_DIRECTX_VISIT_MICROSOFT)" IDNO ok
		ExecShell "" "http://www.microsoft.com/directx/"
		Abort
		ok:
	!endif
!else
		; Check that full version is installed.
		IfFileExists "$INSTDIR\Program\${PRODUCT_FAMILY}.exe" proceed_with_patch
		MessageBox MB_YESNO|MB_ICONINFORMATION "$(TEXT_IO_FULL_INSTALL_NOT_FOUND)" IDYES proceed_with_patch
		Abort
		proceed_with_patch:
!endif

		; HACK: Something is setting an error by this time.  Narrow down what.
		ClearErrors

FunctionEnd

!ifdef ALLOW_OPENCANDY
Var DontShowOC
!endif

Function .onInit

	; Force show language selection for debugging
	;!define MUI_LANGDLL_ALWAYSSHOW
	!insertmacro MUI_LANGDLL_DISPLAY

	;$PLUGINSDIR will automatically be removed when the installer closes
	InitPluginsDir

!ifdef ALLOW_OPENCANDY
    IntOp $DontShowOC 0 + 0
	${GetOptions} $CMDLINE "/NOCANDY" $R0
	IfErrors done_opencandy no_opencandy
	no_opencandy:
    IntOp $DontShowOC 1 + 0
	done_opencandy:
	!insertmacro OpenCandyInitRemnant "${PRODUCT_DISPLAY}" "${OPENCANDY_KEY}" "${OPENCANDY_SECRET}" "Software\${PRODUCT_ID}\OpenCandy" $DontShowOC
!endif
		
!ifdef SHOW_AUTORUN
	;
	; Extract files for the InstallOptions page
	;
	!insertmacro MUI_INSTALLOPTIONS_EXTRACT_AS "Installer\custom.ini" "custom.ini"
	
	WriteINIStr $PLUGINSDIR\custom.ini "Field 1" "Text" "$(TEXT_IO_INSTALL)"
	WriteINIStr $PLUGINSDIR\custom.ini "Field 2" "Text" "$(TEXT_IO_PLAY)"
	WriteINIStr $PLUGINSDIR\custom.ini "Field 3" "Text" "$(TEXT_IO_REINSTALL)"

	WriteINIStr $PLUGINSDIR\custom.ini "Field 4" "Text" "${PRODUCT_URL}"
	WriteINIStr $PLUGINSDIR\custom.ini "Field 4" "State" "${PRODUCT_URL}"
	File /oname=$PLUGINSDIR\image.bmp "Installer\custom-${PRODUCT_BITMAP}.bmp"
	WriteINIStr $PLUGINSDIR\custom.ini "Field 5" "Text" $PLUGINSDIR\image.bmp	
!else

	IfErrors do_error do_no_error
	do_error:
	MessageBox MB_OK|MB_ICONSTOP "$(TEXT_IO_FATAL_ERROR_INSTALL)"
	Quit
	do_no_error:
	
	
	call PreInstall
!endif

FunctionEnd

Function .onInstSuccess

!ifdef ALLOW_OPENCANDY
  !insertmacro OpenCandyOnInstSuccess
!endif

FunctionEnd

;--------------------------------
; OnInstFailed

Function .onInstFailed

!ifdef ALLOW_OPENCANDY
  !insertmacro OpenCandyOnInstFailed
!endif

FunctionEnd


;--------------------------------
; OnGUIEnd

Function .onGUIEnd

!ifdef ALLOW_OPENCANDY
  !insertmacro OpenCandyOnGuiEnd
!endif

FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

	; add delete commands to delete whatever files/registry keys/etc you installed here.
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\${PRODUCT_ID}"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_ID}"

!ifdef INSTALL_EXTERNAL_PCKS | INSTALL_INTERNAL_PCKS
	RMDir /r "$INSTDIR\pcks"
!endif

!ifdef ASSOCIATE_SMZIP
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Classes\smzipfile"
	DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Classes\.smzip"
!endif

!ifdef INSTALL_NON_PCK_FILES
	Delete "$INSTDIR\Announcers\instructions.txt"
	RMDir "$INSTDIR\Announcers"

	Delete "$INSTDIR\BGAnimations\instructions.txt"
	RMDir "$INSTDIR\BGAnimations"

	RMDir /r "$INSTDIR\Cache"

	Delete "$INSTDIR\CDTitles\Instructions.txt"
	RMDir "$INSTDIR\CDTitles"

	RMDir /r "$INSTDIR\Characters\default"
	RMDir "$INSTDIR\Characters"

	RMDir /r "$INSTDIR\Cache"

	Delete "$INSTDIR\Packages\instructions.txt"
	RMDir "$INSTDIR\Packages"

	Delete "$INSTDIR\Courses\instructions.txt"
	RMDir /r "$INSTDIR\Courses\Samples"
	RMDir "$INSTDIR\Courses"

	Delete "$INSTDIR\NoteSkins\instructions.txt"
	RMDir /r "$INSTDIR\NoteSkins\common\default"
	RMDir "$INSTDIR\NoteSkins\common"
	RMDir /r "$INSTDIR\NoteSkins\dance\default"
	RMDir /r "$INSTDIR\NoteSkins\dance\flat"
	RMDir /r "$INSTDIR\NoteSkins\dance\note"
	RMDir /r "$INSTDIR\NoteSkins\dance\solo"
	RMDir "$INSTDIR\NoteSkins\dance"
	RMDir /r "$INSTDIR\NoteSkins\pump\classic"
	RMDir /r "$INSTDIR\NoteSkins\pump\default"
	RMDir "$INSTDIR\NoteSkins\pump"
	RMDir /r "$INSTDIR\NoteSkins\para\default"
	RMDir "$INSTDIR\NoteSkins\para"
	RMDir "$INSTDIR\NoteSkins"

	RMDir /r "$INSTDIR\BackgroundEffects"

	RMDir /r "$INSTDIR\BackgroundTransitions"

	Delete "$INSTDIR\RandomMovies\instructions.txt"
	RMDir "$INSTDIR\RandomMovies"

	Delete "$INSTDIR\Songs\Instructions.txt"
	RMDir "$INSTDIR\Songs"	; will delete only if empty

	Delete "$INSTDIR\Themes\instructions.txt"
	RMDir /r "$INSTDIR\Themes\default"
	RMDir "$INSTDIR\Themes"

	Delete "$INSTDIR\Data\*.*"
	RMDir "$INSTDIR\Data"
!endif

!ifdef INSTALL_EXECUTABLES
	Delete "$INSTDIR\Program\${PRODUCT_FAMILY}.exe"
	Delete "$INSTDIR\Program\${PRODUCT_FAMILY}.exe.manifest"
	Delete "$INSTDIR\Program\${PRODUCT_FAMILY}.vdi"
	Delete "$INSTDIR\Program\Texture Font Generator.exe"
!endif
!ifdef ASSOCIATE_SMZIP
	Delete "$INSTDIR\Program\smpackage.ico"
	Call un.RefreshShellIcons
!endif
!ifdef INSTALL_PROGRAM_LIBRARIES
	Delete "$INSTDIR\Program\mfc71.dll"
	Delete "$INSTDIR\Program\msvcr71.dll"
	Delete "$INSTDIR\Program\msvcp71.dll"
	Delete "$INSTDIR\Program\jpeg.dll"
	Delete "$INSTDIR\Program\avcodec.dll"
	Delete "$INSTDIR\Program\avformat.dll"
	Delete "$INSTDIR\Program\dbghelp.dll"
	Delete "$INSTDIR\Program\zlib1.dll"
	RMDir "$INSTDIR\Program"

	Delete "$INSTDIR\Licenses.txt"
	RMDir /r "$INSTDIR\Manual"
!endif

	Delete "$INSTDIR\log.txt"
	Delete "$INSTDIR\info.txt"
	Delete "$INSTDIR\crashinfo.txt"
	Delete "$INSTDIR\${PRODUCT_ID}.lnk"

	RMDir "$INSTDIR"	; will delete only if empty

	SetShellVarContext All
	RMDir /r "$SMPROGRAMS\${PRODUCT_ID}"

	Delete "$INSTDIR\Uninstall.exe"

	DeleteRegKey /ifempty HKEY_LOCAL_MACHINE "SOFTWARE\${PRODUCT_ID}"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

	!insertmacro MUI_UNGETLANGUAGE

FunctionEnd
