#include "global.h"

#if !defined(WITHOUT_NETWORKING)
#include "ScreenPackages.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "RageDisplay.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageFile.h"
#include "ScreenTextEntry.h"
#include "ScreenNetSelectBase.h"	//Used for Quad Utility
#include "ScreenManager.h"
#include <cstdlib>

#if defined(XBOX)
#include "archutils/Xbox/VirtualKeyboard.h"
#endif

#define EXISTINGBG_WIDTH			THEME->GetMetricF(m_sName,"PackagesBGWidth")
#define WEBBG_WIDTH					THEME->GetMetricF(m_sName,"WebBGWidth")
#define	NUM_PACKAGES_SHOW			THEME->GetMetricI(m_sName,"NumPackagesShow")
#define NUM_LINKS_SHOW				THEME->GetMetricI(m_sName,"NumLinksShow")

const ScreenMessage	SM_BackFromURL			= ScreenMessage(SM_User+2);

REGISTER_SCREEN_CLASS( ScreenPackages );
ScreenPackages::ScreenPackages( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	m_iPackagesPos = 0;
	m_iLinksPos = 0;
	m_iDLorLST = 0;
	m_bIsDownloading = false;
	m_bCanDL = THEME->GetMetricB( sClassName, "CanDL" );
	m_sStatus = "";
	m_fLastUpdate = 0;
	m_iTotalBytes = 0;
	m_iDownloaded = 0;

	FOREACH_PlayerNumber( pn )
		GAMESTATE->m_bSideIsJoined[pn] = true;

	m_sprExistingBG.SetName( "PackagesBG" );
	m_sprExistingBG.Load( THEME->GetPathG( m_sName, "PackagesBG" ) );
	SET_XY_AND_ON_COMMAND( m_sprExistingBG );
	this->AddChild( &m_sprExistingBG );

	m_sprWebBG.SetName( "WebBG" );
	m_sprWebBG.Load( THEME->GetPathG( m_sName, "WebBG" ) );
	SET_XY_AND_ON_COMMAND( m_sprWebBG );
	this->AddChild( &m_sprWebBG );

	COMMAND( m_sprExistingBG, "Back" );
	COMMAND( m_sprWebBG, "Away" );

	m_textPackages.LoadFromFont( THEME->GetPathF(m_sName,"default") );
	m_textPackages.SetShadowLength( 0 );
	m_textPackages.SetName( "Packages" );
	m_textPackages.SetMaxWidth( EXISTINGBG_WIDTH );
	SET_XY_AND_ON_COMMAND( m_textPackages );
	this->AddChild( &m_textPackages );
	RefreshPackages();

	m_textWeb.LoadFromFont( THEME->GetPathF(m_sName,"default") );
	m_textWeb.SetShadowLength( 0 );
	m_textWeb.SetName( "Web" );
	m_textWeb.SetMaxWidth( WEBBG_WIDTH );
	SET_XY_AND_ON_COMMAND( m_textWeb );
	this->AddChild( &m_textWeb);
	m_Links.push_back( " " );
	m_LinkTitles.push_back( "--Visit URL--" );

	m_textURL.LoadFromFont( THEME->GetPathF( m_sName,"default") );
	m_textURL.SetShadowLength( 0 );
	m_textURL.SetName( "WebURL" );
	SET_XY_AND_ON_COMMAND( m_textURL );
	this->AddChild( &m_textURL );
	UpdateLinksList();
	
	m_sprWebSel.SetName( "WebSel" );
	m_sprWebSel.Load( THEME->GetPathG( m_sName, "WebSel" ) );
	SET_XY_AND_ON_COMMAND( m_sprWebSel );
	this->AddChild( &m_sprWebSel );

	m_sprDLBG.SetName( "Download" );
	m_sprDLBG.Load( THEME->GetPathG( m_sName, "DownloadBG" ) );
	SET_XY_AND_ON_COMMAND( m_sprDLBG );
	this->AddChild( &m_sprDLBG );

	//(HTTP ELEMENTS)
	m_sprDL.SetName( "Download" );
	m_sprDL.Load( THEME->GetPathG( m_sName, "Download" ) );
	SET_XY_AND_ON_COMMAND( m_sprDL );
	this->AddChild( &m_sprDL );
	m_sprDL.SetDiffuse( THEME->GetMetricC( m_sName, "DownloadProgressColor" ) );

	m_textStatus.LoadFromFont( THEME->GetPathF(m_sName,"default") );
	m_textStatus.SetShadowLength( 0 );
	m_textStatus.SetName( "DownloadStatus" );
	SET_XY_AND_ON_COMMAND( m_textStatus );
	this->AddChild( &m_textStatus );

	UpdateProgress();

	//Workaround:  For some reason, the first download sometimes
	//corrupts; by opening and closing the rage file, this 
	//problem does not occour.  Go figure?
	
	//XXX:  This is a really dirty work around!
	//Why does RageFile do this?

	//It's always some strange number of bytes at the end of the
	//file when it corrupts.
	m_fOutputFile.Open( "Packages/dummy.txt", 0x02 );
	m_fOutputFile.Flush( );
	m_fOutputFile.Close();
}

