#ifndef RAGEDISPLAY_D3D_H
#define RAGEDISPLAY_D3D_H

class RageException_D3DNotInstalled: public exception
{
		const char *what() const { return
				"DirectX 8.1 or greater is not installed.  You can download it from:\n"
				"http://www.microsoft.com/downloads/details.aspx?FamilyID=a19bed22-0b25-4e5d-a584-6389d8a3dad0&displaylang=en"; }
};

class RageException_D3DNoAcceleration: public exception
{
		const char *what() const { return
				"Your system is reporting that Direct3D hardware acceleration is not available.  "
				"Please obtain an updated driver from your video card manufacturer.\n\n"; }
};
				

class RageDisplay_D3D: public RageDisplay
{
public:
	RageDisplay_D3D( VideoModeParams params );
	~RageDisplay_D3D();
	void Update(float fDeltaTime);

	bool IsSoftwareRenderer();
	void ResolutionChanged();
	const PixelFormatDesc *GetPixelFormatDesc(PixelFormat pf) const;

	void BeginFrame();	
	void EndFrame();
	VideoModeParams GetVideoModeParams() const;
	void SetBlendMode( BlendMode mode );
	bool SupportsTextureFormat( PixelFormat pixfmt );
	unsigned CreateTexture( PixelFormat pixfmt, SDL_Surface* img );
	void UpdateTexture( 
		unsigned uTexHandle, 
		SDL_Surface* img,
		int xoffset, int yoffset, int width, int height 
		);
	void DeleteTexture( unsigned uTexHandle );
	void SetTexture( RageTexture* pTexture );
	void SetTextureModeModulate();
	void SetTextureModeGlow( GlowMode m=GLOW_WHITEN );
	void SetTextureWrapping( bool b );
	int GetMaxTextureSize() const;
	void SetTextureFiltering( bool b);
	bool IsZWriteEnabled() const;
	bool IsZTestEnabled() const;
	void SetZWrite( bool b );
	void SetZTest( bool b );
	void ClearZBuffer();
	void SetBackfaceCull( bool b );
	void SetAlphaTest( bool b );
	void SetMaterial( 
		float emissive[4],
		float ambient[4],
		float diffuse[4],
		float specular[4],
		float shininess
		);
	void SetLighting( bool b );
	void SetLightOff( int index );
	void SetLightDirectional( 
		int index, 
		RageColor ambient, 
		RageColor diffuse, 
		RageColor specular, 
		RageVector3 dir );

	void DrawQuads( const RageSpriteVertex v[], int iNumVerts );
	void DrawFan( const RageSpriteVertex v[], int iNumVerts );
	void DrawStrip( const RageSpriteVertex v[], int iNumVerts );
	void DrawTriangles( const RageSpriteVertex v[], int iNumVerts );
	void DrawIndexedTriangles( const RageModelVertex v[], int iNumVerts, const Uint16* pIndices, int iNumIndices );
//	void DrawLineStrip( const RageSpriteVertex v[], int iNumVerts, float LineWidth );

	void SaveScreenshot( CString sPath );
protected:
	CString TryVideoMode( VideoModeParams params, bool &bNewDeviceOut );
	void SetViewport(int shift_left, int shift_down);
	RageMatrix GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf ); 
};

#endif

