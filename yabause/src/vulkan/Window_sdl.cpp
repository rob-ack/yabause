/* -----------------------------------------------------
This source code is public domain ( CC0 )
The code is provided as-is without limitations, requirements and responsibilities.
Creators and contributors to this source code are provided as a token of appreciation
and no one associated with this source code can be held responsible for any possible
damages or losses of any kind.

Original file creator:  Teagan Chouinard (GLFW)
Contributors:
Niko Kauppi (Code maintenance)
----------------------------------------------------- */

#include "BUILD_OPTIONS.h"
#include "Platform.h"

#include "Window.h"
#include "Shared.h"
#include "Renderer.h"

#include <assert.h>
#include <iostream>

#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_vulkan.h>



void Window::_InitOSWindow()
{	
}


void Window::_DeInitOSWindow()
{
}

void Window::_UpdateOSWindow()
{
}

void Window::_InitOSSurface()
{

	if ( SDL_TRUE != SDL_Vulkan_CreateSurface( (SDL_Window*)window, _renderer->GetVulkanInstance(), &_surface ) ){
		assert ( 0 && "SDL could not create the window surface." );
		return;
	}
}