void ScreenPackages::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( THEME->GetMetric (m_sName, "PrevScreen") );
		break;
	case SM_GoToNextScreen:
		SCREENMAN->SetNewScreen( THEME->GetMetric (m_sName, "NextScreen") );
		break;
	case SM_BackFromURL:
		if ( !ScreenTextEntry::s_bCancelledLast )
			EnterURL( ScreenTextEntry::s_sLastAnswer );
		break;
	}
	ScreenWithMenuElements::HandleScreenMessage( SM );
}

void ScreenPackages::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	ScreenWithMenuElements::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenPackages::Update( float fDeltaTime )
{
	HTTPUpdate();

	m_fLastUpdate += fDeltaTime;
	if ( m_fLastUpdate >= 1.0 )
	{
		if ( m_bIsDownloading && m_bGotHeader )
			m_sStatus = ssprintf( "DL @ %d KB/s", int((m_iDownloaded-m_bytesLastUpdate)/1024) );

		m_bytesLastUpdate = m_iDownloaded;
		UpdateProgress();
		m_fLastUpdate = 0;
	}

	ScreenWithMenuElements::Update(fDeltaTime);
}

void ScreenPackages::MenuStart( PlayerNumber pn )
{
	if ( m_iDLorLST == 1 )
	{
		if ( m_iLinksPos == 0 )
		{
#if defined(XBOX)
			XBOX_VKB.Reset(VKMODE_PROFILE); // allow all characters
#endif
			SCREENMAN->TextEntry( SM_BackFromURL, "Enter URL:", "http://" );
		}
		else
		{
			EnterURL( m_Links[m_iLinksPos] );
		}
	}
	ScreenWithMenuElements::MenuStart( pn );
}

void ScreenPackages::MenuUp( PlayerNumber pn, const InputEventType type )
{
	if ( m_bIsDownloading )
		return;
	if ( m_iDLorLST == 0)
	{
		if ( m_iPackagesPos > 0)
		{
			m_iPackagesPos--;
			UpdatePackagesList();
		}
	}
	else
	{
		if ( m_iLinksPos > 0 )
		{
			m_iLinksPos--;
			UpdateLinksList();
		}
	}
	ScreenWithMenuElements::MenuUp( pn, type );
}

void ScreenPackages::MenuDown( PlayerNumber pn, const InputEventType type )
{
	if ( m_bIsDownloading )
		return;
	
	if ( m_iDLorLST == 0)
	{
		if ( m_iPackagesPos < m_Packages.size() - 1 )
		{
			m_iPackagesPos++;
			UpdatePackagesList();
		}
	}
	else
	{
		if ( m_iLinksPos < m_LinkTitles.size() - 1 )
		{
			m_iLinksPos++;
			UpdateLinksList();
		}
	}
	ScreenWithMenuElements::MenuDown( pn, type );
}

void ScreenPackages::MenuLeft( PlayerNumber pn, const InputEventType type )
{
	if ( m_bIsDownloading )
		return;
	if ( !m_bCanDL )
		return;
	
	m_sprExistingBG.StopTweening( );
	m_sprWebBG.StopTweening( );
	
	if ( m_iDLorLST == 0 )
	{
		m_iDLorLST = 1;
		COMMAND( m_sprExistingBG, "Away" );
		COMMAND( m_sprWebBG, "Back" );
	}
	else 
	{	
		m_iDLorLST = 0;
		COMMAND( m_sprExistingBG, "Back" );
		COMMAND( m_sprWebBG, "Away" );
	}
	ScreenWithMenuElements::MenuLeft( pn, type );
}

void ScreenPackages::MenuRight( PlayerNumber pn, const InputEventType type )
{
	if ( m_bIsDownloading )
		return;
	MenuLeft( pn, type );
	ScreenWithMenuElements::MenuRight( pn, type );
}

void ScreenPackages::MenuBack( PlayerNumber pn )
{
	if ( m_bIsDownloading )
	{
		SCREENMAN->SystemMessage( "Download Cancelled." );
		CancelDownload( );
		return;
	}

	TweenOffScreen();
	Back( SM_GoToPrevScreen );
	ScreenWithMenuElements::MenuBack( pn );
}

