#include <math.h>

#include "CGraphicsDevice.h"
#include "files.h"

namespace
{
	const CCoord<int> KDefaultResolution(640,480);
};

CGraphicsDevice::CGraphicsDevice(const char *aCaption, const char* aIcon): iWidth(0), iHeight(0), iBits(0), iBasicModes(0), iLocked(0), iDontLock(0), iSurfaceOK(0), iSDLsurface(0), iRGBSurface(0), iWindow(0), iRenderer(0), iTexture(0), iCursorMode(SDL_DISABLE)
{
	for (int i=0;i<sizeof(iGammaRamp);i++)
		iGammaRamp[i]=(Uint8)i;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO)<0)
	{
        error("CGraphicsDevice::Init: Unable to init SDL_INIT_VIDEO subsystem: %s\n", SDL_GetError());
    }

	if (aCaption)
		iCaptionText=strdup(aCaption);
	else
		iCaptionText=NULL;

	if (aIcon)
        iIconFile=strdup(aIcon);
	else
		iIconFile=NULL;

	ListVideoModes();
}

void CGraphicsDevice::SetCursorMode(int aMode)
{
	DEBUG_ASSERT(aMode==0 || aMode==1);
	iCursorMode=aMode;
}

CGraphicsDevice::~CGraphicsDevice()
{
	std::vector<CCoord<int>*>::iterator iter;

	Close();

	if (iRenderer) { SDL_DestroyRenderer(iRenderer); iRenderer=NULL; }
	if (iWindow)   { SDL_DestroyWindow(iWindow);     iWindow=NULL; }

	free(iCaptionText);
	free(iIconFile);
	
	for (iter=iResolutions.begin();iter!=iResolutions.end();iter++)
//		free(*iter);
		delete *iter;
}


CGraphicsBuffer* CGraphicsDevice::NewBuf()
{	CGraphicsBuffer* gb;

	gb=new CGraphicsBuffer(iWidth,iHeight);
	ASSERT(gb!=NULL);
	return gb;
}

bool CGraphicsDevice::FullScreen()
{
	if (!iWindow) return false;
	return (SDL_GetWindowFlags(iWindow)&SDL_WINDOW_FULLSCREEN_DESKTOP)!=0;
}

int CGraphicsDevice::SurfaceOK()
{
	return iSurfaceOK;
}

int CGraphicsDevice::Locked()
{
	return iLocked;
}

int CGraphicsDevice::SetDontLock()
{
	if (!SurfaceOK()) return 1;
	iDontLock++;
	return 0;
}

int CGraphicsDevice::UnSetDontLock()
{
	if (!SurfaceOK()) return 1;
	iDontLock--;
	return 0;
}


int CGraphicsDevice::RefreshAll()
{
	int retval=0;

	SetDontLock();
	if (!Locked())
	{
		if (iSDLsurface!=NULL&&Width()!=0)
			Present();
	}
	else retval=1;

	UnSetDontLock();
	return retval;

}

void CGraphicsDevice::SaveShot(const char* aName)
{
	SDL_SaveBMP(iSDLsurface,aName);
}

void CGraphicsDevice::CopyToSurface(const CGraphicsBuffer* aBuf, const CRect<int>& rect)
{	
	SDL_Rect r = rect;

	SDL_Surface *surf = aBuf->CopyToSurface( &iDisplayPalette );

	SDL_BlitSurface( surf, &r, iSDLsurface, &r );

	aBuf->DeleteSurface( surf );
}

int CGraphicsDevice::ShowBuf(const CGraphicsBuffer* aBuf, CDrawArea& aDrawArea)
{	
	if (Lock()) 
		return 1;

	int a=0,siz = aDrawArea.Size();

	for (a=0;a<siz;a++)
	{
		CRect<int> r = aDrawArea.Rect(a, Rect());
		CopyToSurface(aBuf, r);
		iDirtyArea.Combine(r);
	}

	if (UnLock()) 
		return 1;

	Update();

	return 0;
}


int CGraphicsDevice::ShowBuf(const CGraphicsBuffer* aBuf, const CRect<int>& rect)
{	
	if (Lock()) 
		return 1;

	CopyToSurface(aBuf, rect);
    iDirtyArea.Combine(CRect<int>(rect));

	if (UnLock()) 
		return 1;

	Update();

	return 0;
}

void CGraphicsDevice::Update()
{
	iDirtyArea.Reset();
	Present();
}

// Convert the 8-bit paletted back-buffer to RGB and present it via the renderer.
// SDL2 has no paletted display mode, so the whole frame is uploaded each time.
void CGraphicsDevice::Present()
{
	if (!iRenderer||!iSDLsurface||!iRGBSurface||!iTexture)
		return;

	SDL_BlitSurface(iSDLsurface, NULL, iRGBSurface, NULL);
	SDL_UpdateTexture(iTexture, NULL, iRGBSurface->pixels, iRGBSurface->pitch);
	SDL_RenderClear(iRenderer);
	SDL_RenderCopy(iRenderer, iTexture, NULL, NULL);
	SDL_RenderPresent(iRenderer);
}