void ScreenPackages::TweenOffScreen( )
{
	OFF_COMMAND( m_sprExistingBG );
	OFF_COMMAND( m_sprWebBG );
	OFF_COMMAND( m_sprWebSel );
	OFF_COMMAND( m_textPackages );
	OFF_COMMAND( m_textWeb );
	OFF_COMMAND( m_sprDL );
	OFF_COMMAND( m_sprDLBG );
	OFF_COMMAND( m_textStatus );

	if ( m_fOutputFile.IsOpen() )
		m_fOutputFile.Close();
}

void ScreenPackages::RefreshPackages()
{
	GetDirListing( "Packages/*.*zip", m_Packages, false, false );
	
	if ( m_iPackagesPos < 0 )
		m_iPackagesPos = 0;
	
	if ( m_iPackagesPos >= m_Packages.size() )
		m_iPackagesPos = m_Packages.size() - 1;

	UpdatePackagesList();
}

void ScreenPackages::UpdatePackagesList()
{
	CString TempText="";
	int min = m_iPackagesPos-NUM_PACKAGES_SHOW;
	int max = m_iPackagesPos+NUM_PACKAGES_SHOW;
	for (int i=min; i<max; i++ )
	{
		if ( ( i >= 0 ) && ( i < m_Packages.size() ) )
			TempText += m_Packages[i] + '\n';
		else
			TempText += '\n';
	}
	m_textPackages.SetText( TempText );
}

void ScreenPackages::UpdateLinksList()
{
	CString TempText="";
	if ( m_iLinksPos >= m_LinkTitles.size() )
		m_iLinksPos = m_LinkTitles.size() - 1;
	int min = m_iLinksPos-NUM_LINKS_SHOW;
	int max = m_iLinksPos+NUM_LINKS_SHOW;
	for (int i=min; i<max; i++ )
	{
		if ( ( i >= 0 ) && ( i < m_LinkTitles.size() ) )
			TempText += m_LinkTitles[i] + '\n';
		else
			TempText += '\n';
	}
	m_textWeb.SetText( TempText );
	if ( ( m_iLinksPos >=0 ) && ( m_iLinksPos < m_Links.size() ) )
		m_textURL.SetText( m_Links[m_iLinksPos] );
	else
		m_textURL.SetText( " " );
}

void ScreenPackages::HTMLParse()
{
	m_Links.clear();
	m_LinkTitles.clear();
	m_Links.push_back( " " );
	m_LinkTitles.push_back( "--Visit URL--" );

	//XXX: VERY DIRTY HTML PARSER!
	//Only designed to find links on websites.
	int i = m_sBUFFER.Find( "<A " );
	int j = 0;
	int k = 0;
	int l = 0;
	int m = 0;

	for ( int mode = 0; mode < 2; mode++ )
	{
		while ( i>=0 )
		{
			k = m_sBUFFER.Find( ">", i+1 );
			l = m_sBUFFER.Find( "HREF", i+1);
			m = m_sBUFFER.Find( "=", l );

			if ( ( l > k ) || ( m > k ) )	//no "href" in this tag.
			{
				if ( mode == 0 )
					i = m_sBUFFER.Find( "<A ", i+1 );
				else
					i = m_sBUFFER.Find( "<a ", i+1 );
				continue;
			}

			l = m_sBUFFER.Find( "</", m+1 );

			//Special case: There is exactly one extra tag in the link.
			j = m_sBUFFER.Find( ">", k+1 );
			if ( j < l )
				k = j;

			CString TempLink = StripOutContainers( m_sBUFFER.substr(m+1,k-m-1) );
			if ( TempLink.substr(0,7).compare("http://") != 0 )
				TempLink = m_sBaseAddress + TempLink;

			CString TempTitle = m_sBUFFER.substr( k+1, l-k-1 );

			m_Links.push_back( TempLink );
			m_LinkTitles.push_back( TempTitle );

			if ( mode == 0 )
				i = m_sBUFFER.Find( "<A ", i+1 );
			else
				i = m_sBUFFER.Find( "<a ", i+1 );
		}
		if ( mode == 0 )
			i = m_sBUFFER.Find( "<a " );
	}
	UpdateLinksList();
}

CString ScreenPackages::URLEncode( const CString &URL )
{
	CString Input = StripOutContainers( URL );
	CString Output;

	for (int k = 0; k < Input.length(); k++ )
	{
		char t = Input.at( k );
		if ( ( t >= '!' ) && ( t <= 'z' ) )
		{
			Output+=t;
		}
		else
			Output += "%" + ssprintf( "%X", t );
	}
	return Output;
}