int CGraphicsDevice::Lock()
{
	if (!SurfaceOK()) return 1;

	while (iDontLock);
	if ( SDL_MUSTLOCK(iSDLsurface))
        while ( SDL_LockSurface(iSDLsurface) < 0 )
		{
			SDL_Delay(10);
		}
	iLocked=1;
	return 0;
}

int CGraphicsDevice::UnLock()
{
	if (!SurfaceOK()) return 1;

  	if (SDL_MUSTLOCK(iSDLsurface))
        SDL_UnlockSurface(iSDLsurface);
	iLocked=0;
	return 0;
}

int CGraphicsDevice::Clear()
{
	if (!Lock()||
		!iSDLsurface) return 1;

	SDL_FillRect( iSDLsurface, NULL, 0 );

	UnLock();
	Present();
	return 0;
}

void CGraphicsDevice::Close()
{
	iSurfaceOK=0;
	iLocked=1;

	if (iTexture)    { SDL_DestroyTexture(iTexture);   iTexture=NULL; }
	if (iRGBSurface) { SDL_FreeSurface(iRGBSurface);   iRGBSurface=NULL; }
	if (iSDLsurface) { SDL_FreeSurface(iSDLsurface);   iSDLsurface=NULL; }
}

/* Return 0 if no error*/
int CGraphicsDevice::SetMode(int aWidth,int aHeight,int aBits, bool aFullScreen, int /*aExtraFlags*/)
{
	Close(); // If we're already in graphics mode
	while (iDontLock);

	// The game always renders into an 8-bit paletted buffer regardless of aBits.
	iBits=aBits ? aBits : 8;

	// Create the window (and renderer) on the first call; afterwards just resize.
	if (!iWindow)
	{
		Uint32 wflags=SDL_WINDOW_RESIZABLE;
		if (aFullScreen)
			wflags|=SDL_WINDOW_FULLSCREEN_DESKTOP;

		iWindow=SDL_CreateWindow(iCaptionText?iCaptionText:"",
		                         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                         aWidth, aHeight, wflags);
		if (!iWindow)
			error("CGraphicsDevice::SetMode: SDL_CreateWindow(%d,%d) failed: %s\n",aWidth,aHeight,SDL_GetError());

		iRenderer=SDL_CreateRenderer(iWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
		if (!iRenderer)
			iRenderer=SDL_CreateRenderer(iWindow, -1, 0);
		if (!iRenderer)
			error("CGraphicsDevice::SetMode: SDL_CreateRenderer failed: %s\n",SDL_GetError());

		if (iIconFile!=NULL && exists(getdatapath(std::string(iIconFile)).c_str()))
		{
			SDL_Surface* icon=SDL_LoadBMP(getdatapath(std::string(iIconFile)).c_str());
			if (icon)
			{
				SDL_SetWindowIcon(iWindow, icon);
				SDL_FreeSurface(icon);
			}
		}
	}
	else
	{
		SDL_SetWindowFullscreen(iWindow, aFullScreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0);
		if (!aFullScreen)
			SDL_SetWindowSize(iWindow, aWidth, aHeight);
	}

	// 8-bit paletted back-buffer the game draws into.
	iSDLsurface=SDL_CreateRGBSurface(0, aWidth, aHeight, 8, 0,0,0,0);
	if (iSDLsurface==NULL)
		error("CGraphicsDevice::SetMode: SDL_CreateRGBSurface(%d,%d,8) failed: %s\n",aWidth,aHeight,SDL_GetError());

	// 32-bit conversion target + streaming texture for presentation.
	iRGBSurface=SDL_CreateRGBSurface(0, aWidth, aHeight, 32,
	                                 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	iTexture=SDL_CreateTexture(iRenderer, SDL_PIXELFORMAT_ARGB8888,
	                           SDL_TEXTUREACCESS_STREAMING, aWidth, aHeight);
	if (iRGBSurface==NULL || iTexture==NULL)
		error("CGraphicsDevice::SetMode: failed to create presentation surface/texture: %s\n",SDL_GetError());

	// Scale the internal resolution to the window/screen.
	SDL_RenderSetLogicalSize(iRenderer, aWidth, aHeight);

	iWidth=aWidth;
	iHeight=aHeight;

	SDL_ShowCursor(iCursorMode);
	SDL_SetWindowTitle(iWindow, iCaptionText?iCaptionText:"");

	iSurfaceOK=1;
	iLocked=0;

	SyncDisplayPalette();
	Clear();

	return 0;
}

int CGraphicsDevice::SetPalette(const CPalette& pal,int mul)
{
	int i;

	if (!SurfaceOK()) return 1;

	if (mul<=0) mul=0;
	if (mul>256) mul=256;

	for(i=0;i<256;i++)
	{
		iPalette.Color(i).r=(pal.Color(i).r*mul)>>8;
		iPalette.Color(i).g=(pal.Color(i).g*mul)>>8;
		iPalette.Color(i).b=(pal.Color(i).b*mul)>>8;
	}
	SyncDisplayPalette();

	return 0;
}

void CGraphicsDevice::GetPalette(CPalette& pal)
{
	pal = iPalette;
}

void CGraphicsDevice::SetGamma(float aGamma)
{
	// Same curve as SDL's old SDL_SetGamma()/SDL_CalculateGammaRamp(): output =
	// 255 * (i/255)^(1/gamma), so gamma>1 brightens and gamma==1 is identity.
	const float inv = 1.0f / (aGamma > 0.0f ? aGamma : 1.0f);
	for (int i=0;i<sizeof(iGammaRamp);i++)
	{
		int v = 255.0 * pow(i/255.0, inv) + 0.5;
		if (v<0)   v=0;
		if (v>255) v=255;
		iGammaRamp[i]=(Uint8)v;
	}

	SyncDisplayPalette();
}

void CGraphicsDevice::SyncDisplayPalette()
{
	if (!SurfaceOK()) return;

	for (int i=0;i<256;i++)
	{
		const SDL_Color& src = iPalette.Color(i);
		SDL_Color& dst = iDisplayPalette.Color(i);
		dst.r = iGammaRamp[src.r];
		dst.g = iGammaRamp[src.g];
		dst.b = iGammaRamp[src.b];
		dst.a = src.a;
	}
	SDL_SetPaletteColors(iSDLsurface->format->palette, iDisplayPalette.ColorData(), 0, 256);
}

void CGraphicsDevice::ListVideoModes()
{
	int i;

	iFullScreenPossible = true;

	/* Get available display modes for the primary display */
	int modeCount = SDL_GetNumDisplayModes(0);
	for(i=0;i<modeCount;++i)
	{
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(0, i, &mode)!=0)
			continue;

		/* We're not interested of modes less than 320x200... are we? */
		if (mode.w>=320&&mode.h>=200)
		{
			/* The list often contains multiple modes with same resolution, ignore those */
			bool added = false;
			for (std::vector<CCoord<int>*>::iterator a = iResolutions.begin(); a != iResolutions.end(); a++) {
				if ( ((*a)->X() == mode.w) && ((*a)->Y() == mode.h) ) {
					added = true;
					break;
				}
			}
			if (added) continue;

			CCoord<int>* res=new CCoord<int>(mode.w,mode.h);
			iResolutions.push_back(res);
		}
	}

	if (iResolutions.empty())
		iFullScreenPossible = false;

	std::vector<CCoord<int>*>::iterator outer;
	std::vector<CCoord<int>*>::iterator inner;

	// Sort modes, Basic bubble :)
	for (outer=iResolutions.begin();outer<iResolutions.end() /*-1*/ ;outer++) {
	 for (inner=outer+1;inner<iResolutions.end();inner++)
	 {
		 if ((*outer)->X()>(*inner)->X()||
			 ((*outer)->X()==(*inner)->X()&&
			  (*outer)->Y()>(*inner)->Y()))
		 {
			 CCoord<int>* tmp;

			 tmp=*outer;
			 *outer=*inner;
			 *inner=tmp;
		 }
	 }
	}

}

int CGraphicsDevice::ResAmount()
{
	return (int)iResolutions.size();
}

const CCoord<int>* CGraphicsDevice::Res(int a)
{
	ASSERT(a>=0);
	ASSERT(a<iResolutions.size());
	return iResolutions[a];
}

void CGraphicsDevice::NextRes( CCoord<int>& aRes )
{
	int modeNo=-1;
	int a;

	for (a=0;a<iResolutions.size()&&modeNo == -1;a++)
		if ( *iResolutions[a] == aRes )
			modeNo = a;

	if ( modeNo == -1 )
	{
		aRes.Set( KDefaultResolution );
		return;
	}

	modeNo++;
	if ( modeNo >= iResolutions.size() )
		modeNo = 0;

	aRes.Set( *iResolutions[modeNo] );
}

void CGraphicsDevice::PrevRes( CCoord<int>& aRes )
{
	int modeNo=-1;
	int a;

	for (a=0;a<iResolutions.size()&&modeNo == -1;a++)
		if ( *iResolutions[a] == aRes )
			modeNo = a;

	if ( modeNo == -1 )
	{
		aRes.Set( KDefaultResolution );
		return;
	}

	modeNo--;
	if ( modeNo < 0 )
		modeNo = iResolutions.size()-1;

	aRes.Set( *iResolutions[modeNo] );
}