CString ScreenPackages::StripOutContainers( const CString & In )
{
	if ( In.length() == 0 )
		return "";

	int i = 0;
	char t = In.at(i);
	while( t == ' ' && i < In.length() )
	{
		t = In.at(++i);
	}

	if( t == '\"' || t == '\'' )
	{
		int j = i+1; 
		char u = In.at(j);
		while( u != t && j < In.length() )
		{
			u = In.at(++j);
		}
		if( j == i )
			return StripOutContainers( In.substr(i+1) );
		else
			return StripOutContainers( In.substr(i+1, j-i-1) );
	}
	return In.substr( i );
}

void ScreenPackages::UpdateProgress()
{
	float DownloadedRatio;

	if( m_iTotalBytes < 1 )
		DownloadedRatio = 0;
	else
		DownloadedRatio = float(m_iDownloaded) / float(m_iTotalBytes);

	float dLY = DownloadedRatio / 2 * m_sprDLBG.GetUnzoomedWidth();

	m_sprDL.SetX( m_sprDLBG.GetX() - m_sprDLBG.GetUnzoomedWidth() / 2.0f + dLY );
	m_sprDL.SetWidth( dLY * 2 );

	m_textStatus.SetText( m_sStatus );
}

void ScreenPackages::CancelDownload( )
{
	m_wSocket.close();
	m_bIsDownloading = false;
	m_iDownloaded = 0;
	m_bGotHeader = false;
	m_fOutputFile.Close();
	m_sStatus = "Failed.";
	m_sBUFFER = "";
	if( !FILEMAN->Remove( "Packages/" + m_sEndName ) )
		SCREENMAN->SystemMessage( "Packages/" + m_sEndName );
}
void ScreenPackages::EnterURL( const CString & sURL )
{
	CString Proto;
	CString Server;
	int Port=80;
	CString Addy;

	if( !ParseHTTPAddress( sURL, Proto, Server, Port, Addy ) )
	{
		m_sStatus = "Invalid URL.";
		UpdateProgress();
		return;
	}

	//Determine if this is a website, or a package?
	//Criteria: does it end with *zip?
	if( Addy.Right(3).CompareNoCase("zip") == 0 )
		m_bIsPackage=true;
	else
		m_bIsPackage = false;

	int k=0,j=0;
	while( k >= 0 )
	{
		j = k+1;
		k = Addy.Find( '/', j );
	}

	//If there is no '/' in the addy, ignore it all
	//for base address.
	if( Addy.Find( '/', 0 ) < 0 )
		j = 0;

	m_sEndName = Addy.Right( Addy.length() - j + 1 );

	if( Port == 80 )
		m_sBaseAddress = "http://" + Server + "/" + Addy.substr(0, j);
	else
		m_sBaseAddress = "http://" + Server + ssprintf( ":%d/", Port ) + Addy.substr(0, j);

	//Open the file...

	//First find out if a file by this name already exists
	//if so, then we gotta ditch out.
	//XXX: This should be fixed by a prompt or something?

	//if we are not talking about a file, let's not worry
	if( m_sEndName != "" && m_bIsPackage )
	{
		CStringArray AddTo;
		GetDirListing( "Packages/"+m_sEndName, AddTo, false, false );
		if ( AddTo.size() > 0 )
		{
			m_sStatus = "File Already Exists";
			UpdateProgress();
			return;
		}

		if( !m_fOutputFile.Open( "Packages/"+m_sEndName, RageFile::WRITE ) )
		{
			m_sStatus = m_fOutputFile.GetError();
			UpdateProgress();
			return;
		}
	}
	//Continue...

	Addy = URLEncode( Addy );

	if ( Addy != "/" )
		Addy = "/" + Addy;

	m_wSocket.close();
	m_wSocket.create();

	m_wSocket.blocking = true;

	if( !m_wSocket.connect( Server, Port ) )
	{
		m_sStatus = "Failed to connect.";
		UpdateProgress();
		return;
	}
	
	//Produce HTTP header

	CString Header="";

	Header = "GET "+Addy+" HTTP/1.0\r\n";
	Header+= "Host: " + Server + "\r\n";
	Header+= "Connection: closed\r\n\r\n";

	m_wSocket.SendData( Header.c_str(), Header.length() );
	m_sStatus = "Header Sent.";
	m_wSocket.blocking = false;
	m_bIsDownloading = true;
	m_sBUFFER = "";
	m_bGotHeader = false;
	UpdateProgress();
	return;
}
void ScreenPackages::HTTPUpdate()
{
	if( !m_bIsDownloading )
		return;

	int BytesGot=0;
	//Keep this as a code block
	//as there may be need to "if" it out some time.
	/* If you need a conditional for a large block of code, stick it in
	 * a function and return. */
	int Size = 1; 
	while( Size > 0 )
	{
		char Buffer[1024];
		Size = m_wSocket.ReadData( Buffer, 1024 );
		m_sBUFFER.append( Buffer, Size );
		BytesGot += Size;
	}
	if( !m_bGotHeader )
	{
		m_sStatus = "Waiting for header.";
		//We don't know if we are using unix-style or dos-style
		int HeaderEnd = m_sBUFFER.find("\n\n");
		if( HeaderEnd < 0 )
			HeaderEnd = m_sBUFFER.find("\r\n\r\n");
		if( HeaderEnd >= 0 )
		{
			int i = m_sBUFFER.find(" ");
			int j = m_sBUFFER.find(" ",i+1);
			int k = m_sBUFFER.Find("\n",j+1);
			if ( i < 0 || j < 0 || k < 0 )
			{
				m_iResponseCode = -100;
				m_iResponceName = "Malformed responce.";
			}
			m_iResponseCode = atoi(m_sBUFFER.substr(i+1,j-i).c_str());
			m_iResponceName = m_sBUFFER.substr( j+1, k-j).c_str();

			i = m_sBUFFER.find("Content-Length:");
			j = m_sBUFFER.find("\n", i+1 );

			if( i > 0 )
				m_iTotalBytes = atoi(m_sBUFFER.substr(i+16,j-i).c_str());
			else
				m_iTotalBytes = -1;	//We don't know, so go until disconnect

			m_bGotHeader = true;
			m_sBUFFER = m_sBUFFER.substr( HeaderEnd + 4 );
		}
	}
	else
	{
		if( m_bIsPackage )
		{
			m_iDownloaded += m_sBUFFER.length();
			m_fOutputFile.Write( m_sBUFFER );
			m_sBUFFER = "";
		}
		else
		{
			m_iDownloaded = m_sBUFFER.length();
		}
		if ( ( ( m_iTotalBytes <= m_iDownloaded ) && ( m_iTotalBytes != -1 ) ) ||
						//We have the full doc. (And we knew how big it was)
			( ( m_iTotalBytes == -1 ) && 
				( ( m_wSocket.state == 8 ) || 
					( m_wSocket.state == 0 ) ) ) )
					//XXX: Work around;  these should use enumerated types.
					//We didn't know how big it was, and were disconnected
					//So that means we have it all.
		{
			m_wSocket.close();
			m_bIsDownloading = false;
			m_bGotHeader=false;
			m_sStatus = ssprintf( "Done;%dB", int(m_iDownloaded) );

			if( m_iResponseCode < 200 || m_iResponseCode >= 400 )
			{
				m_sStatus = ssprintf( "%d", m_iResponseCode ) + m_iResponceName;
			}
			else
			{
				if( m_bIsPackage && m_iResponseCode < 300 )
				{
					m_fOutputFile.Flush();
					m_fOutputFile.Close();
					FlushDirCache();
					RefreshPackages();
					m_iDownloaded = 0;
				}
				else
					HTMLParse();
			}
		}
	}
}

bool ScreenPackages::ParseHTTPAddress( const CString & URL, CString & Proto, CString & Server, int & Port, CString & Addy )
{
	int i,j,k,l;

	// [PROTO://]SERVER[:PORT][/URL]

	if ( URL.length() == 0 )
		return false;

	//PROTO
	i = URL.Find( "://" );
	if ( i >= 0 )
	{
		Proto = URL.substr( 0, i );
		i+=3;
	}
	else
		i = 0;
	//SERVER
	j = URL.Find( '/', i );
	k = URL.Find( ':', i );
	if ( ( k >= 0 ) && ( j >= 0 ) )
		l = min ( j, k );
	else if ( k >= 0 )
		l = k;
	else if ( j >= 0 )
		l = j;
	else
		l = URL.length();		
	Server = URL.substr( i, l-i );

	//PORT
	if ( URL.substr( l, 1 ).compare(":")==0 )
	{
		l++;
		j = URL.Find( '/', l );
		if ( j < 0 )
			j = URL.length();
		Port = atoi( URL.substr( l, j-l ).c_str() );
		if (Port == 0)
			return false;
		l=j;
	}
	if (l<URL.length())
		l++;
	Addy = URL.substr( l );
	
	return true;
}

#endif
/*
 * (c) 2004 Charles Lohr
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
